#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace dante::term {

// Recebe os eventos decodificados do parser. O Screen implementa isto.
class IVtSink {
public:
    virtual ~IVtSink() = default;

    virtual void print(char32_t cp) = 0;           // glifo imprimível
    virtual void execute(std::uint8_t control) = 0; // controle C0 (LF, CR, BS, HT, BEL...)
    virtual void csi(char finalByte, const std::vector<int>& params) = 0; // ESC [ ... <final>
    virtual void osc(const std::string& data) = 0;  // ESC ] ... (BEL|ST)
    virtual void esc(char finalByte) = 0;           // ESC <final> simples (sem CSI/OSC)
};

// Parser VT/ANSI tabela-driven (baseado no DEC ANSI state machine de Paul Williams,
// vt100.net). Pura lógica — zero Qt/OS. Faz decode UTF-8 dos imprimíveis.
class VtParser {
public:
    explicit VtParser(IVtSink& sink) : sink_(sink) {}

    void feed(std::string_view bytes);

private:
    enum class State : std::uint8_t {
        Ground,
        Esc,
        CsiParam,
        CsiIgnore,
        Osc,
        OscEsc, // dentro de OSC, vimos ESC (esperando '\' do ST)
    };

    void byte(std::uint8_t b);
    void printByte(std::uint8_t b);
    void dispatchCsi(std::uint8_t finalByte);

    IVtSink& sink_;
    State state_ = State::Ground;

    std::vector<int> params_;
    int curParam_ = 0;
    bool hasParam_ = false;

    std::string osc_;

    // decode UTF-8
    char32_t utf_ = 0;
    int utfPending_ = 0;
};

} // namespace dante::term
