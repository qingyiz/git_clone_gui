#include "presentation/BranchSelector.h"
#include "presentation/ChildRepositoryCard.h"
#include "presentation/AppStyle.h"
#include "presentation/MainWindow.h"
#include "application/NavigationConfigurationStore.h"

#include <QDir>
#include <QApplication>
#include <QCompleter>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QSplitterHandle>
#include <QStackedWidget>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using namespace gitclone;

class DummyProcessRunner final : public ProcessRunner {
public:
    using ProcessRunner::ProcessRunner;
    bool start(const ProcessCommand &) override { return false; }
    bool isRunning() const override { return false; }
    void terminate() override { }
    void kill() override { }
};

class ControllableProcessRunner final : public ProcessRunner {
public:
    using ProcessRunner::ProcessRunner;
    bool start(const ProcessCommand &command) override
    {
        lastCommand = command;
        running = true;
        return true;
    }
    bool isRunning() const override { return running; }
    void terminate() override { running = false; }
    void kill() override { running = false; }
    void complete(int exitCode)
    {
        running = false;
        emit finished(exitCode, true);
    }

    ProcessCommand lastCommand;
    bool running = false;
};

class FakeConfigurationStore final : public ConfigurationStore {
public:
    std::optional<CloneRequest> load() const override { return stored; }
    bool save(const CloneRequest &request) override
    {
        ++saveCalls;
        stored = request;
        return saveSucceeds;
    }

    mutable std::optional<CloneRequest> stored;
    int saveCalls = 0;
    bool saveSucceeds = true;
};

class FakeNavigationConfigurationStore final : public NavigationConfigurationStore {
public:
    std::optional<NavigationPage> loadCurrentPage() const override { return stored; }
    bool saveCurrentPage(NavigationPage page) override
    {
        savedPages.append(page);
        stored = page;
        return true;
    }

    mutable std::optional<NavigationPage> stored;
    QList<NavigationPage> savedPages;
};

class FakeRemoteBranchService final : public RemoteBranchService {
public:
    using RemoteBranchService::RemoteBranchService;

    RequestId requestBranches(const QString &repositoryUrl) override
    {
        const RequestId requestId = ++nextRequestId;
        requestedUrls.append(repositoryUrl);
        requestIds.append(requestId);
        return requestId;
    }

    void cancelRequest(RequestId requestId) override
    {
        cancelledRequests.insert(requestId);
    }

    void complete(RequestId requestId, const RemoteBranchCatalog &catalog)
    {
        emit branchesReady(requestId, catalog);
    }

    void fail(RequestId requestId, const QString &message)
    {
        emit branchQueryFailed(requestId, message);
    }

    RequestId nextRequestId = 0;
    QStringList requestedUrls;
    QList<RequestId> requestIds;
    QSet<RequestId> cancelledRequests;
};

class TestPresentation final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cardRoundTripsConfiguration();
    void cardEmitsChangeAndRemoveSignals();
    void cardRenumbersTitleAndBadge();
    void cardDisablesAllEditors();
    void branchSelectorChoosesDefaultAndFiltersSuggestions();
    void branchSelectorIgnoresStaleResultsAndPreservesManualText();
    void branchSelectorUsesCustomRoundedChrome();
    void childCardRequestsBranchesFromItsUrl();
    void firstLaunchCreatesOneCardAndUsesCompactSize();
    void navigationUsesRestrainedDesktopHierarchy();
    void navigationSwitchesPagesAndPreservesCloneState();
    void navigationRestoresAndPersistsCurrentPage();
    void restoresMultipleCardsWithoutSavingImmediately();
    void restoresSavedZeroCards();
    void addsRemovesAndRenumbersCards();
    void autoSavesChangesAfterDebounce();
    void closeFlushesPendingSave();
    void reportsSaveFailureWithoutBlocking();
    void limitsValidationSummaryToThreeLines();
    void completionResultOverridesPostCloneDirectoryValidation();
    void failureUsesInlineStatus();
    void cancellationDoesNotRequestSystemNotification();
    void gitOutputHasResizableLargeArea();
    void renderSnapshot();
};

void TestPresentation::initTestCase()
{
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.6"));
    qRegisterMetaType<NotificationSeverity>();
}

