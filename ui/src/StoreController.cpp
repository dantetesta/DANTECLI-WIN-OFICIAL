#include "dante/ui/StoreController.hpp"

#include <dante/persistence/MigrationRunner.hpp>

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

namespace dante {

StoreController::StoreController(QObject* parent) : QObject(parent), kv_(db_) {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir); // %LOCALAPPDATA%\<Org>\<App> (fora do OneDrive) / ~/Library/... no mac
    if (auto opened = db_.open(dir + "/dante.db"); !opened) {
        qWarning() << "StoreController: DB não abriu:" << QString::fromStdString(opened.error().message);
        return;
    }
    if (auto m = MigrationRunner::apply(db_); !m) {
        qWarning() << "StoreController: migração falhou:" << QString::fromStdString(m.error().message);
    }
}

QString StoreController::load(const QString& key) {
    auto r = kv_.get(key);
    return r ? *r : QString();
}

void StoreController::save(const QString& key, const QString& value) {
    if (auto r = kv_.set(key, value); !r) {
        qWarning() << "StoreController: save falhou p/" << key << ":"
                   << QString::fromStdString(r.error().message);
    }
}

} // namespace dante
