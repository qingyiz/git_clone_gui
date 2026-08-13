#pragma once

#include "application/WorkspaceConfigurationStore.h"

#include <QSettings>

namespace gitclone {

class QSettingsWorkspaceConfigurationStore final : public WorkspaceConfigurationStore {
public:
    QSettingsWorkspaceConfigurationStore(const QString &organization,
                                         const QString &application);
    QSettingsWorkspaceConfigurationStore(const QString &fileName,
                                         QSettings::Format format);

    std::optional<QString> loadRootPath() const override;
    bool saveRootPath(const QString &rootPath) override;

private:
    mutable QSettings m_settings;
};

} // namespace gitclone
