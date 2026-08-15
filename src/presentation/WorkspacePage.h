#pragma once

#include "application/WorkspaceService.h"

#include <QWidget>

class QLabel;
class QFrame;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTabWidget;
class QTimer;
class QTreeWidgetItem;

namespace gitclone {

class RepositoryTree;
class WorkspaceConfigurationStore;

class WorkspacePage final : public QWidget {
    Q_OBJECT

public:
    explicit WorkspacePage(WorkspaceService *service, QWidget *parent = nullptr);
    WorkspacePage(WorkspaceService *service,
                  WorkspaceConfigurationStore *configurationStore,
                  QWidget *parent);
    ~WorkspacePage() override;

    void cancelOperations();

private slots:
    void chooseRoot();
    void startScan();
    void handleRepositorySelection();
    void refreshBranches();
    void switchSelectedBranch();
    void handleScanFinished(const QString &rootPath,
                            const QVector<gitclone::RepositoryInfo> &repositories,
                            int skippedDirectories);
    void handleScanFailed(const QString &rootPath, const QString &message);
    void handleBranchesLoaded(const QString &repositoryPath,
                              const gitclone::BranchCatalog &catalog);
    void handleBranchLoadFailed(const QString &repositoryPath, const QString &message);
    void handleBranchSwitchSucceeded(const QString &repositoryPath,
                                     const QString &branchName);
    void handleBranchSwitchFailed(const QString &repositoryPath, const QString &message);

private:
    enum ItemRole {
        RepositoryPathRole = Qt::UserRole + 1,
        BranchKindRole,
        BranchNameRole
    };

    void createUi();
    void connectUi();
    bool restoreRootPath();
    void flushPendingRootPath();
    void setRepositories(const QString &rootPath,
                         const QVector<RepositoryInfo> &repositories);
    void clearBranchDetails(const QString &message);
    void setBranchCatalog(const BranchCatalog &catalog);
    void setWorkingTreeStatus(const WorkingTreeStatus &status);
    void applyBranchFilter();
    void setStatus(const QString &message, const QString &state = QStringLiteral("normal"));
    void updateSwitchButton();
    QListWidget *activeSwitchList() const;

    WorkspaceService *m_service;
    WorkspaceConfigurationStore *m_configurationStore = nullptr;
    QTimer *m_saveTimer = nullptr;
    QLineEdit *m_rootEdit = nullptr;
    QPushButton *m_browseButton = nullptr;
    QPushButton *m_scanButton = nullptr;
    QPushButton *m_cancelScanButton = nullptr;
    QLabel *m_repositoryCountLabel = nullptr;
    RepositoryTree *m_repositoryTree = nullptr;
    QLabel *m_selectedRepositoryLabel = nullptr;
    QLabel *m_currentBranchLabel = nullptr;
    QFrame *m_worktreeStatusCard = nullptr;
    QLabel *m_worktreeStatusTitle = nullptr;
    QLabel *m_worktreeStatusDetails = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLineEdit *m_branchSearch = nullptr;
    QTabWidget *m_branchTabs = nullptr;
    QListWidget *m_localBranches = nullptr;
    QListWidget *m_remoteCandidates = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_switchButton = nullptr;
    QString m_rootPath;
    QString m_selectedRepositoryPath;
    bool m_scanBusy = false;
    bool m_gitBusy = false;
    bool m_rootSavePending = false;
};

} // namespace gitclone
