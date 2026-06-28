#include "dante/terminal/VtParser.hpp"

namespace dante::term {

namespace {
constexpr int kMaxParams = 32;
constexpr int kMaxParamVal = 65535;
} // namespace

void VtParser::feed(std::string_view bytes) {
    for (const char c : bytes) {
        byte(static_cast<std::uint8_t>(c));
    }
}

void VtParser::printByte(std::uint8_t b) {
    if (utfPending_ > 0) {
        if ((b & 0xC0) == 0x80) { // continuação válida
            utf_ = (utf_ << 6) | (b & 0x3F);
            if (--utfPending_ == 0) {
                sink_.print(utf_);
            }
            return;
        }
        // sequência inválida: descarta o que tinha e reprocessa este byte
        utfPending_ = 0;
        sink_.print(0xFFFD);
    }

    if (b < 0x80) {
        sink_.print(b);
    } else if ((b & 0xE0) == 0xC0) {
        utf_ = b & 0x1F;
        utfPending_ = 1;
    } else if ((b & 0xF0) == 0xE0) {
        utf_ = b & 0x0F;
        utfPending_ = 2;
    } else if ((b & 0xF8) == 0xF0) {
        utf_ = b & 0x07;
        utfPending_ = 3;
    } else {
        sink_.print(0xFFFD); // byte de início inválido
    }
}

void VtParser::dispatchCsi(std::uint8_t finalByte) {
    if (hasParam_) {
        if (static_cast<int>(params_.size()) < kMaxParams) {
            params_.push_back(curParam_);
        }
    }
    sink_.csi(static_cast<char>(finalByte), params_);
}

void VtParser::byte(std::uint8_t b) {
    switch (state_) {
    case State::Ground:
        if (b == 0x1B) {
            state_ = State::Esc;
        } else if (b < 0x20 || b == 0x7F) {
            sink_.execute(b);
        } else {
            printByte(b);
        }
        break;

    case State::Esc:
        if (b == '[') {
            params_.clear();
            curParam_ = 0;
            hasParam_ = false;
            state_ = State::CsiParam;
        } else if (b == ']') {
            osc_.clear();
            state_ = State::Osc;
        } else if (b >= 0x20 && b < 0x7F) {
            sink_.esc(static_cast<char>(b));
            state_ = State::Ground;
        } else {
            state_ = State::Ground;
        }
        break;

    case State::CsiParam:
        if (b >= '0' && b <= '9') {
            curParam_ = curParam_ * 10 + (b - '0');
            if (curParam_ > kMaxParamVal) {
                curParam_ = kMaxParamVal;
            }
            hasParam_ = true;
        } else if (b == ';') {
            if (static_cast<int>(params_.size()) < kMaxParams) {
                params_.push_back(curParam_);
            }
            curParam_ = 0;
            hasParam_ = true; // separador conta o próximo (mesmo vazio)
        } else if (b >= 0x40 && b <= 0x7E) {
            dispatchCsi(b);
            state_ = State::Ground;
        } else if (b == 0x1B) {
            state_ = State::Esc; // aborta o CSI
        } else if (b >= 0x20 && b <= 0x2F) {
            state_ = State::CsiIgnore; // intermediários: ignora a sequência
        } else if (b < 0x20) {
            sink_.execute(b); // controle dentro do CSI: executa, mantém estado
        }
        // 0x3A (':') e privados (0x3C-0x3F) caem aqui e são ignorados silenciosamente
        break;

    case State::CsiIgnore:
        if (b >= 0x40 && b <= 0x7E) {
            state_ = State::Ground;
        } else if (b == 0x1B) {
            state_ = State::Esc;
        }
        break;

    case State::Osc:
        if (b == 0x07) { // BEL termina o OSC
            sink_.osc(osc_);
            state_ = State::Ground;
        } else if (b == 0x1B) {
            state_ = State::OscEsc;
        } else {
            osc_ += static_cast<char>(b);
        }
        break;

    case State::OscEsc:
        // ESC \ = ST (String Terminator)
        sink_.osc(osc_);
        state_ = State::Ground;
        if (b != '\\') {
            byte(b); // não era ST: reprocessa o byte no Ground
        }
        break;
    }
}

} // namespace dante::term
