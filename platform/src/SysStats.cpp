#include <dante/platform/SysStats.hpp>

#ifdef _WIN32

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>

namespace dante {

SysSample sysSample() {
    SysSample s;
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (::GetProcessMemoryInfo(::GetCurrentProcess(),
                               reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
        s.privateBytes = pmc.PrivateUsage; // gotcha #8: PrivateUsage, nao WorkingSet
    }
    FILETIME create{}, exit{}, kernel{}, user{};
    if (::GetProcessTimes(::GetCurrentProcess(), &create, &exit, &kernel, &user)) {
        auto toSeconds = [](const FILETIME& f) {
            ULARGE_INTEGER v;
            v.LowPart = f.dwLowDateTime;
            v.HighPart = f.dwHighDateTime;
            return static_cast<double>(v.QuadPart) / 1e7; // unidades de 100ns -> s
        };
        s.cpuSeconds = toSeconds(kernel) + toSeconds(user);
    }
    return s;
}

} // namespace dante

#elif defined(__APPLE__)

#include <mach/mach.h>

namespace dante {

SysSample sysSample() {
    SysSample s;
    task_vm_info_data_t vm{};
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&vm), &count) ==
        KERN_SUCCESS) {
        s.privateBytes = vm.phys_footprint; // equivalente do PrivateUsage no mac
    }
    double cpu = 0.0;
    mach_task_basic_info_data_t bi{};
    count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&bi),
                  &count) == KERN_SUCCESS) {
        cpu += bi.user_time.seconds + bi.user_time.microseconds / 1e6;
        cpu += bi.system_time.seconds + bi.system_time.microseconds / 1e6;
    }
    task_thread_times_info_data_t tt{};
    count = TASK_THREAD_TIMES_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_THREAD_TIMES_INFO, reinterpret_cast<task_info_t>(&tt),
                  &count) == KERN_SUCCESS) {
        cpu += tt.user_time.seconds + tt.user_time.microseconds / 1e6;
        cpu += tt.system_time.seconds + tt.system_time.microseconds / 1e6;
    }
    s.cpuSeconds = cpu;
    return s;
}

} // namespace dante

#else

namespace dante {
SysSample sysSample() { return {}; }
} // namespace dante

#endif
