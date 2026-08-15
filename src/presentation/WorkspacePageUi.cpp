#include "presentation/WorkspacePage.h"
#include "presentation/RepositoryTree.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

namespace gitclone {
namespace {

QLabel *sectionTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("sectionTitle"));
    return label;
}

QFrame *card(QWidget *parent)
{
    auto *frame = new QFrame(parent);
    frame->setProperty("role", QStringLiteral("card"));
    return frame;
}

QListWidget *branchList(QWidget *parent, const QString &objectName)
{
    auto *list = new QListWidget(parent);
    list->setObjectName(objectName);
    list->setAlternatingRowColors(false);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    return list;
}

} // namespace

void WorkspacePage::setWorkingTreeStatus(const WorkingTreeStatus &status)
{
    if (!status.hasChanges()) {
        m_worktreeStatusCard->setProperty("worktreeState", QStringLiteral("clean"));
        m_worktreeStatusTitle->setText(QStringLiteral("工作区干净"));
        m_worktreeStatusDetails->setText(
            QStringLiteral("没有检测到未提交或未跟踪的改动，可以正常切换分支。"));
    } else {
        QStringList details;
        if (status.conflicts > 0) {
            details.append(QStringLiteral("冲突 %1").arg(status.conflicts));
        }
        if (status.stagedChanges > 0) {
            details.append(QStringLiteral("已暂存 %1").arg(status.stagedChanges));
        }
        if (status.unstagedChanges > 0) {
            details.append(QStringLiteral("未暂存 %1").arg(status.unstagedChanges));
        }
        if (status.untrackedFiles > 0) {
            details.append(QStringLiteral("未跟踪 %1").arg(status.untrackedFiles));
        }
        m_worktreeStatusCard->setProperty("worktreeState", QStringLiteral("dirty"));
        m_worktreeStatusTitle->setText(QStringLiteral("工作区存在未提交改动"));
        m_worktreeStatusDetails->setText(
            QStringLiteral("%1。切换分支前请确认这些改动可以安全带入目标分支，谨慎操作。")
                .arg(details.join(QStringLiteral(" · "))));
    }
    m_worktreeStatusCard->style()->unpolish(m_worktreeStatusCard);
    m_worktreeStatusCard->style()->polish(m_worktreeStatusCard);
}

