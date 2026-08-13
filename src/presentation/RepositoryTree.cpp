#include "presentation/RepositoryTree.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyledItemDelegate>

namespace gitclone {
namespace {

constexpr int RowHeight = 40;
constexpr int RowHorizontalInset = 5;
constexpr int BaseIndent = 10;
constexpr int LevelIndent = 22;
constexpr int ChevronWidth = 18;
constexpr int IconSize = 22;
constexpr int IconTextGap = 9;

QColor textColor(bool selected, bool muted = false)
{
    if (selected) {
        return muted ? QColor(QStringLiteral("#5B7FC7"))
                     : QColor(QStringLiteral("#174EA6"));
    }
    return muted ? QColor(QStringLiteral("#718096"))
                 : QColor(QStringLiteral("#172033"));
}

void drawChevron(QPainter &painter, const QRect &rect, bool expanded, bool selected)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(selected ? QColor(QStringLiteral("#3D6FC4"))
                                 : QColor(QStringLiteral("#8B98AA")),
                        1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    QPainterPath path;
    if (expanded) {
        path.moveTo(rect.center().x() - 4, rect.center().y() - 2);
        path.lineTo(rect.center().x(), rect.center().y() + 2);
        path.lineTo(rect.center().x() + 4, rect.center().y() - 2);
    } else {
        path.moveTo(rect.center().x() - 2, rect.center().y() - 4);
        path.lineTo(rect.center().x() + 2, rect.center().y());
        path.lineTo(rect.center().x() - 2, rect.center().y() + 4);
    }
    painter.drawPath(path);
    painter.restore();
}

void drawFolderIcon(QPainter &painter, const QRect &rect, bool root, bool selected)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    const QColor fill = selected
        ? QColor(QStringLiteral("#D6E7FF"))
        : (root ? QColor(QStringLiteral("#E1EAFE"))
                : QColor(QStringLiteral("#E9EEF6")));
    const QColor stroke = selected || root
        ? QColor(QStringLiteral("#4E7FD4"))
        : QColor(QStringLiteral("#8493A7"));
    painter.setPen(QPen(stroke, 1.35, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(fill);
    const QRectF body(rect.left() + 2.5, rect.top() + 6.5,
                      rect.width() - 5.0, rect.height() - 9.0);
    QPainterPath folder;
    folder.moveTo(body.left(), body.top() + 2.5);
    folder.lineTo(body.left() + 6.0, body.top() + 2.5);
    folder.lineTo(body.left() + 8.5, body.top());
    folder.lineTo(body.left() + 13.0, body.top());
    folder.quadTo(body.right(), body.top(), body.right(), body.top() + 3.0);
    folder.lineTo(body.right(), body.bottom() - 2.0);
    folder.quadTo(body.right(), body.bottom(), body.right() - 2.0, body.bottom());
    folder.lineTo(body.left() + 2.0, body.bottom());
    folder.quadTo(body.left(), body.bottom(), body.left(), body.bottom() - 2.0);
    folder.closeSubpath();
    painter.drawPath(folder);
    painter.restore();
}

void drawRepositoryIcon(QPainter &painter, const QRect &rect, bool selected)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF tile(rect.left() + 1.0, rect.top() + 1.0,
                      rect.width() - 2.0, rect.height() - 2.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(selected ? QColor(QStringLiteral("#C9DEFF"))
                              : QColor(QStringLiteral("#E3EDFF")));
    painter.drawRoundedRect(tile, 6.0, 6.0);

    const QColor branchColor = selected ? QColor(QStringLiteral("#1E5FC4"))
                                        : QColor(QStringLiteral("#326FCE"));
    painter.setPen(QPen(branchColor, 1.6, Qt::SolidLine,
                        Qt::RoundCap, Qt::RoundJoin));
    const QPointF upper(tile.left() + 7.0, tile.top() + 6.0);
    const QPointF lower(tile.left() + 7.0, tile.bottom() - 6.0);
    const QPointF side(tile.right() - 6.0, tile.top() + 9.0);
    QPainterPath branch;
    branch.moveTo(upper);
    branch.lineTo(lower);
    branch.moveTo(upper.x(), upper.y() + 2.0);
    branch.cubicTo(upper.x(), side.y(), side.x(), side.y(), side.x(), side.y());
    painter.drawPath(branch);
    painter.setBrush(QColor(QStringLiteral("#FFFFFF")));
    for (const QPointF &point : {upper, lower, side}) {
        painter.drawEllipse(point, 2.2, 2.2);
    }
    painter.restore();
}

class RepositoryTreeDelegate final : public QStyledItemDelegate {
public:
    explicit RepositoryTreeDelegate(QObject *parent)
        : QStyledItemDelegate(parent)
    {
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(RowHeight);
        return size;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        const bool selected = option.state.testFlag(QStyle::State_Selected);
        const bool hovered = option.state.testFlag(QStyle::State_MouseOver);
        const bool enabled = option.state.testFlag(QStyle::State_Enabled);
        const QRect rowRect = option.rect.adjusted(RowHorizontalInset, 2,
                                                   -RowHorizontalInset, -2);
        if (selected || hovered) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(selected ? QColor(QStringLiteral("#DCEAFF"))
                                       : QColor(QStringLiteral("#F0F5FC")));
            painter->drawRoundedRect(rowRect, 8.0, 8.0);
        }

        const int depth = RepositoryTree::nodeDepth(index);
        const int contentLeft = option.rect.left() + BaseIndent + depth * LevelIndent;
        painter->setPen(QPen(selected ? QColor(QStringLiteral("#B6D0F7"))
                                     : QColor(QStringLiteral("#D8E1ED")),
                            1.0));
        for (int level = 1; level <= depth; ++level) {
            const int guideX = option.rect.left() + BaseIndent
                + (level - 1) * LevelIndent + ChevronWidth / 2;
            painter->drawLine(guideX, option.rect.top(), guideX, option.rect.bottom());
        }
        if (depth > 0) {
            const int guideX = contentLeft - LevelIndent + ChevronWidth / 2;
            painter->drawLine(guideX, option.rect.center().y(),
                              contentLeft + ChevronWidth / 2, option.rect.center().y());
        }

        const QRect chevron = RepositoryTree::chevronRect(option, index);
        if (index.model()->hasChildren(index)) {
            drawChevron(*painter, chevron,
                        option.state.testFlag(QStyle::State_Open), selected);
        }

        const QRect iconRect(contentLeft + ChevronWidth,
                             option.rect.center().y() - IconSize / 2,
                             IconSize, IconSize);
        const auto kind = static_cast<RepositoryNodeKind>(
            index.data(RepositoryNodeKindRole).toInt());
        if (kind == RepositoryNodeKind::Repository) {
            drawRepositoryIcon(*painter, iconRect, selected);
        } else {
            drawFolderIcon(*painter, iconRect,
                           kind == RepositoryNodeKind::Root, selected);
        }

        QFont font = option.font;
        font.setWeight(kind == RepositoryNodeKind::Directory
                           ? QFont::Normal : QFont::DemiBold);
        painter->setFont(font);
        painter->setPen(enabled ? textColor(selected)
                                : QColor(QStringLiteral("#9AA7B7")));
        const int textLeft = iconRect.right() + IconTextGap;
        const QRect textRect(textLeft, option.rect.top(),
                             qMax(0, option.rect.right() - textLeft - 10),
                             option.rect.height());
        const QString elided = option.fontMetrics.elidedText(
            index.data(Qt::DisplayRole).toString(), Qt::ElideRight, textRect.width());
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elided);
        painter->restore();
    }
};

} // namespace

