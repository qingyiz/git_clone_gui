#include "presentation/ClonePage.h"

#include "presentation/AppStyle.h"
#include "presentation/BranchSelector.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QWidget>

namespace gitclone {
namespace {

QLabel *fieldLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setProperty("role", QStringLiteral("fieldLabel"));
    return label;
}

QLabel *sectionTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("sectionTitle"));
    return label;
}

QFrame *cardFrame(QWidget *parent, const QString &objectName = {})
{
    auto *frame = new QFrame(parent);
    frame->setProperty("role", QStringLiteral("card"));
    if (!objectName.isEmpty()) {
        frame->setObjectName(objectName);
    }
    return frame;
}

} // namespace

void ClonePage::createUi()
{
    auto *root = this;
    root->setObjectName(QStringLiteral("appRoot"));
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(20, 16, 20, 18);
    rootLayout->setSpacing(14);

    auto *headerLayout = new QHBoxLayout;
    auto *headerText = new QVBoxLayout;
    headerText->setSpacing(1);
    auto *title = new QLabel(QStringLiteral("多仓库克隆"), root);
    title->setObjectName(QStringLiteral("appTitle"));
    auto *subtitle = new QLabel(QStringLiteral("配置并按顺序克隆父项目与子仓库"), root);
    subtitle->setProperty("role", QStringLiteral("muted"));
    headerText->addWidget(title);
    headerText->addWidget(subtitle);
    headerLayout->addLayout(headerText);
    headerLayout->addStretch(1);
    m_saveStatusLabel = new QLabel(QStringLiteral("配置自动保存"), root);
    m_saveStatusLabel->setObjectName(QStringLiteral("saveStatusLabel"));
    headerLayout->addWidget(m_saveStatusLabel);
    rootLayout->addLayout(headerLayout);

    auto *contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(12);
    auto *configurationPanel = new QFrame(root);
    configurationPanel->setObjectName(QStringLiteral("configurationPanel"));
    configurationPanel->setProperty("role", QStringLiteral("panel"));
    auto *configurationPanelLayout = new QVBoxLayout(configurationPanel);
    configurationPanelLayout->setContentsMargins(14, 13, 9, 13);
    configurationPanelLayout->setSpacing(10);
    configurationPanelLayout->addWidget(sectionTitle(QStringLiteral("项目配置"), configurationPanel));

    auto *scrollArea = new QScrollArea(configurationPanel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *scrollContent = new QWidget(scrollArea);
    scrollContent->setObjectName(QStringLiteral("configurationScrollContent"));
    auto *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 6, 3);
    scrollLayout->setSpacing(9);
    scrollLayout->addWidget(createParentCard(scrollContent));
    scrollLayout->addWidget(createDestinationCard(scrollContent));

    auto *childrenHeader = new QHBoxLayout;
    childrenHeader->setContentsMargins(2, 2, 2, 0);
    childrenHeader->addWidget(sectionTitle(QStringLiteral("子仓库"), scrollContent));
    m_childCountLabel = new QLabel(QStringLiteral("0 个"), scrollContent);
    m_childCountLabel->setObjectName(QStringLiteral("childCountLabel"));
    childrenHeader->addWidget(m_childCountLabel);
    childrenHeader->addStretch(1);
    m_addChildButton = new QPushButton(QStringLiteral("＋ 添加子仓库"), scrollContent);
    m_addChildButton->setObjectName(QStringLiteral("addChildButton"));
    m_addChildButton->setProperty("buttonRole", QStringLiteral("accentGhost"));
    m_addChildButton->setCursor(Qt::PointingHandCursor);
    childrenHeader->addWidget(m_addChildButton);
    scrollLayout->addLayout(childrenHeader);
    m_childCardsLayout = new QVBoxLayout;
    m_childCardsLayout->setSpacing(8);
    scrollLayout->addLayout(m_childCardsLayout);
    auto *privacy = new QLabel(
        QStringLiteral("配置会保存在当前用户设置中。请勿把 Token 或密码直接写进仓库 URL。"),
        scrollContent);
    privacy->setObjectName(QStringLiteral("privacyHint"));
    privacy->setProperty("role", QStringLiteral("muted"));
    privacy->setWordWrap(true);
    scrollLayout->addWidget(privacy);
    scrollLayout->addStretch(1);
    scrollArea->setWidget(scrollContent);
    configurationPanelLayout->addWidget(scrollArea, 1);
    contentLayout->addWidget(configurationPanel, 11);

    auto *executionPanel = new QFrame(root);
    executionPanel->setObjectName(QStringLiteral("executionPanel"));
    executionPanel->setProperty("role", QStringLiteral("panel"));
    auto *executionLayout = new QVBoxLayout(executionPanel);
    executionLayout->setContentsMargins(14, 13, 14, 13);
    executionLayout->setSpacing(10);
    executionLayout->addWidget(sectionTitle(QStringLiteral("执行中心"), executionPanel));
    m_executionSplitter = new QSplitter(Qt::Vertical, executionPanel);
    m_executionSplitter->setObjectName(QStringLiteral("executionSplitter"));
    m_executionSplitter->setChildrenCollapsible(false);
    m_executionSplitter->setHandleWidth(7);
    auto *summaryContainer = new QWidget(m_executionSplitter);
    summaryContainer->setObjectName(QStringLiteral("executionSummaryContainer"));
    auto *summaryLayout = new QVBoxLayout(summaryContainer);
    summaryLayout->setContentsMargins(0, 0, 0, 0);
    summaryLayout->setSpacing(9);
    summaryLayout->addWidget(createPreviewCard(summaryContainer));
    summaryLayout->addWidget(createStatusCard(summaryContainer));
    QFrame *logCard = createLogCard(m_executionSplitter);
    logCard->setMinimumHeight(280);
    m_executionSplitter->addWidget(summaryContainer);
    m_executionSplitter->addWidget(logCard);
    m_executionSplitter->setStretchFactor(0, 0);
    m_executionSplitter->setStretchFactor(1, 1);
    m_executionSplitter->setSizes({196, 434});
    executionLayout->addWidget(m_executionSplitter, 1);
    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    m_cancelButton = new QPushButton(QStringLiteral("取消"), executionPanel);
    m_cancelButton->setObjectName(QStringLiteral("cancelButton"));
    m_startButton = new QPushButton(QStringLiteral("开始克隆"), executionPanel);
    m_startButton->setObjectName(QStringLiteral("startButton"));
    m_startButton->setProperty("buttonRole", QStringLiteral("primary"));
    m_startButton->setDefault(true);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_startButton);
    executionLayout->addLayout(buttonLayout);
    contentLayout->addWidget(executionPanel, 10);
    rootLayout->addLayout(contentLayout, 1);
}

