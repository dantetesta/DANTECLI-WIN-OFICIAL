#include <dante/platform/pty/ConPtyBackend.hpp>

namespace dante {

#ifdef _WIN32

// ponytail: F1 implementa ConPtyBackend de verdade. Na F0 so o contrato.
Result<void> ConPtyBackend::start(const std::string&, std::uint16_t, std::uint16_t) {
    return fail("ConPTY: implementar na F1");
}

Result<void> ConPtyBackend::write(std::string_view) {
    return fail("ConPTY: implementar na F1");
}

Result<std::string> ConPtyBackend::read() {
    return fail("ConPTY: implementar na F1");
}

Result<void> ConPtyBackend::resize(std::uint16_t, std::uint16_t) {
    return fail("ConPTY: implementar na F1");
}

void ConPtyBackend::close() {
    // no-op na F0; F1 fecha HPCON e HANDLEs aqui.
}

#else // fora do Windows nao existe ConPTY.

Result<void> ConPtyBackend::start(const std::string&, std::uint16_t, std::uint16_t) {
    return fail("PTY indisponivel fora do Windows");
}

Result<void> ConPtyBackend::write(std::string_view) {
    return fail("PTY indisponivel fora do Windows");
}

Result<std::string> ConPtyBackend::read() {
    return fail("PTY indisponivel fora do Windows");
}

Result<void> ConPtyBackend::resize(std::uint16_t, std::uint16_t) {
    return fail("PTY indisponivel fora do Windows");
}

void ConPtyBackend::close() {
    // no-op.
}

#endif

} // namespace dante
