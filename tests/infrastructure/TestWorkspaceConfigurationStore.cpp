#include "infrastructure/QSettingsWorkspaceConfigurationStore.h"

#include <QDir>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

using namespace gitclone;

class TestWorkspaceConfigurationStore final : public QObject {
    Q_OBJECT

private slots:
    void returnsNoValueBeforeFirstSave();
    void roundTripsAndOverwritesRootPath();
};

void TestWorkspaceConfigurationStore::returnsNoValueBeforeFirstSave()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    QSettingsWorkspaceConfigurationStore store(
        QDir(temporary.path()).filePath(QStringLiteral("settings.ini")),
        QSettings::IniFormat);

    QVERIFY(!store.loadRootPath().has_value());
}

void TestWorkspaceConfigurationStore::roundTripsAndOverwritesRootPath()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    const QString settingsPath =
        QDir(temporary.path()).filePath(QStringLiteral("settings.ini"));
    QSettingsWorkspaceConfigurationStore store(settingsPath, QSettings::IniFormat);

    QVERIFY(store.saveRootPath(QStringLiteral("  /workspace/one  ")));
    QCOMPARE(store.loadRootPath().value(), QStringLiteral("/workspace/one"));
    QVERIFY(store.saveRootPath(QStringLiteral("/workspace/two")));

    QSettingsWorkspaceConfigurationStore reloaded(settingsPath, QSettings::IniFormat);
    QCOMPARE(reloaded.loadRootPath().value(), QStringLiteral("/workspace/two"));

    QSettings settings(settingsPath, QSettings::IniFormat);
    QCOMPARE(settings.value(QStringLiteral("workspace/schemaVersion")).toInt(), 1);
    QCOMPARE(settings.value(QStringLiteral("workspace/rootPath")).toString(),
             QStringLiteral("/workspace/two"));
    QVERIFY(!settings.contains(QStringLiteral("workspace/repositories")));
}

QTEST_APPLESS_MAIN(TestWorkspaceConfigurationStore)

#include "TestWorkspaceConfigurationStore.moc"
