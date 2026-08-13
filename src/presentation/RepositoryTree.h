#pragma once

#include <QTreeWidget>
#include <QStyleOptionViewItem>

namespace gitclone {

enum class RepositoryNodeKind {
    Root,
    Directory,
    Repository
};

enum RepositoryTreeDataRole {
    RepositoryNodeKindRole = Qt::UserRole + 40
};

class RepositoryTree final : public QTreeWidget {
    Q_OBJECT

public:
    explicit RepositoryTree(QWidget *parent = nullptr);

    static int nodeDepth(const QModelIndex &index);
    static QRect chevronRect(const QStyleOptionViewItem &option,
                             const QModelIndex &index);
    QModelIndex modelIndexForItem(QTreeWidgetItem *item) const;
    QRect chevronRectForItem(QTreeWidgetItem *item) const;
    bool toggleExpansionAt(const QPoint &viewportPosition);

protected:
    void mousePressEvent(QMouseEvent *event) override;
};

} // namespace gitclone
