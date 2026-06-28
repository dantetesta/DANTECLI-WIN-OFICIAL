// F1 — teste de comportamento do ConPTY. No Windows spawna cmd.exe DE VERDADE,
// dirige via reader assincrono (jthread) e confere a saida real + teardown limpo.
// Fora do Windows e no-op (ConPTY nao existe la).
//
// Usa CHECK explicito (nao assert): assert vira no-op com NDEBUG, e o CI builda Release.

#include <dante/platform/pty/ConPtyBackend.hpp>

#include <cstdio>
#include <string>

#ifdef _WIN32

#include <chrono>
#include <cstddef>
#include <mutex>
#include <thread>

// Despeja a saida capturada em forma escapada (controle -> \xHH) p/ diagnostico no CI.
static void dumpEscaped(const char* label, const std::string& s) {
    std::fprintf(stderr, "[dump] %s (%zu bytes): ", label, s.size());
    const std::size_t shown = s.size() < 1000 ? s.size() : 1000;
    for (std::size_t i = 0; i < shown; ++i) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 0x20 && c < 0x7f) {
            std::fputc(c, stderr);
        } else {
            std::fprintf(stderr, "\\x%02X", c);
        }
    }
    std::fputc('\n', stderr);
}

// Le em jthread ate ver o token OU ate o processo encerrar (EOF), com orcamento de tempo.
static std::string driveUntil(dante::ConPtyBackend& pty, const std::string& token,
                              std::chrono::seconds budget) {
    std::string out;
    std::mutex m;
    bool eof = false;

    std::jthread reader([&] {
        for (;;) {
            auto chunk = pty.read(); // bloqueante; erro = EOF
            if (!chunk) {
                break;
            }
            std::scoped_lock lk(m);
            out += *chunk;
        }
        std::scoped_lock lk(m);
        eof = true;
    });

    const auto deadline = std::chrono::steady_clock::now() + budget;
    for (;;) {
        {
            std::scoped_lock lk(m);
            if (eof || out.find(token) != std::string::npos) {
                break;
            }
        }
        if (std::chrono::steady_clock::now() > deadline) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    pty.close();

    std::scoped_lock lk(m);
    return out;
}

int main() {
    using namespace dante;
    using namespace std::chrono_literals;
    int failures = 0;

    // 1) `cmd /c echo TOKEN` deve aparecer no stream do PTY.
    {
        ConPtyBackend pty;
        if (!pty.start("cmd.exe /c echo DANTE_CONPTY_OK", 80, 25)) {
            std::fprintf(stderr, "FAIL s1: start falhou\n");
            ++failures;
        } else {
            const std::string out = driveUntil(pty, "DANTE_CONPTY_OK", 20s);
            dumpEscaped("s1 echo", out);
            if (out.find("DANTE_CONPTY_OK") == std::string::npos) {
                std::fprintf(stderr, "FAIL s1: token nao encontrado\n");
                ++failures;
            }
        }
    }

    // 2) write()/resize()/close(): cmd interativo, redimensiona, escreve comando + exit.
    {
        ConPtyBackend pty;
        if (!pty.start("cmd.exe", 80, 25)) {
            std::fprintf(stderr, "FAIL s2: start falhou\n");
            ++failures;
        } else {
            if (!pty.resize(120, 40)) {
                std::fprintf(stderr, "FAIL s2: resize falhou\n");
                ++failures;
            }
            if (!pty.write("echo DANTE_WROTE_OK\r\nexit\r\n")) {
                std::fprintf(stderr, "FAIL s2: write falhou\n");
                ++failures;
            }
            const std::string out = driveUntil(pty, "DANTE_WROTE_OK", 20s);
            dumpEscaped("s2 write", out);
            if (out.find("DANTE_WROTE_OK") == std::string::npos) {
                std::fprintf(stderr, "FAIL s2: token nao encontrado\n");
                ++failures;
            }
        }
    }

    if (failures > 0) {
        std::fprintf(stderr, "%d cenario(s) falharam\n", failures);
        return 1;
    }
    std::puts("ok: ConPTY spawn/read/write/resize/close");
    return 0;
}

#else // ConPTY e Windows-only.

int main() {
    std::puts("skip: ConPTY indisponivel fora do Windows");
    return 0;
}

#endif
