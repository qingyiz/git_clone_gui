#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace gitclone {

struct ChildRepositoryRequest {
    QString repositoryUrl;
    QString branch;
    QString relativePath;
};

struct CloneRequest {
    QString parentRepositoryUrl;
    QString parentBranch;
    QString parentDirectoryName;
    QString destinationRoot;
    QList<ChildRepositoryRequest> children;
};

struct ProcessCommand {
    QString program;
    QStringList arguments;
    QString workingDirectory;
};

struct ChildClonePlan {
    ProcessCommand command;
    QString targetPath;
};

struct ClonePlan {
    ProcessCommand parentCommand;
    QString parentTargetPath;
    QList<ChildClonePlan> children;
};

struct ValidationResult {
    bool valid = false;
    QStringList errors;
    ClonePlan plan;
};

ValidationResult buildClonePlan(const CloneRequest &request);
QString commandPreview(const ProcessCommand &command);

} // namespace gitclone
