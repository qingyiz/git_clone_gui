#pragma once

#include "application/CloneController.h"
#include "application/ConfigurationStore.h"
#include "application/ProcessRunner.h"
#include "presentation/NotificationSeverity.h"

#include <QMainWindow>

class QCloseEvent;
class QPushButton;
class QStackedWidget;
class QWidget;

namespace gitclone {

class ClonePage;
class NavigationConfigurationStore;
class RemoteBranchService;
class WorkspacePage;
class WorkspaceService;
class WorkspaceConfigurationStore;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(CloneController *controller,
                        ConfigurationStore *configurationStore,
                        QWidget *parent = nullptr);
    MainWindow(CloneController *controller,
               ConfigurationStore *configurationStore,
               RemoteBranchService *branchService,
               QWidget *parent);
    MainWindow(CloneController *controller,
               ConfigurationStore *configurationStore,
               RemoteBranchService *branchService,
               WorkspaceService *workspaceService,
               QWidget *parent);
    MainWindow(CloneController *controller,
               ConfigurationStore *configurationStore,
               RemoteBranchService *branchService,
               WorkspaceService *workspaceService,
               WorkspaceConfigurationStore *workspaceConfigurationStore,
               QWidget *parent);
    MainWindow(CloneController *controller,
               ConfigurationStore *configurationStore,
               RemoteBranchService *branchService,
               WorkspaceService *workspaceService,
               WorkspaceConfigurationStore *workspaceConfigurationStore,
               NavigationConfigurationStore *navigationConfigurationStore,
               QWidget *parent);

    int childCardCount() const;

signals:
    void taskResultNotificationRequested(const QString &title,
                                         const QString &message,
                                         gitclone::NotificationSeverity severity);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void createUi();
    void selectPage(int index);

    ClonePage *m_clonePage = nullptr;
    WorkspacePage *m_workspacePage = nullptr;
    QStackedWidget *m_pages = nullptr;
    QPushButton *m_cloneNavigationButton = nullptr;
    QPushButton *m_workspaceNavigationButton = nullptr;
    NavigationConfigurationStore *m_navigationConfigurationStore = nullptr;
    bool m_closePending = false;
};

} // namespace gitclone