void WorkspacePage::createUi()
{
    setObjectName(QStringLiteral("workspacePage"));
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 20, 24, 22);
    rootLayout->setSpacing(16);

    auto *header = new QHBoxLayout;
    auto *headerText = new QVBoxLayout;
    headerText->setSpacing(2);
    auto *title = new QLabel(QStringLiteral("仓库工作区"), this);
    title->setObjectName(QStringLiteral("appTitle"));
    auto *subtitle = new QLabel(
        QStringLiteral("递归发现本地项目，集中查看并快速切换分支"), this);
    subtitle->setProperty("role", QStringLiteral("muted"));
    headerText->addWidget(title);
    headerText->addWidget(subtitle);
    header->addLayout(headerText);
    header->addStretch(1);
    rootLayout->addLayout(header);

    QFrame *rootCard = card(this);
    auto *rootCardLayout = new QVBoxLayout(rootCard);
    rootCardLayout->setContentsMargins(18, 15, 18, 17);
    rootCardLayout->setSpacing(9);
    rootCardLayout->addWidget(sectionTitle(QStringLiteral("工作目录"), rootCard));
    auto *rootInput = new QHBoxLayout;
    m_rootEdit = new QLineEdit(rootCard);
    m_rootEdit->setObjectName(QStringLiteral("workspaceRootEdit"));
    m_rootEdit->setPlaceholderText(QStringLiteral("选择包含多个项目的现有目录"));
    m_browseButton = new QPushButton(QStringLiteral("浏览…"), rootCard);
    m_browseButton->setObjectName(QStringLiteral("workspaceBrowseButton"));
    m_scanButton = new QPushButton(QStringLiteral("扫描仓库"), rootCard);
    m_scanButton->setObjectName(QStringLiteral("workspaceScanButton"));
    m_scanButton->setProperty("buttonRole", QStringLiteral("primary"));
    m_cancelScanButton = new QPushButton(QStringLiteral("取消扫描"), rootCard);
    m_cancelScanButton->setObjectName(QStringLiteral("workspaceCancelScanButton"));
    m_cancelScanButton->setVisible(false);
    rootInput->addWidget(m_rootEdit, 1);
    rootInput->addWidget(m_browseButton);
    rootInput->addWidget(m_scanButton);
    rootInput->addWidget(m_cancelScanButton);
    rootCardLayout->addLayout(rootInput);
    rootLayout->addWidget(rootCard);

    auto *content = new QHBoxLayout;
    content->setSpacing(16);
    QFrame *treeCard = card(this);
    auto *treeLayout = new QVBoxLayout(treeCard);
    treeLayout->setContentsMargins(16, 14, 16, 16);
    auto *treeHeader = new QHBoxLayout;
    treeHeader->addWidget(sectionTitle(QStringLiteral("仓库树"), treeCard));
    treeHeader->addStretch(1);
    m_repositoryCountLabel = new QLabel(QStringLiteral("0 个"), treeCard);
    m_repositoryCountLabel->setObjectName(QStringLiteral("workspaceRepositoryCount"));
    treeHeader->addWidget(m_repositoryCountLabel);
    treeLayout->addLayout(treeHeader);
    m_repositoryTree = new RepositoryTree(treeCard);
    m_repositoryTree->setObjectName(QStringLiteral("workspaceRepositoryTree"));
    treeLayout->addWidget(m_repositoryTree, 1);
    content->addWidget(treeCard, 4);

    QFrame *branchCard = card(this);
    auto *branchLayout = new QVBoxLayout(branchCard);
    branchLayout->setContentsMargins(16, 14, 16, 16);
    branchLayout->setSpacing(10);
    auto *branchHeader = new QHBoxLayout;
    branchHeader->addWidget(sectionTitle(QStringLiteral("分支详情"), branchCard));
    branchHeader->addStretch(1);
    m_refreshButton = new QPushButton(QStringLiteral("刷新"), branchCard);
    m_refreshButton->setObjectName(QStringLiteral("workspaceRefreshButton"));
    branchHeader->addWidget(m_refreshButton);
    branchLayout->addLayout(branchHeader);
    m_selectedRepositoryLabel = new QLabel(branchCard);
    m_selectedRepositoryLabel->setObjectName(QStringLiteral("workspaceSelectedRepository"));
    m_selectedRepositoryLabel->setProperty("role", QStringLiteral("muted"));
    m_selectedRepositoryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    branchLayout->addWidget(m_selectedRepositoryLabel);
    m_currentBranchLabel = new QLabel(branchCard);
    m_currentBranchLabel->setObjectName(QStringLiteral("workspaceCurrentBranch"));
    m_currentBranchLabel->setProperty("role", QStringLiteral("branchBadge"));
    branchLayout->addWidget(m_currentBranchLabel);

    m_worktreeStatusCard = new QFrame(branchCard);
    m_worktreeStatusCard->setObjectName(QStringLiteral("workspaceWorktreeStatusCard"));
    m_worktreeStatusCard->setProperty("worktreeState", QStringLiteral("neutral"));
    auto *worktreeStatusLayout = new QVBoxLayout(m_worktreeStatusCard);
    worktreeStatusLayout->setContentsMargins(13, 10, 13, 10);
    worktreeStatusLayout->setSpacing(3);
    m_worktreeStatusTitle = new QLabel(m_worktreeStatusCard);
    m_worktreeStatusTitle->setObjectName(QStringLiteral("workspaceWorktreeStatusTitle"));
    m_worktreeStatusDetails = new QLabel(m_worktreeStatusCard);
    m_worktreeStatusDetails->setObjectName(QStringLiteral("workspaceWorktreeStatusDetails"));
    m_worktreeStatusDetails->setWordWrap(true);
    worktreeStatusLayout->addWidget(m_worktreeStatusTitle);
    worktreeStatusLayout->addWidget(m_worktreeStatusDetails);
    branchLayout->addWidget(m_worktreeStatusCard);

    m_branchSearch = new QLineEdit(branchCard);
    m_branchSearch->setObjectName(QStringLiteral("workspaceBranchSearch"));
    m_branchSearch->setPlaceholderText(
        QStringLiteral("搜索分支（支持少量错字）…"));
    m_branchSearch->setClearButtonEnabled(true);
    branchLayout->addWidget(m_branchSearch);

    m_branchTabs = new QTabWidget(branchCard);
    m_branchTabs->setObjectName(QStringLiteral("workspaceBranchTabs"));
    m_localBranches = branchList(m_branchTabs, QStringLiteral("workspaceLocalBranches"));
    m_remoteCandidates = branchList(m_branchTabs, QStringLiteral("workspaceRemoteCandidates"));
    m_branchTabs->addTab(m_localBranches, QStringLiteral("本地分支"));
    m_branchTabs->addTab(m_remoteCandidates, QStringLiteral("远端待跟踪"));
    branchLayout->addWidget(m_branchTabs, 1);
    auto *actions = new QHBoxLayout;
    auto *hint = new QLabel(QStringLiteral("双击分支也可以直接切换"), branchCard);
    hint->setProperty("role", QStringLiteral("muted"));
    actions->addWidget(hint);
    actions->addStretch(1);
    m_switchButton = new QPushButton(QStringLiteral("切换到所选分支"), branchCard);
    m_switchButton->setObjectName(QStringLiteral("workspaceSwitchButton"));
    m_switchButton->setProperty("buttonRole", QStringLiteral("primary"));
    actions->addWidget(m_switchButton);
    branchLayout->addLayout(actions);
    content->addWidget(branchCard, 6);
    rootLayout->addLayout(content, 1);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("workspaceStatusLabel"));
    m_statusLabel->setWordWrap(true);
    rootLayout->addWidget(m_statusLabel);
}

} // namespace gitclone