CloneRequest savedRequest(const QString &destination, int childCount)
{
    CloneRequest request;
    request.parentRepositoryUrl = QStringLiteral("git@github.com:acme/platform.git");
    request.parentBranch = QStringLiteral("main");
    request.parentDirectoryName = QStringLiteral("platform-workspace");
    request.destinationRoot = destination;
    for (int index = 0; index < childCount; ++index) {
        request.children.append({
            QStringLiteral("git@github.com:acme/service-%1.git").arg(index + 1),
            QStringLiteral("develop"),
            QStringLiteral("services/service-%1").arg(index + 1)
        });
    }
    return request;
}

void TestPresentation::cardRoundTripsConfiguration()
{
    ChildRepositoryCard card(0);
    const ChildRepositoryRequest expected {QStringLiteral("https://example.com/a.git"),
                                           QStringLiteral("feature/a"),
                                           QStringLiteral("modules/a")};
    card.setConfiguration(expected);
    const ChildRepositoryRequest actual = card.configuration();
    QCOMPARE(actual.repositoryUrl, expected.repositoryUrl);
    QCOMPARE(actual.branch, expected.branch);
    QCOMPARE(actual.relativePath, expected.relativePath);
}

void TestPresentation::cardEmitsChangeAndRemoveSignals()
{
    ChildRepositoryCard card(0);
    QSignalSpy changeSpy(&card, &ChildRepositoryCard::configurationChanged);
    QSignalSpy removeSpy(&card, &ChildRepositoryCard::removeRequested);

    card.findChild<QLineEdit *>(QStringLiteral("childRepositoryUrlEdit"))->setText(QStringLiteral("x"));
    card.findChild<QPushButton *>(QStringLiteral("removeChildButton"))->click();

    QCOMPARE(changeSpy.size(), 1);
    QCOMPARE(removeSpy.size(), 1);
    QCOMPARE(qvariant_cast<ChildRepositoryCard *>(removeSpy.at(0).at(0)), &card);
}

void TestPresentation::cardRenumbersTitleAndBadge()
{
    ChildRepositoryCard card(0);
    card.setIndex(4);
    QCOMPARE(card.index(), 4);
    QCOMPARE(card.findChild<QLabel *>(QStringLiteral("cardNumberBadge"))->text(), QStringLiteral("5"));
    QCOMPARE(card.findChild<QLabel *>(QStringLiteral("cardTitle"))->text(), QStringLiteral("子仓库 5"));
}

void TestPresentation::cardDisablesAllEditors()
{
    ChildRepositoryCard card(0);
    card.setEditable(false);
    const QList<QLineEdit *> edits = card.findChildren<QLineEdit *>();
    QCOMPARE(edits.size(), 3);
    for (QLineEdit *edit : edits) {
        QVERIFY(!edit->isEnabled());
    }
    QVERIFY(!card.findChild<QPushButton *>(QStringLiteral("removeChildButton"))->isEnabled());
}

void TestPresentation::branchSelectorChoosesDefaultAndFiltersSuggestions()
{
    FakeRemoteBranchService service;
    BranchSelector selector(&service, nullptr, 5);
    QSignalSpy branchSpy(&selector, &BranchSelector::branchChanged);
    selector.setRepositoryUrl(QStringLiteral("https://example.com/repository.git"));
    QTRY_COMPARE_WITH_TIMEOUT(service.requestedUrls.size(), 1, 1000);

    RemoteBranchCatalog catalog;
    catalog.defaultBranch = QStringLiteral("main");
    catalog.branches = QStringList {QStringLiteral("main"), QStringLiteral("develop"),
                                    QStringLiteral("feature/login"),
                                    QStringLiteral("release/1.0")};
    service.complete(service.requestIds.first(), catalog);

    QCOMPARE(selector.branchText(), QStringLiteral("main"));
    QCOMPARE(selector.suggestionCount(), 4);
    QVERIFY(!branchSpy.isEmpty());
    selector.completer()->setCompletionPrefix(QStringLiteral("login"));
    QCOMPARE(selector.completer()->completionModel()->rowCount(), 1);
    QCOMPARE(selector.completer()->completionModel()->index(0, 0).data().toString(),
             QStringLiteral("feature/login"));
    QCOMPARE(selector.completer()->filterMode(), Qt::MatchContains);
}

