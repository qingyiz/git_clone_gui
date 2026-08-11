#include "core/CloneRequest.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

namespace gitclone {
namespace {

QString quoteForDisplay(const QString &argument)
{
    static const QRegularExpression safeCharacters(QStringLiteral("^[A-Za-z0-9_./:@%+=,-]+$"));
    if (!argument.isEmpty() && safeCharacters.match(argument).hasMatch()) {
        return argument;
    }

    QString escaped = argument;
    escaped.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QStringLiteral("'") + escaped + QStringLiteral("'");
}

bool isSingleDirectoryName(const QString &name)
{
    return !name.isEmpty()
        && name != QStringLiteral(".")
        && name != QStringLiteral("..")
        && !name.contains(QLatin1Char('/'))
        && !name.contains(QLatin1Char('\\'));
}

QString normalizeRelativePath(QString path)
{
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return QDir::cleanPath(path);
}

bool containsParentTraversal(QString path)
{
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return path.split(QLatin1Char('/'), Qt::KeepEmptyParts).contains(QStringLiteral(".."));
}

bool isUnsafeRelativePath(const QString &path)
{
    const QString slashNormalized = QString(path).replace(QLatin1Char('\\'), QLatin1Char('/'));
    static const QRegularExpression windowsAbsolutePath(QStringLiteral("^[A-Za-z]:/"));
    return QDir::isAbsolutePath(path)
        || QDir::isAbsolutePath(slashNormalized)
        || windowsAbsolutePath.match(slashNormalized).hasMatch()
        || slashNormalized.startsWith(QStringLiteral("//"))
        || containsParentTraversal(path);
}

void requireValue(const QString &value, const QString &label, QStringList &errors)
{
    if (value.trimmed().isEmpty()) {
        errors.append(QStringLiteral("%1不能为空。").arg(label));
    }
}

QString pathIdentity(QString path)
{
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    path = path.toCaseFolded();
#endif
    return path;
}

} // namespace

ValidationResult buildClonePlan(const CloneRequest &request)
{
    ValidationResult result;

    const QString parentUrl = request.parentRepositoryUrl.trimmed();
    const QString parentBranch = request.parentBranch.trimmed();
    const QString parentDirectory = request.parentDirectoryName.trimmed();
    const QString destinationInput = request.destinationRoot.trimmed();

    requireValue(parentUrl, QStringLiteral("父仓库 URL"), result.errors);
    requireValue(parentBranch, QStringLiteral("父项目分支"), result.errors);
    requireValue(parentDirectory, QStringLiteral("父项目目录名"), result.errors);
    requireValue(destinationInput, QStringLiteral("目标根目录"), result.errors);

    if (!parentDirectory.isEmpty() && !isSingleDirectoryName(parentDirectory)) {
        result.errors.append(QStringLiteral("父项目目录名必须是单个目录名称，不能包含路径分隔符，也不能是 . 或 ..。"));
    }

    const QFileInfo destinationInfo(destinationInput);
    if (!destinationInput.isEmpty() && (!destinationInfo.exists() || !destinationInfo.isDir())) {
        result.errors.append(QStringLiteral("目标根目录不存在或不是目录。"));
    }

    QString destinationRoot;
    QString parentTarget;
    if (destinationInfo.exists() && destinationInfo.isDir() && isSingleDirectoryName(parentDirectory)) {
        destinationRoot = QDir::cleanPath(destinationInfo.absoluteFilePath());
        parentTarget = QDir::cleanPath(QDir(destinationRoot).absoluteFilePath(parentDirectory));
        if (QFileInfo::exists(parentTarget)) {
            result.errors.append(QStringLiteral("父项目目标目录已存在，应用不会覆盖或删除它：%1").arg(parentTarget));
        }
    }

    QSet<QString> childTargets;
    for (int index = 0; index < request.children.size(); ++index) {
        const ChildRepositoryRequest &child = request.children.at(index);
        const QString prefix = QStringLiteral("子仓库 #%1").arg(index + 1);
        const QString childUrl = child.repositoryUrl.trimmed();
        const QString childBranch = child.branch.trimmed();
        const QString childInputPath = child.relativePath.trimmed();

        requireValue(childUrl, prefix + QStringLiteral(" URL"), result.errors);
        requireValue(childBranch, prefix + QStringLiteral("分支"), result.errors);
        requireValue(childInputPath, prefix + QStringLiteral("相对路径"), result.errors);

        QString normalizedChildPath;
        if (!childInputPath.isEmpty()) {
            if (isUnsafeRelativePath(childInputPath)) {
                result.errors.append(prefix + QStringLiteral("路径必须是父项目内的安全相对路径，不能是绝对路径或包含 ..。"));
            } else {
                normalizedChildPath = normalizeRelativePath(childInputPath);
                if (normalizedChildPath.isEmpty() || normalizedChildPath == QStringLiteral(".")) {
                    result.errors.append(prefix + QStringLiteral("路径必须指向父项目内的子目录。"));
                }
            }
        }

        QString childTarget;
        if (!parentTarget.isEmpty() && !normalizedChildPath.isEmpty()) {
            childTarget = QDir::cleanPath(QDir(parentTarget).absoluteFilePath(normalizedChildPath));
            QString parentPrefix = parentTarget;
            if (!parentPrefix.endsWith(QDir::separator())) {
                parentPrefix.append(QDir::separator());
            }
            if (!childTarget.startsWith(parentPrefix) || childTarget == parentTarget) {
                result.errors.append(prefix + QStringLiteral("目标路径不在父项目目录内。"));
            } else if (childTargets.contains(pathIdentity(childTarget))) {
                result.errors.append(prefix + QStringLiteral("目标路径与其他子仓库重复：%1").arg(childTarget));
            } else {
                childTargets.insert(pathIdentity(childTarget));
            }
        }

        if (!childUrl.isEmpty() && !childBranch.isEmpty() && !childTarget.isEmpty()) {
            result.plan.children.append({
                {QStringLiteral("git"),
                 {QStringLiteral("clone"), QStringLiteral("--branch"), childBranch,
                  QStringLiteral("--single-branch"), childUrl, childTarget},
                 parentTarget},
                childTarget
            });
        }
    }

    if (!result.errors.isEmpty()) {
        result.plan.children.clear();
        return result;
    }

    result.plan.parentTargetPath = parentTarget;
    result.plan.parentCommand = {
        QStringLiteral("git"),
        {QStringLiteral("clone"), QStringLiteral("--branch"), parentBranch,
         QStringLiteral("--single-branch"), parentUrl, parentTarget},
        destinationRoot
    };
    result.valid = true;
    return result;
}

QString commandPreview(const ProcessCommand &command)
{
    QStringList words;
    words.reserve(command.arguments.size() + 1);
    words.append(quoteForDisplay(command.program));
    for (const QString &argument : command.arguments) {
        words.append(quoteForDisplay(argument));
    }
    return words.join(QLatin1Char(' '));
}

} // namespace gitclone
