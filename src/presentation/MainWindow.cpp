#include "presentation/MainWindow.h"

#include "presentation/AppStyle.h"
#include "presentation/ClonePage.h"
#include "presentation/WorkspacePage.h"

#include "application/WorkspaceService.h"
#include "application/NavigationConfigurationStore.h"

#include <QButtonGroup>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace gitclone {
namespace {

class DisabledWorkspaceService final : public WorkspaceService {
public:
    using WorkspaceService::WorkspaceService;
    void scan(const QString &rootPath) override
    {
        emit scanFailed(rootPath, QStringLiteral("工作区服务未配置。"));
    }
    void cancelScan() override { }
    void loadBranches(const QString &repositoryPath) override
    {
        emit branchLoadFailed(repositoryPath, QStringLiteral("工作区服务未配置。"));
    }
    void switchBranch(const QString &repositoryPath, const BranchTarget &) override
    {
        emit branchSwitchFailed(repositoryPath, QStringLiteral("工作区服务未配置。"));
    }
    void cancelGitOperation() override { }
};

WorkspaceService *fallbackWorkspaceService(QObject *parent)
{
    return new DisabledWorkspaceService(parent);
}

} // namespace

MainWindow::MainWindow(CloneController *controller,
                       ConfigurationStore *configurationStore,
                       QWidget *parent)
    : MainWindow(controller, configurationStore, nullptr, nullptr, nullptr, nullptr,
                 parent)
{
}

MainWindow::MainWindow(CloneController *controller,
                       ConfigurationStore *configurationStore,
                       RemoteBranchService *branchService,
                       QWidget *parent)
    : MainWindow(controller, configurationStore, branchService, nullptr, nullptr, nullptr,
                 parent)
{
}

MainWindow::MainWindow(CloneController *controller,
                       ConfigurationStore *configurationStore,
                       RemoteBranchService *branchService,
                       WorkspaceService *workspaceService,
                       QWidget *parent)
    : MainWindow(controller, configurationStore, branchService, workspaceService,
                 nullptr, nullptr, parent)
{
}

MainWindow::MainWindow(CloneController *controller,
                       ConfigurationStore *configurationStore,
                       RemoteBranchService *branchService,
                       WorkspaceService *workspaceService,
                       WorkspaceConfigurationStore *workspaceConfigurationStore,
                       QWidget *parent)
    : MainWindow(controller, configurationStore, branchService, workspaceService,
                 workspaceConfigurationStore, nullptr, parent)
{
}

MainWindow::MainWindow(CloneController *controller,
                       ConfigurationStore *configurationStore,
                       RemoteBranchService *branchService,
                       WorkspaceService *workspaceService,
                       WorkspaceConfigurationStore *workspaceConfigurationStore,
                       NavigationConfigurationStore *navigationConfigurationStore,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_clonePage(new ClonePage(controller, configurationStore, branchService, this))
    , m_workspacePage(new WorkspacePage(
          workspaceService == nullptr ? fallbackWorkspaceService(this) : workspaceService,
          workspaceConfigurationStore,
          this))
    , m_navigationConfigurationStore(navigationConfigurationStore)
{
    createUi();
    connect(m_clonePage, &ClonePage::taskResultNotificationRequested,
            this, &MainWindow::taskResultNotificationRequested);
    connect(m_clonePage, &ClonePage::closeReady, this, [this] {
        if (m_closePending) {
            m_closePending = false;
            QTimer::singleShot(0, this, &QWidget::close);
        }
    });
}

int MainWindow::childCardCount() const
{
    return m_clonePage->childCardCount();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_workspacePage->cancelOperations();
    if (m_clonePage->requestClose()) {
        event->accept();
        return;
    }
    m_closePending = true;
    event->ignore();
}

void MainWindow::createUi()
{
    setWindowTitle(QStringLiteral("GitCloneGui"));
    resize(1160, 780);
    setMinimumSize(960, 680);
    setStyleSheet(applicationStyleSheet());

    auto *root = new QWidget(this);
    root->setObjectName(QStringLiteral("windowRoot"));
    auto *layout = new QHBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *sidebar = new QFrame(root);
    sidebar->setObjectName(QStringLiteral("navigationSidebar"));
    sidebar->setFixedWidth(188);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(16, 20, 16, 18);
    sidebarLayout->setSpacing(8);
    auto *brand = new QLabel(QStringLiteral("GitCloneGui"), sidebar);
    brand->setObjectName(QStringLiteral("navigationBrand"));
    sidebarLayout->addWidget(brand);
    auto *caption = new QLabel(QStringLiteral("开发工作台"), sidebar);
    caption->setProperty("role", QStringLiteral("muted"));
    sidebarLayout->addWidget(caption);
    sidebarLayout->addSpacing(18);

    m_cloneNavigationButton = new QPushButton(QStringLiteral("  仓库克隆"), sidebar);
    m_cloneNavigationButton->setObjectName(QStringLiteral("cloneNavigationButton"));
    m_workspaceNavigationButton = new QPushButton(QStringLiteral("  仓库工作区"), sidebar);
    m_workspaceNavigationButton->setObjectName(QStringLiteral("workspaceNavigationButton"));
    for (QPushButton *button : {m_cloneNavigationButton, m_workspaceNavigationButton}) {
        button->setCheckable(true);
        button->setProperty("buttonRole", QStringLiteral("navigation"));
        button->setCursor(Qt::PointingHandCursor);
        sidebarLayout->addWidget(button);
    }
    auto *group = new QButtonGroup(this);
    group->setExclusive(true);
    group->addButton(m_cloneNavigationButton, 0);
    group->addButton(m_workspaceNavigationButton, 1);
    connect(group, qOverload<int>(&QButtonGroup::idClicked),
            this, &MainWindow::selectPage);
    sidebarLayout->addStretch(1);
    auto *version = new QLabel(
        QStringLiteral("版本 %1\n本地 Git 工具")
            .arg(QCoreApplication::applicationVersion()),
        sidebar);
    version->setObjectName(QStringLiteral("navigationVersionLabel"));
    sidebarLayout->addWidget(version);
    layout->addWidget(sidebar);

    m_pages = new QStackedWidget(root);
    m_pages->setObjectName(QStringLiteral("mainPageStack"));
    m_pages->addWidget(m_clonePage);
    m_pages->addWidget(m_workspacePage);
    layout->addWidget(m_pages, 1);
    setCentralWidget(root);
    int initialPage = 0;
    if (m_navigationConfigurationStore != nullptr) {
        const std::optional<NavigationPage> savedPage =
            m_navigationConfigurationStore->loadCurrentPage();
        if (savedPage == NavigationPage::Workspace) {
            initialPage = 1;
        }
    }
    selectPage(initialPage);
}

void MainWindow::selectPage(int index)
{
    m_pages->setCurrentIndex(index);
    m_cloneNavigationButton->setChecked(index == 0);
    m_workspaceNavigationButton->setChecked(index == 1);
    if (m_navigationConfigurationStore != nullptr) {
        m_navigationConfigurationStore->saveCurrentPage(
            index == 1 ? NavigationPage::Workspace : NavigationPage::Clone);
    }
}

} // namespace gitclone
