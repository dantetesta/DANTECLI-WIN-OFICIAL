#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace dante::orch {

// Roteamento FAIL-CLOSED (gotcha #4). Num canvas multi-projeto, um agente NÃO pode delegar
// pro homônimo de outro fluxo. `resolve` só procura o alvo DENTRO do fluxo do chamador; se não
// achar ali, NEGA (string vazia) — jamais cai num match global. Lógica pura (zero Qt/OS).
class Router {
public:
    // Registra que `sessionId` (um terminal/agente) pertence a `flowId` com o papel `agentName`.
    void addMember(const std::string& flowId, const std::string& sessionId,
                   const std::string& agentName);
    // Remove uma sessão (terminal fechado) — some do fluxo.
    void removeSession(const std::string& sessionId);

    // Resolve o sessionId do agente chamado `name` no MESMO fluxo do chamador.
    // Vazio = negado (não existe no fluxo — nunca resolve fora dele).
    std::string resolve(const std::string& callerSession, const std::string& name) const;

    // Fluxo de uma sessão ("" se desconhecida).
    std::string flowOf(const std::string& sessionId) const;

private:
    struct Member {
        std::string flowId;
        std::string sessionId;
        std::string agentName;
    };
    std::unordered_map<std::string, std::string> sessionToFlow_; // sessionId -> flowId
    std::vector<Member> members_;
};

} // namespace dante::orch
