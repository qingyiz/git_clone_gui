#pragma once

#include "presentation/NotificationSeverity.h"

#include <QObject>

class QSystemTrayIcon;
class QTimer;

namespace gitclone {

class DesktopNotifier final : public QObject {
    Q_OBJECT

public:
    explicit DesktopNotifier(QObject *parent = nullptr);

public slots:
    void showMessage(const QString &title,
                     const QString &message,
                     gitclone::NotificationSeverity severity);

private:
    QSystemTrayIcon *m_trayIcon;
    QTimer *m_hideTimer;
};

} // namespace gitclone
