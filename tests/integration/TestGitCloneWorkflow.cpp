#include "application/CloneController.h"
#include "infrastructure/GitProcessRunner.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using namespace gitclone;

class TestGitCloneWorkflow final : public QObject {
    Q_OBJECT

private slots:
    void clonesParentAndTwoChildrenInOrder();

private:
    static void runGit(const QStringList &arguments);
    static void createRepository(const QString &path, const QString &branch,
                                 const QString &fileName, const QByteArray &content);
};

void TestGitCloneWorkflow::runGit(const QStringList &arguments)
{
    QProcess git;
    git.setProcessChannelMode(QProcess::MergedChannels);
    git.start(QStringLiteral("git"), arguments);
    QVERIFY2(git.waitForStarted(5000), qPrintable(git.errorString()));
    QVERIFY2(git.waitForFinished(10000), qPrintable(git.errorString()));
    const QByteArray output = git.readAll();
    QVERIFY2(git.exitStatus() == QProcess::NormalExit && git.exitCode() == 0,
             output.constData());
}

void TestGitCloneWorkflow::createRepository(const QString &path, const QString &branch,
                                            const QString &fileName, const QByteArray &content)
{
    runGit({QStringLiteral("init"), QStringLiteral("--initial-branch"), branch, path});
    runGit({QStringLiteral("-C"), path, QStringLiteral("config"),
            QStringLiteral("user.name"), QStringLiteral("GitCloneGui Test")});
    runGit({QStringLiteral("-C"), path, QStringLiteral("config"),
            QStringLiteral("user.email"), QStringLiteral("gitclonegui@example.invalid")});
    QFile file(QDir(path).filePath(fileName));
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(content), content.size());
    file.close();
    runGit({QStringLiteral("-C"), path, QStringLiteral("add"), fileName});
    runGit({QStringLiteral("-C"), path, QStringLiteral("commit"),
            QStringLiteral("-m"), QStringLiteral("test fixture")});
}

void TestGitCloneWorkflow::clonesParentAndTwoChildrenInOrder()
{
    qRegisterMetaType<CloneController::Outcome>();
    QTemporaryDir sandbox;
    QVERIFY(sandbox.isValid());

    const QString parentSource = QDir(sandbox.path()).filePath(QStringLiteral("parent-source"));
    const QString childASource = QDir(sandbox.path()).filePath(QStringLiteral("child-a-source"));
    const QString childBSource = QDir(sandbox.path()).filePath(QStringLiteral("child-b-source"));
    const QString destination = QDir(sandbox.path()).filePath(QStringLiteral("destination"));
    QVERIFY(QDir().mkpath(destination));
    createRepository(parentSource, QStringLiteral("parent-branch"),
                     QStringLiteral("parent.txt"), QByteArray("parent\n"));
    createRepository(childASource, QStringLiteral("child-a-branch"),
                     QStringLiteral("child-a.txt"), QByteArray("child-a\n"));
    createRepository(childBSource, QStringLiteral("child-b-branch"),
                     QStringLiteral("child-b.txt"), QByteArray("child-b\n"));

    CloneRequest request;
    request.parentRepositoryUrl = parentSource;
    request.parentBranch = QStringLiteral("parent-branch");
    request.parentDirectoryName = QStringLiteral("workspace");
    request.destinationRoot = destination;
    request.children = {
        {childASource, QStringLiteral("child-a-branch"), QStringLiteral("modules/child-a")},
        {childBSource, QStringLiteral("child-b-branch"), QStringLiteral("plugins/child-b")}
    };

    GitProcessRunner runner;
    CloneController controller(&runner);
    QSignalSpy resultSpy(&controller, &CloneController::jobFinished);
    QVERIFY(controller.start(request));
    QVERIFY2(resultSpy.wait(15000), "真实 Git 父子队列未在 15 秒内完成");
    QCOMPARE(resultSpy.at(0).at(0).value<CloneController::Outcome>(),
             CloneController::Outcome::Completed);
    QVERIFY(QFileInfo::exists(QDir(destination).filePath(QStringLiteral("workspace/parent.txt"))));
    QVERIFY(QFileInfo::exists(QDir(destination).filePath(QStringLiteral("workspace/modules/child-a/child-a.txt"))));
    QVERIFY(QFileInfo::exists(QDir(destination).filePath(QStringLiteral("workspace/plugins/child-b/child-b.txt"))));
}

QTEST_GUILESS_MAIN(TestGitCloneWorkflow)
#include "TestGitCloneWorkflow.moc"
