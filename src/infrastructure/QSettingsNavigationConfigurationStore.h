#pragma once

#include "application/NavigationConfigurationStore.h"

#include <QSettings>

namespace gitclone {

class QSettingsNavigationConfigurationStore final : public NavigationConfigurationStore {
public:
    QSettingsNavigationConfigurationStore(const QString &organization,
                                          const QString &application);
    QSettingsNavigationConfigurationStore(const QString &fileName,
                                          QSettings::Format format);

    std::optional<NavigationPage> loadCurrentPage() const override;
    bool saveCurrentPage(NavigationPage page) override;

private:
    mutable QSettings m_settings;
};

} // namespace gitclone
