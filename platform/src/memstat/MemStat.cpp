#include <dante/platform/memstat/MemStat.hpp>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace dante {

#ifdef _WIN32

Result<std::uint64_t> currentProcessPrivateBytes() {
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (!GetProcessMemoryInfo(GetCurrentProcess(),
                              reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                              sizeof(pmc))) {
        return fail("MemStat: GetProcessMemoryInfo falhou");
    }
    // gotcha #8: PrivateUsage, nao WorkingSetSize.
    return static_cast<std::uint64_t>(pmc.PrivateUsage);
}

#else

Result<std::uint64_t> currentProcessPrivateBytes() {
    return fail("MemStat indisponivel fora do Windows");
}

#endif

} // namespace dante
