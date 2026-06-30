#include "dante/ui/FilesController.hpp"

#include <QDir>
#include <QFileInfo>
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
