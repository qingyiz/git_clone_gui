#include "infrastructure/GitWorkspaceService.h"

#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QtConcurrent>

#include <algorithm>

namespace gitclone {
namespace {

bool stableNameLessThan(const QString &left, const QString &right)
{
    const int folded = QString::compare(left, right, Qt::CaseInsensitive);
    return folded == 0 ? left < right : folded < 0;
}

} // namespace

GitWorkspaceService::GitWorkspaceService(QObject *parent)
    : WorkspaceService(parent)
{
    m_gitProcess.setProcessChannelMode(QProcess::MergedChannels);
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("GIT_TERMINAL_PROMPT"), QStringLiteral("0"));
    m_gitProcess.setProcessEnvironment(environment);
    m_gitTimeout.setSingleShot(true);
    m_gitTimeout.setInterval(15000);

    connect(&m_gitProcess, &QProcess::readyRead, this, [this] {
        m_gitOutput.append(m_gitProcess.readAll());
    });
    connect(&m_gitProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &GitWorkspaceService::handleGitFinished);
    connect(&m_gitProcess, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart) {
                    failGit(QStringLiteral("无法启动 Git：%1").arg(m_gitProcess.errorString()));
                }
            });
    connect(&m_gitTimeout, &QTimer::timeout, this, [this] {
        m_gitProcess.kill();
        failGit(QStringLiteral("Git 操作超时。"));
    });
}

GitWorkspaceService::~GitWorkspaceService()
{
    cancelScan();
    for (QFutureWatcher<WorkspaceScanResult> *watcher : m_scanWatchers) {
        watcher->waitForFinished();
    }
    cancelGitOperation();
}

void GitWorkspaceService::scan(const QString &rootPath)
{
    cancelScan();
    const QFileInfo rootInfo(rootPath);
    if (!rootInfo.exists() || !rootInfo.isDir()) {
        emit scanFailed(rootPath, QStringLiteral("工作目录不存在或不是目录。"));
        return;
    }
    if (!rootInfo.isReadable()) {
        emit scanFailed(rootPath, QStringLiteral("工作目录不可读。"));
        return;
    }

    const quint64 generation = ++m_scanGeneration;
    m_scanCancelled = std::make_shared<std::atomic_bool>(false);
    const auto token = m_scanCancelled;
    emit scanBusyChanged(true);
    auto *watcher = new QFutureWatcher<WorkspaceScanResult>(this);
    m_scanWatchers.append(watcher);
    connect(watcher, &QFutureWatcher<WorkspaceScanResult>::finished, this,
            [this, generation, watcher] {
                const WorkspaceScanResult result = watcher->result();
                m_scanWatchers.removeAll(watcher);
                watcher->deleteLater();
                if (generation != m_scanGeneration) {
                    return;
                }
                m_scanCancelled.reset();
                emit scanBusyChanged(false);
                if (result.cancelled) {
                    return;
                }
                if (!result.error.isEmpty()) {
                    emit scanFailed(result.rootPath, result.error);
                    return;
                }
                emit scanFinished(result.rootPath, result.repositories,
                                  result.skippedDirectories);
            });
    watcher->setFuture(QtConcurrent::run([rootPath, token] {
        return scanWorkspaceDirectories(rootPath, token);
    }));
}

void GitWorkspaceService::cancelScan()
{
    if (m_scanCancelled) {
        m_scanCancelled->store(true);
        m_scanCancelled.reset();
        ++m_scanGeneration;
        emit scanBusyChanged(false);
    }
}

void GitWorkspaceService::loadBranches(const QString &repositoryPath)
{
    cancelGitOperation();
    m_repositoryPath = QDir(repositoryPath).absolutePath();
    m_currentBranch.clear();
    startGit(GitOperation::LoadCurrent,
             {QStringLiteral("-C"), m_repositoryPath, QStringLiteral("branch"),
              QStringLiteral("--show-current")});
}

void GitWorkspaceService::switchBranch(const QString &repositoryPath,
                                       const BranchTarget &target)
{
    cancelGitOperation();
    m_repositoryPath = QDir(repositoryPath).absolutePath();
    m_switchBranchName = target.name;
    QStringList arguments {QStringLiteral("-C"), m_repositoryPath,
                           QStringLiteral("switch")};
    if (target.kind == BranchTarget::Kind::Remote) {
        arguments.append(QStringLiteral("--track"));
    }
    arguments.append(QStringLiteral("--"));
    arguments.append(target.name);
    startGit(GitOperation::Switch, arguments);
}

void GitWorkspaceService::cancelGitOperation()
{
    if (m_gitOperation == GitOperation::None) {
        return;
    }
    m_gitTimeout.stop();
    m_gitProcess.disconnect(this);
    if (m_gitProcess.state() != QProcess::NotRunning) {
        m_gitProcess.kill();
        m_gitProcess.waitForFinished(1000);
    }
    connect(&m_gitProcess, &QProcess::readyRead, this, [this] {
        m_gitOutput.append(m_gitProcess.readAll());
    });
    connect(&m_gitProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &GitWorkspaceService::handleGitFinished);
    connect(&m_gitProcess, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart) {
                    failGit(QStringLiteral("无法启动 Git：%1").arg(m_gitProcess.errorString()));
                }
            });
    resetGitOperation();
    emit gitBusyChanged(false);
}

