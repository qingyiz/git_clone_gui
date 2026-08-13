#include "infrastructure/QSettingsNavigationConfigurationStore.h"

namespace gitclone {
namespace {

constexpr int kNavigationSchemaVersion = 1;

QString storedPageName(NavigationPage page)
{
    return page == NavigationPage::Workspace
        ? QStringLiteral("workspace") : QStringLiteral("clone");
}

} // namespace

QSettingsNavigationConfigurationStore::QSettingsNavigationConfigurationStore(
    const QString &organization, const QString &application)
    : m_settings(organization, application)
{
}

QSettingsNavigationConfigurationStore::QSettingsNavigationConfigurationStore(
    const QString &fileName, QSettings::Format format)
    : m_settings(fileName, format)
{
}

std::optional<NavigationPage> QSettingsNavigationConfigurationStore::loadCurrentPage() const
{
    m_settings.sync();
    if (m_settings.status() != QSettings::NoError
        || m_settings.value(QStringLiteral("navigation/schemaVersion")).toInt()
            != kNavigationSchemaVersion) {
        return std::nullopt;
    }
    const QString page =
        m_settings.value(QStringLiteral("navigation/currentPage")).toString();
    if (page == QStringLiteral("clone")) {
        return NavigationPage::Clone;
    }
    if (page == QStringLiteral("workspace")) {
        return NavigationPage::Workspace;
    }
    return std::nullopt;
}

bool QSettingsNavigationConfigurationStore::saveCurrentPage(NavigationPage page)
{
    m_settings.setValue(QStringLiteral("navigation/schemaVersion"),
                        kNavigationSchemaVersion);
    m_settings.setValue(QStringLiteral("navigation/currentPage"), storedPageName(page));
    m_settings.sync();
    return m_settings.status() == QSettings::NoError;
}

} // namespace gitclone
