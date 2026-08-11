#include "application/CloneController.h"

#include <QDir>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using namespace gitclone;

class FakeProcessRunner final : public ProcessRunner {
    Q_OBJECT

public:
    using ProcessRunner::ProcessRunner;

    bool start(const ProcessCommand &command) override
    {
        if (running || rejectStart) {
            return false;
        }
        commands.append(command);
        running = true;
        return true;
    }

    bool isRunning() const override { return running; }
    void terminate() override { ++terminateCalls; }
    void kill() override
    {
        ++killCalls;
        running = false;
        emit finished(-1, false);
    }
    void complete(int exitCode, bool normalExit = true)
    {
        running = false;
        emit finished(exitCode, normalExit);
    }
    void fail(const QString &message)
    {
        running = false;
        emit errorOccurred(message);
    }
    void output(const QString &text) { emit outputReceived(text); }

    QList<ProcessCommand> commands;
    bool running = false;
    bool rejectStart = false;
    int terminateCalls = 0;
    int killCalls = 0;
};

class TestCloneController final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void runsParentThenAllChildren();
    void completesAfterParentWithNoChildren();
    void parentFailureDoesNotStartChildren();
    void middleChildFailureStopsQueue();
    void rejectsConcurrentStartAndForwardsOutput();
    void cancelTerminatesAndEventuallyKills();
    void startErrorReturnsToIdle();
};

CloneRequest requestFor(const QString &root, int childCount = 2)
{
    CloneRequest request;
    request.parentRepositoryUrl = QStringLiteral("https://example.com/parent.git");
    request.parentBranch = QStringLiteral("parent-branch");
    request.parentDirectoryName = QStringLiteral("parent");
    request.destinationRoot = root;
    for (int index = 0; index < childCount; ++index) {
        request.children.append({
            QStringLiteral("https://example.com/child-%1.git").arg(index + 1),
            QStringLiteral("child-branch-%1").arg(index + 1),
            QStringLiteral("modules/child-%1").arg(index + 1)
        });
    }
    return request;
}

void TestCloneController::initTestCase()
{
    qRegisterMetaType<CloneController::State>();
    qRegisterMetaType<CloneController::Outcome>();
}

void TestCloneController::runsParentThenAllChildren()
{
    QTemporaryDir root;
    FakeProcessRunner runner;
    CloneController controller(&runner);
    QSignalSpy resultSpy(&controller, &CloneController::jobFinished);
    QSignalSpy statusSpy(&controller, &CloneController::statusChanged);

    QVERIFY(controller.start(requestFor(root.path(), 2)));
    QCOMPARE(runner.commands.size(), 1);
    runner.complete(0);
    QCOMPARE(controller.state(), CloneController::State::CloningChild);
    QCOMPARE(runner.commands.size(), 2);
    QVERIFY(statusSpy.last().at(0).toString().contains(QStringLiteral("1/2")));
    runner.complete(0);
    QCOMPARE(runner.commands.size(), 3);
    QVERIFY(statusSpy.last().at(0).toString().contains(QStringLiteral("2/2")));
    runner.complete(0);

    QCOMPARE(controller.state(), CloneController::State::Idle);
    QCOMPARE(resultSpy.size(), 1);
    QCOMPARE(resultSpy.at(0).at(0).value<CloneController::Outcome>(),
             CloneController::Outcome::Completed);
    QVERIFY(resultSpy.at(0).at(1).toString().contains(QStringLiteral("2 个子仓库")));
}

void TestCloneController::completesAfterParentWithNoChildren()
{
    QTemporaryDir root;
    FakeProcessRunner runner;
    CloneController controller(&runner);
    QSignalSpy resultSpy(&controller, &CloneController::jobFinished);

    QVERIFY(controller.start(requestFor(root.path(), 0)));
    runner.complete(0);

    QCOMPARE(runner.commands.size(), 1);
    QCOMPARE(controller.state(), CloneController::State::Idle);
    QVERIFY(resultSpy.at(0).at(1).toString().contains(QStringLiteral("0 个子仓库")));
}

void TestCloneController::parentFailureDoesNotStartChildren()
{
    QTemporaryDir root;
    FakeProcessRunner runner;
    CloneController controller(&runner);

    QVERIFY(controller.start(requestFor(root.path(), 3)));
    runner.complete(128);

    QCOMPARE(runner.commands.size(), 1);
    QCOMPARE(controller.state(), CloneController::State::Idle);
}

void TestCloneController::middleChildFailureStopsQueue()
{
    QTemporaryDir root;
    FakeProcessRunner runner;
    CloneController controller(&runner);
    QSignalSpy resultSpy(&controller, &CloneController::jobFinished);

    QVERIFY(controller.start(requestFor(root.path(), 3)));
    runner.complete(0);
    runner.complete(0);
    runner.complete(7);

    QCOMPARE(runner.commands.size(), 3);
    QCOMPARE(controller.state(), CloneController::State::Idle);
    QVERIFY(resultSpy.at(0).at(1).toString().contains(QStringLiteral("2/3")));
}

void TestCloneController::rejectsConcurrentStartAndForwardsOutput()
{
    QTemporaryDir root;
    FakeProcessRunner runner;
    CloneController controller(&runner);
    QSignalSpy outputSpy(&controller, &CloneController::logReceived);

    QVERIFY(controller.start(requestFor(root.path())));
    QVERIFY(!controller.start(requestFor(root.path())));
    runner.output(QStringLiteral("remote: test\n"));

    QCOMPARE(runner.commands.size(), 1);
    QVERIFY(outputSpy.last().at(0).toString().contains(QStringLiteral("remote: test")));
}

void TestCloneController::cancelTerminatesAndEventuallyKills()
{
    QTemporaryDir root;
    FakeProcessRunner runner;
    CloneController controller(&runner, nullptr, 10);
    QSignalSpy resultSpy(&controller, &CloneController::jobFinished);

    QVERIFY(controller.start(requestFor(root.path())));
    controller.cancel();

    QCOMPARE(controller.state(), CloneController::State::Cancelling);
    QCOMPARE(runner.terminateCalls, 1);
    QTRY_COMPARE_WITH_TIMEOUT(runner.killCalls, 1, 1000);
    QCOMPARE(controller.state(), CloneController::State::Idle);
    QCOMPARE(resultSpy.at(0).at(0).value<CloneController::Outcome>(),
             CloneController::Outcome::Cancelled);
}

void TestCloneController::startErrorReturnsToIdle()
{
    QTemporaryDir root;
    FakeProcessRunner runner;
    CloneController controller(&runner);

    QVERIFY(controller.start(requestFor(root.path())));
    runner.fail(QStringLiteral("failed to start"));

    QCOMPARE(controller.state(), CloneController::State::Idle);
    QCOMPARE(runner.commands.size(), 1);
}

QTEST_MAIN(TestCloneController)
#include "TestCloneController.moc"