void TestPresentation::branchSelectorIgnoresStaleResultsAndPreservesManualText()
{
    FakeRemoteBranchService service;
    BranchSelector selector(&service, nullptr, 5);
    selector.setBranchText(QStringLiteral("manual/topic"));
    selector.setRepositoryUrl(QStringLiteral("repository-a"));
    QTRY_COMPARE_WITH_TIMEOUT(service.requestedUrls.size(), 1, 1000);
    const RemoteBranchService::RequestId firstRequest = service.requestIds.last();
    selector.setRepositoryUrl(QStringLiteral("repository-b"));
    QTRY_COMPARE_WITH_TIMEOUT(service.requestedUrls.size(), 2, 1000);
    const RemoteBranchService::RequestId secondRequest = service.requestIds.last();

    service.complete(firstRequest,
                     {QStringLiteral("old"), QStringList {QStringLiteral("old")}});
    QCOMPARE(selector.suggestionCount(), 0);
    QCOMPARE(selector.branchText(), QStringLiteral("manual/topic"));
    QVERIFY(service.cancelledRequests.contains(firstRequest));

    service.complete(secondRequest,
                     {QStringLiteral("main"),
                      QStringList {QStringLiteral("main"), QStringLiteral("feature/new")}});
    QCOMPARE(selector.suggestionCount(), 2);
    QCOMPARE(selector.branchText(), QStringLiteral("manual/topic"));
}

void TestPresentation::branchSelectorUsesCustomRoundedChrome()
{
    BranchSelector selector(nullptr);
    selector.resize(260, 44);
    selector.setBranchText(QStringLiteral("main"));
    selector.addItem(QStringLiteral("main"));
    selector.show();
    QTest::qWait(30);

    const QImage image = selector.grab().toImage();
    QVERIFY(!image.isNull());
    int darkRightEdgePixels = 0;
    for (int y = 4; y < image.height() - 4; ++y) {
        if (image.pixelColor(image.width() - 2, y).lightness() < 100) {
            ++darkRightEdgePixels;
        }
    }
    QCOMPARE(darkRightEdgePixels, 0);

    selector.showPopup();
    QVERIFY(selector.isPopupIndicatorExpanded());
    selector.hidePopup();
    QVERIFY(!selector.isPopupIndicatorExpanded());
}

void TestPresentation::childCardRequestsBranchesFromItsUrl()
{
    FakeRemoteBranchService service;
    ChildRepositoryCard card(0, &service, nullptr);
    card.setConfiguration({QStringLiteral("git@example.com:team/child.git"),
                           QStringLiteral("feature/saved"), QStringLiteral("modules/child")});

    QTRY_COMPARE_WITH_TIMEOUT(service.requestedUrls.size(), 1, 1000);
    QCOMPARE(service.requestedUrls.first(), QStringLiteral("git@example.com:team/child.git"));
    QCOMPARE(card.configuration().branch, QStringLiteral("feature/saved"));
}

void TestPresentation::firstLaunchCreatesOneCardAndUsesCompactSize()
{
    DummyProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    MainWindow window(&controller, &store);

    QCOMPARE(window.childCardCount(), 1);
    QCOMPARE(window.size(), QSize(1160, 780));
    QCOMPARE(window.minimumSize(), QSize(960, 680));
    QVERIFY(window.findChild<QWidget *>(QStringLiteral("configurationPanel")) != nullptr);
    QVERIFY(window.findChild<QWidget *>(QStringLiteral("executionPanel")) != nullptr);
    QCOMPARE(window.findChild<QLabel *>(QStringLiteral("navigationVersionLabel"))->text(),
             QStringLiteral("v0.1.6  ·  本地运行"));
}

