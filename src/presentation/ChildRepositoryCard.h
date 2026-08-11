#pragma once

#include "core/CloneRequest.h"

#include <QFrame>

class QLabel;
class QLineEdit;
class QPushButton;

namespace gitclone {

class ChildRepositoryCard final : public QFrame {
    Q_OBJECT

public:
    explicit ChildRepositoryCard(int index, QWidget *parent = nullptr);

    int index() const;
    void setIndex(int index);
    ChildRepositoryRequest configuration() const;
    void setConfiguration(const ChildRepositoryRequest &configuration);
    void setEditable(bool editable);

signals:
    void configurationChanged();
    void removeRequested(gitclone::ChildRepositoryCard *card);

private:
    void createUi();
    void updateTitle();

    int m_index;
    QLabel *m_numberLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLineEdit *m_repositoryEdit = nullptr;
    QLineEdit *m_branchEdit = nullptr;
    QLineEdit *m_pathEdit = nullptr;
    QPushButton *m_removeButton = nullptr;
};

} // namespace gitclone
