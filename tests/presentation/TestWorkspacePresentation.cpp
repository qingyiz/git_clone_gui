#include "application/WorkspaceService.h"
#include "application/WorkspaceConfigurationStore.h"
#include "presentation/BranchNameMatcher.h"
#include "presentation/WorkspacePage.h"
#include "presentation/AppStyle.h"
#include "presentation/RepositoryTree.h"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFrame>
#include <QPixmap>
#include <QSignalSpy>
#include <QScrollBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QMouseEvent>
#include <QtTest>

using namespace gitclone;

class FakeWorkspaceService final : public WorkspaceService {
public:
    using WorkspaceService::WorkspaceService;

    void scan(const QString &rootPath) override
    {
        scannedRoots.append(rootPath);
        emit scanBusyChanged(true);
    }
    void cancelScan() override { ++cancelScanCalls; }
    void loadBranches(const QString &repositoryPath) override
    {
        loadedRepositories.append(repositoryPath);
        emit gitBusyChanged(true);
    }
    void switchBranch(const QString &repositoryPath, const BranchTarget &target) override
    {
        switchedRepository = repositoryPath;
        switchedTarget = target;
        emit gitBusyChanged(true);
    }
    void cancelGitOperation() override { ++cancelGitCalls; }

    QStringList scannedRoots;
    QStringList loadedRepositories;
    QString switchedRepository;
    BranchTarget switchedTarget;
    int cancelScanCalls = 0;
    int cancelGitCalls = 0;
};

class FakeWorkspaceConfigurationStore final : public WorkspaceConfigurationStore {
public:
    std::optional<QString> loadRootPath() const override { return loadedRootPath; }
    bool saveRootPath(const QString &rootPath) override
    {
        savedRootPaths.append(rootPath);
        return saveSucceeds;
    }

    std::optional<QString> loadedRootPath;
    QStringList savedRootPaths;
    bool saveSucceeds = true;
};

class TestWorkspacePresentation final : public QObject {
    Q_OBJECT

private slots:
    void scansAndBuildsNestedRepositoryTree();
    void restoresAndPersistsWorkspaceRootWithoutAutomaticScan();
    void automaticallyScansValidRestoredWorkspaceOnce();
    void reportsWorkspaceRootSaveFailureWithoutClearingInput();
    void repositoryTreeUsesSemanticIconsAndUnifiedRows();
    void loadsAndSwitchesLocalAndRemoteBranches();
    void branchNameMatcherToleratesBoundedTypos();
    void filtersThousandBranchListsWithoutGitRequests();
    void renderWorkspaceSnapshot();
};

namespace {

int visibleItemCount(const QListWidget *list)
{
    int visible = 0;
    for (int row = 0; row < list->count(); ++row) {
        if (!list->item(row)->isHidden()) {
            ++visible;
        }
    }
    return visible;
}

} // namespace

void TestWorkspacePresentation::restoresAndPersistsWorkspaceRootWithoutAutomaticScan()
{
    FakeWorkspaceService service;
    FakeWorkspaceConfigurationStore store;
    store.loadedRootPath = QStringLiteral("/workspace/restored");
    WorkspacePage page(&service, &store, nullptr);
    QLineEdit *rootEdit = page.findChild<QLineEdit *>(QStringLiteral("workspaceRootEdit"));

    QCOMPARE(rootEdit->text(), QDir::toNativeSeparators(QStringLiteral("/workspace/restored")));
    QVERIFY(service.scannedRoots.isEmpty());
    QVERIFY(store.savedRootPaths.isEmpty());

    rootEdit->setText(QStringLiteral("/workspace/stale"));
    rootEdit->setText(QStringLiteral("/workspace/latest"));
    QTRY_COMPARE_WITH_TIMEOUT(store.savedRootPaths.size(), 1, 1000);
    QCOMPARE(store.savedRootPaths.last(), QStringLiteral("/workspace/latest"));
    QVERIFY(service.scannedRoots.isEmpty());
}

