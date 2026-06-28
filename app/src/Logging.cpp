#include "Logging.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QStandardPaths>
#include <QTextStream>

namespace dante {
namespace {

// ponytail: global lock; um log de app raramente é gargalo. Trocar por fila
// assíncrona só se aparecer contenção real.
QMutex g_logMutex;

void messageHandler(QtMsgType type, const QMessageLogContext&, const QString& message) {
    QMutexLocker locker(&g_logMutex);

    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/logs";
    QDir().mkpath(dir);

    QFile file(dir + "/dante.log");
    if (!file.open(QIODevice::Append | QIODevice::Text))
        return;

    const char* level = "INFO";
    switch (type) {
        case QtDebugMsg:    level = "DEBUG"; break;
        case QtInfoMsg:     level = "INFO";  break;
        case QtWarningMsg:  level = "WARN";  break;
        case QtCriticalMsg: level = "CRIT";  break;
        case QtFatalMsg:    level = "FATAL"; break;
    }

    QTextStream out(&file);
    out << QDateTime::currentDateTime().toString(Qt::ISODate)
        << " [" << level << "] " << message << '\n';
}

} // namespace

void installFileLogger() {
    qInstallMessageHandler(messageHandler);
}

} // namespace dante
