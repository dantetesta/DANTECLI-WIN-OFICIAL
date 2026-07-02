// Round-trip da persistência: migração cria a tabela kv, set/get devolve o valor, e um
// segundo set na mesma chave faz upsert (não duplica). Lógica pura de dados — roda no mac e no
// CI. CHECK (não assert) p/ valer no Release.

#include <dante/persistence/Database.hpp>
#include <dante/persistence/KvStore.hpp>
#include <dante/persistence/MigrationRunner.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>

#include <cstdio>

using namespace dante;

static int g_fail = 0;
#define CHECK(cond, msg)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            std::fprintf(stderr, "FAIL: %s\n", (msg));    \
            ++g_fail;                                     \
        }                                                 \
    } while (0)

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv); // QSqlDatabase precisa de uma instância viva

    QTemporaryDir tmp;
    CHECK(tmp.isValid(), "tempdir válido");
    const QString dbPath = tmp.path() + "/t.db";

    {
        Database db;
        CHECK(db.open(dbPath).has_value(), "abrir DB");
        auto mig = MigrationRunner::apply(db);
        CHECK(mig.has_value() && *mig == 1, "migração -> v1");

        KvStore kv(db);
        CHECK(!kv.get("favorites").has_value(), "chave ausente = erro (usa default)");
        CHECK(kv.set("favorites", "[{\"title\":\"a\"}]").has_value(), "set");
        auto got = kv.get("favorites");
        CHECK(got.has_value() && *got == "[{\"title\":\"a\"}]", "get devolve o valor salvo");

        CHECK(kv.set("favorites", "[]").has_value(), "upsert (mesma chave)");
        auto got2 = kv.get("favorites");
        CHECK(got2.has_value() && *got2 == "[]", "upsert sobrescreve, não duplica");
    }

    // Reabrir: os dados sobrevivem (migração idempotente não zera nada — gotcha #2).
    {
        Database db;
        CHECK(db.open(dbPath).has_value(), "reabrir DB");
        auto mig = MigrationRunner::apply(db);
        CHECK(mig.has_value() && *mig == 1, "reabrir mantém v1 (idempotente)");
        KvStore kv(db);
        auto got = kv.get("favorites");
        CHECK(got.has_value() && *got == "[]", "valor persistiu após reabrir");
    }

    if (g_fail > 0) {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", g_fail);
        return 1;
    }
    std::puts("ok: KvStore + MigrationRunner");
    return 0;
}