void TestWorkspacePresentation::automaticallyScansValidRestoredWorkspaceOnce()
{
    QTemporaryDir temporary;
    QVERIFY(temporary.isValid());
    FakeWorkspaceService service;
    FakeWorkspaceConfigurationStore store;
    store.loadedRootPath = temporary.path();
    WorkspacePage page(&service, &store, nullptr);

    QTRY_COMPARE_WITH_TIMEOUT(service.scannedRoots.size(), 1, 1000);
    QCOMPARE(service.scannedRoots.first(), QDir::toNativeSeparators(temporary.path()));
    QVERIFY(store.savedRootPaths.isEmpty());
    QTest::qWait(50);
    QCOMPARE(service.scannedRoots.size(), 1);
}

void TestWorkspacePresentation::reportsWorkspaceRootSaveFailureWithoutClearingInput()
{
    FakeWorkspaceService service;
    FakeWorkspaceConfigurationStore store;
    store.saveSucceeds = false;
    WorkspacePage page(&service, &store, nullptr);
    QLineEdit *rootEdit = page.findChild<QLineEdit *>(QStringLiteral("workspaceRootEdit"));
    QLabel *status = page.findChild<QLabel *>(QStringLiteral("workspaceStatusLabel"));

    rootEdit->setText(QStringLiteral("/workspace/usable"));
    QTRY_COMPARE_WITH_TIMEOUT(store.savedRootPaths.size(), 1, 1000);
    QCOMPARE(rootEdit->text(), QStringLiteral("/workspace/usable"));
    QVERIFY(status->text().contains(QStringLiteral("保存失败")));
    QCOMPARE(status->property("statusState").toString(), QStringLiteral("error"));
}

void TestWorkspacePresentation::scansAndBuildsNestedRepositoryTree()
{
    FakeWorkspaceService service;
    WorkspacePage page(&service);
    QLineEdit *rootEdit = page.findChild<QLineEdit *>(QStringLiteral("workspaceRootEdit"));
    QPushButton *scanButton =
        page.findChild<QPushButton *>(QStringLiteral("workspaceScanButton"));
    QTreeWidget *tree =
        page.findChild<QTreeWidget *>(QStringLiteral("workspaceRepositoryTree"));

    rootEdit->setText(QStringLiteral("/workspace"));
    scanButton->click();
    QCOMPARE(service.scannedRoots, QStringList({QStringLiteral("/workspace")}));
    QVERIFY(!scanButton->isEnabled());

    QVector<RepositoryInfo> repositories {
        {QStringLiteral("/workspace"), QStringLiteral(".")},
        {QStringLiteral("/workspace/apps/api"), QStringLiteral("apps/api")},
        {QStringLiteral("/workspace/apps/api/plugins/auth"),
         QStringLiteral("apps/api/plugins/auth")}
    };
    emit service.scanBusyChanged(false);
    emit service.scanFinished(QStringLiteral("/workspace"), repositories, 2);

    QCOMPARE(tree->topLevelItemCount(), 1);
    QTreeWidgetItem *root = tree->topLevelItem(0);
    QCOMPARE(root->text(0), QStringLiteral("workspace"));
    QCOMPARE(root->data(0, RepositoryNodeKindRole).toInt(),
             static_cast<int>(RepositoryNodeKind::Repository));
    QCOMPARE(root->childCount(), 1);
    QCOMPARE(page.findChild<QLabel *>(QStringLiteral("workspaceRepositoryCount"))->text(),
             QStringLiteral("3 个"));
    QVERIFY(page.findChild<QLabel *>(QStringLiteral("workspaceStatusLabel"))->text()
                .contains(QStringLiteral("跳过 2 个")));
}

