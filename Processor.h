#pragma once
#ifndef SYS_PROCESSOR_H
#define SYS_PROCESSOR_H

#include <cstdint>
#include <cstring>
#include <vector>

#include <cpuid.h>

template <class To, class From>
inline To bit_cast(const From &from) noexcept {
    To dst;
    std::memcpy(&dst, &from, sizeof(To));
    return dst;
}

namespace sys {

struct Regs {
    std::uint32_t eax, ebx, ecx, edx;
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

    const char *    getVendorId() const noexcept;
    const char *    getBrandId() const noexcept;

    std::uint32_t   getType() const noexcept;
    std::uint32_t   getFamilyId() const noexcept;
    std::uint32_t   getModel() const noexcept;
    std::uint32_t   getStepping() const noexcept;
    std::uint32_t   getExtendedFamilyId() const noexcept;
    std::uint32_t   getExtendedModelId() const noexcept;

    bool            isIntel() const noexcept;
    bool            isAMD() const noexcept;

private:
    void                detectTopology() noexcept;

    std::uint32_t       vendorId[4] {};
    std::vector<Regs>   leaves;
    std::uint32_t       brand[12];

    std::vector<LogicalCore> logicalCores;
};

inline const char * Processor::getVendorId() const noexcept {
    return bit_cast<char *>(&vendorId);
}

inline const char * Processor::getBrandId() const noexcept {
    return bit_cast<char *>(&brand);
}

inline bool Processor::isIntel() const noexcept {
    return vendorId[0] == signature_INTEL_ebx && vendorId[2] == signature_INTEL_ecx && vendorId[1] == signature_INTEL_edx;
}

inline bool Processor::isAMD() const noexcept {
    return vendorId[0] == signature_AMD_ebx && vendorId[2] == signature_AMD_ecx && vendorId[1] == signature_AMD_edx;
}

inline std::uint32_t Processor::getType() const noexcept {
    return (leaves[1].eax & 0x00003000) >> 12;
}

inline std::uint32_t Processor::getFamilyId() const noexcept {
    return (leaves[1].eax & 0x00000F00) >> 8;
}

inline std::uint32_t Processor::getModel() const noexcept {
    return (leaves[1].eax & 0x000000F0) >> 4;
}

inline std::uint32_t Processor::getStepping() const noexcept {
    return (leaves[1].eax & 0x0000000F);
}

inline std::uint32_t Processor::getExtendedFamilyId() const noexcept {
    return (leaves[1].eax & 0x0FF00000) >> 24;
}

inline std::uint32_t Processor::getExtendedModelId() const noexcept {
    return (leaves[1].eax & 0x000F0000) >> 16;
}

}

#endif // SYS_PROCESSOR_H

