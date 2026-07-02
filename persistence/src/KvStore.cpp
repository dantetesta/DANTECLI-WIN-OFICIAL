#include <dante/persistence/KvStore.hpp>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace dante {

Result<QString> KvStore::get(const QString& key) const {
    if (!db_.isOpen()) {
        return fail("KvStore: database is not open");
    }
    QSqlDatabase handle = QSqlDatabase::database(db_.connectionName(), /*open=*/false);
    QSqlQuery query(handle);
    query.prepare(QStringLiteral("SELECT value FROM kv WHERE key = ?"));
    query.addBindValue(key);
    if (!query.exec()) {
        return fail(("KvStore: select falhou: " + query.lastError().text()).toStdString());
    }
    if (!query.next()) {
        return fail("KvStore: chave ausente");
    }
    return query.value(0).toString();
}

Result<void> KvStore::set(const QString& key, const QString& value) {
    if (!db_.isOpen()) {
        return fail("KvStore: database is not open");
    }
    QSqlDatabase handle = QSqlDatabase::database(db_.connectionName(), /*open=*/false);
    QSqlQuery query(handle);
    query.prepare(QStringLiteral("INSERT INTO kv (key, value) VALUES (?, ?) "
                                 "ON CONFLICT(key) DO UPDATE SET value = excluded.value"));
    query.addBindValue(key);
    query.addBindValue(value);
    if (!query.exec()) {
        return fail(("KvStore: upsert falhou: " + query.lastError().text()).toStdString());
    }
    return {};
}

} // namespace dante
