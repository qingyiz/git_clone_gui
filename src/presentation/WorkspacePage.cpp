#include "presentation/WorkspacePage.h"
#include "presentation/BranchNameMatcher.h"
#include "presentation/RepositoryTree.h"

#include "application/WorkspaceConfigurationStore.h"

#include <QDir>
#include <QFileDialog>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QTimer>
#include <QTreeWidget>

namespace gitclone {

WorkspacePage::WorkspacePage(WorkspaceService *service, QWidget *parent)
    : WorkspacePage(service, nullptr, parent)
{
}

WorkspacePage::WorkspacePage(WorkspaceService *service,
                             WorkspaceConfigurationStore *configurationStore,
                             QWidget *parent)
    : QWidget(parent)
    , m_service(service)
    , m_configurationStore(configurationStore)
    , m_saveTimer(new QTimer(this))
{
    Q_ASSERT(m_service != nullptr);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(300);
    createUi();
    const bool shouldAutoScan = restoreRootPath();
    connectUi();
    clearBranchDetails(QStringLiteral("选择工作目录并扫描，然后选中一个仓库。"));
    if (shouldAutoScan) {
        QTimer::singleShot(0, this, &WorkspacePage::startScan);
    }
}

WorkspacePage::~WorkspacePage()
{
    flushPendingRootPath();
}

void WorkspacePage::cancelOperations()
{
    flushPendingRootPath();
    m_service->cancelScan();
    m_service->cancelGitOperation();
}

void WorkspacePage::chooseRoot()
{
    const QString initial = m_rootEdit->text().trimmed().isEmpty()
        ? QDir::homePath() : m_rootEdit->text().trimmed();
    const QString directory = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择 Git 工作目录"), initial,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!directory.isEmpty()) {
        m_rootEdit->setText(QDir::toNativeSeparators(directory));
        startScan();
    }
}

void WorkspacePage::startScan()
{
    const QString rootPath = m_rootEdit->text().trimmed();
    if (rootPath.isEmpty()) {
        setStatus(QStringLiteral("请先选择工作目录。"), QStringLiteral("error"));
        return;
    }
    setStatus(QStringLiteral("正在扫描目录中的 Git 仓库…"), QStringLiteral("loading"));
    m_service->scan(rootPath);
}

void WorkspacePage::handleRepositorySelection()
{
    QTreeWidgetItem *item = m_repositoryTree->currentItem();
    const QString repositoryPath = item == nullptr
        ? QString() : item->data(0, RepositoryPathRole).toString();
    if (repositoryPath.isEmpty()) {
        m_selectedRepositoryPath.clear();
        clearBranchDetails(QStringLiteral("请选择带 Git 标识的仓库节点。"));
        return;
    }
    m_selectedRepositoryPath = repositoryPath;
    m_selectedRepositoryLabel->setText(QDir::toNativeSeparators(repositoryPath));
    refreshBranches();
}

void WorkspacePage::refreshBranches()
{
    if (m_selectedRepositoryPath.isEmpty() || m_gitBusy) {
        return;
    }
    setStatus(QStringLiteral("正在读取分支与工作区状态…"),
              QStringLiteral("loading"));
    m_worktreeStatusCard->setProperty("worktreeState", QStringLiteral("neutral"));
    m_worktreeStatusTitle->setText(QStringLiteral("正在检查工作区状态…"));
    m_worktreeStatusDetails->setText(QStringLiteral("读取完成后会显示是否存在未提交改动。"));
    m_worktreeStatusCard->style()->unpolish(m_worktreeStatusCard);
    m_worktreeStatusCard->style()->polish(m_worktreeStatusCard);
    m_service->loadBranches(m_selectedRepositoryPath);
}

void WorkspacePage::switchSelectedBranch()
{
    QListWidget *list = activeSwitchList();
    QListWidgetItem *item = list == nullptr ? nullptr : list->currentItem();
    if (item == nullptr || item->isHidden()
        || m_selectedRepositoryPath.isEmpty() || m_gitBusy) {
        return;
    }
    BranchTarget target;
    target.kind = static_cast<BranchTarget::Kind>(item->data(BranchKindRole).toInt());
    target.name = item->data(BranchNameRole).toString();
    setStatus(QStringLiteral("正在切换到 %1…").arg(target.name),
              QStringLiteral("loading"));
    m_service->switchBranch(m_selectedRepositoryPath, target);
}

