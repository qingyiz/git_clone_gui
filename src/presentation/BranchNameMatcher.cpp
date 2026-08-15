#include "presentation/BranchNameMatcher.h"

#include <QVector>

#include <algorithm>

namespace gitclone {
namespace {

int maximumFuzzyEdits(int queryLength)
{
    if (queryLength < 3) {
        return 0;
    }
    if (queryLength <= 4) {
        return 1;
    }
    if (queryLength <= 8) {
        return 2;
    }
    return 3;
}

int minimumSubstringDistance(const QString &query, const QString &candidate)
{
    QVector<int> previousPrevious(candidate.size() + 1, 0);
    QVector<int> previous(candidate.size() + 1, 0);
    QVector<int> current(candidate.size() + 1, 0);

    for (int queryIndex = 1; queryIndex <= query.size(); ++queryIndex) {
        current[0] = queryIndex;
        for (int candidateIndex = 1; candidateIndex <= candidate.size();
             ++candidateIndex) {
            const int substitutionCost =
                query.at(queryIndex - 1) == candidate.at(candidateIndex - 1) ? 0 : 1;
            current[candidateIndex] = std::min(
                {previous[candidateIndex] + 1,
                 current[candidateIndex - 1] + 1,
                 previous[candidateIndex - 1] + substitutionCost});

            if (queryIndex > 1 && candidateIndex > 1
                && query.at(queryIndex - 1) == candidate.at(candidateIndex - 2)
                && query.at(queryIndex - 2) == candidate.at(candidateIndex - 1)) {
                current[candidateIndex] = std::min(
                    current[candidateIndex],
                    previousPrevious[candidateIndex - 2] + 1);
            }
        }
        previousPrevious.swap(previous);
        previous.swap(current);
    }

    return *std::min_element(previous.cbegin(), previous.cend());
}

} // namespace

bool fuzzyBranchNameMatches(const QString &branchName, const QString &query)
{
    const QString normalizedQuery = query.trimmed().toCaseFolded();
    if (normalizedQuery.isEmpty()) {
        return true;
    }

    const QString normalizedBranch = branchName.toCaseFolded();
    if (normalizedBranch.contains(normalizedQuery)) {
        return true;
    }

    const int maximumEdits = maximumFuzzyEdits(normalizedQuery.size());
    return maximumEdits > 0
        && minimumSubstringDistance(normalizedQuery, normalizedBranch) <= maximumEdits;
}

} // namespace gitclone
