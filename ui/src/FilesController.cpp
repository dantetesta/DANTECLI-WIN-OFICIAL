#include "dante/ui/FilesController.hpp"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QSet>
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

QString FilesController::fileKind(const QString& path) const {
    const QString ext = QFileInfo(path).suffix().toLower();
    static const QSet<QString> img{"png", "jpg", "jpeg", "gif", "bmp", "webp", "svg", "ico"};
    static const QSet<QString> vid{"mp4", "mov", "mkv", "webm", "avi", "m4v"};
    static const QSet<QString> aud{"mp3", "wav", "ogg", "flac", "m4a", "aac"};
    // Binários/documentos: abrir no app externo (PDF/office/preview embutido = F5+).
    static const QSet<QString> ext_ext{"pdf", "zip", "rar", "7z", "gz", "exe", "dll", "xlsx",
                                       "docx", "pptx", "odt", "bin"};
    if (img.contains(ext)) {
        return "image";
    }
    if (vid.contains(ext)) {
        return "video";
    }
    if (aud.contains(ext)) {
        return "audio";
    }
    if (ext_ext.contains(ext)) {
        return "other";
    }
    return "editor"; // texto/código por padrão
}

QVariantMap FilesController::readFile(const QString& path) const {
    QVariantMap m;
    m["ok"] = false;
    m["text"] = QString();
    m["plain"] = false;
    m["tooBig"] = false;
    const QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile()) {
        return m;
    }
    if (fi.size() > 5 * 1024 * 1024) { // >5MB: não carrega no editor
        m["tooBig"] = true;
        return m;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return m;
    }
    m["text"] = QString::fromUtf8(f.readAll());
    m["plain"] = fi.size() > 500 * 1024; // plain-mode: sem realce em arquivos grandes (PRD H)
    m["ok"] = true;
    return m;
}

bool FilesController::writeFile(const QString& path, const QString& text) const {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    const QByteArray data = text.toUtf8();
    return f.write(data) == data.size();
}

void FilesController::openExternally(const QString& path) const {
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
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
