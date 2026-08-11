#include "presentation/MainWindow.h"

#include "presentation/AppStyle.h"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
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

void MainWindow::createUi()
{
    setWindowTitle(QStringLiteral("GitCloneGui · 多仓库克隆"));
    resize(1160, 780);
    setMinimumSize(960, 680);
    setStyleSheet(applicationStyleSheet());

    auto *root = new QWidget(this);
    root->setObjectName(QStringLiteral("appRoot"));
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(24, 20, 24, 22);
    rootLayout->setSpacing(18);

    auto *headerLayout = new QHBoxLayout;
    headerLayout->setSpacing(12);
    auto *icon = new QLabel(QStringLiteral("G"), root);
    icon->setObjectName(QStringLiteral("appIcon"));
    icon->setAlignment(Qt::AlignCenter);
    icon->setFixedSize(44, 44);
    headerLayout->addWidget(icon);
    auto *headerText = new QVBoxLayout;
    headerText->setSpacing(1);
    auto *title = new QLabel(QStringLiteral("多仓库克隆"), root);
    title->setObjectName(QStringLiteral("appTitle"));
    auto *subtitle = new QLabel(QStringLiteral("一次配置父项目与多个子仓库，按顺序安全克隆"), root);
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
    contentLayout->setSpacing(16);
    auto *configurationPanel = new QFrame(root);
    configurationPanel->setObjectName(QStringLiteral("configurationPanel"));
    configurationPanel->setProperty("role", QStringLiteral("panel"));
    auto *configurationPanelLayout = new QVBoxLayout(configurationPanel);
    configurationPanelLayout->setContentsMargins(16, 16, 10, 16);
    configurationPanelLayout->setSpacing(12);
    configurationPanelLayout->addWidget(sectionTitle(QStringLiteral("项目配置"), configurationPanel));

    auto *scrollArea = new QScrollArea(configurationPanel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *scrollContent = new QWidget(scrollArea);
    scrollContent->setObjectName(QStringLiteral("configurationScrollContent"));
    auto *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 7, 4);
    scrollLayout->setSpacing(12);
    scrollLayout->addWidget(createParentCard(scrollContent));
    scrollLayout->addWidget(createDestinationCard(scrollContent));

    auto *childrenHeader = new QHBoxLayout;
    childrenHeader->setContentsMargins(2, 4, 2, 0);
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
    m_childCardsLayout->setSpacing(10);
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
    executionLayout->setContentsMargins(16, 16, 16, 16);
    executionLayout->setSpacing(12);
    executionLayout->addWidget(sectionTitle(QStringLiteral("执行中心"), executionPanel));
    executionLayout->addWidget(createPreviewCard(executionPanel));
    executionLayout->addWidget(createStatusCard(executionPanel));
    executionLayout->addWidget(createLogCard(executionPanel), 1);
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
    setCentralWidget(root);
}

QFrame *MainWindow::createParentCard(QWidget *parent)
{
    QFrame *card = cardFrame(parent, QStringLiteral("parentCard"));
    auto *layout = new QGridLayout(card);
    layout->setContentsMargins(18, 16, 18, 18);
    layout->setHorizontalSpacing(12);
    layout->setVerticalSpacing(7);
    layout->addWidget(sectionTitle(QStringLiteral("父项目"), card), 0, 0, 1, 2);
    m_parentRepositoryEdit = new QLineEdit(card);
    m_parentRepositoryEdit->setObjectName(QStringLiteral("parentRepositoryUrlEdit"));
    m_parentRepositoryEdit->setPlaceholderText(QStringLiteral("git@github.com:team/parent.git"));
    m_parentBranchEdit = new QLineEdit(card);
    m_parentBranchEdit->setObjectName(QStringLiteral("parentBranchEdit"));
    m_parentBranchEdit->setPlaceholderText(QStringLiteral("main / feature/..."));
    m_parentDirectoryEdit = new QLineEdit(card);
    m_parentDirectoryEdit->setObjectName(QStringLiteral("parentDirectoryEdit"));
    m_parentDirectoryEdit->setPlaceholderText(QStringLiteral("parent-project"));
    layout->addWidget(fieldLabel(QStringLiteral("仓库 URL"), card), 1, 0, 1, 2);
    layout->addWidget(m_parentRepositoryEdit, 2, 0, 1, 2);
    layout->addWidget(fieldLabel(QStringLiteral("分支"), card), 3, 0);
    layout->addWidget(fieldLabel(QStringLiteral("项目目录名"), card), 3, 1);
    layout->addWidget(m_parentBranchEdit, 4, 0);
    layout->addWidget(m_parentDirectoryEdit, 4, 1);
    return card;
}

QFrame *MainWindow::createDestinationCard(QWidget *parent)
{
    QFrame *card = cardFrame(parent, QStringLiteral("destinationCard"));
    auto *layout = new QGridLayout(card);
    layout->setContentsMargins(18, 16, 18, 18);
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

QFrame *MainWindow::createPreviewCard(QWidget *parent)
{
    QFrame *card = cardFrame(parent, QStringLiteral("previewCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 16);
    layout->setSpacing(8);
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
    m_previewEdit->setMinimumHeight(120);
    m_previewEdit->setMaximumHeight(170);
    layout->addWidget(m_previewEdit);
    return card;
}

QFrame *MainWindow::createStatusCard(QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setObjectName(QStringLiteral("statusCard"));
    card->setProperty("role", QStringLiteral("statusCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 13, 16, 14);
    layout->setSpacing(7);
    m_validationLabel = new QLabel(card);
    m_validationLabel->setObjectName(QStringLiteral("validationSummary"));
    m_validationLabel->setWordWrap(true);
    layout->addWidget(m_validationLabel);
    m_statusLabel = new QLabel(QStringLiteral("等待配置任务"), card);
    m_statusLabel->setObjectName(QStringLiteral("statusLabel"));
    m_statusLabel->setProperty("role", QStringLiteral("muted"));
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);
    m_progressBar = new QProgressBar(card);
    m_progressBar->setTextVisible(false);
    layout->addWidget(m_progressBar);
    return card;
}

QFrame *MainWindow::createLogCard(QWidget *parent)
{
    QFrame *card = cardFrame(parent, QStringLiteral("logCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 16);
    layout->setSpacing(8);
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
