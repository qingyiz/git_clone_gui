#include "infrastructure/GitProcessRunner.h"

namespace gitclone {

GitProcessRunner::GitProcessRunner(QObject *parent)
    : ProcessRunner(parent)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyRead,
            this, &GitProcessRunner::readAvailableOutput);
    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &GitProcessRunner::handleFinished);
    connect(&m_process, &QProcess::errorOccurred,
            this, &GitProcessRunner::handleError);
}

bool GitProcessRunner::start(const ProcessCommand &command)
{
    if (isRunning()) {
        return false;
    }

    m_process.setWorkingDirectory(command.workingDirectory);
    m_process.start(command.program, command.arguments);
    return true;
}

bool GitProcessRunner::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

void GitProcessRunner::terminate()
{
    if (isRunning()) {
        m_process.terminate();
    }
}

void GitProcessRunner::kill()
{
    if (isRunning()) {
        m_process.kill();
    }
}

void GitProcessRunner::readAvailableOutput()
{
    const QByteArray output = m_process.readAll();
    if (!output.isEmpty()) {
        emit outputReceived(QString::fromLocal8Bit(output));
    }
}

void GitProcessRunner::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    readAvailableOutput();
    emit finished(exitCode, exitStatus == QProcess::NormalExit);
}

void GitProcessRunner::handleError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        emit errorOccurred(m_process.errorString());
    }
}

} // namespace gitclone