void WorkspacePage::handleScanFinished(
    const QString &rootPath, const QVector<RepositoryInfo> &repositories,
    int skippedDirectories)
{
    m_rootPath = rootPath;
    setRepositories(rootPath, repositories);
    const QString skipped = skippedDirectories > 0
        ? QStringLiteral("，跳过 %1 个不可读目录").arg(skippedDirectories) : QString();
    setStatus(QStringLiteral("扫描完成：发现 %1 个 Git 仓库%2。")
                  .arg(repositories.size()).arg(skipped),
              QStringLiteral("success"));
}

void WorkspacePage::handleScanFailed(const QString &rootPath, const QString &message)
{
    setStatus(QStringLiteral("扫描失败 · %1：%2")
                  .arg(QDir::toNativeSeparators(rootPath), message),
              QStringLiteral("error"));
}

void WorkspacePage::handleBranchesLoaded(const QString &repositoryPath,
                                         const BranchCatalog &catalog)
{
    if (repositoryPath != m_selectedRepositoryPath) {
        return;
    }
    setBranchCatalog(catalog);
    setStatus(QStringLiteral("分支已刷新。本地 %1 个，远端待跟踪 %2 个。")
                  .arg(catalog.localBranches.size()).arg(catalog.remoteCandidates.size()),
              QStringLiteral("success"));
}

void WorkspacePage::handleBranchLoadFailed(const QString &repositoryPath,
                                           const QString &message)
{
    if (repositoryPath == m_selectedRepositoryPath) {
        setStatus(QStringLiteral("读取分支失败 · %1：%2")
                      .arg(QDir::toNativeSeparators(repositoryPath), message),
                  QStringLiteral("error"));
    }
}

void WorkspacePage::handleBranchSwitchSucceeded(const QString &repositoryPath,
                                                const QString &branchName)
{
    if (repositoryPath != m_selectedRepositoryPath) {
        return;
    }
    setStatus(QStringLiteral("已切换分支：%1，正在刷新…").arg(branchName),
              QStringLiteral("success"));
    m_service->loadBranches(repositoryPath);
}

void WorkspacePage::handleBranchSwitchFailed(const QString &repositoryPath,
                                             const QString &message)
{
    if (repositoryPath == m_selectedRepositoryPath) {
        setStatus(QStringLiteral("切换失败 · %1：%2")
                      .arg(QDir::toNativeSeparators(repositoryPath), message),
                  QStringLiteral("error"));
    }
}

void WorkspacePage::connectUi()
{
    connect(m_browseButton, &QPushButton::clicked, this, &WorkspacePage::chooseRoot);
    connect(m_scanButton, &QPushButton::clicked, this, &WorkspacePage::startScan);
    connect(m_rootEdit, &QLineEdit::returnPressed, this, &WorkspacePage::startScan);
    connect(m_rootEdit, &QLineEdit::textChanged, this, [this] {
        if (m_configurationStore != nullptr) {
            m_rootSavePending = true;
            m_saveTimer->start();
        }
    });
    connect(m_saveTimer, &QTimer::timeout, this,
            &WorkspacePage::flushPendingRootPath);
    connect(m_cancelScanButton, &QPushButton::clicked, m_service,
            &WorkspaceService::cancelScan);
    connect(m_repositoryTree, &QTreeWidget::itemSelectionChanged,
            this, &WorkspacePage::handleRepositorySelection);
    connect(m_refreshButton, &QPushButton::clicked,
            this, &WorkspacePage::refreshBranches);
    connect(m_switchButton, &QPushButton::clicked,
            this, &WorkspacePage::switchSelectedBranch);
    connect(m_localBranches, &QListWidget::itemSelectionChanged,
            this, &WorkspacePage::updateSwitchButton);
    connect(m_remoteCandidates, &QListWidget::itemSelectionChanged,
            this, &WorkspacePage::updateSwitchButton);
    connect(m_branchSearch, &QLineEdit::textChanged,
            this, [this] { applyBranchFilter(); });
    connect(m_branchTabs, &QTabWidget::currentChanged,
            this, [this] { updateSwitchButton(); });
    connect(m_localBranches, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem *) { switchSelectedBranch(); });
    connect(m_remoteCandidates, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem *) { switchSelectedBranch(); });

    connect(m_service, &WorkspaceService::scanBusyChanged, this, [this](bool busy) {
        m_scanBusy = busy;
        m_rootEdit->setEnabled(!busy);
        m_browseButton->setEnabled(!busy);
        m_scanButton->setEnabled(!busy);
        m_cancelScanButton->setVisible(busy);
    });
    connect(m_service, &WorkspaceService::scanFinished,
            this, &WorkspacePage::handleScanFinished);
    connect(m_service, &WorkspaceService::scanFailed,
            this, &WorkspacePage::handleScanFailed);
    connect(m_service, &WorkspaceService::gitBusyChanged, this, [this](bool busy) {
        m_gitBusy = busy;
        m_repositoryTree->setEnabled(!busy);
        m_refreshButton->setEnabled(!busy && !m_selectedRepositoryPath.isEmpty());
        updateSwitchButton();
    });
    connect(m_service, &WorkspaceService::branchesLoaded,
            this, &WorkspacePage::handleBranchesLoaded);
    connect(m_service, &WorkspaceService::branchLoadFailed,
            this, &WorkspacePage::handleBranchLoadFailed);
    connect(m_service, &WorkspaceService::branchSwitchSucceeded,
            this, &WorkspacePage::handleBranchSwitchSucceeded);
    connect(m_service, &WorkspaceService::branchSwitchFailed,
            this, &WorkspacePage::handleBranchSwitchFailed);
}

