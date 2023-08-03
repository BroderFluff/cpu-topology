#include "Processor.h"

#include <cstdint>
#include <cstdio>
#include <x86intrin.h>

#if !defined(_MSC_VER)
#include <cpuid.h>
#include <pthread.h>
#include <sched.h>
#endif

#include "Thread.h"

namespace sys {

Processor::Processor() noexcept {
    /* const unsigned long long eflags = __readeflags();
    __writeeflags(eflags | (1UL << 21UL)); */

    // Get the maximum number of leaves and allocate
    std::uint32_t maxLeaves;
    __get_cpuid(0, &maxLeaves, &vendorId[0], &vendorId[2], &vendorId[1]);
    leaves.resize(maxLeaves);

    // Run CPUID for all leaves
    std::uint32_t i = 0;
    for (auto & [ eax, ebx, ecx, edx ]: leaves) {
        __get_cpuid(i, &eax, &ebx, &ecx, &edx);
        ++i;
    }

    Regs regs;
    __get_cpuid(0x80000000, &regs.eax, &regs.ebx, &regs.ecx, &regs.edx);

    if (regs.eax > 0x80000004) {
        __get_cpuid(0x80000002, &brand[0], &brand[1], &brand[2], &brand[3]);
        __get_cpuid(0x80000003, &brand[4], &brand[5], &brand[6], &brand[7]);
        __get_cpuid(0x80000004, &brand[8], &brand[9], &brand[10], &brand[11]);
    }

    detectTopology();
}

Processor::~Processor() {
}

/*struct Thread {
    pthread_t th;
    pthread_attr_t attr;
};

static void *threadRoutine(void *arg) {
    Regs regs;

    __get_cpuid_count(0xB, 0, &regs.eax, &regs.ebx, &regs.ecx, &regs.edx);

   // __get_cpuid(0x7, )
   
   const auto bitShift = regs.eax & 0x0000000F;
   const auto x2apic = regs.edx;

    *static_cast<LogicalCore *>(arg) = {
        .x2apic = x2apic,
        .core = x2apic >> bitShift,
    };

    return nullptr;
}
*/

void Processor::detectTopology() noexcept {
    Regs leaf;
    __get_cpuid_count(0xB, 1, &leaf.eax, &leaf.ebx, &leaf.ecx, &leaf.edx);

    const std::uint32_t numCores = getNumCores();
    logicalCores.resize(numCores, { .x2apic = -1U });

    std::vector<Thread> threads(numCores);

    std::uint32_t i = 0;
    for (auto& th : threads) {
        th = {
            [&, i]() {
                Regs regs;
                __get_cpuid_count(0xB, 0, &regs.eax, &regs.ebx, &regs.ecx, &regs.edx);
   
                const std::uint32_t bitShift = regs.eax & 0x0000000F;
                const std::uint32_t x2apic = regs.edx;

                *static_cast<LogicalCore *>(&logicalCores[i]) = {
                    .x2apic = x2apic,
                    .core = x2apic >> bitShift,
                };

                return nullptr;
            }
        };
        th.start(1ULL << i);
        ++i;
    }

    for (auto &th : threads) {
        th.join();
    }

#if 0

    cpu_set_t *cpusetp = CPU_ALLOC(numCores);
    std::size_t s = CPU_ALLOC_SIZE(numCores);

    std::uint32_t i = 0;
    for (auto &th: threads) {
        CPU_ZERO_S(s, cpusetp);
        CPU_SET_S(i, s, cpusetp);

        pthread_attr_init(&th.attr);
        pthread_attr_setaffinity_np(&th.attr, s, cpusetp);
        pthread_create(&th.th, &th.attr, threadRoutine, &logicalCores[i]);

        ++i;
    }

    for (auto &th: threads) {
        pthread_join(th.th, nullptr);
        pthread_attr_destroy(&th.attr);
    }

#endif

    for (const auto &core: logicalCores) {
        std::printf("core x2apic: 0x%x\ncore: %d\n", core.x2apic, core.core);
    }
}

std::uint32_t Processor::getNumCores() const noexcept {
#ifdef _MSC_VER
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    return systemInfo.dwNumberOfProcessors;
#else
    cpu_set_t setp;
    sched_getaffinity(0, sizeof(cpu_set_t), &setp);
    return static_cast<std::uint32_t>(CPU_COUNT(&setp));
#endif
}

}