QFrame *ClonePage::createParentCard(QWidget *parent)
{
    QFrame *card = cardFrame(parent, QStringLiteral("parentCard"));
    auto *layout = new QGridLayout(card);
    layout->setContentsMargins(14, 12, 14, 14);
    layout->setHorizontalSpacing(10);
    layout->setVerticalSpacing(6);
    layout->addWidget(sectionTitle(QStringLiteral("父项目"), card), 0, 0, 1, 2);
    m_parentRepositoryEdit = new QLineEdit(card);
    m_parentRepositoryEdit->setObjectName(QStringLiteral("parentRepositoryUrlEdit"));
    m_parentRepositoryEdit->setPlaceholderText(QStringLiteral("git@github.com:team/parent.git"));
    m_parentBranchSelector = new BranchSelector(m_branchService, card);
    m_parentBranchSelector->setObjectName(QStringLiteral("parentBranchSelector"));
    m_parentBranchSelector->setEditorObjectName(QStringLiteral("parentBranchEdit"));
    m_parentDirectoryEdit = new QLineEdit(card);
    m_parentDirectoryEdit->setObjectName(QStringLiteral("parentDirectoryEdit"));
    m_parentDirectoryEdit->setPlaceholderText(QStringLiteral("parent-project"));
    layout->addWidget(fieldLabel(QStringLiteral("仓库 URL"), card), 1, 0, 1, 2);
    layout->addWidget(m_parentRepositoryEdit, 2, 0, 1, 2);
    layout->addWidget(fieldLabel(QStringLiteral("分支（可选择/搜索）"), card), 3, 0);
    layout->addWidget(fieldLabel(QStringLiteral("项目目录名"), card), 3, 1);
    layout->addWidget(m_parentBranchSelector, 4, 0);
    layout->addWidget(m_parentDirectoryEdit, 4, 1);
    return card;
}

