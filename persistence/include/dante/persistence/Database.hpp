#pragma once

#include <QString>

#include <dante/util/Result.hpp>

namespace dante {

// Thin RAII wrapper over a QSQLITE QSqlDatabase connection.
// ponytail: holds only the Qt connection *name*, not a QSqlDatabase member, so
// Qt6::Sql stays out of this header and the one-way dep (core <- persistence)
// holds. The live handle is fetched by name in the .cpp via QSqlDatabase::database().
class Database {
public:
    Database() = default;
    ~Database();

    // ponytail: non-copyable + non-movable (a declared dtor already kills the
    // implicit moves). F0 never needs to move a connection; add moves if it does.
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Opens a SQLite database at path and enables WAL journaling.
    Result<void> open(const QString& path);

    // Qt connection name; empty until open() succeeds.
    const QString& connectionName() const { return m_connectionName; }
    bool isOpen() const { return !m_connectionName.isEmpty(); }

private:
    void close();
    QString m_connectionName;
};

} // namespace dante
