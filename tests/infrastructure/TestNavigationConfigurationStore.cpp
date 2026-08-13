#include "infrastructure/QSettingsNavigationConfigurationStore.h"

#include <QDir>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

using namespace gitclone;

class TestNavigationConfigurationStore final : public QObject {
    Q_OBJECT

private slots:
    void returnsNoValueForMissingOrUnknownConfiguration();
    void roundTripsBothPages();
};

void TestNavigationConfigurationStore::returnsNoValueForMissingOrUnknownConfiguration()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString path = QDir(temporary.path()).filePath(QStringLiteral("settings.ini"));
    QSettingsNavigationConfigurationStore store(path, QSettings::IniFormat);
    QVERIFY(!store.loadCurrentPage().has_value());

    QSettings settings(path, QSettings::IniFormat);
    settings.setValue(QStringLiteral("navigation/schemaVersion"), 1);
    settings.setValue(QStringLiteral("navigation/currentPage"), QStringLiteral("unknown"));
    settings.sync();
    QVERIFY(!store.loadCurrentPage().has_value());
}

void TestNavigationConfigurationStore::roundTripsBothPages()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    QSettingsNavigationConfigurationStore store(
        QDir(temporary.path()).filePath(QStringLiteral("settings.ini")),
        QSettings::IniFormat);

    QVERIFY(store.saveCurrentPage(NavigationPage::Workspace));
    QCOMPARE(store.loadCurrentPage().value(), NavigationPage::Workspace);
    QVERIFY(store.saveCurrentPage(NavigationPage::Clone));
    QCOMPARE(store.loadCurrentPage().value(), NavigationPage::Clone);
}

QTEST_APPLESS_MAIN(TestNavigationConfigurationStore)

#include "TestNavigationConfigurationStore.moc"
