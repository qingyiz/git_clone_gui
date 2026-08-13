#include "application/CloneController.h"
#include "infrastructure/GitProcessRunner.h"
#include "infrastructure/GitRemoteBranchService.h"
#include "infrastructure/QSettingsConfigurationStore.h"
#include "infrastructure/QSettingsNavigationConfigurationStore.h"
#include "infrastructure/QSettingsWorkspaceConfigurationStore.h"
#include "infrastructure/GitWorkspaceService.h"
#include "presentation/DesktopNotifier.h"
#include "presentation/MainWindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("GitCloneGui"));
    QCoreApplication::setApplicationVersion(QStringLiteral(GIT_CLONE_GUI_VERSION));
    QCoreApplication::setOrganizationName(QStringLiteral("明日工具"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("mingr.tools"));

    gitclone::GitProcessRunner runner;
    gitclone::GitRemoteBranchService branchService;
    gitclone::GitWorkspaceService workspaceService;
    gitclone::CloneController controller(&runner);
    gitclone::QSettingsConfigurationStore configurationStore(
        QCoreApplication::organizationName(), QCoreApplication::applicationName());
    gitclone::QSettingsWorkspaceConfigurationStore workspaceConfigurationStore(
        QCoreApplication::organizationName(), QCoreApplication::applicationName());
    gitclone::QSettingsNavigationConfigurationStore navigationConfigurationStore(
        QCoreApplication::organizationName(), QCoreApplication::applicationName());
    gitclone::DesktopNotifier desktopNotifier;
    gitclone::MainWindow window(&controller, &configurationStore, &branchService,
                                &workspaceService, &workspaceConfigurationStore,
                                &navigationConfigurationStore, nullptr);
    QObject::connect(&window,
                     &gitclone::MainWindow::taskResultNotificationRequested,
                     &desktopNotifier,
                     &gitclone::DesktopNotifier::showMessage);
    window.show();

    return application.exec();
}
