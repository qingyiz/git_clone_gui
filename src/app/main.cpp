#include "application/CloneController.h"
#include "infrastructure/GitProcessRunner.h"
#include "infrastructure/QSettingsConfigurationStore.h"
#include "presentation/MainWindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("GitCloneGui"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("明日工具"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("mingr.tools"));

    gitclone::GitProcessRunner runner;
    gitclone::CloneController controller(&runner);
    gitclone::QSettingsConfigurationStore configurationStore(
        QCoreApplication::organizationName(), QCoreApplication::applicationName());
    gitclone::MainWindow window(&controller, &configurationStore);
    window.show();

    return application.exec();
}
