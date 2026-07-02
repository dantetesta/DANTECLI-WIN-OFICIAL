#include "dante/terminal/Screen.hpp"

#include <algorithm>

namespace dante::term {

namespace {
int clampi(int v, int lo, int hi) {
    return std::max(lo, std::min(v, hi));
}
} // namespace

Screen::Screen(int cols, int rows)
    : cols_(std::max(1, cols)), rows_(std::max(1, rows)),
      grid_(static_cast<std::size_t>(cols_) * static_cast<std::size_t>(rows_)), parser_(*this) {}

void Screen::resize(int cols, int rows) {
    cols = std::max(1, cols);
    rows = std::max(1, rows);
    if (cols == cols_ && rows == rows_) {
        return;
    }
    std::vector<Cell> next(static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows));
    const int cc = std::min(cols, cols_);
    const int rr = std::min(rows, rows_);
    for (int r = 0; r < rr; ++r) {
        for (int c = 0; c < cc; ++c) {
            next[static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) +
                 static_cast<std::size_t>(c)] = cell(r, c);
        }
    }
    grid_ = std::move(next);
    cols_ = cols;
    rows_ = rows;
    row_ = clampi(row_, 0, rows_ - 1);
    col_ = clampi(col_, 0, cols_ - 1);
}

Cell Screen::blank() const {
    Cell c;
    c.bg = curBg_; // erase usa o fundo atual
    return c;
}

Cell& Screen::cell(int row, int col) {
    return grid_[static_cast<std::size_t>(row) * static_cast<std::size_t>(cols_) +
                 static_cast<std::size_t>(col)];
}

const Cell& Screen::at(int row, int col) const {
    static const Cell kBlank{};
    if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
        return kBlank;
    }
    return grid_[static_cast<std::size_t>(row) * static_cast<std::size_t>(cols_) +
                 static_cast<std::size_t>(col)];
}

int Screen::paramOr(const std::vector<int>& p, std::size_t i, int def) {
    if (i < p.size() && p[i] != 0) {
        return p[i];
    }
    return def;
}

void Screen::print(char32_t cp) {
    Cell& dst = cell(row_, col_);
    dst.ch = cp;
    dst.fg = curFg_;
    dst.bg = curBg_;
    dst.bold = curBold_;
    ++col_;
    if (col_ >= cols_) { // autowrap
        col_ = 0;
        newline();
    }
}

void Screen::execute(std::uint8_t control) {
    switch (control) {
    case 0x08: // BS
        if (col_ > 0) {
            --col_;
        }
        break;
    case 0x09: // HT — próxima parada de tab (a cada 8)
        col_ = std::min((col_ / 8 + 1) * 8, cols_ - 1);
        break;
    case 0x0A: // LF
    case 0x0B: // VT
    case 0x0C: // FF
        newline();
        break;
    case 0x0D: // CR
        col_ = 0;
        break;
    default:
        break; // BEL e outros: ignora
    }
}

void Screen::newline() {
    ++row_;
    if (row_ >= rows_) {
        scrollUp(1);
        row_ = rows_ - 1;
    }
}

void Screen::scrollUp(int n) {
    n = clampi(n, 0, rows_);
    if (n == 0) {
        return;
    }
    for (int r = 0; r < rows_ - n; ++r) {
        for (int c = 0; c < cols_; ++c) {
            cell(r, c) = cell(r + n, c);
        }
    }
    for (int r = rows_ - n; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            cell(r, c) = blank();
        }
    }
}

void Screen::scrollDown(int n) {
    n = clampi(n, 0, rows_);
    if (n == 0) {
        return;
    }
    for (int r = rows_ - 1; r >= n; --r) {
        for (int c = 0; c < cols_; ++c) {
            cell(r, c) = cell(r - n, c);
        }
    }
    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < cols_; ++c) {
            cell(r, c) = blank();
        }
    }
}

void Screen::eraseInLine(int mode) {
    int from = 0;
    int to = cols_ - 1;
    if (mode == 0) {
        from = col_;
    } else if (mode == 1) {
        to = col_;
    }
    for (int c = from; c <= to; ++c) {
        cell(row_, c) = blank();
    }
}

void Screen::eraseInDisplay(int mode) {
    if (mode == 2) {
        for (auto& cc : grid_) {
            cc = blank();
        }
        return;
    }
    if (mode == 0) { // cursor até o fim
        eraseInLine(0);
        for (int r = row_ + 1; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                cell(r, c) = blank();
            }
        }
    } else if (mode == 1) { // início até o cursor
        for (int r = 0; r < row_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                cell(r, c) = blank();
            }
        }
        eraseInLine(1);
    }
}