void TestWorkspacePresentation::repositoryTreeUsesSemanticIconsAndUnifiedRows()
{
    FakeWorkspaceService service;
    WorkspacePage page(&service);
    page.setStyleSheet(applicationStyleSheet());
    page.resize(972, 780);
    page.show();
    emit service.scanFinished(
        QStringLiteral("/workspace"),
        QVector<RepositoryInfo> {
            {QStringLiteral("/workspace/apps/api"), QStringLiteral("apps/api")},
            {QStringLiteral("/workspace/apps/api/plugins/auth"),
             QStringLiteral("apps/api/plugins/auth")}},
        0);

    auto *tree = page.findChild<RepositoryTree *>(
        QStringLiteral("workspaceRepositoryTree"));
    QVERIFY(tree != nullptr);
    QTreeWidgetItem *root = tree->topLevelItem(0);
    QTreeWidgetItem *apps = root->child(0);
    QTreeWidgetItem *api = apps->child(0);
    QCOMPARE(root->data(0, RepositoryNodeKindRole).toInt(),
             static_cast<int>(RepositoryNodeKind::Root));
    QCOMPARE(apps->data(0, RepositoryNodeKindRole).toInt(),
             static_cast<int>(RepositoryNodeKind::Directory));
    QCOMPARE(api->data(0, RepositoryNodeKindRole).toInt(),
             static_cast<int>(RepositoryNodeKind::Repository));
    QVERIFY(!root->text(0).contains(QStringLiteral("◆")));
    QVERIFY(!api->text(0).contains(QStringLiteral("◆")));

    tree->expandAll();
    QTest::qWait(30);
    const QModelIndex apiIndex = tree->modelIndexForItem(api);
    QVERIFY(tree->sizeHintForIndex(apiIndex).height() >= 38);
    QVERIFY(tree->verticalScrollBar()->sizeHint().width() <= 8);

    tree->setCurrentItem(api);
    QTest::qWait(20);
    const QImage image = tree->viewport()->grab().toImage();
    const QRect row = tree->visualItemRect(api);
    QVERIFY(!image.isNull());
    const qreal deviceScale = image.devicePixelRatio();
    const QColor left = image.pixelColor(qRound(8 * deviceScale),
                                         qRound(row.center().y() * deviceScale));
    const QColor middle = image.pixelColor(qRound(row.center().x() * deviceScale),
                                           qRound(row.center().y() * deviceScale));
    QVERIFY(left.blue() > left.red());
    QVERIFY(middle.blue() > middle.red());
    QVERIFY(qAbs(left.red() - middle.red()) < 20);
    QVERIFY(qAbs(left.green() - middle.green()) < 20);
    QVERIFY(qAbs(left.blue() - middle.blue()) < 20);

    tree->scrollToItem(root, QAbstractItemView::PositionAtTop);
    QTest::qWait(20);
    const QRect chevron = tree->chevronRectForItem(root);
    QVERIFY(chevron.isValid());
    const bool initiallyExpanded = root->isExpanded();
    const int loadsBeforeChevron = service.loadedRepositories.size();
    QVERIFY(tree->toggleExpansionAt(chevron.center()));
    QCOMPARE(root->isExpanded(), !initiallyExpanded);
    QCOMPARE(service.loadedRepositories.size(), loadsBeforeChevron);
}