void TestPresentation::navigationUsesRestrainedDesktopHierarchy()
{
    DummyProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    MainWindow window(&controller, &store);
    window.show();
    QTest::qWait(30);

    QWidget *sidebar =
        window.findChild<QWidget *>(QStringLiteral("navigationSidebar"));
    QWidget *mark = window.findChild<QWidget *>(QStringLiteral("navigationMark"));
    QLabel *title = window.findChild<QLabel *>(QStringLiteral("appTitle"));
    QVERIFY(sidebar != nullptr);
    QCOMPARE(sidebar->width(), 164);
    QVERIFY(sidebar->width() < 188);
    QVERIFY(mark != nullptr);
    QCOMPARE(mark->size(), QSize(28, 28));
    QVERIFY(qobject_cast<QLabel *>(mark) == nullptr);
    QCOMPARE(mark->property("iconSemantic").toString(), QStringLiteral("gitBranch"));
    QCOMPARE(mark->accessibleName(), QStringLiteral("Git 分支"));

    const QImage markImage = mark->grab().toImage().convertToFormat(QImage::Format_ARGB32);
    QVERIFY(!markImage.isNull());
    int surfacePixels = 0;
    int branchPixels = 0;
    for (int y = 0; y < markImage.height(); ++y) {
        for (int x = 0; x < markImage.width(); ++x) {
            const QColor pixel = markImage.pixelColor(x, y);
            if (pixel.red() >= 225 && pixel.red() <= 245
                && pixel.green() >= 230 && pixel.green() <= 248
                && pixel.blue() >= 235 && pixel.blue() <= 252) {
                ++surfacePixels;
            }
            if (pixel.red() >= 45 && pixel.red() <= 110
                && pixel.green() >= 65 && pixel.green() <= 135
                && pixel.blue() >= 100 && pixel.blue() <= 170
                && pixel.blue() > pixel.red() + 25) {
                ++branchPixels;
            }
        }
    }
    QVERIFY(surfacePixels > 100);
    QVERIFY(branchPixels > 15);
    QVERIFY(title != nullptr);
    QVERIFY(title->font().pixelSize() <= 20);

    const QString style = applicationStyleSheet();
    const qsizetype selectorStart = style.indexOf(
        QStringLiteral("QPushButton[buttonRole=\"navigation\"]:checked"));
    QVERIFY(selectorStart >= 0);
    const qsizetype selectorEnd = style.indexOf(QLatin1Char('}'), selectorStart);
    QVERIFY(selectorEnd > selectorStart);
    const QString checkedRule = style.mid(selectorStart,
                                          selectorEnd - selectorStart);
    QVERIFY(checkedRule.contains(QStringLiteral("background: #E1E4E8")));
    QVERIFY(!checkedRule.contains(QStringLiteral("background: #2F6FEB")));
}

void TestPresentation::navigationSwitchesPagesAndPreservesCloneState()
{
    DummyProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    MainWindow window(&controller, &store);
    QStackedWidget *pages =
        window.findChild<QStackedWidget *>(QStringLiteral("mainPageStack"));
    QPushButton *cloneButton =
        window.findChild<QPushButton *>(QStringLiteral("cloneNavigationButton"));
    QPushButton *workspaceButton =
        window.findChild<QPushButton *>(QStringLiteral("workspaceNavigationButton"));
    QLineEdit *parentDirectory =
        window.findChild<QLineEdit *>(QStringLiteral("parentDirectoryEdit"));

    QCOMPARE(pages->currentIndex(), 0);
    QVERIFY(cloneButton->isChecked());
    QVERIFY(!workspaceButton->isChecked());
    parentDirectory->setText(QStringLiteral("kept-across-pages"));

    workspaceButton->click();
    QCOMPARE(pages->currentIndex(), 1);
    QVERIFY(workspaceButton->isChecked());
    QCOMPARE(pages->currentWidget()->objectName(), QStringLiteral("workspacePage"));

    cloneButton->click();
    QCOMPARE(pages->currentIndex(), 0);
    QVERIFY(cloneButton->isChecked());
    QCOMPARE(parentDirectory->text(), QStringLiteral("kept-across-pages"));
}

void TestPresentation::navigationRestoresAndPersistsCurrentPage()
{
    DummyProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    FakeNavigationConfigurationStore navigationStore;
    navigationStore.stored = NavigationPage::Workspace;
    MainWindow window(&controller, &store, nullptr, nullptr, nullptr,
                      &navigationStore, nullptr);
    QStackedWidget *pages =
        window.findChild<QStackedWidget *>(QStringLiteral("mainPageStack"));
    QPushButton *cloneButton =
        window.findChild<QPushButton *>(QStringLiteral("cloneNavigationButton"));

    QCOMPARE(pages->currentIndex(), 1);
    QCOMPARE(navigationStore.savedPages.last(), NavigationPage::Workspace);
    cloneButton->click();
    QCOMPARE(pages->currentIndex(), 0);
    QCOMPARE(navigationStore.savedPages.last(), NavigationPage::Clone);
}

void TestPresentation::restoresMultipleCardsWithoutSavingImmediately()
{
    QTemporaryDir destination;
    DummyProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    store.stored = savedRequest(destination.path(), 2);
    MainWindow window(&controller, &store);

    QCOMPARE(window.childCardCount(), 2);
    QCOMPARE(store.saveCalls, 0);
    QCOMPARE(window.findChild<QLineEdit *>(QStringLiteral("parentRepositoryUrlEdit"))->text(),
             store.stored->parentRepositoryUrl);
    const QList<ChildRepositoryCard *> cards = window.findChildren<ChildRepositoryCard *>();
    QCOMPARE(cards.size(), 2);
    QCOMPARE(cards.at(1)->configuration().relativePath, QStringLiteral("services/service-2"));
}