void Screen::applySgr(const std::vector<int>& params) {
    if (params.empty()) {
        curFg_ = Color::defaultColor();
        curBg_ = Color::defaultColor();
        curBold_ = false;
        return;
    }
    for (std::size_t i = 0; i < params.size(); ++i) {
        const int p = params[i];
        if (p == 0) {
            curFg_ = Color::defaultColor();
            curBg_ = Color::defaultColor();
            curBold_ = false;
        } else if (p == 1) {
            curBold_ = true;
        } else if (p == 22) {
            curBold_ = false;
        } else if (p >= 30 && p <= 37) {
            curFg_ = Color::indexed(static_cast<std::uint8_t>(p - 30));
        } else if (p == 39) {
            curFg_ = Color::defaultColor();
        } else if (p >= 40 && p <= 47) {
            curBg_ = Color::indexed(static_cast<std::uint8_t>(p - 40));
        } else if (p == 49) {
            curBg_ = Color::defaultColor();
        } else if (p >= 90 && p <= 97) {
            curFg_ = Color::indexed(static_cast<std::uint8_t>(p - 90 + 8));
        } else if (p >= 100 && p <= 107) {
            curBg_ = Color::indexed(static_cast<std::uint8_t>(p - 100 + 8));
        } else if (p == 38 || p == 48) {
            Color& target = (p == 38) ? curFg_ : curBg_;
            if (i + 1 < params.size() && params[i + 1] == 5 && i + 2 < params.size()) {
                target = Color::indexed(static_cast<std::uint8_t>(params[i + 2] & 0xFF));
                i += 2;
            } else if (i + 1 < params.size() && params[i + 1] == 2 && i + 4 < params.size()) {
                target = Color::rgb(static_cast<std::uint8_t>(params[i + 2] & 0xFF),
                                    static_cast<std::uint8_t>(params[i + 3] & 0xFF),
                                    static_cast<std::uint8_t>(params[i + 4] & 0xFF));
                i += 4;
            }
        }
        // outros SGR (itálico, sublinhado...) ignorados neste marco
    }
}

void Screen::csi(char priv, char finalByte, const std::vector<int>& params) {
    // Sequências privadas (ESC[?…, ESC[<…, ESC[>…) são modos DEC (cursor visível, tela
    // alternativa, mouse…) — não implementados neste marco. Ignorar em vez de tratar como
    // CSI público (senão ESC[?2J apagaria a tela como se fosse ESC[2J).
    if (priv != 0) {
        return;
    }
    switch (finalByte) {
    case 'A':
        row_ = clampi(row_ - paramOr(params, 0, 1), 0, rows_ - 1);
        break;
    case 'B':
        row_ = clampi(row_ + paramOr(params, 0, 1), 0, rows_ - 1);
        break;
    case 'C':
        col_ = clampi(col_ + paramOr(params, 0, 1), 0, cols_ - 1);
        break;
    case 'D':
        col_ = clampi(col_ - paramOr(params, 0, 1), 0, cols_ - 1);
        break;
    case 'G': // CHA — coluna absoluta (1-based)
        col_ = clampi(paramOr(params, 0, 1) - 1, 0, cols_ - 1);
        break;
    case 'd': // VPA — linha absoluta
        row_ = clampi(paramOr(params, 0, 1) - 1, 0, rows_ - 1);
        break;
    case 'H': // CUP
    case 'f': // HVP
        row_ = clampi(paramOr(params, 0, 1) - 1, 0, rows_ - 1);
        col_ = clampi(paramOr(params, 1, 1) - 1, 0, cols_ - 1);
        break;
    case 'J':
        eraseInDisplay(paramOr(params, 0, 0));
        break;
    case 'K':
        eraseInLine(paramOr(params, 0, 0));
        break;
    case 'm':
        applySgr(params);
        break;
    case 'S':
        scrollUp(paramOr(params, 0, 1));
        break;
    case 'T':
        scrollDown(paramOr(params, 0, 1));
        break;
    default:
        break; // demais CSI ignorados neste marco
    }
}

void Screen::osc(const std::string& data) {
    // OSC 0;<t> e 2;<t> definem o título da janela.
    const auto semi = data.find(';');
    if (semi == std::string::npos) {
        return;
    }
    const std::string code = data.substr(0, semi);
    if (code == "0" || code == "2") {
        title_ = data.substr(semi + 1);
    }
}

void Screen::esc(char /*finalByte*/) {
    // sequências ESC simples (RIS, índices...) ignoradas neste marco
}

} // namespace dante::term
