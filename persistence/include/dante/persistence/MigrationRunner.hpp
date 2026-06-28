#pragma once

#include <dante/persistence/Database.hpp>
#include <dante/util/Result.hpp>

namespace dante {

// Idempotent schema migrator.
// F0: only ensures the schema_version table exists and reports the current
// version (0 on a fresh database). Real migration steps land in F3.
//
// gotcha #2 (migration-safe): every column added by a future migration must be
// OPTIONAL with a DEFAULT, so existing rows and older binaries keep working.
// Never add a NOT NULL column without a default.
class MigrationRunner {
public:
    // Ensures schema_version exists; returns the current schema version.
    static Result<int> apply(Database& db);
};

} // namespace dante
