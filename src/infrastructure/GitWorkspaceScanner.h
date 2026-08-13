#pragma once

#include "application/WorkspaceService.h"

#include <atomic>
#include <memory>

namespace gitclone {

struct WorkspaceScanResult {
    QString rootPath;
    QVector<RepositoryInfo> repositories;
    int skippedDirectories = 0;
    QString error;
    bool cancelled = false;
};

WorkspaceScanResult scanWorkspaceDirectories(
    const QString &rootPath,
    const std::shared_ptr<std::atomic_bool> &cancelled);

} // namespace gitclone
