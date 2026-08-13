#include "application/WorkspaceService.h"

#include <QSet>

#include <algorithm>

namespace gitclone {
namespace {

bool stableBranchLessThan(const QString &left, const QString &right)
{
    const int folded = QString::compare(left, right, Qt::CaseInsensitive);
    return folded == 0 ? left < right : folded < 0;
}

QString remoteShortName(const QString &remoteBranch)
{
    const int separator = remoteBranch.indexOf(QLatin1Char('/'));
    return separator < 0 ? QString() : remoteBranch.mid(separator + 1);
}

} // namespace

QStringList remoteBranchCandidates(const QStringList &localBranches,
                                   const QStringList &remoteBranches)
{
    QSet<QString> localNames;
    for (const QString &branch : localBranches) {
        localNames.insert(branch);
    }

    QSet<QString> candidates;
    for (const QString &remoteBranch : remoteBranches) {
        const QString shortName = remoteShortName(remoteBranch);
        if (shortName.isEmpty() || shortName == QStringLiteral("HEAD")
            || localNames.contains(shortName)) {
            continue;
        }
        candidates.insert(remoteBranch);
    }

    QStringList result = candidates.values();
    std::sort(result.begin(), result.end(), stableBranchLessThan);
    return result;
}

} // namespace gitclone