void TestPresentation::restoresSavedZeroCards()
{
    QTemporaryDir destination;
    DummyProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    store.stored = savedRequest(destination.path(), 0);
    MainWindow window(&controller, &store);

    QCOMPARE(window.childCardCount(), 0);
}

void TestPresentation::addsRemovesAndRenumbersCards()
{
    DummyProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    MainWindow window(&controller, &store);

    window.findChild<QPushButton *>(QStringLiteral("addChildButton"))->click();
    QCOMPARE(window.childCardCount(), 2);
    QList<ChildRepositoryCard *> cards = window.findChildren<ChildRepositoryCard *>();
    cards.first()->findChild<QPushButton *>(QStringLiteral("removeChildButton"))->click();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    QCOMPARE(window.childCardCount(), 1);
    cards = window.findChildren<ChildRepositoryCard *>();
    QCOMPARE(cards.size(), 1);
    QCOMPARE(cards.first()->index(), 0);
    QCOMPARE(cards.first()->findChild<QLabel *>(QStringLiteral("cardTitle"))->text(),
             QStringLiteral("子仓库 1"));
}

void TestPresentation::autoSavesChangesAfterDebounce()
{
    DummyProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    MainWindow window(&controller, &store);

    window.findChild<QLineEdit *>(QStringLiteral("parentBranchEdit"))->setText(QStringLiteral("feature/saved"));
    QTRY_COMPARE_WITH_TIMEOUT(store.saveCalls, 1, 1000);
    QVERIFY(store.stored.has_value());
    QCOMPARE(store.stored->parentBranch, QStringLiteral("feature/saved"));
}

void TestPresentation::closeFlushesPendingSave()
{
    DummyProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    MainWindow window(&controller, &store);
    window.show();
    window.findChild<QLineEdit *>(QStringLiteral("parentDirectoryEdit"))->setText(QStringLiteral("flush-me"));

    QVERIFY(window.close());

    QCOMPARE(store.saveCalls, 1);
    QVERIFY(store.stored.has_value());
    QCOMPARE(store.stored->parentDirectoryName, QStringLiteral("flush-me"));
}

void TestPresentation::reportsSaveFailureWithoutBlocking()
{
    DummyProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    store.saveSucceeds = false;
    MainWindow window(&controller, &store);
    window.findChild<QLineEdit *>(QStringLiteral("parentBranchEdit"))->setText(QStringLiteral("cannot-save"));

    QTRY_COMPARE_WITH_TIMEOUT(store.saveCalls, 1, 1000);
    QCOMPARE(window.findChild<QLabel *>(QStringLiteral("saveStatusLabel"))->text(),
             QStringLiteral("配置保存失败"));
    QVERIFY(window.findChild<QLineEdit *>(QStringLiteral("parentBranchEdit"))->isEnabled());
}

void TestPresentation::limitsValidationSummaryToThreeLines()
{
    DummyProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    MainWindow window(&controller, &store);

    const QString summary = window.findChild<QLabel *>(QStringLiteral("validationSummary"))->text();
    QVERIFY(summary.split(QLatin1Char('\n')).size() <= 3);
    QVERIFY(summary.contains(QStringLiteral("另有")));
}

void TestPresentation::completionResultOverridesPostCloneDirectoryValidation()
{
    QTemporaryDir destination;
    ControllableProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    store.stored = savedRequest(destination.path(), 0);
    MainWindow window(&controller, &store);
    window.show();
    QSignalSpy notificationSpy(&window, &MainWindow::taskResultNotificationRequested);

    window.findChild<QPushButton *>(QStringLiteral("startButton"))->click();
    QVERIFY(runner.running);
    const QString targetPath = destination.path() + QStringLiteral("/platform-workspace");
    QVERIFY(QDir().mkpath(targetPath));
    QFile marker(targetPath + QStringLiteral("/.git"));
    QVERIFY(marker.open(QIODevice::WriteOnly));
    marker.close();
    runner.complete(0);

    QWidget *statusCard = window.findChild<QWidget *>(QStringLiteral("statusCard"));
    QCOMPARE(statusCard->property("statusState").toString(), QStringLiteral("success"));
    QCOMPARE(window.findChild<QLabel *>(QStringLiteral("validationSummary"))->text(),
             QStringLiteral("✓ 克隆完成"));
    QVERIFY(window.findChild<QLabel *>(QStringLiteral("statusLabel"))->text().contains(targetPath));
    QVERIFY(!window.findChild<QPlainTextEdit *>(QStringLiteral("gitOutputEdit"))->toPlainText().isEmpty());
    QVERIFY(QApplication::activeModalWidget() == nullptr);
    QCOMPARE(notificationSpy.size(), 1);
    const QList<QVariant> notification = notificationSpy.first();
    QCOMPARE(notification.at(0).toString(), QStringLiteral("GitCloneGui · 克隆完成"));
    QVERIFY(notification.at(1).toString().contains(targetPath));
    QVERIFY(notification.at(1).toString().contains(QStringLiteral("0 个子仓库")));
    QCOMPARE(static_cast<int>(qvariant_cast<NotificationSeverity>(notification.at(2))),
             static_cast<int>(NotificationSeverity::Information));
}