QFrame *ClonePage::createDestinationCard(QWidget *parent)
{
    QFrame *card = cardFrame(parent, QStringLiteral("destinationCard"));
    auto *layout = new QGridLayout(card);
    layout->setContentsMargins(14, 12, 14, 14);
    layout->setHorizontalSpacing(8);
    layout->setVerticalSpacing(7);
    layout->addWidget(sectionTitle(QStringLiteral("保存位置"), card), 0, 0, 1, 2);
    layout->addWidget(fieldLabel(QStringLiteral("父项目目录的上一级目录"), card), 1, 0, 1, 2);
    m_destinationRootEdit = new QLineEdit(card);
    m_destinationRootEdit->setObjectName(QStringLiteral("destinationRootEdit"));
    m_destinationRootEdit->setPlaceholderText(QStringLiteral("选择一个现有目录"));
    m_browseButton = new QPushButton(QStringLiteral("浏览…"), card);
    m_browseButton->setObjectName(QStringLiteral("browseButton"));
    layout->addWidget(m_destinationRootEdit, 2, 0);
    layout->addWidget(m_browseButton, 2, 1);
    return card;
}

QFrame *ClonePage::createPreviewCard(QWidget *parent)
{
    QFrame *card = cardFrame(parent, QStringLiteral("previewCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(13, 11, 13, 13);
    layout->setSpacing(7);
    auto *header = new QHBoxLayout;
    header->addWidget(sectionTitle(QStringLiteral("命令预览"), card));
    header->addStretch(1);
    auto *safeLabel = new QLabel(QStringLiteral("不经过 shell"), card);
    safeLabel->setProperty("role", QStringLiteral("muted"));
    header->addWidget(safeLabel);
    layout->addLayout(header);
    m_previewEdit = new QPlainTextEdit(card);
    m_previewEdit->setObjectName(QStringLiteral("commandPreviewEdit"));
    m_previewEdit->setReadOnly(true);
    m_previewEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_previewEdit->setMinimumHeight(90);
    m_previewEdit->setMaximumHeight(125);
    layout->addWidget(m_previewEdit);
    return card;
}

QFrame *ClonePage::createStatusCard(QWidget *parent)
{
    m_statusCard = new QFrame(parent);
    m_statusCard->setObjectName(QStringLiteral("statusCard"));
    m_statusCard->setProperty("role", QStringLiteral("statusCard"));
    m_statusCard->setProperty("statusState", QStringLiteral("normal"));
    auto *layout = new QVBoxLayout(m_statusCard);
    layout->setContentsMargins(13, 10, 13, 11);
    layout->setSpacing(6);
    m_validationLabel = new QLabel(m_statusCard);
    m_validationLabel->setObjectName(QStringLiteral("validationSummary"));
    m_validationLabel->setWordWrap(true);
    layout->addWidget(m_validationLabel);
    m_statusLabel = new QLabel(QStringLiteral("等待配置任务"), m_statusCard);
    m_statusLabel->setObjectName(QStringLiteral("statusLabel"));
    m_statusLabel->setProperty("role", QStringLiteral("muted"));
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);
    m_progressBar = new QProgressBar(m_statusCard);
    m_progressBar->setTextVisible(false);
    layout->addWidget(m_progressBar);
    return m_statusCard;
}

QFrame *ClonePage::createLogCard(QWidget *parent)
{
    QFrame *card = cardFrame(parent, QStringLiteral("logCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(13, 11, 13, 13);
    layout->setSpacing(7);
    layout->addWidget(sectionTitle(QStringLiteral("Git 输出"), card));
    m_logEdit = new QPlainTextEdit(card);
    m_logEdit->setObjectName(QStringLiteral("gitOutputEdit"));
    m_logEdit->setReadOnly(true);
    m_logEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_logEdit->document()->setMaximumBlockCount(10000);
    m_logEdit->setPlaceholderText(QStringLiteral("克隆开始后，Git 输出会实时显示在这里。"));
    layout->addWidget(m_logEdit, 1);
    return card;
}

} // namespace gitclone
