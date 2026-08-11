#pragma once

#include "application/ProcessRunner.h"

#include <QProcess>

namespace gitclone {

class GitProcessRunner final : public ProcessRunner {
    Q_OBJECT

public:
    explicit GitProcessRunner(QObject *parent = nullptr);

    bool start(const ProcessCommand &command) override;
    bool isRunning() const override;
    void terminate() override;
    void kill() override;

private slots:
    void readAvailableOutput();
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleError(QProcess::ProcessError error);

private:
    QProcess m_process;
};

} // namespace gitclone
