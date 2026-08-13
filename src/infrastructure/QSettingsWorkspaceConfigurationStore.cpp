#include "infrastructure/QSettingsWorkspaceConfigurationStore.h"

namespace gitclone {
namespace {

constexpr int kWorkspaceSchemaVersion = 1;

} // namespace

QSettingsWorkspaceConfigurationStore::QSettingsWorkspaceConfigurationStore(
    const QString &organization, const QString &application)
    : m_settings(organization, application)
{
}

QSettingsWorkspaceConfigurationStore::QSettingsWorkspaceConfigurationStore(
    const QString &fileName, QSettings::Format format)
    : m_settings(fileName, format)
{
}

std::optional<QString> QSettingsWorkspaceConfigurationStore::loadRootPath() const
{
    m_settings.sync();
    if (m_settings.status() != QSettings::NoError
        || m_settings.value(QStringLiteral("workspace/schemaVersion")).toInt()
            != kWorkspaceSchemaVersion) {
        return std::nullopt;
    }
    return m_settings.value(QStringLiteral("workspace/rootPath")).toString();
}

bool QSettingsWorkspaceConfigurationStore::saveRootPath(const QString &rootPath)
{
    m_settings.setValue(QStringLiteral("workspace/schemaVersion"),
                        kWorkspaceSchemaVersion);
    m_settings.setValue(QStringLiteral("workspace/rootPath"), rootPath.trimmed());
    m_settings.sync();
    return m_settings.status() == QSettings::NoError;
}

} // namespace gitclone
