#include "infrastructure/QSettingsConfigurationStore.h"

namespace gitclone {
namespace {

constexpr int kSchemaVersion = 1;

} // namespace

QSettingsConfigurationStore::QSettingsConfigurationStore(const QString &organization,
                                                         const QString &application)
    : m_settings(organization, application)
{
}

QSettingsConfigurationStore::QSettingsConfigurationStore(const QString &fileName,
                                                         QSettings::Format format)
    : m_settings(fileName, format)
{
}

std::optional<CloneRequest> QSettingsConfigurationStore::load() const
{
    m_settings.sync();
    if (m_settings.status() != QSettings::NoError
        || m_settings.value(QStringLiteral("schemaVersion")).toInt() != kSchemaVersion) {
        return std::nullopt;
    }

    CloneRequest request;
    request.parentRepositoryUrl = m_settings.value(QStringLiteral("parent/url")).toString();
    request.parentBranch = m_settings.value(QStringLiteral("parent/branch")).toString();
    request.parentDirectoryName = m_settings.value(QStringLiteral("parent/directory")).toString();
    request.destinationRoot = m_settings.value(QStringLiteral("destinationRoot")).toString();

    const int childCount = m_settings.beginReadArray(QStringLiteral("children"));
    for (int index = 0; index < childCount; ++index) {
        m_settings.setArrayIndex(index);
        request.children.append({
            m_settings.value(QStringLiteral("url")).toString(),
            m_settings.value(QStringLiteral("branch")).toString(),
            m_settings.value(QStringLiteral("path")).toString()
        });
    }
    m_settings.endArray();
    return request;
}

bool QSettingsConfigurationStore::save(const CloneRequest &request)
{
    m_settings.setValue(QStringLiteral("schemaVersion"), kSchemaVersion);
    m_settings.setValue(QStringLiteral("parent/url"), request.parentRepositoryUrl);
    m_settings.setValue(QStringLiteral("parent/branch"), request.parentBranch);
    m_settings.setValue(QStringLiteral("parent/directory"), request.parentDirectoryName);
    m_settings.setValue(QStringLiteral("destinationRoot"), request.destinationRoot);

    m_settings.beginWriteArray(QStringLiteral("children"), request.children.size());
    for (int index = 0; index < request.children.size(); ++index) {
        m_settings.setArrayIndex(index);
        const ChildRepositoryRequest &child = request.children.at(index);
        m_settings.setValue(QStringLiteral("url"), child.repositoryUrl);
        m_settings.setValue(QStringLiteral("branch"), child.branch);
        m_settings.setValue(QStringLiteral("path"), child.relativePath);
    }
    m_settings.endArray();
    m_settings.sync();
    return m_settings.status() == QSettings::NoError;
}

} // namespace gitclone
