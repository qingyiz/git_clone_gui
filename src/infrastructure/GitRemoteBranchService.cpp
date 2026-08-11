#include "infrastructure/GitRemoteBranchService.h"

#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTimer>

#include <algorithm>

namespace gitclone {

struct GitRemoteBranchService::QueryContext {
    QProcess *process = nullptr;
    QTimer *timeout = nullptr;
    QString repositoryUrl;
    QByteArray output;
};

namespace {

int branchPriority(const QString &branch, const QString &defaultBranch)
{
    if (!defaultBranch.isEmpty()
        && branch.compare(defaultBranch, Qt::CaseInsensitive) == 0) {
        return 0;
    }

    const QString name = branch.toCaseFolded();
    const QStringList primary {QStringLiteral("main"), QStringLiteral("master"),
                               QStringLiteral("trunk")};
    const int primaryIndex = primary.indexOf(name);
    if (primaryIndex >= 0) {
        return 10 + primaryIndex;
    }

    const QStringList development {QStringLiteral("develop"),
                                   QStringLiteral("development"),
                                   QStringLiteral("dev")};
    const int developmentIndex = development.indexOf(name);
    if (developmentIndex >= 0) {
        return 20 + developmentIndex;
    }

    const QStringList namespaces {QStringLiteral("release/"), QStringLiteral("hotfix/"),
                                  QStringLiteral("feature/"), QStringLiteral("bugfix/"),
                                  QStringLiteral("fix/")};
    for (int index = 0; index < namespaces.size(); ++index) {
        if (name.startsWith(namespaces.at(index))) {
            return 30 + index;
        }
    }
    return 100;
}

} // namespace

GitRemoteBranchService::GitRemoteBranchService(QObject *parent, int timeoutMs)
    : RemoteBranchService(parent)
    , m_timeoutMs(timeoutMs)
{
}

GitRemoteBranchService::~GitRemoteBranchService()
{
    const QList<RequestId> requestIds = m_queries.keys();
    for (RequestId requestId : requestIds) {
        releaseQuery(requestId, true);
    }
}

RemoteBranchService::RequestId
GitRemoteBranchService::requestBranches(const QString &repositoryUrl)
{
    const RequestId requestId = ++m_nextRequestId;
    const QString url = repositoryUrl.trimmed();

    auto deliverFailure = [this, requestId](const QString &message) {
        m_pendingDeliveries.insert(requestId);
        QTimer::singleShot(0, this, [this, requestId, message] {
            if (m_pendingDeliveries.remove(requestId)) {
                emit branchQueryFailed(requestId, message);
            }
        });
    };

    if (url.isEmpty()) {
        deliverFailure(QStringLiteral("仓库 URL 为空，无法读取远程分支。"));
        return requestId;
    }

    const auto cached = m_cache.constFind(url);
    if (cached != m_cache.constEnd()) {
        const RemoteBranchCatalog catalog = cached.value();
        m_pendingDeliveries.insert(requestId);
        QTimer::singleShot(0, this, [this, requestId, catalog] {
            if (m_pendingDeliveries.remove(requestId)) {
                emit branchesReady(requestId, catalog);
            }
        });
        return requestId;
    }

    const QString gitExecutable = QStandardPaths::findExecutable(QStringLiteral("git"));
    if (gitExecutable.isEmpty()) {
        deliverFailure(QStringLiteral("找不到 Git，无法读取远程分支。"));
        return requestId;
    }

    auto *context = new QueryContext;
    context->repositoryUrl = url;
    context->process = new QProcess(this);
    context->timeout = new QTimer(context->process);
    context->timeout->setSingleShot(true);
    context->timeout->setInterval(m_timeoutMs);
    m_queries.insert(requestId, context);

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("GIT_TERMINAL_PROMPT"), QStringLiteral("0"));
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    context->process->setProcessEnvironment(environment);
    context->process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(context->process, &QProcess::readyReadStandardOutput,
            this, [this, requestId] {
                const auto found = m_queries.find(requestId);
                if (found != m_queries.end()) {
                    found.value()->output.append(found.value()->process->readAllStandardOutput());
                }
            });
    connect(context->process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this, requestId](int exitCode, QProcess::ExitStatus exitStatus) {
                const auto found = m_queries.find(requestId);
                if (found == m_queries.end()) {
                    return;
                }
                found.value()->output.append(found.value()->process->readAllStandardOutput());
                if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                    completeQuery(requestId);
                } else {
                    failQuery(requestId,
                              QStringLiteral("无法读取远程分支，请检查 URL、网络或认证配置。"));
                }
            });
    connect(context->process, &QProcess::errorOccurred,
            this, [this, requestId](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart) {
                    failQuery(requestId, QStringLiteral("Git 分支查询进程无法启动。"));
                }
            });
    connect(context->timeout, &QTimer::timeout, this, [this, requestId] {
        failQuery(requestId, QStringLiteral("读取远程分支超时，请检查网络或认证配置。"));
    });

    context->timeout->start();
    context->process->start(gitExecutable,
                            {QStringLiteral("ls-remote"), QStringLiteral("--symref"),
                             url, QStringLiteral("HEAD"), QStringLiteral("refs/heads/*")});
    return requestId;
}

