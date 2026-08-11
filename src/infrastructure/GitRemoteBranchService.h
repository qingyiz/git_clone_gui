#pragma once

#include "application/RemoteBranchService.h"

#include <QHash>
#include <QSet>

namespace gitclone {

class GitRemoteBranchService final : public RemoteBranchService {
    Q_OBJECT

public:
    explicit GitRemoteBranchService(QObject *parent = nullptr,
                                    int timeoutMs = 15000);
    ~GitRemoteBranchService() override;

    RequestId requestBranches(const QString &repositoryUrl) override;
    void cancelRequest(RequestId requestId) override;

private:
    struct QueryContext;

    void completeQuery(RequestId requestId);
    void failQuery(RequestId requestId, const QString &message);
    void releaseQuery(RequestId requestId, bool stopProcess);
    static RemoteBranchCatalog parseCatalog(const QByteArray &output);
    static void orderBranches(RemoteBranchCatalog &catalog);

    RequestId m_nextRequestId = 0;
    int m_timeoutMs;
    QHash<RequestId, QueryContext *> m_queries;
    QHash<QString, RemoteBranchCatalog> m_cache;
    QSet<RequestId> m_pendingDeliveries;
};

} // namespace gitclone
