// F1 — teste de comportamento do ConPTY. No Windows spawna cmd.exe DE VERDADE,
// dirige via reader assincrono (jthread) e confere a saida real + teardown limpo.
// Fora do Windows e no-op (ConPTY nao existe la).
//
// Usa CHECK (nao assert): assert vira no-op com NDEBUG, e o CI builda Release tambem.

#include <dante/platform/pty/ConPtyBackend.hpp>

#include <cstdio>
#include <string>

#define CHECK(cond, msg)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            std::fprintf(stderr, "FAIL: %s\n", (msg));    \
            return 1;                                     \
        }                                                 \
    } while (0)

#ifdef _WIN32

#include <chrono>
#include <mutex>
#include <thread>

// Le em jthread ate ver o token OU ate o processo encerrar (EOF), com orcamento de
// tempo. Sempre chama close() no fim (idempotente; destrava o reader se preciso).
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
    pty.close(); // forca teardown se o filho ainda vive; reader destrava e sai do escopo

    std::scoped_lock lk(m);
    return out;
}

int main() {
    using namespace dante;
    using namespace std::chrono_literals;

    // 1) Saida deterministica: `cmd /c echo TOKEN` precisa aparecer no stream do PTY.
    {
        ConPtyBackend pty;
        CHECK(pty.start("cmd.exe /c echo DANTE_CONPTY_OK", 80, 25), "start(echo) falhou");
        const std::string out = driveUntil(pty, "DANTE_CONPTY_OK", 20s);
        CHECK(out.find("DANTE_CONPTY_OK") != std::string::npos,
              "token de saida nao encontrado no output do ConPTY");
    }

    // 2) write()/resize()/close(): cmd interativo, redimensiona, escreve comando + exit.
    {
        ConPtyBackend pty;
        CHECK(pty.start("cmd.exe", 80, 25), "start(cmd) falhou");
        CHECK(pty.resize(120, 40), "resize falhou");
        CHECK(pty.write("echo DANTE_WROTE_OK\r\nexit\r\n"), "write falhou");
        const std::string out = driveUntil(pty, "DANTE_WROTE_OK", 20s);
        CHECK(out.find("DANTE_WROTE_OK") != std::string::npos,
              "saida do comando escrito nao encontrada");
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
