#include "Processor.h"

#include <cstdint>
#include <cpuid.h>

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

    




}

Processor::~Processor() {
}

}