BranchCatalog GitWorkspaceService::parseBranchRefs(const QString &currentBranch,
                                                   const QByteArray &output)
{
    BranchCatalog catalog;
    catalog.currentBranch = currentBranch;
    const QList<QByteArray> lines = output.split('\n');
    for (QByteArray line : lines) {
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        const QString ref = QString::fromUtf8(line).trimmed();
        if (ref.startsWith(QStringLiteral("refs/heads/"))) {
            catalog.localBranches.append(ref.mid(QStringLiteral("refs/heads/").size()));
        } else if (ref.startsWith(QStringLiteral("refs/remotes/"))) {
            const QString remote = ref.mid(QStringLiteral("refs/remotes/").size());
            if (!remote.endsWith(QStringLiteral("/HEAD"))) {
                catalog.remoteBranches.append(remote);
            }
        }
    }
    catalog.localBranches.removeDuplicates();
    catalog.remoteBranches.removeDuplicates();
    std::sort(catalog.localBranches.begin(), catalog.localBranches.end(), stableNameLessThan);
    std::sort(catalog.remoteBranches.begin(), catalog.remoteBranches.end(), stableNameLessThan);
    catalog.remoteCandidates = remoteBranchCandidates(catalog.localBranches,
                                                      catalog.remoteBranches);
    return catalog;
}

WorkingTreeStatus GitWorkspaceService::parseWorkingTreeStatus(const QByteArray &output)
{
    WorkingTreeStatus status;
    const QList<QByteArray> records = output.split('\0');
    for (int index = 0; index < records.size(); ++index) {
        const QByteArray &record = records.at(index);
        if (record.size() < 3) {
            continue;
        }
        const char staged = record.at(0);
        const char unstaged = record.at(1);
        const QByteArray pair = record.left(2);
        if (pair == "??") {
            ++status.untrackedFiles;
        } else if (pair == "DD" || pair == "AU" || pair == "UD" || pair == "UA"
                   || pair == "DU" || pair == "AA" || pair == "UU") {
            ++status.conflicts;
        } else {
            if (staged != ' ' && staged != '!' && staged != '?') {
                ++status.stagedChanges;
            }
            if (unstaged != ' ' && unstaged != '!' && unstaged != '?') {
                ++status.unstagedChanges;
            }
        }
        if (staged == 'R' || staged == 'C' || unstaged == 'R' || unstaged == 'C') {
            ++index;
        }
    }
    return status;
}

void GitWorkspaceService::startGit(GitOperation operation, const QStringList &arguments)
{
    m_gitOperation = operation;
    m_gitOutput.clear();
    emit gitBusyChanged(true);
    m_gitTimeout.start();
    m_gitProcess.start(QStringLiteral("git"), arguments);
}

void GitWorkspaceService::handleGitFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_gitOutput.append(m_gitProcess.readAll());
    m_gitTimeout.stop();
    const GitOperation completedOperation = m_gitOperation;
    const QString repositoryPath = m_repositoryPath;
    const QString output = QString::fromLocal8Bit(m_gitOutput).trimmed();

    if (completedOperation == GitOperation::LoadCurrent && exitStatus == QProcess::NormalExit
        && exitCode == 0) {
        m_currentBranch = output;
        startGit(GitOperation::LoadRefs,
                 {QStringLiteral("-C"), repositoryPath, QStringLiteral("for-each-ref"),
                  QStringLiteral("--format=%(refname)"), QStringLiteral("refs/heads"),
                  QStringLiteral("refs/remotes")});
        return;
    }
    if (completedOperation == GitOperation::LoadRefs && exitStatus == QProcess::NormalExit
        && exitCode == 0) {
        m_pendingCatalog = parseBranchRefs(m_currentBranch, m_gitOutput);
        startGit(GitOperation::LoadStatus,
                 {QStringLiteral("-C"), repositoryPath, QStringLiteral("status"),
                  QStringLiteral("--porcelain=v1"), QStringLiteral("-z"),
                  QStringLiteral("--untracked-files=normal")});
        return;
    }
    if (completedOperation == GitOperation::LoadStatus && exitStatus == QProcess::NormalExit
        && exitCode == 0) {
        m_pendingCatalog.workingTreeStatus = parseWorkingTreeStatus(m_gitOutput);
        const BranchCatalog catalog = m_pendingCatalog;
        resetGitOperation();
        emit gitBusyChanged(false);
        emit branchesLoaded(repositoryPath, catalog);
        return;
    }
    if (completedOperation == GitOperation::Switch && exitStatus == QProcess::NormalExit
        && exitCode == 0) {
        const QString branchName = m_switchBranchName;
        resetGitOperation();
        emit gitBusyChanged(false);
        emit branchSwitchSucceeded(repositoryPath, branchName);
        return;
    }
    failGit(output.isEmpty()
                ? QStringLiteral("Git 操作失败（退出码 %1）。").arg(exitCode)
                : output);
}

void GitWorkspaceService::failGit(const QString &message)
{
    if (m_gitOperation == GitOperation::None) {
        return;
    }
    const GitOperation failedOperation = m_gitOperation;
    const QString repositoryPath = m_repositoryPath;
    resetGitOperation();
    emit gitBusyChanged(false);
    if (failedOperation == GitOperation::Switch) {
        emit branchSwitchFailed(repositoryPath, message);
    } else {
        emit branchLoadFailed(repositoryPath, message);
    }
}

void GitWorkspaceService::resetGitOperation()
{
    m_gitTimeout.stop();
    m_gitOperation = GitOperation::None;
    m_gitOutput.clear();
    m_currentBranch.clear();
    m_pendingCatalog = BranchCatalog {};
    m_switchBranchName.clear();
}

} // namespace gitclone