void GitRemoteBranchService::cancelRequest(RequestId requestId)
{
    if (m_pendingDeliveries.remove(requestId)) {
        return;
    }
    releaseQuery(requestId, true);
}

void GitRemoteBranchService::completeQuery(RequestId requestId)
{
    const auto found = m_queries.find(requestId);
    if (found == m_queries.end()) {
        return;
    }
    QueryContext *context = found.value();
    RemoteBranchCatalog catalog = parseCatalog(context->output);
    orderBranches(catalog);
    m_cache.insert(context->repositoryUrl, catalog);
    releaseQuery(requestId, false);
    emit branchesReady(requestId, catalog);
}

void GitRemoteBranchService::failQuery(RequestId requestId, const QString &message)
{
    if (!m_queries.contains(requestId)) {
        return;
    }
    releaseQuery(requestId, true);
    emit branchQueryFailed(requestId, message);
}

void GitRemoteBranchService::releaseQuery(RequestId requestId, bool stopProcess)
{
    QueryContext *context = m_queries.take(requestId);
    if (context == nullptr) {
        return;
    }
    context->timeout->stop();
    if (stopProcess && context->process->state() != QProcess::NotRunning) {
        context->process->kill();
    }
    context->process->disconnect(this);
    context->process->deleteLater();
    delete context;
}

RemoteBranchCatalog GitRemoteBranchService::parseCatalog(const QByteArray &output)
{
    RemoteBranchCatalog catalog;
    QSet<QString> branches;
    const QList<QByteArray> lines = output.split('\n');
    for (QByteArray line : lines) {
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        const int separator = line.indexOf('\t');
        if (separator < 0) {
            continue;
        }
        const QString left = QString::fromUtf8(line.left(separator));
        const QString right = QString::fromUtf8(line.mid(separator + 1));
        if (left.startsWith(QStringLiteral("ref: refs/heads/"))
            && right == QStringLiteral("HEAD")) {
            catalog.defaultBranch = left.mid(QStringLiteral("ref: refs/heads/").size());
            continue;
        }
        if (right.startsWith(QStringLiteral("refs/heads/"))) {
            branches.insert(right.mid(QStringLiteral("refs/heads/").size()));
        }
    }
    if (!catalog.defaultBranch.isEmpty()) {
        branches.insert(catalog.defaultBranch);
    }
    catalog.branches = branches.values();
    return catalog;
}

void GitRemoteBranchService::orderBranches(RemoteBranchCatalog &catalog)
{
    std::sort(catalog.branches.begin(), catalog.branches.end(),
              [&catalog](const QString &left, const QString &right) {
                  const int leftPriority = branchPriority(left, catalog.defaultBranch);
                  const int rightPriority = branchPriority(right, catalog.defaultBranch);
                  if (leftPriority != rightPriority) {
                      return leftPriority < rightPriority;
                  }
                  const int foldedComparison = QString::compare(left, right, Qt::CaseInsensitive);
                  return foldedComparison == 0 ? left < right : foldedComparison < 0;
              });
}

} // namespace gitclone
