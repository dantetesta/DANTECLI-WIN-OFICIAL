#pragma once

#include <dante/persistence/Database.hpp>
#include <dante/persistence/KvStore.hpp>

#include <QObject>
#include <QString>

namespace dante {

// Ponte QML <-> persistência. Abre o SQLite (WAL) em %LOCALAPPDATA%\...\dante.db, roda as
// migrações e expõe load/save de strings à UI. Favoritos/snippets são JSON serializado no QML
// e guardados por chave. Falhas de I/O são best-effort (nunca derrubam a UI) — o app só
// perde a persistência daquela operação, cai no comportamento em-memória.
class StoreController : public QObject {
    Q_OBJECT
public:
    explicit StoreController(QObject* parent = nullptr);

    Q_INVOKABLE QString load(const QString& key);              // "" se ausente/erro
    Q_INVOKABLE void save(const QString& key, const QString& value);

private:
    Database db_;
    KvStore kv_;
};

} // namespace dante
