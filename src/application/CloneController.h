#pragma once

#include "application/ProcessRunner.h"
#include "core/CloneRequest.h"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

namespace gitclone {

class CloneController final : public QObject {
    Q_OBJECT

public:
    enum class State {
        Idle,
        CloningParent,
        CloningChild,
        Cancelling
    };
    Q_ENUM(State)

    enum class Outcome {
        Completed,
        Failed,
        Cancelled
    };
    Q_ENUM(Outcome)

    explicit CloneController(ProcessRunner *runner,
                             QObject *parent = nullptr,
                             int cancelTimeoutMs = 3000);

    State state() const;
    bool isRunning() const;
    bool start(const CloneRequest &request);

public slots:
    void cancel();

signals:
    void stateChanged(gitclone::CloneController::State state);
    void runningChanged(bool running);
    void statusChanged(const QString &status);
    void logReceived(const QString &text);
    void validationFailed(const QStringList &errors);
    void jobFinished(gitclone::CloneController::Outcome outcome,
                     const QString &message,
                     const QString &parentTargetPath);

private slots:
    void handleProcessFinished(int exitCode, bool normalExit);
    void handleProcessError(const QString &message);
    void handleCancelTimeout();

private:
    void setState(State state);
    bool startStage(const ProcessCommand &command, State state, const QString &label);
    bool startChild(int index);
    void finish(Outcome outcome, const QString &message);
    QString currentStageLabel() const;

    ProcessRunner *m_runner;
    QTimer m_cancelTimer;
    QElapsedTimer m_jobTimer;
    State m_state = State::Idle;
    ClonePlan m_plan;
    int m_currentChildIndex = -1;
};

} // namespace gitclone

Q_DECLARE_METATYPE(gitclone::CloneController::State)
Q_DECLARE_METATYPE(gitclone::CloneController::Outcome)
