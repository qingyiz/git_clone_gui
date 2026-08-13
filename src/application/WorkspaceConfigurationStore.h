#pragma once

#include <QString>

#include <optional>

namespace gitclone {

class WorkspaceConfigurationStore {
public:
    virtual ~WorkspaceConfigurationStore() = default;

    virtual std::optional<QString> loadRootPath() const = 0;
    virtual bool saveRootPath(const QString &rootPath) = 0;
};

} // namespace gitclone
