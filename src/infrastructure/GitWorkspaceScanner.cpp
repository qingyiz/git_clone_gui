#include "infrastructure/GitWorkspaceScanner.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <algorithm>

#ifdef Q_OS_UNIX
#include <dirent.h>
#include <sys/stat.h>
#endif

namespace gitclone {
namespace {

bool stablePathLessThan(const RepositoryInfo &left, const RepositoryInfo &right)
{
    const int folded = QString::compare(left.relativePath, right.relativePath,
                                        Qt::CaseInsensitive);
    return folded == 0 ? left.relativePath < right.relativePath : folded < 0;
}

struct DirectoryScan {
    QStringList childDirectories;
    bool repository = false;
    bool readable = true;
    bool cancelled = false;
};

#ifdef Q_OS_UNIX
DirectoryScan scanDirectoryEntries(
    const QString &directoryPath,
    const std::shared_ptr<std::atomic_bool> &cancelled)
{
    DirectoryScan scan;
    const QByteArray encodedDirectory = QFile::encodeName(directoryPath);
    DIR *directory = opendir(encodedDirectory.constData());
    if (directory == nullptr) {
        scan.readable = false;
        return scan;
    }

    while (dirent *entry = readdir(directory)) {
        if (cancelled->load()) {
            scan.cancelled = true;
            break;
        }
        const QByteArray name(entry->d_name);
        if (name == "." || name == "..") {
            continue;
        }

        unsigned char type = entry->d_type;
        if (type == DT_UNKNOWN) {
            QByteArray encodedChild = encodedDirectory;
            if (!encodedChild.endsWith('/')) {
                encodedChild.append('/');
            }
            encodedChild.append(name);
            struct stat metadata {};
            if (lstat(encodedChild.constData(), &metadata) != 0) {
                continue;
            }
            if (S_ISLNK(metadata.st_mode)) {
                type = DT_LNK;
            } else if (S_ISDIR(metadata.st_mode)) {
                type = DT_DIR;
            } else if (S_ISREG(metadata.st_mode)) {
                type = DT_REG;
            }
        }

        if (name == ".git") {
            scan.repository = type == DT_DIR || type == DT_REG;
        } else if (type == DT_DIR) {
            scan.childDirectories.append(
                QDir(directoryPath).filePath(QFile::decodeName(name)));
        }
    }
    closedir(directory);
    return scan;
}
#else
DirectoryScan scanDirectoryEntries(
    const QString &directoryPath,
    const std::shared_ptr<std::atomic_bool> &cancelled)
{
    DirectoryScan scan;
    const QFileInfo directoryInfo(directoryPath);
    if (!directoryInfo.isReadable()) {
        scan.readable = false;
        return scan;
    }
    const QFileInfo marker(QDir(directoryPath).filePath(QStringLiteral(".git")));
    scan.repository = marker.exists() && !marker.isSymLink()
        && (marker.isDir() || marker.isFile());
    const QStringList children = QDir(directoryPath).entryList(
        QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System
            | QDir::NoSymLinks,
        QDir::NoSort);
    for (const QString &childName : children) {
        if (cancelled->load()) {
            scan.cancelled = true;
            break;
        }
        if (childName != QStringLiteral(".git")) {
            scan.childDirectories.append(QDir(directoryPath).filePath(childName));
        }
    }
    return scan;
}
#endif

} // namespace

WorkspaceScanResult scanWorkspaceDirectories(
    const QString &rootPath,
    const std::shared_ptr<std::atomic_bool> &cancelled)
{
    WorkspaceScanResult result;
    result.rootPath = QDir::cleanPath(QDir(rootPath).absolutePath());
    QStringList pending {result.rootPath};

    while (!pending.isEmpty()) {
        if (cancelled->load()) {
            result.cancelled = true;
            return result;
        }
        const QString directoryPath = pending.takeLast();
        const DirectoryScan directory = scanDirectoryEntries(directoryPath, cancelled);
        if (directory.cancelled) {
            result.cancelled = true;
            return result;
        }
        if (!directory.readable) {
            ++result.skippedDirectories;
            continue;
        }

        if (directory.repository) {
            RepositoryInfo repository;
            repository.absolutePath = QDir::cleanPath(directoryPath);
            repository.relativePath =
                QDir(result.rootPath).relativeFilePath(repository.absolutePath);
            if (repository.relativePath.isEmpty()) {
                repository.relativePath = QStringLiteral(".");
            }
            result.repositories.append(repository);
        }
        for (const QString &childPath : directory.childDirectories) {
            pending.append(childPath);
        }
    }

    std::sort(result.repositories.begin(), result.repositories.end(), stablePathLessThan);
    return result;
}

} // namespace gitclone