RepositoryTree::RepositoryTree(QWidget *parent)
    : QTreeWidget(parent)
{
    setHeaderHidden(true);
    setRootIsDecorated(false);
    setIndentation(0);
    setAnimated(true);
    setMouseTracking(true);
    setUniformRowHeights(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setExpandsOnDoubleClick(true);
    setItemDelegate(new RepositoryTreeDelegate(this));
    viewport()->setAttribute(Qt::WA_Hover, true);
}

int RepositoryTree::nodeDepth(const QModelIndex &index)
{
    int depth = 0;
    QModelIndex parent = index.parent();
    while (parent.isValid()) {
        ++depth;
        parent = parent.parent();
    }
    return depth;
}

QRect RepositoryTree::chevronRect(const QStyleOptionViewItem &option,
                                  const QModelIndex &index)
{
    const int left = option.rect.left() + BaseIndent
        + nodeDepth(index) * LevelIndent;
    return QRect(left, option.rect.center().y() - ChevronWidth / 2,
                 ChevronWidth, ChevronWidth);
}

QModelIndex RepositoryTree::modelIndexForItem(QTreeWidgetItem *item) const
{
    return indexFromItem(item);
}

QRect RepositoryTree::chevronRectForItem(QTreeWidgetItem *item) const
{
    const QModelIndex index = indexFromItem(item);
    QStyleOptionViewItem option;
    option.rect = visualRect(index);
    return chevronRect(option, index);
}

bool RepositoryTree::toggleExpansionAt(const QPoint &viewportPosition)
{
    QModelIndex index = indexAt(viewportPosition);
    if (!index.isValid()) {
        index = indexAt(QPoint(viewport()->width() / 2, viewportPosition.y()));
    }
    if (index.isValid() && model()->hasChildren(index)) {
        QStyleOptionViewItem option;
        option.rect = visualRect(index);
        const QRect hitRect = chevronRect(option, index).adjusted(-5, -5, 5, 5);
        if (hitRect.contains(viewportPosition)) {
            setExpanded(index, !isExpanded(index));
            viewport()->update(option.rect);
            return true;
        }
    }
    return false;
}

void RepositoryTree::mousePressEvent(QMouseEvent *event)
{
    if (toggleExpansionAt(event->pos())) {
        event->accept();
        return;
    }
    QTreeWidget::mousePressEvent(event);
}

} // namespace gitclone
