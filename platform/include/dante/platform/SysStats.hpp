#pragma once

#include <cstdint>

namespace dante {

// Amostra de uso de recursos do PRÓPRIO processo. Cross-platform.
struct SysSample {
    std::uint64_t privateBytes = 0; // memória (PrivateUsage no Win / phys_footprint no mac)
    double cpuSeconds = 0.0;        // CPU acumulado (user+system) em segundos
};

SysSample sysSample();

} // namespace dante
