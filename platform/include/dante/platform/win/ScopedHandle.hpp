#pragma once

// RAII para handles Win32. Header autossuficiente: garante as macros antes de
// <windows.h> pra HPCON/ConPTY ficarem disponiveis independente de quem inclui.
#ifdef _WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00 // Windows 10 — exigido pelas APIs de ConPTY (pseudoconsole)
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <utility>

namespace dante::win {

// unique_ptr-like para HANDLE: fecha com CloseHandle. Trata nullptr e
// INVALID_HANDLE_VALUE como "vazio".
class unique_handle {
public:
    unique_handle() noexcept = default;
    explicit unique_handle(HANDLE h) noexcept : h_(h) {}
    ~unique_handle() { reset(); }

    unique_handle(unique_handle&& o) noexcept : h_(std::exchange(o.h_, nullptr)) {}
    unique_handle& operator=(unique_handle&& o) noexcept {
        if (this != &o) {
            reset();
            h_ = std::exchange(o.h_, nullptr);
        }
        return *this;
    }
    unique_handle(const unique_handle&) = delete;
    unique_handle& operator=(const unique_handle&) = delete;

    [[nodiscard]] HANDLE get() const noexcept { return h_; }
    [[nodiscard]] bool valid() const noexcept { return h_ != nullptr && h_ != INVALID_HANDLE_VALUE; }

    // Para APIs que escrevem o handle de saida (ex.: CreatePipe).
    HANDLE* put() noexcept {
        reset();
        return &h_;
    }
    [[nodiscard]] HANDLE release() noexcept { return std::exchange(h_, nullptr); }

    void reset(HANDLE h = nullptr) noexcept {
        if (valid()) {
            ::CloseHandle(h_);
        }
        h_ = h;
    }

private:
    HANDLE h_ = nullptr;
};

// RAII para o pseudoconsole (HPCON): fecha com ClosePseudoConsole.
class unique_pseudoconsole {
public:
    unique_pseudoconsole() noexcept = default;
    explicit unique_pseudoconsole(HPCON h) noexcept : h_(h) {}
    ~unique_pseudoconsole() { reset(); }

    unique_pseudoconsole(unique_pseudoconsole&& o) noexcept : h_(std::exchange(o.h_, nullptr)) {}
    unique_pseudoconsole& operator=(unique_pseudoconsole&& o) noexcept {
        if (this != &o) {
            reset();
            h_ = std::exchange(o.h_, nullptr);
        }
        return *this;
    }
    unique_pseudoconsole(const unique_pseudoconsole&) = delete;
    unique_pseudoconsole& operator=(const unique_pseudoconsole&) = delete;

    [[nodiscard]] HPCON get() const noexcept { return h_; }
    [[nodiscard]] bool valid() const noexcept { return h_ != nullptr; }

    void reset(HPCON h = nullptr) noexcept {
        if (h_) {
            ::ClosePseudoConsole(h_);
        }
        h_ = h;
    }

private:
    HPCON h_ = nullptr;
};

} // namespace dante::win

#endif // _WIN32
