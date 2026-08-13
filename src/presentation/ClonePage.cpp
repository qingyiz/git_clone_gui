#include "presentation/ClonePage.h"

#include "presentation/BranchSelector.h"
#include "presentation/ChildRepositoryCard.h"

#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace gitclone {

ClonePage::ClonePage(CloneController *controller,
                       ConfigurationStore *configurationStore,
                       QWidget *parent)
    : ClonePage(controller, configurationStore, nullptr, parent)
{
}

ClonePage::ClonePage(CloneController *controller,
                       ConfigurationStore *configurationStore,
                       RemoteBranchService *branchService,
                       QWidget *parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_configurationStore(configurationStore)
    , m_branchService(branchService)
{
    Q_ASSERT(m_controller != nullptr);
    Q_ASSERT(m_configurationStore != nullptr);
    createUi();
    connectUi();
    restoreConfiguration();
    setRunning(false);
}

int ClonePage::childCardCount() const
{
    return m_childCards.size();
}

bool ClonePage::requestClose()
{
    if (m_saveTimer->isActive()) {
        m_saveTimer->stop();
        saveConfigurationNow();
    }
    if (!m_controller->isRunning()) {
        return true;
    }
    m_closeAfterCancel = true;
    m_controller->cancel();
    m_statusLabel->setText(QStringLiteral("正在结束 Git 进程，结束后关闭窗口…"));
    return false;
}

void ClonePage::chooseDestinationRoot()
{
    const QString initial = m_destinationRootEdit->text().trimmed().isEmpty()
        ? QDir::homePath()
        : m_destinationRootEdit->text().trimmed();
    const QString directory = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择目标根目录"), initial,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!directory.isEmpty()) {
        m_destinationRootEdit->setText(QDir::toNativeSeparators(directory));
    }
}

void ClonePage::addEmptyChildCard()
{
    addChildCard();
    renumberChildCards();
    updatePreview();
    scheduleConfigurationSave();
}

void ClonePage::updatePreview()
{
    const ValidationResult validation = buildClonePlan(currentRequest());
    m_configurationValid = validation.valid;
    setValidationSummary(validation.errors);
    m_startButton->setEnabled(!m_running && m_configurationValid);
    if (!validation.valid) {
        m_previewEdit->setPlainText(QStringLiteral(
            "填写完整且有效的项目参数后，这里会按执行顺序显示全部 Git 命令。"));
        return;
    }

    QStringList lines;
    lines.append(QStringLiteral("[父项目]\n%1")
                     .arg(commandPreview(validation.plan.parentCommand)));
    for (int index = 0; index < validation.plan.children.size(); ++index) {
        lines.append(QStringLiteral("[子仓库 %1]\n%2")
                         .arg(index + 1)
                         .arg(commandPreview(validation.plan.children.at(index).command)));
    }
    m_previewEdit->setPlainText(lines.join(QStringLiteral("\n\n")));
}

void ClonePage::startClone()
{
    saveConfigurationNow();
    m_logEdit->clear();
    m_controller->start(currentRequest());
}

void ClonePage::setRunning(bool running)
{
    m_running = running;
    setConfigurationEnabled(!running);
    m_cancelButton->setEnabled(running);
    if (running) {
        m_startButton->setEnabled(false);
        m_progressBar->setRange(0, 0);
    } else {
        m_progressBar->setRange(0, 1);
        m_progressBar->setValue(0);
        updatePreview();
    }
}

void ClonePage::appendLog(const QString &text)
{
    m_logEdit->moveCursor(QTextCursor::End);
    m_logEdit->insertPlainText(text);
    m_logEdit->moveCursor(QTextCursor::End);
}

void ClonePage::showValidationErrors(const QStringList &errors)
{
    setValidationSummary(errors);
}

void ClonePage::handleStateChanged(CloneController::State state)
{
    switch (state) {
    case CloneController::State::CloningParent:
        m_progressBar->setFormat(QStringLiteral("父项目"));
        break;
    case CloneController::State::CloningChild:
        m_progressBar->setFormat(QStringLiteral("子仓库"));
        break;
    case CloneController::State::Cancelling:
        m_progressBar->setFormat(QStringLiteral("取消中"));
        break;
    case CloneController::State::Idle:
        m_progressBar->setFormat(QString());
        break;
    }
}

