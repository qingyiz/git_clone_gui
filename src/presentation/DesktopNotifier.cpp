#include "presentation/DesktopNotifier.h"

#include <QFont>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <QTimer>

namespace gitclone {
namespace {

QIcon notificationIcon()
{
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QStringLiteral("#2563EB")));
    painter.drawRoundedRect(pixmap.rect().adjusted(2, 2, -2, -2), 16, 16);

    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(34);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, QStringLiteral("G"));
    return QIcon(pixmap);
}

} // namespace

DesktopNotifier::DesktopNotifier(QObject *parent)
    : QObject(parent)
    , m_trayIcon(new QSystemTrayIcon(this))
    , m_hideTimer(new QTimer(this))
{
    m_trayIcon->setIcon(notificationIcon());
    m_trayIcon->setToolTip(QStringLiteral("GitCloneGui"));

    m_hideTimer->setSingleShot(true);
    m_hideTimer->setInterval(12000);
    connect(m_hideTimer, &QTimer::timeout, m_trayIcon, &QSystemTrayIcon::hide);
}

void DesktopNotifier::showMessage(const QString &title,
                                  const QString &message,
                                  NotificationSeverity severity)
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()
        || !QSystemTrayIcon::supportsMessages()) {
        return;
    }

    if (!m_trayIcon->isVisible()) {
        m_trayIcon->show();
    }
    const QSystemTrayIcon::MessageIcon icon = severity == NotificationSeverity::Critical
        ? QSystemTrayIcon::Critical
        : QSystemTrayIcon::Information;
    m_trayIcon->showMessage(title, message, icon, 10000);
    m_hideTimer->start();
}

} // namespace gitclone
