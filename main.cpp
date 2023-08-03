#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <bit>


#if 0

#define BIT_CHECK(val, bits) \
    (((val) & (bits)) == (bits))

#ifdef _MSC_VER
#include <intrin.h>

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
#endif

#ifndef bit_HYBRID
static constexpr std::uint32_t bit_HYBRID = (1U << 15);
#endif

#ifndef bit_PREFTCHWT1
#   ifdef __GNUG__
#       define bit_PREFTCHWT1 bit_PREFETCHWT1
#   else
#       define bit_PREFTCHWT1 (1)
#   endif
#endif

#ifndef bit_HTT
#   define bit_HTT 0x80000000
#endif

#include "Processor.h"




namespace sys {

    template <class To, class From>
inline To bit_cast(const From &from) noexcept {
    To dst;
    std::memcpy(&dst, &from, sizeof(To));
    return dst;
}

struct CPUInfo {
    std::uint32_t nleafs;
    std::uint32_t vendorId[4] {};
    std::uint32_t brand[12];
    bool isIntel {};
    bool isAMD {};
    struct Regs {
        std::uint32_t eax, ebx, ecx, edx;
        /* std::uint32_t &operator[](int i) { return (&eax)[i]; } */
    };
    void dumpRegs(const Regs &regs) {
        printf("eax: 0x%x, ebx: 0x%x, ecx: 0x%x, edx: 0x%x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
    }
    std::vector<Regs> leafs;

    CPUInfo() noexcept {
        Regs vendor;
        __get_cpuid(0, &nleafs, &vendor.ebx,  &vendor.ecx, &vendor.edx);

        if (
            vendor.ebx == signature_INTEL_ebx &&
            vendor.ecx == signature_INTEL_ecx &&
            vendor.edx == signature_INTEL_edx
        ) {
            isIntel = true;
        } else if (
            vendor.ebx == signature_AMD_ebx &&
            vendor.ecx == signature_AMD_ecx &&
            vendor.edx == signature_AMD_edx
        ) {
            isAMD = true;
        }

        vendorId[0] = vendor.ebx;
        vendorId[2] = vendor.ecx;
        vendorId[1] = vendor.edx;

        leafs.resize(nleafs);
        std::uint32_t i = 0;
#if __cpp_structured_bindings == 201606L
        for (auto & [ eax, ebx, ecx, edx ]: leafs) {
            __get_cpuid(i, &eax, &ebx, &ecx, &edx);
            ++i;
        }
#else
        for (auto &leaf: leafs) {
            __get_cpuid_count(i, 0, &leaf.eax, &leaf.ebx, &leaf.ecx, &leaf.edx);
            ++i;
        }
#endif

        const auto &leaf = leafs[1];

        printf("extended family id: 0x%x\n", (leaf.eax & 0x0FF00000) >> 24);
        printf("extended model id: 0x%x\n", (leaf.eax &  0x000F0000) >> 16);
        printf("Type:      0x%x\n", (leaf.eax & 0x3000) >> 12);
        printf("Family ID: 0x%x\n", (leaf.eax & 0x00000F00) >> 8);
        printf("Model:     0x%x\n", (leaf.eax & 0xF0) >> 4);
        printf("stepping:  0x%x\n", (leaf.eax & 0xF));


	
	Regs regs;	
	__get_cpuid(0x80000000, &regs.eax, &regs.ebx, &regs.ecx, &regs.edx);
	const std::uint32_t nleafExt = regs.eax;

    std::printf("Highest function: 0x%x\n", nleafs);

	std::printf("highes extended function: 0x%x\n", nleafExt);




        detectTopology();
    }

    const char *getVendorId() const noexcept { return bit_cast<char *>(&vendorId); }
    const char *getBrandId() const noexcept { return bit_cast<char *>(&brand); }

    struct LogicalCore {
        std::uint32_t nodeIndex;
        std::uint32_t coreIndex;
        std::uint32_t siblings;
    };

    enum class CpuNodeType : std::uint32_t {
        Invalid,
        SMT,
        Core,
    };

    struct CpuNode {
        CpuNode() noexcept = default;

        std::uint32_t       index { -1U };
        std::uint32_t       id { -1U };
        std::uint32_t       coreId;
        std::uint32_t       threadId;
    };

    struct Cpu {
        std::uint32_t numNodes;
        CpuNode *nodes = nullptr;
	    std::uint32_t index = 0;
    };

    Cpu cpu;

    static void* threadRoutine(void *p) {
            Regs leaf;

            Regs subleaf0;

            // First query level (SMT):
            __get_cpuid_count(0xB, 0, &leaf.eax, &leaf.ebx, &leaf.ecx, &leaf.edx);

            const std::uint32_t bitShift = leaf.eax & 0x0F;
            const std::uint32_t siblings = leaf.ebx & 0xFF;
            const std::uint32_t levelType = (leaf.ecx & 0xFF00) >> 8;
            const std::uint32_t x2apicId = leaf.edx;

            const std::uint32_t coreId = x2apicId >> bitShift;

            Cpu *cpu = static_cast<Cpu *>(p);
            std::printf("\nindex: %d\nbitShift: %d\n", cpu->index, bitShift);
            std::printf("siblings: %d\n", siblings);
            std::printf("levelType: %d\n", levelType);
            std::printf("x2apicId: 0x%x\n", x2apicId);
            std::printf("core Id: 0x%x\n", coreId);

            // Second query level (Core)
            __get_cpuid_count(0xB, 1, &leaf.eax, &leaf.ebx, &leaf.ecx, &leaf.edx);

            const std::uint32_t bitShift2 = (leaf.eax & 0x0F);
            const std::uint32_t numLogicalCores = leaf.ebx & 0xFF;
            const std::uint32_t levelType2 = (leaf.ecx & 0xFF00) >> 8;

            std::printf("bitShift2: %d\n", bitShift2);
            std::printf("siblings2: %d\n", numLogicalCores);
            std::printf("levelType2: %d\n", levelType2);
            std::printf("chip Id: %d\n", x2apicId >> bitShift2);

/*             if (hasHYBRID()) {
                Regs regs;
                __get_cpuid_count(0x1A, 0, &regs.eax, &regs.ebx, &regs.ecx, &regs.edx);
                printf("core type: 0x%x\n", (regs.eax & 0xffff0000) >> 16);
            } */

	        if (!cpu->nodes) {
		        cpu->nodes = new CpuNode[numLogicalCores];
		        cpu->numNodes = numLogicalCores;
	        }

            CpuNode &node = cpu->nodes[cpu->index];

            if (node.index == -1U) {
                node.index      = cpu->index;
                node.id         = x2apicId;
                node.coreId     = coreId;
                node.threadId = x2apicId & bitShift;
                //node = { siblings, new std::uint32_t [siblings] };
            }

            //node.[x2apicId - coreId] = x2apicId;

        return nullptr;
    }

    void detectTopology() {
        if (isIntel) {

            const std::uint32_t maxNumIds = (leafs[1].ebx & 0xFF0000) >> 16;
            printf("max x2APIC IDs: %d\n", maxNumIds);

            if (!hasHTT()) {
                // Single core CPU.
                return;
            }

            dumpRegs(leafs[4]);

             const std::uint32_t numCores = ((leafs[4].eax & 0xFC000000) >> 26) + 1;
            printf("num cores: %d\n", numCores);

            cpu_set_t *cpusetp = CPU_ALLOC(maxNumIds);
            std::size_t size = CPU_ALLOC_SIZE(maxNumIds);

            for (;;) {
                pthread_t t;
                pthread_attr_t attr;

                pthread_attr_init(&attr);

                CPU_ZERO_S(size, cpusetp);
                CPU_SET_S(cpu.index, size, cpusetp);

                pthread_attr_setaffinity_np(&attr, size, cpusetp);
                pthread_create(&t, &attr, threadRoutine, &cpu);
                pthread_join(t, nullptr);
                pthread_attr_destroy(&attr);
                
                if (cpu.index >= cpu.numNodes) {
                    break;
                }
                
                cpu.index++;
            }

            CPU_FREE(cpusetp);
            
            if (nleafs >= 0x0000001F) {
                puts("0x1F exists.");

            }

            // logical cpu
            // core within chip
            // chip within NUMA domain
            // NUMA domain
        }

    }

    // leaf 1
    // ecx:
    inline bool hasSSE3() const noexcept { return BIT_CHECK(leafs[1].ecx, bit_SSE3); }
    inline bool hasSSSE3() const noexcept { return BIT_CHECK(leafs[1].ecx, bit_SSSE3); }
    inline bool hasSSE41() const noexcept { return BIT_CHECK(leafs[1].ecx, bit_SSE4_1); }
    inline bool hasSSE42() const noexcept { return BIT_CHECK(leafs[1].ecx, bit_SSE4_2); }
    inline bool hasAVX() const noexcept { return BIT_CHECK(leafs[1].ecx, bit_AVX); }

    // edx:
    inline bool hasHTT() const noexcept { return BIT_CHECK(leafs[1].edx, bit_HTT); }

    // edx:
    inline bool hasMMX() const noexcept { return BIT_CHECK(leafs[1].edx, bit_MMX); }
    inline bool hasSSE() const noexcept { return BIT_CHECK(leafs[1].edx, bit_SSE); }
    inline bool hasSSE2() const noexcept { return BIT_CHECK(leafs[1].edx, bit_SSE2); }

    // leaf 7
    // ebx:
    inline bool hasSHA() const noexcept { return BIT_CHECK(leafs[7].ebx, bit_SHA); }
    inline bool hasAVX2() const noexcept { return BIT_CHECK(leafs[7].ebx, bit_AVX2); }

    // edx:
    inline bool hasHYBRID() const noexcept { return BIT_CHECK(leafs[7].edx, bit_HYBRID); }


};



}

#endif

#include "Processor.h"

namespace sys {

inline static const Processor cpu;

}


int main() {

    puts(sys::cpu.getVendorId());
    puts(sys::cpu.getBrandId());

    printf("Family ID: %d\n", sys::cpu.getFamilyId());
    printf("Model: %d\n", sys::cpu.getModel());
    std::printf("Num logical cores: %d\n", sys::cpu.getNumCores());

    printf("INTEL: %s\n", sys::cpu.isIntel() ? "true" : "false");
    printf("AMD: %s\n", sys::cpu.isAMD() ? "true" : "false");
    printf("SSE: %s\n", sys::cpu.hasSSE() ? "true" : "false");
    printf("SSE2: %s\n", sys::cpu.hasSSE2() ? "true" : "false");
    printf("SSE3: %s\n", sys::cpu.hasSSE3() ? "true" : "false");
    printf("SSSE3: %s\n", sys::cpu.hasSSSE3() ? "true" : "false");
    printf("AVX: %s\n", sys::cpu.hasAVX() ? "true" : "false");
    printf("HYBRID: %s\n", sys::cpu.hasHYBRID() ? "true" : "false");

    sys::cpu.forEachThread([](const sys::LogicalCore &core) {
        std::printf("core: %d, chip: %d, core type: %d\n", core.core, core.chip, core.coreType);
    });

    return 0;
}