void ClonePage::handleFinished(CloneController::Outcome outcome,
                                const QString &message,
                                const QString &parentTargetPath)
{
    Q_UNUSED(parentTargetPath)
    if (m_closeAfterCancel) {
        m_closeAfterCancel = false;
        emit closeReady();
        return;
    }
    setTaskResult(outcome, message);
    if (outcome == CloneController::Outcome::Completed) {
        emit taskResultNotificationRequested(
            QStringLiteral("GitCloneGui · 克隆完成"),
            message,
            NotificationSeverity::Information);
    } else if (outcome == CloneController::Outcome::Failed) {
        emit taskResultNotificationRequested(
            QStringLiteral("GitCloneGui · 克隆失败"),
            message,
            NotificationSeverity::Critical);
    }
}

void ClonePage::saveConfigurationNow()
{
    if (m_loadingConfiguration) {
        return;
    }
    if (m_configurationStore->save(currentRequest())) {
        m_saveStatusLabel->setText(QStringLiteral("已自动保存"));
        m_saveStatusLabel->setStyleSheet(QString());
    } else {
        m_saveStatusLabel->setText(QStringLiteral("配置保存失败"));
        m_saveStatusLabel->setStyleSheet(QStringLiteral("color:#B91C1C;background:#FEE2E2;"));
    }
}

CloneRequest ClonePage::currentRequest() const
{
    CloneRequest request;
    request.parentRepositoryUrl = m_parentRepositoryEdit->text();
    request.parentBranch = m_parentBranchSelector->branchText();
    request.parentDirectoryName = m_parentDirectoryEdit->text();
    request.destinationRoot = m_destinationRootEdit->text();
    for (const ChildRepositoryCard *card : m_childCards) {
        request.children.append(card->configuration());
    }
    return request;
}

void ClonePage::connectUi()
{
    const QList<QLineEdit *> edits {m_parentRepositoryEdit,
                                    m_parentDirectoryEdit, m_destinationRootEdit};
    for (QLineEdit *edit : edits) {
        connect(edit, &QLineEdit::textChanged, this, [this] {
            updatePreview();
            scheduleConfigurationSave();
        });
    }
    connect(m_parentRepositoryEdit, &QLineEdit::textChanged,
            m_parentBranchSelector, &BranchSelector::setRepositoryUrl);
    connect(m_parentBranchSelector, &BranchSelector::branchChanged, this, [this] {
        updatePreview();
        scheduleConfigurationSave();
    });
    connect(m_browseButton, &QPushButton::clicked, this, &ClonePage::chooseDestinationRoot);
    connect(m_addChildButton, &QPushButton::clicked, this, &ClonePage::addEmptyChildCard);
    connect(m_startButton, &QPushButton::clicked, this, &ClonePage::startClone);
    connect(m_cancelButton, &QPushButton::clicked, m_controller, &CloneController::cancel);
    connect(m_controller, &CloneController::runningChanged, this, &ClonePage::setRunning);
    connect(m_controller, &CloneController::stateChanged, this, &ClonePage::handleStateChanged);
    connect(m_controller, &CloneController::statusChanged, m_statusLabel, &QLabel::setText);
    connect(m_controller, &CloneController::logReceived, this, &ClonePage::appendLog);
    connect(m_controller, &CloneController::validationFailed, this, &ClonePage::showValidationErrors);
    connect(m_controller, &CloneController::jobFinished, this, &ClonePage::handleFinished);
    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(300);
    connect(m_saveTimer, &QTimer::timeout, this, &ClonePage::saveConfigurationNow);
}

void ClonePage::restoreConfiguration()
{
    m_loadingConfiguration = true;
    const std::optional<CloneRequest> stored = m_configurationStore->load();
    if (stored.has_value()) {
        m_parentRepositoryEdit->setText(stored->parentRepositoryUrl);
        m_parentBranchSelector->setRepositoryUrl(stored->parentRepositoryUrl);
        m_parentBranchSelector->setBranchText(stored->parentBranch);
        m_parentDirectoryEdit->setText(stored->parentDirectoryName);
        m_destinationRootEdit->setText(stored->destinationRoot);
        for (const ChildRepositoryRequest &child : stored->children) {
            addChildCard(child);
        }
        m_saveStatusLabel->setText(QStringLiteral("已恢复上次配置"));
    } else {
        addChildCard();
        m_saveStatusLabel->setText(QStringLiteral("首次使用 · 自动保存"));
    }
    m_loadingConfiguration = false;
    renumberChildCards();
    updatePreview();
}