bool WorkspacePage::restoreRootPath()
{
    if (m_configurationStore == nullptr) {
        return false;
    }
    const std::optional<QString> rootPath = m_configurationStore->loadRootPath();
    if (!rootPath.has_value()) {
        return false;
    }
    m_rootEdit->setText(QDir::toNativeSeparators(rootPath.value()));
    const QFileInfo rootInfo(rootPath.value());
    return rootInfo.exists() && rootInfo.isDir() && rootInfo.isReadable();
}

void WorkspacePage::flushPendingRootPath()
{
    if (m_configurationStore == nullptr || !m_rootSavePending) {
        return;
    }
    m_saveTimer->stop();
    m_rootSavePending = false;
    if (!m_configurationStore->saveRootPath(m_rootEdit->text().trimmed())) {
        setStatus(QStringLiteral("工作目录配置保存失败，当前输入仍可继续使用。"),
                  QStringLiteral("error"));
    }
}

void WorkspacePage::setRepositories(const QString &rootPath,
                                    const QVector<RepositoryInfo> &repositories)
{
    m_repositoryTree->clear();
    m_selectedRepositoryPath.clear();
    clearBranchDetails(QStringLiteral("从左侧仓库树选择一个仓库。"));
    auto *root = new QTreeWidgetItem(m_repositoryTree,
                                     {QFileInfo(rootPath).fileName().isEmpty()
                                          ? QDir::toNativeSeparators(rootPath)
                                          : QFileInfo(rootPath).fileName()});
    root->setToolTip(0, QDir::toNativeSeparators(rootPath));
    root->setData(0, RepositoryNodeKindRole,
                  static_cast<int>(RepositoryNodeKind::Root));
    QHash<QString, QTreeWidgetItem *> nodes;
    nodes.insert(QString(), root);

    for (const RepositoryInfo &repository : repositories) {
        if (repository.relativePath == QStringLiteral(".")) {
            root->setData(0, RepositoryPathRole, repository.absolutePath);
            continue;
        }
        const QStringList parts = repository.relativePath.split(QLatin1Char('/'),
                                                                 Qt::SkipEmptyParts);
        QString key;
        QTreeWidgetItem *parent = root;
        for (const QString &part : parts) {
            key = key.isEmpty() ? part : key + QLatin1Char('/') + part;
            QTreeWidgetItem *node = nodes.value(key, nullptr);
            if (node == nullptr) {
                node = new QTreeWidgetItem(parent, {part});
                node->setData(0, RepositoryNodeKindRole,
                              static_cast<int>(RepositoryNodeKind::Directory));
                nodes.insert(key, node);
            }
            parent = node;
        }
        parent->setData(0, RepositoryNodeKindRole,
                        static_cast<int>(RepositoryNodeKind::Repository));
        parent->setData(0, RepositoryPathRole, repository.absolutePath);
        parent->setToolTip(0, QDir::toNativeSeparators(repository.absolutePath));
    }
    if (!root->data(0, RepositoryPathRole).toString().isEmpty()) {
        root->setData(0, RepositoryNodeKindRole,
                      static_cast<int>(RepositoryNodeKind::Repository));
    }
    root->setExpanded(true);
    m_repositoryTree->expandToDepth(1);
    m_repositoryCountLabel->setText(QStringLiteral("%1 个").arg(repositories.size()));
}

