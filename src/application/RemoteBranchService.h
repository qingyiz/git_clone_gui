#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace gitclone {

struct RemoteBranchCatalog {
    QString defaultBranch;
    QStringList branches;
};

class RemoteBranchService : public QObject {
    Q_OBJECT

public:
    using RequestId = quint64;

    explicit RemoteBranchService(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~RemoteBranchService() override = default;

    virtual RequestId requestBranches(const QString &repositoryUrl) = 0;
    virtual void cancelRequest(RequestId requestId) = 0;

signals:
    void branchesReady(gitclone::RemoteBranchService::RequestId requestId,
                       const gitclone::RemoteBranchCatalog &catalog);
    void branchQueryFailed(gitclone::RemoteBranchService::RequestId requestId,
                           const QString &message);
};

} // namespace gitclone

Q_DECLARE_METATYPE(gitclone::RemoteBranchCatalog)
