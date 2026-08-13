#include "infrastructure/GitWorkspaceService.h"

#include <QDir>
#include <QFile>
#include <QElapsedTimer>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <algorithm>

using namespace gitclone;

namespace {

bool runGit(const QString &workingDirectory, const QStringList &arguments)
{
    QProcess process;
    process.setWorkingDirectory(workingDirectory);
    process.start(QStringLiteral("git"), arguments);
    return process.waitForFinished(10000) && process.exitCode() == 0;
}

void initializeRepository(const QString &path)
{
    QVERIFY(QDir().mkpath(path));
    QVERIFY(runGit(path, {QStringLiteral("init"), QStringLiteral("-q"),
                          QStringLiteral("--initial-branch=main")}));
    QVERIFY(runGit(path, {QStringLiteral("config"), QStringLiteral("user.email"),
                          QStringLiteral("test@example.com")}));
    QVERIFY(runGit(path, {QStringLiteral("config"), QStringLiteral("user.name"),
                          QStringLiteral("Test User")}));
    QFile file(QDir(path).filePath(QStringLiteral("README.md")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("test\n");
    file.close();
    QVERIFY(runGit(path, {QStringLiteral("add"), QStringLiteral("README.md")}));
    QVERIFY(runGit(path, {QStringLiteral("commit"), QStringLiteral("-qm"),
                          QStringLiteral("initial")}));
}

} // namespace

class TestGitWorkspaceService final : public QObject {
    Q_OBJECT

private slots:
    void scansRootNestedAndGitFileRepositories();
    void scansTenThousandDirectoriesWithStableResults();
    void newerScanSupersedesOlderResult();
    void benchmarksConfiguredWorkspace();
    void loadsBranchesAndSwitchesLocalBranch();
    void loadsCleanAndDirtyWorkingTreeStatus();
    void detectsConflictedWorkingTreeStatus();
    void switchesRemoteTrackingBranch();
};

void TestGitWorkspaceService::benchmarksConfiguredWorkspace()
{
    const QString rootPath = qEnvironmentVariable("GIT_CLONE_GUI_SCAN_BENCHMARK_ROOT");
    if (rootPath.isEmpty()) {
        QSKIP("未设置 GIT_CLONE_GUI_SCAN_BENCHMARK_ROOT", 0);
    }
    GitWorkspaceService service;
    QSignalSpy resultSpy(&service, &WorkspaceService::scanFinished);
    QElapsedTimer timer;
    timer.start();
    service.scan(rootPath);
    QVERIFY(resultSpy.wait(120000));
    const qint64 elapsedMilliseconds = timer.elapsed();
    const QVector<RepositoryInfo> repositories =
        qvariant_cast<QVector<RepositoryInfo>>(resultSpy.first().at(1));
    qInfo().noquote() << QStringLiteral("SCAN_BENCHMARK_MS=%1 REPOSITORIES=%2")
                             .arg(elapsedMilliseconds)
                             .arg(repositories.size());
}

void TestGitWorkspaceService::scansRootNestedAndGitFileRepositories()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    initializeRepository(temporary.path());
    initializeRepository(QDir(temporary.path()).filePath(QStringLiteral("apps/child")));
    const QString linkedShape = QDir(temporary.path()).filePath(QStringLiteral("worktree-shape"));
    QVERIFY(QDir().mkpath(linkedShape));
    QFile marker(QDir(linkedShape).filePath(QStringLiteral(".git")));
    QVERIFY(marker.open(QIODevice::WriteOnly));
    marker.write("gitdir: ../missing\n");
    marker.close();

    GitWorkspaceService service;
    QSignalSpy resultSpy(&service, &WorkspaceService::scanFinished);
    service.scan(temporary.path());
    QVERIFY(resultSpy.wait(10000));
    const QVector<RepositoryInfo> repositories =
        qvariant_cast<QVector<RepositoryInfo>>(resultSpy.first().at(1));
    QCOMPARE(repositories.size(), 3);
    QCOMPARE(repositories.at(0).relativePath, QStringLiteral("."));
    QCOMPARE(repositories.at(1).relativePath, QStringLiteral("apps/child"));
    QCOMPARE(repositories.at(2).relativePath, QStringLiteral("worktree-shape"));
}

void TestGitWorkspaceService::scansTenThousandDirectoriesWithStableResults()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    for (int group = 0; group < 100; ++group) {
        for (int item = 0; item < 100; ++item) {
            const QString path = QDir(temporary.path()).filePath(
                QStringLiteral("group-%1/item-%2").arg(group, 3, 10, QLatin1Char('0'))
                    .arg(item, 3, 10, QLatin1Char('0')));
            QVERIFY(QDir().mkpath(path));
        }
    }
    initializeRepository(QDir(temporary.path()).filePath(
        QStringLiteral("group-025/item-050/project")));
    initializeRepository(QDir(temporary.path()).filePath(
        QStringLiteral("group-075/item-010/project")));

