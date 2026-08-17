#pragma once

#include <QWidget>

namespace gitclone {

class NavigationBrandMark final : public QWidget {
public:
    explicit NavigationBrandMark(QWidget *parent = nullptr);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

} // namespace gitclone