void TestWorkspacePresentation::loadsAndSwitchesLocalAndRemoteBranches()
{
    FakeWorkspaceService service;
    WorkspacePage page(&service);
    QTreeWidget *tree =
        page.findChild<QTreeWidget *>(QStringLiteral("workspaceRepositoryTree"));
    emit service.scanFinished(
        QStringLiteral("/workspace"),
        QVector<RepositoryInfo> {{QStringLiteral("/workspace/project"),
                                  QStringLiteral("project")}},
        0);
    QTreeWidgetItem *repository = tree->topLevelItem(0)->child(0);
    tree->setCurrentItem(repository);
    QCOMPARE(service.loadedRepositories.last(), QStringLiteral("/workspace/project"));

    BranchCatalog catalog;
    catalog.currentBranch = QStringLiteral("main");
    catalog.localBranches = QStringList({QStringLiteral("develop"),
                                         QStringLiteral("main")});
    catalog.remoteBranches = QStringList({QStringLiteral("origin/main"),
                                          QStringLiteral("origin/feature/new")});
    catalog.remoteCandidates = QStringList({QStringLiteral("origin/feature/new")});
    emit service.gitBusyChanged(false);
    emit service.branchesLoaded(QStringLiteral("/workspace/project"), catalog);

    QListWidget *local =
        page.findChild<QListWidget *>(QStringLiteral("workspaceLocalBranches"));
    QListWidget *remote =
        page.findChild<QListWidget *>(QStringLiteral("workspaceRemoteCandidates"));
    QTabWidget *tabs =
        page.findChild<QTabWidget *>(QStringLiteral("workspaceBranchTabs"));
    QPushButton *switchButton =
        page.findChild<QPushButton *>(QStringLiteral("workspaceSwitchButton"));
    QCOMPARE(local->count(), 2);
    QCOMPARE(remote->count(), 1);
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->tabText(0), QStringLiteral("本地分支"));
    QCOMPARE(tabs->tabText(1), QStringLiteral("远端待跟踪"));
    QVERIFY(page.findChild<QListWidget *>(QStringLiteral("workspaceAllRemoteBranches"))
            == nullptr);
    QVERIFY(page.findChild<QLabel *>(QStringLiteral("workspaceCurrentBranch"))->text()
                .contains(QStringLiteral("main")));
    QFrame *worktreeCard =
        page.findChild<QFrame *>(QStringLiteral("workspaceWorktreeStatusCard"));
    QCOMPARE(worktreeCard->property("worktreeState").toString(), QStringLiteral("clean"));
    QVERIFY(page.findChild<QLabel *>(QStringLiteral("workspaceWorktreeStatusTitle"))->text()
                .contains(QStringLiteral("工作区干净")));

    catalog.workingTreeStatus.stagedChanges = 2;
    catalog.workingTreeStatus.unstagedChanges = 1;
    catalog.workingTreeStatus.untrackedFiles = 3;
    emit service.branchesLoaded(QStringLiteral("/workspace/project"), catalog);
    QCOMPARE(worktreeCard->property("worktreeState").toString(), QStringLiteral("dirty"));
    QVERIFY(page.findChild<QLabel *>(QStringLiteral("workspaceWorktreeStatusTitle"))->text()
                .contains(QStringLiteral("未提交改动")));
    const QString warning =
        page.findChild<QLabel *>(QStringLiteral("workspaceWorktreeStatusDetails"))->text();
    QVERIFY(warning.contains(QStringLiteral("已暂存 2")));
    QVERIFY(warning.contains(QStringLiteral("未暂存 1")));
    QVERIFY(warning.contains(QStringLiteral("未跟踪 3")));
    QVERIFY(warning.contains(QStringLiteral("谨慎")));

    local->setCurrentRow(0);
    QVERIFY(switchButton->isEnabled());
    switchButton->click();
    QCOMPARE(service.switchedRepository, QStringLiteral("/workspace/project"));
    QCOMPARE(static_cast<int>(service.switchedTarget.kind),
             static_cast<int>(BranchTarget::Kind::Local));
    QCOMPARE(service.switchedTarget.name, QStringLiteral("develop"));

    emit service.gitBusyChanged(false);
    tabs->setCurrentWidget(remote);
    remote->setCurrentRow(0);
    switchButton->click();
    QCOMPARE(static_cast<int>(service.switchedTarget.kind),
             static_cast<int>(BranchTarget::Kind::Remote));
    QCOMPARE(service.switchedTarget.name, QStringLiteral("origin/feature/new"));
}