    GitWorkspaceService service;
    QList<qint64> durations;
    for (int run = 0; run < 3; ++run) {
        QSignalSpy resultSpy(&service, &WorkspaceService::scanFinished);
        QElapsedTimer timer;
        timer.start();
        service.scan(temporary.path());
        QVERIFY(resultSpy.wait(20000));
        durations.append(timer.elapsed());
        const QVector<RepositoryInfo> repositories =
            qvariant_cast<QVector<RepositoryInfo>>(resultSpy.first().at(1));
        QCOMPARE(repositories.size(), 2);
        QVERIFY(repositories.at(0).relativePath < repositories.at(1).relativePath);
    }
    std::sort(durations.begin(), durations.end());
    qInfo() << "SCAN_10000_MEDIAN_MS=" << durations.at(1);
    QVERIFY2(durations.at(1) <= 1500,
             qPrintable(QStringLiteral("10,000 目录扫描中位数为 %1 ms")
                            .arg(durations.at(1))));
}

void TestGitWorkspaceService::newerScanSupersedesOlderResult()
{
    QTemporaryDir largeRoot;
    QTemporaryDir currentRoot;
    QVERIFY(largeRoot.isValid());
    QVERIFY(currentRoot.isValid());
    for (int index = 0; index < 2000; ++index) {
        QVERIFY(QDir().mkpath(QDir(largeRoot.path()).filePath(
            QStringLiteral("deep/%1/child").arg(index, 4, 10, QLatin1Char('0')))));
    }
    initializeRepository(QDir(currentRoot.path()).filePath(QStringLiteral("current")));

    GitWorkspaceService service;
    QSignalSpy resultSpy(&service, &WorkspaceService::scanFinished);
    service.scan(largeRoot.path());
    service.scan(currentRoot.path());
    QVERIFY(resultSpy.wait(10000));
    QCOMPARE(resultSpy.size(), 1);
    QCOMPARE(resultSpy.first().at(0).toString(), QDir(currentRoot.path()).absolutePath());
    const QVector<RepositoryInfo> repositories =
        qvariant_cast<QVector<RepositoryInfo>>(resultSpy.first().at(1));
    QCOMPARE(repositories.size(), 1);
    QCOMPARE(repositories.first().relativePath, QStringLiteral("current"));
    QTest::qWait(100);
    QCOMPARE(resultSpy.size(), 1);
}

void TestGitWorkspaceService::loadsBranchesAndSwitchesLocalBranch()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    initializeRepository(temporary.path());
    QVERIFY(runGit(temporary.path(), {QStringLiteral("branch"), QStringLiteral("develop")}));

    GitWorkspaceService service;
    QSignalSpy branchSpy(&service, &WorkspaceService::branchesLoaded);
    service.loadBranches(temporary.path());
    QVERIFY(branchSpy.wait(10000));
    BranchCatalog catalog = qvariant_cast<BranchCatalog>(branchSpy.first().at(1));
    QCOMPARE(catalog.currentBranch, QStringLiteral("main"));
    QCOMPARE(catalog.localBranches,
             QStringList({QStringLiteral("develop"), QStringLiteral("main")}));

    QSignalSpy switchSpy(&service, &WorkspaceService::branchSwitchSucceeded);
    BranchTarget target;
    target.kind = BranchTarget::Kind::Local;
    target.name = QStringLiteral("develop");
    service.switchBranch(temporary.path(), target);
    QVERIFY(switchSpy.wait(10000));
    QCOMPARE(switchSpy.first().at(1).toString(), QStringLiteral("develop"));
    QVERIFY(runGit(temporary.path(), {QStringLiteral("branch"), QStringLiteral("--show-current")}));
}

void TestGitWorkspaceService::loadsCleanAndDirtyWorkingTreeStatus()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    initializeRepository(temporary.path());

    GitWorkspaceService service;
    QSignalSpy branchSpy(&service, &WorkspaceService::branchesLoaded);
    service.loadBranches(temporary.path());
    QVERIFY(branchSpy.wait(10000));
    BranchCatalog catalog = qvariant_cast<BranchCatalog>(branchSpy.takeFirst().at(1));
    QVERIFY(!catalog.workingTreeStatus.hasChanges());

    QFile staged(QDir(temporary.path()).filePath(QStringLiteral("staged.txt")));
    QVERIFY(staged.open(QIODevice::WriteOnly));
    staged.write("staged\n");
    staged.close();
    QVERIFY(runGit(temporary.path(), {QStringLiteral("add"), QStringLiteral("staged.txt")}));
    QFile readme(QDir(temporary.path()).filePath(QStringLiteral("README.md")));
    QVERIFY(readme.open(QIODevice::Append));
    readme.write("unstaged\n");
    readme.close();
    QFile untracked(QDir(temporary.path()).filePath(QStringLiteral("untracked.txt")));
    QVERIFY(untracked.open(QIODevice::WriteOnly));
    untracked.write("untracked\n");
    untracked.close();

    service.loadBranches(temporary.path());
    QVERIFY(branchSpy.wait(10000));
    catalog = qvariant_cast<BranchCatalog>(branchSpy.takeFirst().at(1));
    QCOMPARE(catalog.workingTreeStatus.stagedChanges, 1);
    QCOMPARE(catalog.workingTreeStatus.unstagedChanges, 1);
    QCOMPARE(catalog.workingTreeStatus.untrackedFiles, 1);
    QCOMPARE(catalog.workingTreeStatus.conflicts, 0);
    QVERIFY(catalog.workingTreeStatus.hasChanges());
}

