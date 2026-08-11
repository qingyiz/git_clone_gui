#include "presentation/ChildRepositoryCard.h"
#include "presentation/MainWindow.h"

#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
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

class TestPresentation final : public QObject {
    Q_OBJECT

private slots:
    void cardRoundTripsConfiguration();
    void cardEmitsChangeAndRemoveSignals();
    void cardRenumbersTitleAndBadge();
    void cardDisablesAllEditors();
    void firstLaunchCreatesOneCardAndUsesCompactSize();
    void restoresMultipleCardsWithoutSavingImmediately();
    void restoresSavedZeroCards();
    void addsRemovesAndRenumbersCards();
    void autoSavesChangesAfterDebounce();
    void closeFlushesPendingSave();
    void reportsSaveFailureWithoutBlocking();
    void limitsValidationSummaryToThreeLines();
    void renderSnapshot();
};

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
