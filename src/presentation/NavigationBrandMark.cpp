#include "presentation/NavigationBrandMark.h"

#include <QPainter>
#include <QPainterPath>
#include <QVariant>

namespace gitclone {

NavigationBrandMark::NavigationBrandMark(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(sizeHint());
    setProperty("iconSemantic", QStringLiteral("gitBranch"));
    setAccessibleName(QStringLiteral("Git 分支"));
}

QSize NavigationBrandMark::sizeHint() const
{
    return {28, 28};
}

void NavigationBrandMark::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(QPen(QColor(QStringLiteral("#C8D0DA")), 1.0));
    painter.setBrush(QColor(QStringLiteral("#E8EDF3")));
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 6.0, 6.0);

    const QColor branchColor(QStringLiteral("#4A668E"));
    QPen branchPen(branchColor, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(branchPen);
    painter.setBrush(Qt::NoBrush);

    QPainterPath branch;
    branch.moveTo(9.5, 7.0);
    branch.lineTo(9.5, 20.5);
    branch.moveTo(9.5, 15.0);
    branch.cubicTo(14.0, 15.0, 14.0, 9.0, 18.5, 9.0);
    painter.drawPath(branch);

    painter.setBrush(QColor(QStringLiteral("#F7F9FB")));
    for (const QPointF &node : {QPointF(9.5, 7.0), QPointF(9.5, 20.5),
                                QPointF(18.5, 9.0)}) {
        painter.drawEllipse(node, 2.15, 2.15);
    }
}

} // namespace gitclone
