#include "dante/ui/FilesController.hpp"

#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QVariantMap>

namespace dante {

FilesController::FilesController(QObject* parent)
    : QObject(parent), root_(QDir::homePath()) {}

void FilesController::setRoot(const QString& p) {
    if (p.isEmpty() || p == root_) {
        return;
    }
    root_ = p;
    emit rootChanged();
}

void FilesController::setShowHidden(bool v) {
    if (v == showHidden_) {
        return;
    }
    showHidden_ = v;
    emit showHiddenChanged();
}

QVariantList FilesController::list(const QString& path) const {
    QDir dir(path.isEmpty() ? root_ : path);
    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
    if (showHidden_) {
        filters |= QDir::Hidden;
    }
    const auto entries = dir.entryInfoList(
        filters, QDir::DirsFirst | QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);

    QVariantList out;
    out.reserve(entries.size());
    for (const QFileInfo& fi : entries) {
        QVariantMap m;
        m["name"] = fi.fileName();
        m["dir"] = fi.isDir();
        m["path"] = fi.absoluteFilePath();
        out.append(m);
    }
    return out;
}

QString FilesController::home() const { return QDir::homePath(); }

QString FilesController::standard(const QString& which) const {
    QStandardPaths::StandardLocation loc = QStandardPaths::HomeLocation;
    if (which == "desktop") {
        loc = QStandardPaths::DesktopLocation;
    } else if (which == "documents") {
        loc = QStandardPaths::DocumentsLocation;
    } else if (which == "downloads") {
        loc = QStandardPaths::DownloadLocation;
    }
    const QString p = QStandardPaths::writableLocation(loc);
    return p.isEmpty() ? QDir::homePath() : p;
}

QString FilesController::baseName(const QString& path) const {
    const QString name = QDir(path).dirName();
    return name.isEmpty() ? path : name; // raiz ("/") nao tem dirName
}

QString FilesController::parentDir(const QString& path) const {
    QDir d(path);
    return d.cdUp() ? d.absolutePath() : path;
}

void FilesController::revealInOS(const QString& path) const {
#if defined(Q_OS_MAC)
    QProcess::startDetached("open", {"-R", path});
#elif defined(Q_OS_WIN)
    QProcess::startDetached("explorer.exe", {"/select," + QDir::toNativeSeparators(path)});
#else
    QProcess::startDetached("xdg-open", {QFileInfo(path).absolutePath()});
#endif
}

void FilesController::copyPath(const QString& path) const {
    if (auto* cb = QGuiApplication::clipboard()) {
        cb->setText(path);
    }
}

bool FilesController::makeFolder(const QString& parent, const QString& name) const {
    if (name.trimmed().isEmpty()) {
        return false;
    }
    return QDir(parent).mkdir(name);
}

bool FilesController::makeFile(const QString& parent, const QString& name) const {
    if (name.trimmed().isEmpty()) {
        return false;
    }
    QFile f(QDir(parent).absoluteFilePath(name));
    if (f.exists()) {
        return false;
    }
    return f.open(QIODevice::WriteOnly) && (f.close(), true);
}

bool FilesController::renamePath(const QString& path, const QString& newName) const {
    if (newName.trimmed().isEmpty()) {
        return false;
    }
    const QFileInfo fi(path);
    return QFile::rename(path, fi.absoluteDir().absoluteFilePath(newName));
}

bool FilesController::trashPath(const QString& path) const {
    return QFile::moveToTrash(path);
}

QString FilesController::displayPath(const QString& path) const {
    const QString h = QDir::homePath();
    if (path == h) {
        return "~";
    }
    if (path.startsWith(h + "/")) {
        return "~" + path.mid(h.size());
    }
    return path;
}

} // namespace dante
