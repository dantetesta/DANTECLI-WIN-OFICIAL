#pragma once

#include <dante/util/Result.hpp>

#include <cstdint>

namespace dante {

// Bytes privados (committed, nao compartilhados) do processo atual.
// gotcha #8: usar PrivateUsage (PROCESS_MEMORY_COUNTERS_EX), NAO WorkingSetSize.
Result<std::uint64_t> currentProcessPrivateBytes();

} // namespace dante