void TestPresentation::failureUsesInlineStatus()
{
    QTemporaryDir destination;
    ControllableProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    store.stored = savedRequest(destination.path(), 0);
    MainWindow window(&controller, &store);
    window.show();
    QSignalSpy notificationSpy(&window, &MainWindow::taskResultNotificationRequested);

    window.findChild<QPushButton *>(QStringLiteral("startButton"))->click();
    runner.complete(128);

    QCOMPARE(window.findChild<QWidget *>(QStringLiteral("statusCard"))
                 ->property("statusState").toString(),
             QStringLiteral("error"));
    QCOMPARE(window.findChild<QLabel *>(QStringLiteral("validationSummary"))->text(),
             QStringLiteral("克隆失败"));
    QVERIFY(QApplication::activeModalWidget() == nullptr);
    QCOMPARE(notificationSpy.size(), 1);
    const QList<QVariant> notification = notificationSpy.first();
    QCOMPARE(notification.at(0).toString(), QStringLiteral("GitCloneGui · 克隆失败"));
    QVERIFY(notification.at(1).toString().contains(QStringLiteral("父项目克隆失败")));
    QCOMPARE(static_cast<int>(qvariant_cast<NotificationSeverity>(notification.at(2))),
             static_cast<int>(NotificationSeverity::Critical));
}

void TestPresentation::cancellationDoesNotRequestSystemNotification()
{
    QTemporaryDir destination;
    ControllableProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    store.stored = savedRequest(destination.path(), 0);
    MainWindow window(&controller, &store);
    QSignalSpy notificationSpy(&window, &MainWindow::taskResultNotificationRequested);

    window.findChild<QPushButton *>(QStringLiteral("startButton"))->click();
    QVERIFY(runner.running);
    window.findChild<QPushButton *>(QStringLiteral("cancelButton"))->click();

    QCOMPARE(notificationSpy.size(), 0);
    QCOMPARE(window.findChild<QWidget *>(QStringLiteral("statusCard"))
                 ->property("statusState").toString(),
             QStringLiteral("normal"));
}

void TestPresentation::gitOutputHasResizableLargeArea()
{
    DummyProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    MainWindow window(&controller, &store);
    window.show();
    QTest::qWait(50);

    QSplitter *splitter = window.findChild<QSplitter *>(QStringLiteral("executionSplitter"));
    QWidget *logCard = window.findChild<QWidget *>(QStringLiteral("logCard"));
    QVERIFY(splitter != nullptr);
    QCOMPARE(splitter->count(), 2);
    QVERIFY2(logCard->height() >= 280, qPrintable(QString::number(logCard->height())));
    QVERIFY(splitter->handle(1) != nullptr);
    QVERIFY(splitter->handle(1)->isEnabled());
    QCOMPARE(window.findChild<QPlainTextEdit *>(QStringLiteral("gitOutputEdit"))
                 ->document()->maximumBlockCount(),
             10000);
}

void TestPresentation::renderSnapshot()
{
    const QString snapshotPath = qEnvironmentVariable("GIT_CLONE_GUI_SNAPSHOT");
    if (snapshotPath.isEmpty()) {
        QSKIP("未设置 GIT_CLONE_GUI_SNAPSHOT", 0);
    }

    QTemporaryDir destination;
    DummyProcessRunner runner;
    CloneController controller(&runner);
    FakeConfigurationStore store;
    store.stored = savedRequest(destination.path(), 3);
    MainWindow window(&controller, &store);
    window.show();
    QTest::qWait(250);
    const QPixmap snapshot = window.grab();
    QVERIFY(!snapshot.isNull());
    QVERIFY2(snapshot.save(snapshotPath), qPrintable(snapshotPath));
    QVERIFY(QFileInfo::exists(snapshotPath));
}

QTEST_MAIN(TestPresentation)
#include "TestPresentation.moc"
