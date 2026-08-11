#include "infrastructure/GitRemoteBranchService.h"

#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using namespace gitclone;

class TestGitRemoteBranchService final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void readsAndOrdersRemoteBranches();
    void servesRepeatedUrlFromCache();
    void reportsInvalidRepositoryWithoutBlocking();

private:
    static bool runGit(const QString &workingDirectory, const QStringList &arguments);
    static bool createRepository(const QString &path);
};

void TestGitRemoteBranchService::initTestCase()
{
    qRegisterMetaType<RemoteBranchCatalog>();
    qRegisterMetaType<RemoteBranchService::RequestId>(
        "gitclone::RemoteBranchService::RequestId");
}

bool TestGitRemoteBranchService::runGit(const QString &workingDirectory,
                                        const QStringList &arguments)
{
    QProcess process;
    process.setWorkingDirectory(workingDirectory);
    process.start(QStringLiteral("git"), arguments);
    return process.waitForFinished(5000) && process.exitStatus() == QProcess::NormalExit
        && process.exitCode() == 0;
}

bool TestGitRemoteBranchService::createRepository(const QString &path)
{
    if (!runGit({}, {QStringLiteral("init"), QStringLiteral("-b"),
                     QStringLiteral("main"), path})) {
        return false;
    }
    if (!runGit(path, {QStringLiteral("config"), QStringLiteral("user.name"),
                       QStringLiteral("Branch Test")})
        || !runGit(path, {QStringLiteral("config"), QStringLiteral("user.email"),
                          QStringLiteral("branch-test@example.invalid")})) {
        return false;
    }
    QFile marker(path + QStringLiteral("/marker.txt"));
    if (!marker.open(QIODevice::WriteOnly) || marker.write("test\n") < 0) {
        return false;
    }
    marker.close();
    if (!runGit(path, {QStringLiteral("add"), QStringLiteral("marker.txt")})
        || !runGit(path, {QStringLiteral("commit"), QStringLiteral("-m"),
                          QStringLiteral("initial")})) {
        return false;
    }
    const QStringList branches {QStringLiteral("zzz-old"), QStringLiteral("feature/login"),
                                QStringLiteral("release/1.0"), QStringLiteral("develop")};
    for (const QString &branch : branches) {
        if (!runGit(path, {QStringLiteral("branch"), branch})) {
            return false;
        }
    }
    return true;
}

void TestGitRemoteBranchService::readsAndOrdersRemoteBranches()
{
    QTemporaryDir repository;
    QVERIFY(createRepository(repository.path()));
    GitRemoteBranchService service(nullptr, 3000);
    QSignalSpy readySpy(&service, &RemoteBranchService::branchesReady);
    QSignalSpy failureSpy(&service, &RemoteBranchService::branchQueryFailed);

    const RemoteBranchService::RequestId requestId =
        service.requestBranches(repository.path());

    QTRY_COMPARE_WITH_TIMEOUT(readySpy.size(), 1, 5000);
    QCOMPARE(failureSpy.size(), 0);
    QCOMPARE(readySpy.at(0).at(0).toULongLong(), requestId);
    const RemoteBranchCatalog catalog =
        qvariant_cast<RemoteBranchCatalog>(readySpy.at(0).at(1));
    QCOMPARE(catalog.defaultBranch, QStringLiteral("main"));
    QCOMPARE(catalog.branches.first(), QStringLiteral("main"));
    QCOMPARE(catalog.branches.size(), 5);
    QVERIFY(catalog.branches.indexOf(QStringLiteral("develop"))
            < catalog.branches.indexOf(QStringLiteral("zzz-old")));
    QVERIFY(catalog.branches.indexOf(QStringLiteral("release/1.0"))
            < catalog.branches.indexOf(QStringLiteral("zzz-old")));
}

void TestGitRemoteBranchService::servesRepeatedUrlFromCache()
{
    QTemporaryDir repository;
    QVERIFY(createRepository(repository.path()));
    GitRemoteBranchService service(nullptr, 3000);
    QSignalSpy readySpy(&service, &RemoteBranchService::branchesReady);

    service.requestBranches(repository.path());
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.size(), 1, 5000);
    const RemoteBranchService::RequestId cachedRequest =
        service.requestBranches(repository.path());
    QTRY_COMPARE_WITH_TIMEOUT(readySpy.size(), 2, 1000);
    QCOMPARE(readySpy.at(1).at(0).toULongLong(), cachedRequest);
    QCOMPARE(qvariant_cast<RemoteBranchCatalog>(readySpy.at(0).at(1)).branches,
             qvariant_cast<RemoteBranchCatalog>(readySpy.at(1).at(1)).branches);
}

void TestGitRemoteBranchService::reportsInvalidRepositoryWithoutBlocking()
{
    GitRemoteBranchService service(nullptr, 1000);
    QSignalSpy failureSpy(&service, &RemoteBranchService::branchQueryFailed);

    service.requestBranches(QStringLiteral("/path/that/does/not/exist"));

    QTRY_COMPARE_WITH_TIMEOUT(failureSpy.size(), 1, 3000);
    QVERIFY(failureSpy.at(0).at(1).toString().contains(QStringLiteral("无法读取远程分支")));
}

QTEST_MAIN(TestGitRemoteBranchService)
#include "TestGitRemoteBranchService.moc"
