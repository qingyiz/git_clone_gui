#include "infrastructure/QSettingsConfigurationStore.h"

#include <QDir>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

using namespace gitclone;

class TestConfigurationStore final : public QObject {
    Q_OBJECT

private slots:
    void returnsEmptyForFirstLaunch();
    void roundTripsAllFieldsAndOrder();
    void distinguishesSavedZeroChildren();
    void overwritesPreviousChildArray();
    void doesNotPersistRuntimeOrLogKeys();
};

QString settingsFile(const QTemporaryDir &directory)
{
    return QDir(directory.path()).filePath(QStringLiteral("settings.ini"));
}

CloneRequest sampleRequest()
{
    CloneRequest request;
    request.parentRepositoryUrl = QStringLiteral("git@example.com:team/父 项目.git");
    request.parentBranch = QStringLiteral("feature/main && safe");
    request.parentDirectoryName = QStringLiteral("workspace");
    request.destinationRoot = QStringLiteral("/tmp/code root");
    request.children = {
        {QStringLiteral("https://example.com/a.git"), QStringLiteral("main"), QStringLiteral("modules/a")},
        {QStringLiteral("ssh://example.com/b.git"), QStringLiteral("dev/二"), QStringLiteral("plugins/b")}
    };
    return request;
}

void compareRequests(const CloneRequest &actual, const CloneRequest &expected)
{
    QCOMPARE(actual.parentRepositoryUrl, expected.parentRepositoryUrl);
    QCOMPARE(actual.parentBranch, expected.parentBranch);
    QCOMPARE(actual.parentDirectoryName, expected.parentDirectoryName);
    QCOMPARE(actual.destinationRoot, expected.destinationRoot);
    QCOMPARE(actual.children.size(), expected.children.size());
    for (int index = 0; index < actual.children.size(); ++index) {
        QCOMPARE(actual.children.at(index).repositoryUrl, expected.children.at(index).repositoryUrl);
        QCOMPARE(actual.children.at(index).branch, expected.children.at(index).branch);
        QCOMPARE(actual.children.at(index).relativePath, expected.children.at(index).relativePath);
    }
}

void TestConfigurationStore::returnsEmptyForFirstLaunch()
{
    QTemporaryDir directory;
    QSettingsConfigurationStore store(settingsFile(directory), QSettings::IniFormat);
    QVERIFY(!store.load().has_value());
}

void TestConfigurationStore::roundTripsAllFieldsAndOrder()
{
    QTemporaryDir directory;
    QSettingsConfigurationStore store(settingsFile(directory), QSettings::IniFormat);
    const CloneRequest expected = sampleRequest();

    QVERIFY(store.save(expected));
    const std::optional<CloneRequest> actual = store.load();

    QVERIFY(actual.has_value());
    compareRequests(*actual, expected);
}

void TestConfigurationStore::distinguishesSavedZeroChildren()
{
    QTemporaryDir directory;
    QSettingsConfigurationStore store(settingsFile(directory), QSettings::IniFormat);
    CloneRequest expected = sampleRequest();
    expected.children.clear();

    QVERIFY(store.save(expected));
    const std::optional<CloneRequest> actual = store.load();

    QVERIFY(actual.has_value());
    QVERIFY(actual->children.isEmpty());
}

void TestConfigurationStore::overwritesPreviousChildArray()
{
    QTemporaryDir directory;
    QSettingsConfigurationStore store(settingsFile(directory), QSettings::IniFormat);
    CloneRequest expected = sampleRequest();
    QVERIFY(store.save(expected));
    expected.children = {expected.children.first()};
    QVERIFY(store.save(expected));

    const std::optional<CloneRequest> actual = store.load();
    QVERIFY(actual.has_value());
    QCOMPARE(actual->children.size(), 1);
    QCOMPARE(actual->children.first().relativePath, QStringLiteral("modules/a"));
}

void TestConfigurationStore::doesNotPersistRuntimeOrLogKeys()
{
    QTemporaryDir directory;
    const QString fileName = settingsFile(directory);
    QSettingsConfigurationStore store(fileName, QSettings::IniFormat);
    QVERIFY(store.save(sampleRequest()));

    QSettings raw(fileName, QSettings::IniFormat);
    const QStringList keys = raw.allKeys();
    for (const QString &key : keys) {
        QVERIFY(!key.contains(QStringLiteral("log"), Qt::CaseInsensitive));
        QVERIFY(!key.contains(QStringLiteral("token"), Qt::CaseInsensitive));
        QVERIFY(!key.contains(QStringLiteral("password"), Qt::CaseInsensitive));
        QVERIFY(!key.contains(QStringLiteral("running"), Qt::CaseInsensitive));
    }
}

QTEST_GUILESS_MAIN(TestConfigurationStore)
#include "TestConfigurationStore.moc"
