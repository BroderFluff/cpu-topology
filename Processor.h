#pragma once
#ifndef SYS_PROCESSOR_H
#define SYS_PROCESSOR_H

#include <cstdint>
#include <cstring>
#include <vector>
#include <span>

#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <bit>


#define BIT_CHECK(val, bits) \
    (((val) & (bits)) == (bits))

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <intrin.h>

#define INLINE __forceinline

static int cpui[4];

#define __get_cpuid(leaf, eax, ebx, ecx, edx) \
    __cpuid(cpui, leaf); \
    *eax = cpui[0]; *ebx = cpui[1]; *ecx = cpui[2]; *edx = cpui[3];

#define __get_cpuid_count(leaf, subleaf, eax, ebx, ecx, edx) \
    __cpuidex(cpui, leaf, subleaf); \
    *eax = cpui[0]; *ebx = cpui[1]; *ecx = cpui[2]; *edx = cpui[3];

#define bit_SSE3    0x00000001
#define bit_PCLMUL  0x00000002
#define bit_AVX     0x10000000

#define bit_SSE     0x02000000
#define bit_SSE2    0x04000000

#define bit_SSSE3   0x00000200
#define bit_SSE4_1  0x00080000
#define bit_SSE4_2  0x00100000

#define signature_AMD_ebx 0x68747541
#define signature_AMD_edx 0x69746e65
#define signature_AMD_ecx 0x444d4163

#define signature_INTEL_ebx 0x756e6547
#define signature_INTEL_edx 0x49656e69
#define signature_INTEL_ecx 0x6c65746e

#else
#include <pthread.h>
#include <sched.h>
#include <cpuid.h>

#define INLINE __attribute__((always_inline)) inline
#endif

#ifndef bit_HYBRID
static constexpr std::uint32_t bit_HYBRID = (1U << 15);
#endif

#define BIT_CHECK(val, bits) \
    (((val) & (bits)) == (bits))

template <class To, class From>
inline To bit_cast(const From &from) noexcept {
    To dst;
    std::memcpy(&dst, &from, sizeof(To));
    return dst;
}

namespace sys {

struct Regs {
    std::uint32_t   eax, ebx, ecx, edx;
};

struct LogicalCore {
    std::uint32_t   index;
    std::uint32_t   x2apic;
    std::uint32_t   chip;
    std::uint32_t   core;
    std::uint32_t   coreType;
};

class Processor {
public:
                    Processor() noexcept;
                    ~Processor();

    std::uint32_t   getNumCores() const noexcept;

    const char *    getVendorId() const noexcept;
    const char *    getBrandId() const noexcept;

    bool            isIntel() const noexcept;
    bool            isAMD() const noexcept;

    std::uint32_t   getType() const noexcept;
    std::uint32_t   getFamilyId() const noexcept;
    std::uint32_t   getModel() const noexcept;
    std::uint32_t   getStepping() const noexcept;
    std::uint32_t   getExtendedFamilyId() const noexcept;
    std::uint32_t   getExtendedModelId() const noexcept;

    // ecx:
    INLINE bool     hasSSE3() const noexcept { return BIT_CHECK(leaves[1].ecx, bit_SSE3); }
    INLINE bool     hasSSSE3() const noexcept { return BIT_CHECK(leaves[1].ecx, bit_SSSE3); }
    INLINE bool     hasSSE41() const noexcept { return BIT_CHECK(leaves[1].ecx, bit_SSE4_1); }
    INLINE bool     hasSSE42() const noexcept { return BIT_CHECK(leaves[1].ecx, bit_SSE4_2); }
    INLINE bool     hasAVX() const noexcept { return BIT_CHECK(leaves[1].ecx, bit_AVX); }

    // edx:
    INLINE bool     hasHTT() const noexcept { return BIT_CHECK(leaves[1].edx, bit_HTT); }
    INLINE bool     hasMMX() const noexcept { return BIT_CHECK(leaves[1].edx, bit_MMX); }
    INLINE bool     hasSSE() const noexcept { return BIT_CHECK(leaves[1].edx, bit_SSE); }
    INLINE bool     hasSSE2() const noexcept { return BIT_CHECK(leaves[1].edx, bit_SSE2); }
    INLINE bool     hasHYBRID() const noexcept { return BIT_CHECK(leaves[7].edx, bit_HYBRID); }

    template <class Func>
    INLINE void forEachThread(Func &&f) const noexcept {
        for (const auto &it: logicalCores) {
            f(it);
        }
    }

    std::span<const LogicalCore> getCores() const noexcept { return logicalCores; }    

private:
    void              detectTopology() noexcept;

    std::uint32_t     vendorId[4] {};
    std::vector<Regs> leaves;
    std::uint32_t     brand[12];

    std::vector<LogicalCore> logicalCores;
};

INLINE const char * Processor::getVendorId() const noexcept {
    return bit_cast<char *>(&vendorId);
}

INLINE const char * Processor::getBrandId() const noexcept {
    return bit_cast<const char *>(&brand);
}

INLINE bool Processor::isIntel() const noexcept {
    return vendorId[0] == signature_INTEL_ebx && vendorId[2] == signature_INTEL_ecx && vendorId[1] == signature_INTEL_edx;
}

INLINE bool Processor::isAMD() const noexcept {
    return vendorId[0] == signature_AMD_ebx && vendorId[2] == signature_AMD_ecx && vendorId[1] == signature_AMD_edx;
}

INLINE std::uint32_t Processor::getType() const noexcept {
    return (leaves[1].eax & 0x00003000) >> 12;
}

INLINE std::uint32_t Processor::getFamilyId() const noexcept {
    return (leaves[1].eax & 0x00000F00) >> 8;
}

INLINE std::uint32_t Processor::getModel() const noexcept {
    return (leaves[1].eax & 0x000000F0) >> 4;
}

INLINE std::uint32_t Processor::getStepping() const noexcept {
    return (leaves[1].eax & 0x0000000F);
}

INLINE std::uint32_t Processor::getExtendedFamilyId() const noexcept {
    return (leaves[1].eax & 0x0FF00000) >> 24;
}

INLINE std::uint32_t Processor::getExtendedModelId() const noexcept {
    return (leaves[1].eax & 0x000F0000) >> 16;
}

}

#endif // SYS_PROCESSOR_H

