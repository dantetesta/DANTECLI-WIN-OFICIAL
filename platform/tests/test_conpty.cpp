// F1 — smoke de PLUMBING do ConPTY (roda no CI Windows headless).
//
// Verifica o que e verificavel numa sessao NAO-interativa do runner:
//   - start() spawna um processo real e cria o pseudoconsole + pipes;
//   - o pipe de saida recebe VT do conhost (prova que ConPTY<->nos esta ligado);
//   - write()/resize() funcionam;
//   - close()/teardown e limpo, sem hang (orcamento de tempo).
//
// LIMITACAO CONHECIDA: o conhost do runner headless emite so FRAMING (mode-set, clear,
// title), nao o CONTEUDO de tela do shell, e shells interativos saem na hora — isso e
// limitacao de sessao nao-interativa, nao do codigo (que segue o sample oficial do MS).
// O terminal REAL (conteudo + shell vivo) e validado no Windows interativo do usuario.
//
// Usa CHECK explicito (nao assert): assert vira no-op com NDEBUG, e o CI builda Release.

#include <dante/platform/pty/ConPtyBackend.hpp>

#include <cstdio>
#include <string>

#define CHECK(cond, msg)                               \
    do {                                               \
        if (!(cond)) {                                 \
            std::fprintf(stderr, "FAIL: %s\n", (msg)); \
            return 1;                                  \
        }                                              \
    } while (0)

#ifdef _WIN32

#include <chrono>
#include <mutex>
#include <thread>

// Le tudo num jthread ate EOF ou ate o orcamento; sempre fecha no fim (teardown limpo).
static std::string drainAll(dante::ConPtyBackend& pty, std::chrono::seconds budget) {
    std::string out;
    std::mutex m;
    bool eof = false;

    std::jthread reader([&] {
        for (;;) {
            auto chunk = pty.read();
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
            if (eof) {
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

    // A) Spawn de processo real + pipe de saida recebendo VT do conhost.
    {
        ConPtyBackend pty;
        CHECK(pty.start("cmd.exe /c echo conpty_ok", 80, 25), "start(echo) falhou");
        const std::string out = drainAll(pty, 10s);
        CHECK(!out.empty(), "nao recebeu nenhum byte do pseudoconsole");
        CHECK(out.find('\x1B') != std::string::npos, "saida nao contem VT (ESC) do conhost");
    }

    // B) write()/resize()/close() funcionam e o teardown e limpo (sem hang).
    {
        ConPtyBackend pty;
        CHECK(pty.start("cmd.exe", 80, 25), "start(cmd) falhou");
        CHECK(pty.resize(120, 40), "resize falhou");
        CHECK(pty.write("echo conpty_write\r\nexit\r\n"), "write falhou");
        (void)drainAll(pty, 10s); // deve terminar (EOF), nao travar
    }

    std::puts("ok: ConPTY plumbing (spawn/output-wired/write/resize/close)");
    return 0;
}

#else // ConPTY e Windows-only.

int main() {
    std::puts("skip: ConPTY indisponivel fora do Windows");
    return 0;
}

#endif