void TestGitWorkspaceService::detectsConflictedWorkingTreeStatus()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    initializeRepository(temporary.path());
    QVERIFY(runGit(temporary.path(), {QStringLiteral("switch"), QStringLiteral("-c"),
                                      QStringLiteral("conflict-source")}));
    QFile source(QDir(temporary.path()).filePath(QStringLiteral("README.md")));
    QVERIFY(source.open(QIODevice::WriteOnly | QIODevice::Truncate));
    source.write("source\n");
    source.close();
    QVERIFY(runGit(temporary.path(), {QStringLiteral("commit"), QStringLiteral("-qam"),
                                      QStringLiteral("source change")}));
    QVERIFY(runGit(temporary.path(), {QStringLiteral("switch"), QStringLiteral("main")}));
    QFile target(QDir(temporary.path()).filePath(QStringLiteral("README.md")));
    QVERIFY(target.open(QIODevice::WriteOnly | QIODevice::Truncate));
    target.write("target\n");
    target.close();
    QVERIFY(runGit(temporary.path(), {QStringLiteral("commit"), QStringLiteral("-qam"),
                                      QStringLiteral("target change")}));
    QVERIFY(!runGit(temporary.path(), {QStringLiteral("merge"),
                                       QStringLiteral("conflict-source")}));

    GitWorkspaceService service;
    QSignalSpy branchSpy(&service, &WorkspaceService::branchesLoaded);
    service.loadBranches(temporary.path());
    QVERIFY(branchSpy.wait(10000));
    const BranchCatalog catalog =
        qvariant_cast<BranchCatalog>(branchSpy.takeFirst().at(1));
    QCOMPARE(catalog.workingTreeStatus.conflicts, 1);
    QVERIFY(catalog.workingTreeStatus.hasChanges());
}

void TestGitWorkspaceService::switchesRemoteTrackingBranch()
{
    QTemporaryDir remote;
    QTemporaryDir working;
    QVERIFY(remote.isValid());
    QVERIFY(working.isValid());
    QVERIFY(runGit(remote.path(), {QStringLiteral("init"), QStringLiteral("-q"),
                                   QStringLiteral("--bare")}));
    initializeRepository(working.path());
    QVERIFY(runGit(working.path(), {QStringLiteral("remote"), QStringLiteral("add"),
                                    QStringLiteral("origin"), remote.path()}));
    QVERIFY(runGit(working.path(), {QStringLiteral("push"), QStringLiteral("-q"),
                                    QStringLiteral("origin"), QStringLiteral("main")}));
    QVERIFY(runGit(working.path(), {QStringLiteral("branch"), QStringLiteral("feature/remote")}));
    QVERIFY(runGit(working.path(), {QStringLiteral("push"), QStringLiteral("-q"),
                                    QStringLiteral("origin"), QStringLiteral("feature/remote")}));
    QVERIFY(runGit(working.path(), {QStringLiteral("branch"), QStringLiteral("-D"),
                                    QStringLiteral("feature/remote")}));

    GitWorkspaceService service;
    QSignalSpy branchSpy(&service, &WorkspaceService::branchesLoaded);
    service.loadBranches(working.path());
    QVERIFY(branchSpy.wait(10000));
    const BranchCatalog catalog = qvariant_cast<BranchCatalog>(branchSpy.first().at(1));
    QVERIFY(catalog.remoteCandidates.contains(QStringLiteral("origin/feature/remote")));
    QVERIFY(!catalog.remoteCandidates.contains(QStringLiteral("origin/main")));

    QSignalSpy switchSpy(&service, &WorkspaceService::branchSwitchSucceeded);
    BranchTarget target;
    target.kind = BranchTarget::Kind::Remote;
    target.name = QStringLiteral("origin/feature/remote");
    service.switchBranch(working.path(), target);
    QVERIFY(switchSpy.wait(10000));
    QCOMPARE(switchSpy.first().at(1).toString(), target.name);

    QSignalSpy refreshedSpy(&service, &WorkspaceService::branchesLoaded);
    service.loadBranches(working.path());
    QVERIFY(refreshedSpy.wait(10000));
    const BranchCatalog refreshed = qvariant_cast<BranchCatalog>(refreshedSpy.first().at(1));
    QCOMPARE(refreshed.currentBranch, QStringLiteral("feature/remote"));
    QVERIFY(!refreshed.remoteCandidates.contains(QStringLiteral("origin/feature/remote")));
}

QTEST_GUILESS_MAIN(TestGitWorkspaceService)

#include "TestGitWorkspaceService.moc"
