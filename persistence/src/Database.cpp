#include <dante/persistence/Database.hpp>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

namespace dante {

namespace {
// Unique per live instance: the object address never collides while it exists,
// and Database is non-movable so the address stays valid for the connection's life.
QString makeConnectionName(const void* self) {
    return QStringLiteral("dante_db_")
           + QString::number(static_cast<qulonglong>(reinterpret_cast<quintptr>(self)), 16);
}
} // namespace

Database::~Database() {
    close();
}

Result<void> Database::open(const QString& path) {
    if (isOpen()) {
        return std::unexpected(Error{"Database already open"});
    }

    const QString name = makeConnectionName(this);
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), name);
    db.setDatabaseName(path);

    if (!db.open()) {
        const QString err = db.lastError().text();
        QSqlDatabase::removeDatabase(name);
        return std::unexpected(Error{("Failed to open SQLite database: " + err).toStdString()});
    }

    // WAL: concurrent readers during a write, and crash-safe. Trivial + cross-platform.
    QSqlQuery(db).exec(QStringLiteral("PRAGMA journal_mode=WAL"));

    m_connectionName = name;
    return {};
}

void Database::close() {
    if (m_connectionName.isEmpty()) {
        return;
    }
    // Scope the handle so it's destroyed before removeDatabase(); otherwise Qt
    // warns that the connection is still in use. ponytail: keep the braces.
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, /*open=*/false);
        if (db.isOpen()) {
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(m_connectionName);
    m_connectionName.clear();
}

} // namespace dante
