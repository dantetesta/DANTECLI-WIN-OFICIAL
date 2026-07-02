// Testes do parser VT + Screen. Lógica pura — roda em qualquer plataforma (mac + CI),
// independente do ConPTY. CHECK (não assert) p/ valer no Release.

#include "dante/terminal/Screen.hpp"

#include <cstdio>
#include <string>

using namespace dante::term;

static int g_fail = 0;
#define CHECK(cond, msg)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            std::fprintf(stderr, "FAIL: %s\n", (msg));    \
            ++g_fail;                                     \
        }                                                 \
    } while (0)

int main() {
    // Texto simples + avanço do cursor.
    {
        Screen s(20, 5);
        s.feed("Hi");
        CHECK(s.at(0, 0).ch == U'H', "texto: H");
        CHECK(s.at(0, 1).ch == U'i', "texto: i");
        CHECK(s.cursorCol() == 2 && s.cursorRow() == 0, "cursor apos texto");
    }

    // CR/LF.
    {
        Screen s(20, 5);
        s.feed("ab\r\ncd");
        CHECK(s.at(0, 0).ch == U'a', "linha0 a");
        CHECK(s.at(1, 0).ch == U'c', "linha1 c");
        CHECK(s.cursorRow() == 1 && s.cursorCol() == 2, "cursor apos \\r\\n");
    }

    // SGR cor de frente (vermelho = índice 1) + reset.
    {
        Screen s(20, 5);
        s.feed("\x1b[31mR\x1b[0mX");
        CHECK(s.at(0, 0).ch == U'R', "sgr: R");
        CHECK(s.at(0, 0).fg == Color::indexed(1), "sgr: R vermelho");
        CHECK(s.at(0, 1).fg == Color::defaultColor(), "sgr: X resetado");
    }

    // Negrito.
    {
        Screen s(20, 5);
        s.feed("\x1b[1mB\x1b[22mN");
        CHECK(s.at(0, 0).bold, "bold ligado");
        CHECK(!s.at(0, 1).bold, "bold desligado");
    }

    // 256 cores e truecolor.
    {
        Screen s(20, 5);
        s.feed("\x1b[38;5;196mZ");
        CHECK(s.at(0, 0).fg == Color::indexed(196), "256-color");
        Screen s2(20, 5);
        s2.feed("\x1b[38;2;10;20;30mT");
        CHECK(s2.at(0, 0).fg == Color::rgb(10, 20, 30), "truecolor");
    }

    // CUP (posicionar cursor) 1-based.
    {
        Screen s(20, 5);
        s.feed("\x1b[2;3HX");
        CHECK(s.at(1, 2).ch == U'X', "CUP linha2 col3");
    }

    // Erase in line (do cursor ao fim).
    {
        Screen s(20, 5);
        s.feed("ABC\x1b[1G\x1b[K");
        CHECK(s.at(0, 0).ch == U' ' && s.at(0, 2).ch == U' ', "EL limpou a linha");
    }

    // UTF-8.
    {
        Screen s(20, 5);
        s.feed("\xC3\xA1");                            // 'á' em UTF-8
        CHECK(s.at(0, 0).ch == 0xE1, "utf-8 a"); // U+00E1 (á), codepoint numérico
    }

    // Scroll: 2 linhas, 3 linhas de texto -> a primeira sobe.
    {
        Screen s(10, 2);
        s.feed("L0\r\nL1\r\nL2");
        CHECK(s.at(0, 0).ch == U'L' && s.at(0, 1).ch == U'1', "scroll: linha0 = L1");
        CHECK(s.at(1, 0).ch == U'L' && s.at(1, 1).ch == U'2', "scroll: linha1 = L2");
    }

    // Autowrap.
    {
        Screen s(3, 3);
        s.feed("abcd");
        CHECK(s.at(0, 0).ch == U'a' && s.at(0, 2).ch == U'c', "autowrap linha0");
        CHECK(s.at(1, 0).ch == U'd', "autowrap -> linha1");
    }

    // Resize preserva o canto superior-esquerdo e faz clamp do cursor.
    {
        Screen s(10, 4);
        s.feed("abc\r\ndef");
        s.resize(6, 2);
        CHECK(s.at(0, 0).ch == U'a' && s.at(0, 2).ch == U'c', "resize preservou linha0");
        CHECK(s.at(1, 0).ch == U'd', "resize preservou linha1");
        CHECK(s.cols() == 6 && s.rows() == 2, "resize dims");
        CHECK(s.cursorRow() < 2 && s.cursorCol() < 6, "cursor clampado no resize");
    }

    // CSI privado (ESC[?…) NÃO deve agir como CSI público: ESC[?2J não apaga a tela.
    {
        Screen s(20, 5);
        s.feed("ABC\x1b[?2J");
        CHECK(s.at(0, 0).ch == U'A' && s.at(0, 2).ch == U'C', "CSI privado ?2J ignorado");
    }

    // OSC sem terminador não cresce sem limite (cap 4096). Não deve estourar memória.
    {
        Screen s(20, 5);
        std::string big = "\x1b]0;";
        big.append(10000, 'x'); // título gigante sem BEL/ST
        big += "\x07";
        s.feed(big);
        CHECK(s.title().size() <= 4096, "OSC capado em 4096");
    }

    if (g_fail > 0) {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", g_fail);
        return 1;
    }
    std::puts("ok: VtParser + Screen");
    return 0;
}
