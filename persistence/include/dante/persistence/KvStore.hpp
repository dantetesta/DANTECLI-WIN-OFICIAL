#pragma once

#include <dante/persistence/Database.hpp>
#include <dante/util/Result.hpp>

#include <QString>

namespace dante {

// Chave-valor simples sobre a tabela `kv` (criada pela migração v1). Favoritos e snippets
// são serializados como JSON e guardados aqui — no volume do app (dezenas de linhas) isso é
// mais enxuto e migração-safe que um schema relacional. Prepared statements RAII (via
// QSqlQuery), Qt6::Sql confinado a persistence/ (a UI fala com StoreController).
class KvStore {
public:
    explicit KvStore(Database& db) : db_(db) {}

    // Erro se a chave não existir (o chamador trata ausência como "usar default").
    Result<QString> get(const QString& key) const;
    Result<void> set(const QString& key, const QString& value);

private:
    Database& db_;
};

} // namespace dante
