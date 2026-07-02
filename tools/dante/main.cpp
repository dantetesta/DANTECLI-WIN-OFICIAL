// dante.exe — CLI que roda DENTRO dos terminais e fala com o app via Named Pipe.
// O app seta a env DANTE_PIPE ao spawnar o shell (1 pipe por sessão/fluxo). O app roteia
// fail-closed (gotcha #4) e responde. Qt-free (linka dante::ipc + core).
//
// Uso: dante <verbo> [args...]   verbos: ask|note|check|connect|folder|portal

#include <dante/platform/ipc/NamedPipe.hpp>

#include <cstdio>
#include <cstdlib>
#include <string>

namespace {
// Escaping mínimo p/ o JSON (aspas e barra). ponytail: suficiente p/ args de linha de comando;
// trocar por lib JSON se o protocolo crescer.
std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (const char c : s) {
        if (c == '"' || c == '\\') {
            out += '\\';
        }
        out += c;
    }
    return out;
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr,
                     "uso: dante <verbo> [args...]\n  verbos: ask|note|check|connect|folder|portal\n");
        return 2;
    }

    const char* env = std::getenv("DANTE_PIPE");
    const std::string pipe = env != nullptr ? env : "default";

    std::string json = "{\"verb\":\"" + jsonEscape(argv[1]) + "\",\"args\":[";
    for (int i = 2; i < argc; ++i) {
        if (i > 2) {
            json += ",";
        }
        json += "\"" + jsonEscape(argv[i]) + "\"";
    }
    json += "]}";

    auto r = dante::pipeClientSend(pipe, json);
    if (!r) {
        std::fprintf(stderr, "dante: %s\n", r.error().message.c_str());
        return 1;
    }
    std::printf("%s\n", r->c_str());
    return 0;
}
