// Round-trip do Named Pipe: servidor (thread) + cliente. No Windows testa o IPC de verdade;
// fora do Windows o start() falha e o teste passa como skip (igual ao teste do ConPTY).

#include <dante/platform/ipc/NamedPipe.hpp>

#include <chrono>
#include <cstdio>
#include <thread>

using namespace dante;

int main() {
    NamedPipeServer srv;
    auto s = srv.start("test123", [](const std::string& req) { return "echo:" + req; });
    if (!s) {
        std::puts("skip: Named Pipe indisponível fora do Windows");
        return 0;
    }

    // O servidor cria o pipe numa thread — pode não estar pronto no 1º tiro. Retry curto.
    Result<std::string> r = fail("");
    for (int i = 0; i < 40; ++i) {
        r = pipeClientSend("test123", "ping");
        if (r) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    srv.stop();

    if (!r) {
        std::fprintf(stderr, "FAIL: cliente não recebeu resposta: %s\n", r.error().message.c_str());
        return 1;
    }
    if (*r != "echo:ping") {
        std::fprintf(stderr, "FAIL: resposta inesperada: '%s'\n", r->c_str());
        return 1;
    }
    std::puts("ok: Named Pipe round-trip");
    return 0;
}
