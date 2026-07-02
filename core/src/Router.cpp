#include "dante/orchestration/Router.hpp"

#include <algorithm>

namespace dante::orch {

void Router::addMember(const std::string& flowId, const std::string& sessionId,
                       const std::string& agentName) {
    removeSession(sessionId); // reregistrar move a sessão de fluxo sem duplicar
    sessionToFlow_[sessionId] = flowId;
    members_.push_back({flowId, sessionId, agentName});
}

void Router::removeSession(const std::string& sessionId) {
    sessionToFlow_.erase(sessionId);
    members_.erase(std::remove_if(members_.begin(), members_.end(),
                                  [&](const Member& m) { return m.sessionId == sessionId; }),
                   members_.end());
}

std::string Router::flowOf(const std::string& sessionId) const {
    auto it = sessionToFlow_.find(sessionId);
    return it == sessionToFlow_.end() ? std::string{} : it->second;
}

std::string Router::resolve(const std::string& callerSession, const std::string& name) const {
    const std::string flow = flowOf(callerSession);
    if (flow.empty()) {
        return {}; // chamador fora de qualquer fluxo → nega
    }
    for (const Member& m : members_) {
        if (m.flowId == flow && m.agentName == name) {
            return m.sessionId; // achou no MESMO fluxo
        }
    }
    return {}; // não existe no fluxo do chamador → NEGA (sem fallback global)
}

} // namespace dante::orch
