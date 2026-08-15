#pragma once

#include <QString>

namespace gitclone {

bool fuzzyBranchNameMatches(const QString &branchName, const QString &query);

} // namespace gitclone