void TestWorkspacePresentation::filtersThousandBranchListsWithoutGitRequests()
{
    FakeWorkspaceService service;
    WorkspacePage page(&service);
    QTreeWidget *tree =
        page.findChild<QTreeWidget *>(QStringLiteral("workspaceRepositoryTree"));
    emit service.scanFinished(
        QStringLiteral("/workspace"),
        QVector<RepositoryInfo> {{QStringLiteral("/workspace/project"),
                                  QStringLiteral("project")}},
        0);
    tree->setCurrentItem(tree->topLevelItem(0)->child(0));
    QCOMPARE(service.loadedRepositories.size(), 1);

    BranchCatalog catalog;
    catalog.currentBranch = QStringLiteral("feature/local-0000");
    catalog.localBranches.reserve(1000);
    catalog.remoteCandidates.reserve(1000);
    for (int index = 0; index < 999; ++index) {
        catalog.localBranches.append(
            QStringLiteral("feature/local-%1").arg(index, 4, 10, QLatin1Char('0')));
        catalog.remoteCandidates.append(
            QStringLiteral("origin/feature/remote-%1")
                .arg(index, 4, 10, QLatin1Char('0')));
    }
    catalog.localBranches.append(QStringLiteral("HotFix/TARGET-Local"));
    catalog.remoteCandidates.append(QStringLiteral("upstream/TARGET-Remote"));
    emit service.gitBusyChanged(false);
    emit service.branchesLoaded(QStringLiteral("/workspace/project"), catalog);

    QLineEdit *search =
        page.findChild<QLineEdit *>(QStringLiteral("workspaceBranchSearch"));
    QListWidget *local =
        page.findChild<QListWidget *>(QStringLiteral("workspaceLocalBranches"));
    QListWidget *remote =
        page.findChild<QListWidget *>(QStringLiteral("workspaceRemoteCandidates"));
    QTabWidget *tabs =
        page.findChild<QTabWidget *>(QStringLiteral("workspaceBranchTabs"));
    QPushButton *switchButton =
        page.findChild<QPushButton *>(QStringLiteral("workspaceSwitchButton"));
    QCOMPARE(local->count(), 1000);
    QCOMPARE(remote->count(), 1000);

    const int gitCallsBeforeFilter = service.loadedRepositories.size();
    QElapsedTimer timer;
    timer.start();
    search->setText(QStringLiteral(" targat "));
    const qint64 elapsedMilliseconds = timer.elapsed();
    QVERIFY2(elapsedMilliseconds <= 250,
             qPrintable(QStringLiteral("千级分支筛选耗时 %1ms")
                            .arg(elapsedMilliseconds)));
    QCOMPARE(service.loadedRepositories.size(), gitCallsBeforeFilter);
    QCOMPARE(visibleItemCount(local), 1);
    QCOMPARE(visibleItemCount(remote), 1);

    local->setCurrentRow(999);
    QVERIFY(switchButton->isEnabled());
    search->setText(QStringLiteral("no-such-branch"));
    QCOMPARE(visibleItemCount(local), 0);
    QCOMPARE(visibleItemCount(remote), 0);
    QVERIFY(!switchButton->isEnabled());

    search->setText(QStringLiteral("TARGTE"));
    tabs->setCurrentWidget(remote);
    QCOMPARE(visibleItemCount(remote), 1);
    remote->setCurrentRow(999);
    QVERIFY(switchButton->isEnabled());
    switchButton->click();
    QCOMPARE(static_cast<int>(service.switchedTarget.kind),
             static_cast<int>(BranchTarget::Kind::Remote));
    QCOMPARE(service.switchedTarget.name, QStringLiteral("upstream/TARGET-Remote"));

    emit service.gitBusyChanged(false);
    emit service.branchesLoaded(QStringLiteral("/workspace/project"), catalog);
    QCOMPARE(search->text(), QStringLiteral("TARGTE"));
    QCOMPARE(visibleItemCount(local), 1);
    QCOMPARE(visibleItemCount(remote), 1);
    search->clear();
    QCOMPARE(visibleItemCount(local), 1000);
    QCOMPARE(visibleItemCount(remote), 1000);
    QCOMPARE(service.loadedRepositories.size(), gitCallsBeforeFilter);
}

void TestWorkspacePresentation::branchNameMatcherToleratesBoundedTypos()
{
    struct MatchCase {
        QString branchName;
        QString query;
        bool expected;
    };
    const QVector<MatchCase> cases {
        {QStringLiteral("feature/dashboard"), QStringLiteral(" DASHBOARD "), true},
        {QStringLiteral("main"), QStringLiteral("ma"), true},
        {QStringLiteral("main"), QStringLiteral("mi"), false},
        {QStringLiteral("main"), QStringLiteral("man"), true},
        {QStringLiteral("main"), QStringLiteral("mian"), true},
        {QStringLiteral("main"), QStringLiteral("mxxn"), false},
        {QStringLiteral("feature"), QStringLiteral("feture"), true},
        {QStringLiteral("abcdef"), QStringLiteral("abcxdef"), true},
        {QStringLiteral("abcdef"), QStringLiteral("abqdef"), true},
        {QStringLiteral("abcdef"), QStringLiteral("abdcef"), true},
        {QStringLiteral("abcdef"), QStringLiteral("abxyef"), true},
        {QStringLiteral("abcdef"), QStringLiteral("abxyzf"), false},
        {QStringLiteral("origin/feature/dashboard"),
         QStringLiteral("feture/dashbord"), true},
        {QStringLiteral("abcdefghijkl"), QStringLiteral("abcdWXYhijkl"), true},
        {QStringLiteral("abcdefghijkl"), QStringLiteral("abcdWXYZijkl"), false},
        {QStringLiteral("anything"), QString(), true}
    };

    for (const MatchCase &matchCase : cases) {
        QCOMPARE(fuzzyBranchNameMatches(matchCase.branchName, matchCase.query),
                 matchCase.expected);
    }
}

