#pragma once

#include "core/CloneRequest.h"

#include <QObject>

namespace gitclone {

class ProcessRunner : public QObject {
    Q_OBJECT

public:
    explicit ProcessRunner(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~ProcessRunner() override = default;

    virtual bool start(const ProcessCommand &command) = 0;
    virtual bool isRunning() const = 0;
    virtual void terminate() = 0;
    virtual void kill() = 0;

signals:
    void outputReceived(const QString &text);
    void finished(int exitCode, bool normalExit);
    void errorOccurred(const QString &message);
};

} // namespace gitclone