void WorkspacePage::clearBranchDetails(const QString &message)
{
    m_selectedRepositoryLabel->setText(message);
    m_currentBranchLabel->setText(QStringLiteral("当前分支：—"));
    m_worktreeStatusCard->setProperty("worktreeState", QStringLiteral("neutral"));
    m_worktreeStatusTitle->setText(QStringLiteral("工作区状态待检查"));
    m_worktreeStatusDetails->setText(QStringLiteral("选中仓库后会自动检查未提交改动。"));
    m_worktreeStatusCard->style()->unpolish(m_worktreeStatusCard);
    m_worktreeStatusCard->style()->polish(m_worktreeStatusCard);
    m_localBranches->clear();
    m_remoteCandidates->clear();
    m_refreshButton->setEnabled(false);
    updateSwitchButton();
}

void WorkspacePage::setBranchCatalog(const BranchCatalog &catalog)
{
    m_currentBranchLabel->setText(catalog.isDetached()
        ? QStringLiteral("当前分支：分离 HEAD")
        : QStringLiteral("当前分支：%1").arg(catalog.currentBranch));
    setWorkingTreeStatus(catalog.workingTreeStatus);
    m_localBranches->clear();
    for (const QString &branch : catalog.localBranches) {
        auto *item = new QListWidgetItem(branch, m_localBranches);
        item->setData(BranchKindRole, static_cast<int>(BranchTarget::Kind::Local));
        item->setData(BranchNameRole, branch);
        if (branch == catalog.currentBranch) {
            item->setText(QStringLiteral("✓ %1").arg(branch));
            item->setToolTip(QStringLiteral("当前分支"));
        }
    }
    m_remoteCandidates->clear();
    for (const QString &branch : catalog.remoteCandidates) {
        auto *item = new QListWidgetItem(branch, m_remoteCandidates);
        item->setData(BranchKindRole, static_cast<int>(BranchTarget::Kind::Remote));
        item->setData(BranchNameRole, branch);
        item->setToolTip(QStringLiteral("切换时创建同名本地跟踪分支"));
    }
    applyBranchFilter();
    m_refreshButton->setEnabled(!m_gitBusy);
    updateSwitchButton();
}

void WorkspacePage::applyBranchFilter()
{
    const QString query = m_branchSearch->text().trimmed();
    const QList<QListWidget *> lists {m_localBranches, m_remoteCandidates};
    for (QListWidget *list : lists) {
        for (int row = 0; row < list->count(); ++row) {
            QListWidgetItem *item = list->item(row);
            const QString branchName = item->data(BranchNameRole).toString();
            item->setHidden(!fuzzyBranchNameMatches(branchName, query));
        }
        if (list->currentItem() != nullptr && list->currentItem()->isHidden()) {
            list->setCurrentItem(nullptr);
        }
    }
    updateSwitchButton();
}

void WorkspacePage::setStatus(const QString &message, const QString &state)
{
    m_statusLabel->setText(message);
    m_statusLabel->setProperty("statusState", state);
    m_statusLabel->style()->unpolish(m_statusLabel);
    m_statusLabel->style()->polish(m_statusLabel);
}

void WorkspacePage::updateSwitchButton()
{
    QListWidget *list = activeSwitchList();
    m_switchButton->setEnabled(!m_gitBusy && list != nullptr
                               && list->currentItem() != nullptr
                               && !list->currentItem()->isHidden()
                               && !m_selectedRepositoryPath.isEmpty());
}

QListWidget *WorkspacePage::activeSwitchList() const
{
    if (m_branchTabs->currentWidget() == m_localBranches) {
        return m_localBranches;
    }
    if (m_branchTabs->currentWidget() == m_remoteCandidates) {
        return m_remoteCandidates;
    }
    return nullptr;
}

} // namespace gitclone