void TestWorkspacePresentation::renderWorkspaceSnapshot()
{
    const QString snapshotPath = qEnvironmentVariable("GIT_CLONE_GUI_WORKSPACE_SNAPSHOT");
    if (snapshotPath.isEmpty()) {
        QSKIP("未设置 GIT_CLONE_GUI_WORKSPACE_SNAPSHOT", 0);
    }

    FakeWorkspaceService service;
    WorkspacePage page(&service);
    page.setStyleSheet(applicationStyleSheet());
    page.resize(972, 780);
    page.show();
    emit service.scanFinished(
        QStringLiteral("/Users/me/work"),
        QVector<RepositoryInfo> {
            {QStringLiteral("/Users/me/work/platform"), QStringLiteral("platform")},
            {QStringLiteral("/Users/me/work/platform/services/account"),
             QStringLiteral("platform/services/account")},
            {QStringLiteral("/Users/me/work/platform/services/billing"),
             QStringLiteral("platform/services/billing")},
            {QStringLiteral("/Users/me/work/ai/video_factory"),
             QStringLiteral("ai/video_factory")},
            {QStringLiteral("/Users/me/work/ai/model_runner"),
             QStringLiteral("ai/model_runner")},
            {QStringLiteral("/Users/me/work/tools/automation"),
             QStringLiteral("tools/automation")},
            {QStringLiteral("/Users/me/work/tools/formatters"),
             QStringLiteral("tools/formatters")},
            {QStringLiteral("/Users/me/work/apps/desktop"),
             QStringLiteral("apps/desktop")},
            {QStringLiteral("/Users/me/work/apps/mobile"),
             QStringLiteral("apps/mobile")},
            {QStringLiteral("/Users/me/work/libraries/network"),
             QStringLiteral("libraries/network")},
            {QStringLiteral("/Users/me/work/libraries/storage"),
             QStringLiteral("libraries/storage")},
            {QStringLiteral("/Users/me/work/examples/demo-a"),
             QStringLiteral("examples/demo-a")},
            {QStringLiteral("/Users/me/work/examples/demo-b"),
             QStringLiteral("examples/demo-b")},
            {QStringLiteral("/Users/me/work/plugins/auth"),
             QStringLiteral("plugins/auth")},
            {QStringLiteral("/Users/me/work/plugins/analytics"),
             QStringLiteral("plugins/analytics")},
            {QStringLiteral("/Users/me/work/infra/deploy"),
             QStringLiteral("infra/deploy")},
            {QStringLiteral("/Users/me/work/docs/site"),
             QStringLiteral("docs/site")}},
        0);
    QTreeWidget *tree =
        page.findChild<QTreeWidget *>(QStringLiteral("workspaceRepositoryTree"));
    tree->expandAll();
    QTreeWidgetItem *platform = tree->topLevelItem(0)->child(0);
    for (int index = 0; index < tree->topLevelItem(0)->childCount(); ++index) {
        QTreeWidgetItem *candidate = tree->topLevelItem(0)->child(index);
        if (candidate->text(0) == QStringLiteral("platform")) {
            platform = candidate;
            break;
        }
    }
    tree->setCurrentItem(platform);
    emit service.gitBusyChanged(false);
    BranchCatalog catalog;
    catalog.currentBranch = QStringLiteral("main");
    catalog.localBranches = QStringList({QStringLiteral("develop"),
                                         QStringLiteral("feature/dashboard"),
                                         QStringLiteral("main")});
    catalog.remoteBranches = QStringList({QStringLiteral("origin/main"),
                                          QStringLiteral("origin/feature/new-ui"),
                                          QStringLiteral("upstream/release/2.0")});
    catalog.remoteCandidates = QStringList({QStringLiteral("origin/feature/new-ui"),
                                            QStringLiteral("upstream/release/2.0")});
    catalog.workingTreeStatus.stagedChanges = 2;
    catalog.workingTreeStatus.unstagedChanges = 1;
    catalog.workingTreeStatus.untrackedFiles = 3;
    emit service.branchesLoaded(QStringLiteral("/Users/me/work/platform"), catalog);
    QTest::qWait(120);
    const QPixmap snapshot = page.grab();
    QVERIFY(!snapshot.isNull());
    QVERIFY2(snapshot.save(snapshotPath), qPrintable(snapshotPath));
    QVERIFY(QFileInfo::exists(snapshotPath));
}

QTEST_MAIN(TestWorkspacePresentation)

#include "TestWorkspacePresentation.moc"
