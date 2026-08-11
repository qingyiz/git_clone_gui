#include "presentation/ChildRepositoryCard.h"

#include "presentation/BranchSelector.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace gitclone {

ChildRepositoryCard::ChildRepositoryCard(int index, QWidget *parent)
    : ChildRepositoryCard(index, nullptr, parent)
{
}

ChildRepositoryCard::ChildRepositoryCard(int index,
                                         RemoteBranchService *branchService,
                                         QWidget *parent)
    : QFrame(parent)
    , m_index(index)
{
    setObjectName(QStringLiteral("childRepositoryCard"));
    setProperty("role", QStringLiteral("card"));
    createUi(branchService);
    updateTitle();
}

int ChildRepositoryCard::index() const
{
    return m_index;
}

void ChildRepositoryCard::setIndex(int index)
{
    if (m_index == index) {
        return;
    }
    m_index = index;
    updateTitle();
}

ChildRepositoryRequest ChildRepositoryCard::configuration() const
{
    return {m_repositoryEdit->text(), m_branchSelector->branchText(), m_pathEdit->text()};
}

void ChildRepositoryCard::setConfiguration(const ChildRepositoryRequest &configuration)
{
    const QSignalBlocker repositoryBlocker(m_repositoryEdit);
    const QSignalBlocker branchBlocker(m_branchSelector);
    const QSignalBlocker pathBlocker(m_pathEdit);
    m_repositoryEdit->setText(configuration.repositoryUrl);
    m_branchSelector->setBranchText(configuration.branch);
    m_branchSelector->setRepositoryUrl(configuration.repositoryUrl);
    m_pathEdit->setText(configuration.relativePath);
}

void ChildRepositoryCard::setEditable(bool editable)
{
    m_repositoryEdit->setEnabled(editable);
    m_branchSelector->setEnabled(editable);
    m_pathEdit->setEnabled(editable);
    m_removeButton->setEnabled(editable);
}

void ChildRepositoryCard::createUi(RemoteBranchService *branchService)
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(18, 16, 18, 18);
    rootLayout->setSpacing(14);

    auto *headerLayout = new QHBoxLayout;
    headerLayout->setSpacing(10);
    m_numberLabel = new QLabel(this);
    m_numberLabel->setObjectName(QStringLiteral("cardNumberBadge"));
    m_numberLabel->setAlignment(Qt::AlignCenter);
    m_numberLabel->setFixedSize(28, 28);
    headerLayout->addWidget(m_numberLabel);

    auto *titlesLayout = new QVBoxLayout;
    titlesLayout->setSpacing(1);
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName(QStringLiteral("cardTitle"));
    auto *subtitle = new QLabel(QStringLiteral("克隆到父项目内部"), this);
    subtitle->setProperty("role", QStringLiteral("muted"));
    titlesLayout->addWidget(m_titleLabel);
    titlesLayout->addWidget(subtitle);
    headerLayout->addLayout(titlesLayout);
    headerLayout->addStretch(1);

    m_removeButton = new QPushButton(QStringLiteral("移除"), this);
    m_removeButton->setObjectName(QStringLiteral("removeChildButton"));
    m_removeButton->setProperty("buttonRole", QStringLiteral("dangerGhost"));
    m_removeButton->setCursor(Qt::PointingHandCursor);
    headerLayout->addWidget(m_removeButton);
    rootLayout->addLayout(headerLayout);

    auto *fieldsLayout = new QGridLayout;
    fieldsLayout->setHorizontalSpacing(12);
    fieldsLayout->setVerticalSpacing(6);
    m_repositoryEdit = new QLineEdit(this);
    m_repositoryEdit->setObjectName(QStringLiteral("childRepositoryUrlEdit"));
    m_repositoryEdit->setPlaceholderText(QStringLiteral("git@github.com:team/repository.git"));
    m_branchSelector = new BranchSelector(branchService, this);
    m_branchSelector->setObjectName(QStringLiteral("childBranchSelector"));
    m_branchSelector->setEditorObjectName(QStringLiteral("childBranchEdit"));
    m_pathEdit = new QLineEdit(this);
    m_pathEdit->setObjectName(QStringLiteral("childRelativePathEdit"));
    m_pathEdit->setPlaceholderText(QStringLiteral("modules/repository"));

    auto addField = [this, fieldsLayout](int row, const QString &label, QWidget *edit) {
        auto *fieldLabel = new QLabel(label, this);
        fieldLabel->setProperty("role", QStringLiteral("fieldLabel"));
        fieldsLayout->addWidget(fieldLabel, row, 0);
        fieldsLayout->addWidget(edit, row + 1, 0);
    };
    addField(0, QStringLiteral("仓库 URL"), m_repositoryEdit);
    addField(2, QStringLiteral("分支（输入 URL 后可选择/搜索）"), m_branchSelector);
    addField(4, QStringLiteral("父项目内路径"), m_pathEdit);
    rootLayout->addLayout(fieldsLayout);

    const QList<QLineEdit *> edits {m_repositoryEdit, m_pathEdit};
    for (QLineEdit *edit : edits) {
        connect(edit, &QLineEdit::textChanged, this, &ChildRepositoryCard::configurationChanged);
    }
    connect(m_repositoryEdit, &QLineEdit::textChanged,
            m_branchSelector, &BranchSelector::setRepositoryUrl);
    connect(m_branchSelector, &BranchSelector::branchChanged,
            this, &ChildRepositoryCard::configurationChanged);
    connect(m_removeButton, &QPushButton::clicked, this, [this] {
        emit removeRequested(this);
    });
}

void ChildRepositoryCard::updateTitle()
{
    m_numberLabel->setText(QString::number(m_index + 1));
    m_titleLabel->setText(QStringLiteral("子仓库 %1").arg(m_index + 1));
}

} // namespace gitclone
