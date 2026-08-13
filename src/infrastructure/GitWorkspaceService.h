#pragma once

#include "application/WorkspaceService.h"
#include "infrastructure/GitWorkspaceScanner.h"

#include <QFutureWatcher>
#include <QList>
#include <QProcess>
#include <QTimer>

#include <atomic>
#include <memory>

namespace gitclone {

class GitWorkspaceService final : public WorkspaceService {
    Q_OBJECT

public:
    explicit GitWorkspaceService(QObject *parent = nullptr);
    ~GitWorkspaceService() override;

    void scan(const QString &rootPath) override;
    void cancelScan() override;
    void loadBranches(const QString &repositoryPath) override;
    void switchBranch(const QString &repositoryPath,
                      const BranchTarget &target) override;
    void cancelGitOperation() override;

private:
    enum class GitOperation {
        None,
        LoadCurrent,
        LoadRefs,
        LoadStatus,
        Switch
    };

    static BranchCatalog parseBranchRefs(const QString &currentBranch,
                                         const QByteArray &output);
    static WorkingTreeStatus parseWorkingTreeStatus(const QByteArray &output);
    void startGit(GitOperation operation, const QStringList &arguments);
    void handleGitFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void failGit(const QString &message);
    void resetGitOperation();

    QList<QFutureWatcher<WorkspaceScanResult> *> m_scanWatchers;
    std::shared_ptr<std::atomic_bool> m_scanCancelled;
    quint64 m_scanGeneration = 0;
    QProcess m_gitProcess;
    QTimer m_gitTimeout;
    GitOperation m_gitOperation = GitOperation::None;
    QString m_repositoryPath;
    QString m_currentBranch;
    BranchCatalog m_pendingCatalog;
    QString m_switchBranchName;
    QByteArray m_gitOutput;
};

} // namespace gitclone
