#pragma once

#include <dante/util/Result.hpp>

#include <string>

namespace dante {

// gotcha #6: ler LAZY (descriptografar so quando o segredo for realmente usado).
class ISecretStore {
public:
    virtual ~ISecretStore() = default;
    virtual Result<void> store(const std::string& key, const std::string& value) = 0;
    virtual Result<std::string> load(const std::string& key) = 0;
};

// DPAPI = F6. Na F0 so stubs.
class DpapiSecrets final : public ISecretStore {
public:
    Result<void> store(const std::string& key, const std::string& value) override;
    Result<std::string> load(const std::string& key) override;
};

} // namespace dante
