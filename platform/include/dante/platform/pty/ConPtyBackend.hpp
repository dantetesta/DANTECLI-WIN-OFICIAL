#pragma once

#include <dante/services/IPtyBackend.hpp>
#include <dante/util/Result.hpp>

#include <cstdint>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#endif

namespace dante {

// ponytail: F1 implementa ConPtyBackend de verdade (CreatePseudoConsole + pipes + processo).
//           Na F0 isto e so o contrato; todos os metodos retornam Result de erro.
class ConPtyBackend final : public IPtyBackend {
public:
    ConPtyBackend() = default;
    ~ConPtyBackend() override { close(); }

    ConPtyBackend(const ConPtyBackend&) = delete;
    ConPtyBackend& operator=(const ConPtyBackend&) = delete;

    Result<void> start(const std::string& command, std::uint16_t cols, std::uint16_t rows) override;
    Result<void> write(std::string_view data) override;
    Result<std::string> read() override;
    Result<void> resize(std::uint16_t cols, std::uint16_t rows) override;
    void close() override;

private:
#ifdef _WIN32
    // F1 preenche e libera estes recursos.
    HPCON  pseudoConsole_{nullptr};
    HANDLE inputWrite_{nullptr};
    HANDLE outputRead_{nullptr};
    HANDLE process_{nullptr};
#endif
};

} // namespace dante
