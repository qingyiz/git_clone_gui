#pragma once

#include "core/CloneRequest.h"

#include <QFrame>

class QLabel;
class QLineEdit;
class QPushButton;

namespace gitclone {

class BranchSelector;
class RemoteBranchService;

class ChildRepositoryCard final : public QFrame {
    Q_OBJECT

public:
    explicit ChildRepositoryCard(int index, QWidget *parent = nullptr);
    ChildRepositoryCard(int index,
                        RemoteBranchService *branchService,
                        QWidget *parent);

    int index() const;
    void setIndex(int index);
    ChildRepositoryRequest configuration() const;
    void setConfiguration(const ChildRepositoryRequest &configuration);
    void setEditable(bool editable);

signals:
    void configurationChanged();
    void removeRequested(gitclone::ChildRepositoryCard *card);

private:
    void createUi(RemoteBranchService *branchService);
    void updateTitle();

    int m_index;
    QLabel *m_numberLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLineEdit *m_repositoryEdit = nullptr;
    BranchSelector *m_branchSelector = nullptr;
    QLineEdit *m_pathEdit = nullptr;
    QPushButton *m_removeButton = nullptr;
};

} // namespace gitclone