ChildRepositoryCard *ClonePage::addChildCard(const ChildRepositoryRequest &configuration)
{
    auto *card = new ChildRepositoryCard(m_childCards.size(), m_branchService, this);
    card->setConfiguration(configuration);
    connect(card, &ChildRepositoryCard::configurationChanged, this, [this] {
        updatePreview();
        scheduleConfigurationSave();
    });
    connect(card, &ChildRepositoryCard::removeRequested, this, &ClonePage::removeChildCard);
    m_childCards.append(card);
    m_childCardsLayout->addWidget(card);
    return card;
}

void ClonePage::removeChildCard(ChildRepositoryCard *card)
{
    const int index = m_childCards.indexOf(card);
    if (index < 0) {
        return;
    }
    m_childCards.removeAt(index);
    m_childCardsLayout->removeWidget(card);
    card->deleteLater();
    renumberChildCards();
    updatePreview();
    scheduleConfigurationSave();
}

void ClonePage::renumberChildCards()
{
    for (int index = 0; index < m_childCards.size(); ++index) {
        m_childCards.at(index)->setIndex(index);
    }
    m_childCountLabel->setText(QStringLiteral("%1 个").arg(m_childCards.size()));
}

void ClonePage::scheduleConfigurationSave()
{
    if (!m_loadingConfiguration) {
        m_saveStatusLabel->setText(QStringLiteral("保存中…"));
        m_saveTimer->start();
    }
}

void ClonePage::setConfigurationEnabled(bool enabled)
{
    m_parentRepositoryEdit->setEnabled(enabled);
    m_parentBranchSelector->setEnabled(enabled);
    m_parentDirectoryEdit->setEnabled(enabled);
    m_destinationRootEdit->setEnabled(enabled);
    m_browseButton->setEnabled(enabled);
    m_addChildButton->setEnabled(enabled);
    for (ChildRepositoryCard *card : m_childCards) {
        card->setEditable(enabled);
    }
}

void ClonePage::setValidationSummary(const QStringList &errors)
{
    m_statusCard->setProperty("statusState", QStringLiteral("normal"));
    if (errors.isEmpty()) {
        m_validationLabel->setProperty("validationState", QStringLiteral("ready"));
        m_validationLabel->setText(QStringLiteral("✓ 配置有效，可以开始克隆"));
    } else {
        QStringList summary;
        const int visibleCount = errors.size() > 3 ? 2 : errors.size();
        for (int index = 0; index < visibleCount; ++index) {
            summary.append(QStringLiteral("• %1").arg(errors.at(index)));
        }
        if (errors.size() > visibleCount) {
            summary.append(QStringLiteral("另有 %1 个问题待处理").arg(errors.size() - visibleCount));
        }
        m_validationLabel->setProperty("validationState", QStringLiteral("error"));
        m_validationLabel->setText(summary.join(QLatin1Char('\n')));
    }
    m_validationLabel->style()->unpolish(m_validationLabel);
    m_validationLabel->style()->polish(m_validationLabel);
    m_statusCard->style()->unpolish(m_statusCard);
    m_statusCard->style()->polish(m_statusCard);
}

void ClonePage::setTaskResult(CloneController::Outcome outcome, const QString &message)
{
    QString state = QStringLiteral("normal");
    QString summary = QStringLiteral("任务已取消");
    QString validationState = QStringLiteral("ready");
    if (outcome == CloneController::Outcome::Completed) {
        state = QStringLiteral("success");
        summary = QStringLiteral("✓ 克隆完成");
    } else if (outcome == CloneController::Outcome::Failed) {
        state = QStringLiteral("error");
        summary = QStringLiteral("克隆失败");
        validationState = QStringLiteral("error");
    }
    m_statusCard->setProperty("statusState", state);
    m_validationLabel->setProperty("validationState", validationState);
    m_validationLabel->setText(summary);
    m_statusLabel->setText(message);
    m_validationLabel->style()->unpolish(m_validationLabel);
    m_validationLabel->style()->polish(m_validationLabel);
    m_statusCard->style()->unpolish(m_statusCard);
    m_statusCard->style()->polish(m_statusCard);
}

} // namespace gitclone
