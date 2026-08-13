#include "application/WorkspaceService.h"

#include <QtTest>

using namespace gitclone;

class TestWorkspaceService final : public QObject {
    Q_OBJECT

private slots:
    void remoteCandidatesUseShortNameDifference();
    void remoteCandidatesPreserveDistinctRemotesAndStableOrder();
};

void TestWorkspaceService::remoteCandidatesUseShortNameDifference()
{
    const QStringList candidates = remoteBranchCandidates(
        {QStringLiteral("main"), QStringLiteral("feature/local")},
        {QStringLiteral("origin/HEAD"), QStringLiteral("origin/main"),
         QStringLiteral("origin/feature/local"), QStringLiteral("origin/feature/new")});

    QCOMPARE(candidates, QStringList({QStringLiteral("origin/feature/new")}));
}

void TestWorkspaceService::remoteCandidatesPreserveDistinctRemotesAndStableOrder()
{
    const QStringList candidates = remoteBranchCandidates(
        {},
        {QStringLiteral("upstream/feature/x"), QStringLiteral("origin/zeta"),
         QStringLiteral("origin/feature/x"), QStringLiteral("invalid"),
         QStringLiteral("origin/feature/x")});

    QCOMPARE(candidates,
             QStringList({QStringLiteral("origin/feature/x"), QStringLiteral("origin/zeta"),
                          QStringLiteral("upstream/feature/x")}));
}

QTEST_APPLESS_MAIN(TestWorkspaceService)

#include "TestWorkspaceService.moc"
