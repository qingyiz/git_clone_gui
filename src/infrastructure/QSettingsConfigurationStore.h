#pragma once

#include "application/ConfigurationStore.h"

#include <QSettings>

namespace gitclone {

class QSettingsConfigurationStore final : public ConfigurationStore {
public:
    QSettingsConfigurationStore(const QString &organization, const QString &application);
    QSettingsConfigurationStore(const QString &fileName, QSettings::Format format);

    std::optional<CloneRequest> load() const override;
    bool save(const CloneRequest &request) override;

private:
    mutable QSettings m_settings;
};

} // namespace gitclone
