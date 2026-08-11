#include "core/CloneRequest.h"

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

using namespace gitclone;

class TestCloneRequest final : public QObject {
    Q_OBJECT

private slots:
    void acceptsMultipleChildrenInOrder();
    void acceptsParentWithoutChildren();
    void rejectsUnsafeChildPaths_data();
    void rejectsUnsafeChildPaths();
    void rejectsDuplicateChildTargets();
    void identifiesIncompleteChildByIndex();
    void rejectsExistingParentTarget();
    void keepsShellCharactersInSingleArguments();
    void quotesPreviewForDisplay();
};

CloneRequest validRequest(const QString &root)
{
    CloneRequest request;
    request.parentRepositoryUrl = QStringLiteral("git@example.com:team/parent.git");
    request.parentBranch = QStringLiteral("feature/parent");
    request.parentDirectoryName = QStringLiteral("parent-project");
    request.destinationRoot = root;
    request.children = {
        {QStringLiteral("https://example.com/team/child-a.git"),
         QStringLiteral("feature/a"), QStringLiteral("modules/child-a")},
        {QStringLiteral("https://example.com/team/child-b.git"),
         QStringLiteral("feature/b"), QStringLiteral("modules/child-b")}
    };
    return request;
}

void TestCloneRequest::acceptsMultipleChildrenInOrder()
{
    QTemporaryDir root;
    const ValidationResult result = buildClonePlan(validRequest(root.path()));

    QVERIFY2(result.valid, qPrintable(result.errors.join(QLatin1Char('\n'))));
    QCOMPARE(result.plan.parentTargetPath,
             QDir::cleanPath(root.path() + QStringLiteral("/parent-project")));
    QCOMPARE(result.plan.children.size(), 2);
    QCOMPARE(result.plan.children.at(0).targetPath,
             QDir::cleanPath(root.path() + QStringLiteral("/parent-project/modules/child-a")));
    QCOMPARE(result.plan.children.at(1).command.arguments.at(2), QStringLiteral("feature/b"));
}

void TestCloneRequest::acceptsParentWithoutChildren()
{
    QTemporaryDir root;
    CloneRequest request = validRequest(root.path());
    request.children.clear();

    const ValidationResult result = buildClonePlan(request);

    QVERIFY2(result.valid, qPrintable(result.errors.join(QLatin1Char('\n'))));
    QVERIFY(result.plan.children.isEmpty());
}

void TestCloneRequest::rejectsUnsafeChildPaths_data()
{
    QTest::addColumn<QString>("childPath");
    QTest::newRow("unix absolute") << QStringLiteral("/tmp/child");
    QTest::newRow("windows absolute") << QStringLiteral("C:\\temp\\child");
    QTest::newRow("parent") << QStringLiteral("../child");
    QTest::newRow("nested parent") << QStringLiteral("modules/../../child");
    QTest::newRow("same directory") << QStringLiteral(".");
}

void TestCloneRequest::rejectsUnsafeChildPaths()
{
    QFETCH(QString, childPath);
    QTemporaryDir root;
    CloneRequest request = validRequest(root.path());
    request.children[0].relativePath = childPath;

    const ValidationResult result = buildClonePlan(request);

    QVERIFY(!result.valid);
    QVERIFY(result.errors.join(QLatin1Char('\n')).contains(QStringLiteral("子仓库 #1")));
}

void TestCloneRequest::rejectsDuplicateChildTargets()
{
    QTemporaryDir root;
    CloneRequest request = validRequest(root.path());
    request.children[1].relativePath = request.children[0].relativePath;

    const ValidationResult result = buildClonePlan(request);

    QVERIFY(!result.valid);
    QVERIFY(result.errors.join(QLatin1Char('\n')).contains(QStringLiteral("重复")));
}

void TestCloneRequest::identifiesIncompleteChildByIndex()
{
    QTemporaryDir root;
    CloneRequest request = validRequest(root.path());
    request.children[1].branch.clear();

    const ValidationResult result = buildClonePlan(request);

    QVERIFY(!result.valid);
    QVERIFY(result.errors.join(QLatin1Char('\n')).contains(QStringLiteral("子仓库 #2分支")));
}

void TestCloneRequest::rejectsExistingParentTarget()
{
    QTemporaryDir root;
    QVERIFY(QDir(root.path()).mkdir(QStringLiteral("parent-project")));

    const ValidationResult result = buildClonePlan(validRequest(root.path()));

    QVERIFY(!result.valid);
    QVERIFY(result.errors.join(QLatin1Char('\n')).contains(QStringLiteral("已存在")));
}

void TestCloneRequest::keepsShellCharactersInSingleArguments()
{
    QTemporaryDir root;
    CloneRequest request = validRequest(root.path());
    request.parentRepositoryUrl = QStringLiteral("https://example.com/a.git;touch injected");
    request.children[0].branch = QStringLiteral("feature/a && false");

    const ValidationResult result = buildClonePlan(request);

    QVERIFY(result.valid);
    QCOMPARE(result.plan.parentCommand.arguments.at(4), request.parentRepositoryUrl);
    QCOMPARE(result.plan.children.at(0).command.arguments.at(2), request.children.at(0).branch);
    QCOMPARE(result.plan.children.at(0).command.arguments.size(), 6);
}

void TestCloneRequest::quotesPreviewForDisplay()
{
    const ProcessCommand command {QStringLiteral("git"),
                                  {QStringLiteral("clone"), QStringLiteral("a b"), QStringLiteral("x'y")},
                                  {}};
    QCOMPARE(commandPreview(command), QStringLiteral("git clone 'a b' 'x'\\''y'"));
}

QTEST_MAIN(TestCloneRequest)
#include "TestCloneRequest.moc"
