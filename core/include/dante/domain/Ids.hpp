#pragma once

#include <cstddef>
#include <functional>
#include <string>

namespace dante {

// Strong id wrappers so a SessionId can never be passed where a TabId is wanted.
struct SessionId {
    std::string value;
    bool operator==(const SessionId&) const = default;
};

struct TabId {
    std::string value;
    bool operator==(const TabId&) const = default;
};

// Monotonic ids from a process-local atomic counter (defined in Ids.cpp).
// ponytail: counter is uint64 -> ~1.8e19 ids before wrap, unreachable in a UI
// session; and ids are NOT stable across process restarts. Swap to UUIDv4 the
// day ids must persist (DB rows, IPC handshakes) — until then this is enough.
SessionId newSessionId();
TabId newTabId();

}  // namespace dante

template <>
struct std::hash<dante::SessionId> {
    std::size_t operator()(const dante::SessionId& id) const noexcept {
        return std::hash<std::string>{}(id.value);
    }
};

template <>
struct std::hash<dante::TabId> {
    std::size_t operator()(const dante::TabId& id) const noexcept {
        return std::hash<std::string>{}(id.value);
    }
};
