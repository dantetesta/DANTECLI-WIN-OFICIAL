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
    if (!query.next()) {
        if (!query.exec(QStringLiteral("INSERT INTO schema_version (version) VALUES (0)"))) {
            return std::unexpected(Error{
                ("MigrationRunner: cannot seed schema_version: " + query.lastError().text())
                    .toStdString()});
        }
        return 0;
    }

    return query.value(0).toInt();
}

} // namespace dante
