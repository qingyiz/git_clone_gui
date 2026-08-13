#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace gitclone {

struct RepositoryInfo {
    QString absolutePath;
    QString relativePath;
};

struct WorkingTreeStatus {
    int stagedChanges = 0;
    int unstagedChanges = 0;
    int untrackedFiles = 0;
    int conflicts = 0;

    bool hasChanges() const
    {
        return stagedChanges > 0 || unstagedChanges > 0 || untrackedFiles > 0
            || conflicts > 0;
    }
};

struct BranchCatalog {
    QString currentBranch;
    QStringList localBranches;
    QStringList remoteBranches;
    QStringList remoteCandidates;
    WorkingTreeStatus workingTreeStatus;

    bool isDetached() const { return currentBranch.isEmpty(); }
};

struct BranchTarget {
    enum class Kind {
        Local,
        Remote
    };

    Kind kind = Kind::Local;
    QString name;
};

QStringList remoteBranchCandidates(const QStringList &localBranches,
                                   const QStringList &remoteBranches);

class WorkspaceService : public QObject {
    Q_OBJECT

public:
    explicit WorkspaceService(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~WorkspaceService() override = default;

    virtual void scan(const QString &rootPath) = 0;
    virtual void cancelScan() = 0;
    virtual void loadBranches(const QString &repositoryPath) = 0;
    virtual void switchBranch(const QString &repositoryPath,
                              const BranchTarget &target) = 0;
    virtual void cancelGitOperation() = 0;

signals:
    void scanBusyChanged(bool busy);
    void scanFinished(const QString &rootPath,
                      const QVector<gitclone::RepositoryInfo> &repositories,
                      int skippedDirectories);
    void scanFailed(const QString &rootPath, const QString &message);
    void gitBusyChanged(bool busy);
    void branchesLoaded(const QString &repositoryPath,
                        const gitclone::BranchCatalog &catalog);
    void branchLoadFailed(const QString &repositoryPath, const QString &message);
    void branchSwitchSucceeded(const QString &repositoryPath,
                               const QString &branchName);
    void branchSwitchFailed(const QString &repositoryPath, const QString &message);
};

} // namespace gitclone

Q_DECLARE_METATYPE(gitclone::RepositoryInfo)
Q_DECLARE_METATYPE(QVector<gitclone::RepositoryInfo>)
Q_DECLARE_METATYPE(gitclone::BranchCatalog)
Q_DECLARE_METATYPE(gitclone::WorkingTreeStatus)
Q_DECLARE_METATYPE(gitclone::BranchTarget)
