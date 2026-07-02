// Roteamento fail-closed (gotcha #4): um agente só resolve dentro do próprio fluxo, nunca
// vaza pro homônimo de outro. Lógica pura — roda no mac e no CI.

#include "dante/orchestration/Router.hpp"

#include <cstdio>

using namespace dante::orch;

static int g_fail = 0;
#define CHECK(cond, msg)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            std::fprintf(stderr, "FAIL: %s\n", (msg));    \
            ++g_fail;                                     \
        }                                                 \
    } while (0)

int main() {
    Router r;
    // Dois fluxos, cada um com um agente "qa" e um "lead".
    r.addMember("flowA", "s1", "lead");
    r.addMember("flowA", "s2", "qa");
    r.addMember("flowB", "s3", "lead");
    r.addMember("flowB", "s4", "qa");

    // Do fluxo A: "qa" resolve p/ s2 (o do A), NUNCA s4 (homônimo do B).
    CHECK(r.resolve("s1", "qa") == "s2", "resolve qa dentro do fluxo A");
    // Do fluxo B: "qa" resolve p/ s4.
    CHECK(r.resolve("s3", "qa") == "s4", "resolve qa dentro do fluxo B");
    // Agente inexistente no fluxo → negado (vazio).
    CHECK(r.resolve("s1", "backend").empty(), "nega agente ausente no fluxo");
    // Chamador fora de qualquer fluxo → negado.
    CHECK(r.resolve("desconhecido", "qa").empty(), "nega chamador sem fluxo");

    // Fechar s2 (terminal do qa) tira ele do fluxo → "qa" no A passa a negar.
    r.removeSession("s2");
    CHECK(r.resolve("s1", "qa").empty(), "após remover, qa do A nega (sem cair no B)");
    CHECK(r.resolve("s3", "qa") == "s4", "qa do B intacto");

    // Reregistrar move de fluxo sem duplicar.
    r.addMember("flowB", "s1", "helper");
    CHECK(r.flowOf("s1") == "flowB", "reregistro move o fluxo");
    CHECK(r.resolve("s1", "qa") == "s4", "s1 agora no B resolve o qa do B");

    if (g_fail > 0) {
        std::fprintf(stderr, "%d checagem(ns) falharam\n", g_fail);
        return 1;
    }
    std::puts("ok: Router fail-closed");
    return 0;
}
