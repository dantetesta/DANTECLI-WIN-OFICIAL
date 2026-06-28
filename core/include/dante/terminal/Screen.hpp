#pragma once

#include "dante/terminal/Cell.hpp"
#include "dante/terminal/VtParser.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace dante::term {

// Emulador de tela: grade 2D de células + cursor + atributos atuais. Consome os eventos
// do VtParser e aplica os movimentos/SGR/erase mais comuns (suficiente p/ cmd, ls, git,
// cores). ponytail: só tela visível por enquanto (sem scrollback ring — F2 adiciona).
class Screen : public IVtSink {
public:
    Screen(int cols, int rows);

    void feed(std::string_view bytes) { parser_.feed(bytes); }

    int cols() const { return cols_; }
    int rows() const { return rows_; }
    int cursorRow() const { return row_; }
    int cursorCol() const { return col_; }
    const std::string& title() const { return title_; }
    const Cell& at(int row, int col) const;

    // IVtSink
    void print(char32_t cp) override;
    void execute(std::uint8_t control) override;
    void csi(char finalByte, const std::vector<int>& params) override;
    void osc(const std::string& data) override;
    void esc(char finalByte) override;

private:
    Cell& cell(int row, int col);
    Cell blank() const;
    void newline();
    void scrollUp(int n);
    void scrollDown(int n);
    void eraseInLine(int mode);
    void eraseInDisplay(int mode);
    void applySgr(const std::vector<int>& params);
    static int paramOr(const std::vector<int>& p, std::size_t i, int def);

    int cols_;
    int rows_;
    std::vector<Cell> grid_;
    int row_ = 0;
    int col_ = 0;
    Color curFg_{};
    Color curBg_{};
    bool curBold_ = false;
    std::string title_;

    VtParser parser_; // guarda referência a *this; inicializado por último
};

} // namespace dante::term
