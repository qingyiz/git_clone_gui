#pragma once

#include <QMetaType>

namespace gitclone {

enum class NotificationSeverity {
    Information,
    Critical
};

} // namespace gitclone

Q_DECLARE_METATYPE(gitclone::NotificationSeverity)
