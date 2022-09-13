#include "Processor.h"

#include <cstdint>
#include <cstdio>

#include <cpuid.h>
#include <pthread.h>

namespace sys {

Processor::Processor() noexcept {
    std::uint32_t maxLeaves;
    __get_cpuid(0, &maxLeaves, &vendorId[0], &vendorId[2], &vendorId[1]);

    leaves.resize(maxLeaves);

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

struct Thread {
    pthread_t th;
    pthread_attr_t attr;
};

static void *threadRoutine(void *arg) {
    Regs regs;

    __get_cpuid_count(0xB, 0, &regs.eax, &regs.ebx, &regs.ecx, &regs.edx);

    static_cast<LogicalCore *>(arg)->x2apic = regs.edx;

    return nullptr;
}

void Processor::detectTopology() noexcept {
    Regs leaf;
    __get_cpuid_count(0xB, 1, &leaf.eax, &leaf.ebx, &leaf.ecx, &leaf.edx);

    std::printf("num: %d\n", leaf.ebx & 0xFF);

    const std::uint32_t numCores = leaf.ebx & 0xFF;

    logicalCores.resize(numCores, { -1U });

    std::vector<Thread> threads(numCores);

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

    for (const auto &core: logicalCores) {
        std::printf("core x2apic: 0x%x\n", core.x2apic);
    }





}

}