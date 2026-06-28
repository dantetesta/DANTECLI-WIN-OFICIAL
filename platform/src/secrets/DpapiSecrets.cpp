#include <dante/platform/secrets/DpapiSecrets.hpp>

namespace dante {

#ifdef _WIN32

// ponytail: F6 implementa DPAPI (CryptProtectData/CryptUnprotectData). gotcha #6: load LAZY.
Result<void> DpapiSecrets::store(const std::string&, const std::string&) {
    return fail("DPAPI: implementar na F6");
}

Result<std::string> DpapiSecrets::load(const std::string&) {
    return fail("DPAPI: implementar na F6");
}

#else

Result<void> DpapiSecrets::store(const std::string&, const std::string&) {
    return fail("DPAPI indisponivel fora do Windows");
}

Result<std::string> DpapiSecrets::load(const std::string&) {
    return fail("DPAPI indisponivel fora do Windows");
}

#endif

} // namespace dante
