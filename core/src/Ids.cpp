#include "dante/domain/Ids.hpp"

#include <atomic>
#include <cstdint>

namespace dante {

// One counter per id type, shared process-wide. relaxed is fine: we only need
// each value handed out once, not ordering against other memory.
SessionId newSessionId() {
    static std::atomic<std::uint64_t> counter{0};
    return SessionId{"sess-" + std::to_string(counter.fetch_add(1, std::memory_order_relaxed))};
}

TabId newTabId() {
    static std::atomic<std::uint64_t> counter{0};
    return TabId{"tab-" + std::to_string(counter.fetch_add(1, std::memory_order_relaxed))};
}

}  // namespace dante
