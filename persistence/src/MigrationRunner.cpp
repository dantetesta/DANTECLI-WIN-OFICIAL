#include <dante/persistence/MigrationRunner.hpp>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace dante {

Result<int> MigrationRunner::apply(Database& db) {
    if (!db.isOpen()) {
        return std::unexpected(Error{"MigrationRunner: database is not open"});
    }

    QSqlDatabase handle = QSqlDatabase::database(db.connectionName(), /*open=*/false);
    QSqlQuery query(handle);

    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS schema_version (version INTEGER NOT NULL)"))) {
        return std::unexpected(Error{
            ("MigrationRunner: cannot create schema_version: " + query.lastError().text())
                .toStdString()});
    }

    if (!query.exec(QStringLiteral("SELECT version FROM schema_version LIMIT 1"))) {
        return std::unexpected(Error{
            ("MigrationRunner: cannot read schema_version: " + query.lastError().text())
                .toStdString()});
    }

    // Empty table => fresh DB: seed version 0 exactly once (idempotent).
    int version = 0;
    if (query.next()) {
        version = query.value(0).toInt();
    } else if (!query.exec(QStringLiteral("INSERT INTO schema_version (version) VALUES (0)"))) {
        return std::unexpected(Error{
            ("MigrationRunner: cannot seed schema_version: " + query.lastError().text())
                .toStdString()});
    }

    // Passos versionados e idempotentes (gotcha #2: nunca coluna NOT NULL sem default; nunca
    // apagar dados do usuário). Cada passo roda uma vez; um binário antigo lendo um schema mais
    // novo continua funcionando porque só adicionamos, nunca removemos.
    // v1: tabela chave-valor (favoritos/snippets serializados como JSON).
    if (version < 1) {
        if (!query.exec(QStringLiteral(
                "CREATE TABLE IF NOT EXISTS kv (key TEXT PRIMARY KEY, value TEXT NOT NULL)"))) {
            return std::unexpected(Error{
                ("MigrationRunner: cannot create kv: " + query.lastError().text()).toStdString()});
        }
        if (!query.exec(QStringLiteral("UPDATE schema_version SET version = 1"))) {
            return std::unexpected(Error{
                ("MigrationRunner: cannot bump to v1: " + query.lastError().text()).toStdString()});
        }
        version = 1;
    }

    return version;
}

} // namespace dante
