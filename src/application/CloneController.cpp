#include "application/CloneController.h"

#include <QStandardPaths>

namespace gitclone {

CloneController::CloneController(ProcessRunner *runner, QObject *parent, int cancelTimeoutMs)
    : QObject(parent)
    , m_runner(runner)
{
    Q_ASSERT(m_runner != nullptr);
    m_cancelTimer.setSingleShot(true);
    m_cancelTimer.setInterval(cancelTimeoutMs);

    connect(m_runner, &ProcessRunner::outputReceived,
            this, &CloneController::logReceived);
    connect(m_runner, &ProcessRunner::finished,
            this, &CloneController::handleProcessFinished);
    connect(m_runner, &ProcessRunner::errorOccurred,
            this, &CloneController::handleProcessError);
    connect(&m_cancelTimer, &QTimer::timeout,
            this, &CloneController::handleCancelTimeout);
}

CloneController::State CloneController::state() const
{
    return m_state;
}

bool CloneController::isRunning() const
{
    return m_state != State::Idle;
}

bool CloneController::start(const CloneRequest &request)
{
    if (isRunning()) {
        emit statusChanged(QStringLiteral("已有克隆任务正在运行。"));
        return false;
    }

    const ValidationResult validation = buildClonePlan(request);
    if (!validation.valid) {
        emit validationFailed(validation.errors);
        emit statusChanged(validation.errors.join(QLatin1Char('\n')));
        return false;
    }

    const QString gitExecutable = QStandardPaths::findExecutable(QStringLiteral("git"));
    if (gitExecutable.isEmpty()) {
        const QString message = QStringLiteral("找不到 git 可执行程序，请安装 Git 并确认 PATH 配置。 ").trimmed();
        emit validationFailed({message});
        emit statusChanged(message);
        return false;
    }

    m_plan = validation.plan;
    m_plan.parentCommand.program = gitExecutable;
    for (ChildClonePlan &child : m_plan.children) {
        child.command.program = gitExecutable;
    }
    m_currentChildIndex = -1;
    emit logReceived(QStringLiteral("父项目命令：%1\n").arg(commandPreview(m_plan.parentCommand)));
    return startStage(m_plan.parentCommand, State::CloningParent, QStringLiteral("正在克隆父项目…"));
}

void CloneController::cancel()
{
    if (!isRunning() || m_state == State::Cancelling) {
        return;
    }

    setState(State::Cancelling);
    emit statusChanged(QStringLiteral("正在取消，已下载的文件会保留…"));
    emit logReceived(QStringLiteral("\n正在请求终止 Git 进程…\n"));
    m_runner->terminate();
    if (m_runner->isRunning()) {
        m_cancelTimer.start();
    } else {
        finish(Outcome::Cancelled, QStringLiteral("任务已取消，已有文件已保留。"));
    }
}

void CloneController::handleProcessFinished(int exitCode, bool normalExit)
{
    if (!isRunning()) {
        return;
    }

    if (m_state == State::Cancelling) {
        finish(Outcome::Cancelled, QStringLiteral("任务已取消，已有文件已保留。"));
        return;
    }

    if (!normalExit || exitCode != 0) {
        const QString message = QStringLiteral("%1失败（退出码 %2），已有文件已保留。")
                                    .arg(currentStageLabel())
                                    .arg(exitCode);
        finish(Outcome::Failed, message);
        return;
    }

    if (m_state == State::CloningParent) {
        emit logReceived(QStringLiteral("\n父项目克隆完成。\n"));
        if (m_plan.children.isEmpty()) {
            finish(Outcome::Completed,
                   QStringLiteral("父项目已克隆完成（0 个子仓库）：%1")
                       .arg(m_plan.parentTargetPath));
            return;
        }
        startChild(0);
        return;
    }

    if (m_currentChildIndex + 1 < m_plan.children.size()) {
        startChild(m_currentChildIndex + 1);
        return;
    }

    finish(Outcome::Completed,
           QStringLiteral("父项目和 %1 个子仓库均已克隆完成：%2")
               .arg(m_plan.children.size())
               .arg(m_plan.parentTargetPath));
}

void CloneController::handleProcessError(const QString &message)
{
    if (!isRunning()) {
        return;
    }

    if (m_state == State::Cancelling) {
        finish(Outcome::Cancelled, QStringLiteral("任务已取消，已有文件已保留。"));
        return;
    }

    finish(Outcome::Failed,
           QStringLiteral("%1无法继续：%2。已有文件已保留。")
               .arg(currentStageLabel(), message));
}

void CloneController::handleCancelTimeout()
{
    if (m_state == State::Cancelling && m_runner->isRunning()) {
        emit logReceived(QStringLiteral("Git 未在 3 秒内退出，正在强制结束…\n"));
        m_runner->kill();
    }
}

void CloneController::setState(State state)
{
    if (m_state == state) {
        return;
    }

    const bool wasRunning = isRunning();
    m_state = state;
    emit stateChanged(m_state);
    const bool nowRunning = isRunning();
    if (wasRunning != nowRunning) {
        emit runningChanged(nowRunning);
    }
}

bool CloneController::startStage(const ProcessCommand &command,
                                 State state,
                                 const QString &label)
{
    setState(state);
    emit statusChanged(label);
    if (!m_runner->start(command)) {
        finish(Outcome::Failed,
               QStringLiteral("%1无法启动，进程执行器正忙。已有文件已保留。")
                   .arg(currentStageLabel()));
        return false;
    }
    return true;
}

bool CloneController::startChild(int index)
{
    Q_ASSERT(index >= 0 && index < m_plan.children.size());
    m_currentChildIndex = index;
    const ChildClonePlan &child = m_plan.children.at(index);
    const QString position = QStringLiteral("%1/%2").arg(index + 1).arg(m_plan.children.size());
    emit logReceived(QStringLiteral("子仓库 %1 命令：%2\n")
                         .arg(position, commandPreview(child.command)));
    return startStage(child.command,
                      State::CloningChild,
                      QStringLiteral("正在克隆子仓库 %1…").arg(position));
}

void CloneController::finish(Outcome outcome, const QString &message)
{
    m_cancelTimer.stop();
    const QString parentTarget = m_plan.parentTargetPath;
    setState(State::Idle);
    m_currentChildIndex = -1;
    emit statusChanged(message);
    emit logReceived(QStringLiteral("\n%1\n").arg(message));
    emit jobFinished(outcome, message, parentTarget);
}

QString CloneController::currentStageLabel() const
{
    switch (m_state) {
    case State::CloningParent:
        return QStringLiteral("父项目克隆");
    case State::CloningChild:
        return QStringLiteral("子仓库 %1/%2 克隆")
            .arg(m_currentChildIndex + 1)
            .arg(m_plan.children.size());
    case State::Cancelling:
        return QStringLiteral("取消操作");
    case State::Idle:
        return QStringLiteral("克隆任务");
    }
    return QStringLiteral("克隆任务");
}

} // namespace gitclone
