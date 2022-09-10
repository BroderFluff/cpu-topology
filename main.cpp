#include <cassert>
#include <cstdio>
#include <cstdint>
#include <vector>


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

namespace sys {

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

        leafs.resize(nleafs);
        std::uint32_t i = 0;
#if __cpp_structured_bindings == 201606L
        for (auto & [ eax, ebx, ecx, edx ]: leafs) {
            __get_cpuid_count(i, 0, &eax, &ebx, &ecx, &edx);
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

        if (hasHYBRID()) {  
            Regs regs;
            __get_cpuid_count(0x1A, 0, &regs.eax, &regs.ebx, &regs.ecx, &regs.edx);
            printf("core type: 0x%x\n", (regs.eax & 0xffff0000) >> 16);
        }

        __get_cpuid(0x80000002, &brand[0], &brand[1], &brand[2], &brand[3]);
        __get_cpuid(0x80000003, &brand[4], &brand[5], &brand[6], &brand[7]);
        __get_cpuid(0x80000004, &brand[8], &brand[9], &brand[10], &brand[11]);


        detectTopology();
    }

    struct PhysicalCore;

    struct LogicalCore {
        std::uint32_t coreIndex;
        std::uint32_t siblings;
    };

    struct PhysicalCore {
        std::uint32_t numLogicalCores {};
        std::uint32_t *logicalIds {};
    };

    enum class CpuNodeType : std::uint32_t {
        Invalid,
        SMT,
        Core,
    };

    struct CpuNode {
        CpuNode() noexcept
          : id { -1U }
          , type { CpuNodeType::Invalid} {
        }

        std::uint32_t       id;
        CpuNodeType         type;
        union { 
            LogicalCore     l;
            PhysicalCore    p;
        } core {};
    };

    struct Cpu {
        std::uint32_t numNodes;
        CpuNode *nodes = nullptr;
    };

    Cpu cpu;

    struct thread {
        pthread_t t;
        pthread_attr_t attr;
    };

    static void* threadRoutine(void *p) {
            Regs leaf;

            // First query level one (SMT):
            __get_cpuid_count(0xB, 0, &leaf.eax, &leaf.ebx, &leaf.ecx, &leaf.edx);

            const std::uint32_t levelType = (leaf.ecx & 0xFF00) >> 8;
            assert(levelType == 1);

            Cpu *cpu = static_cast<Cpu *>(p);

            CpuNode &node0 = cpu->nodes[leaf.edx];
            node0.id = leaf.edx;
            node0.type = CpuNodeType::SMT;

            LogicalCore &smt = node0.core.l;
            smt.siblings = (leaf.ebx & 0xFF);

            // Second query level (Core)
            __get_cpuid_count(0xB, 1, &leaf.eax, &leaf.ebx, &leaf.ecx, &leaf.edx);

            const uint32_t coreSiblings = leaf.ebx & 0xFF;
            smt.coreIndex = leaf.edx + coreSiblings;

            CpuNode &node1 = cpu->nodes[smt.coreIndex];
            

            const std::uint32_t bitShift = leaf.eax & 0x0F;
            const std::uint32_t siblings = leaf.ebx & 0xFF;
            const std::uint32_t id = leaf.edx;

            smt.type = CpuNodeType::SMT;
            smt.id = id;
            smt.core.l = LogicalCore();
            smt.core.l.physicalCoreId = (id >> bitShift);

            std::printf("siblings: %d\n", siblings);


            auto &core = cpu->nodes[coreIndex];

            assert(core.type != CpuNodeType::SMT);

            if (core.type == CpuNodeType::Invalid) {
                core.type = CpuNodeType::Core;
                core.id = leaf.edx;
                core.core.p = {};

                //printf("physical core id: %d\n", nextLevelNode.id + siblings);

                auto &physicalCore = core.core.p;
                physicalCore.numLogicalCores = siblings;
                physicalCore.logicalIds = new std::uint32_t[siblings];
                physicalCore.logicalIds[0] = id;
            } else {
                auto &physicalCore = core.core.p;
                physicalCore.logicalIds[1] = id;

                printf("core: %d, threads: %d, %d\n", core.id, physicalCore.logicalIds[0], physicalCore.logicalIds[1]);
            }

            if (siblings == 1) {
                printf("core: %d, thread: %d\n", core.id, id);
            }

            /* if (!levelType) return nullptr;

            if (levelType == 1) {
                printf("Core id: %d\n", id >> bitShift);
            } else if (levelType == 2) {
                printf("Processor id: %d\n", id >> bitShift);
            } */

            //printf("thread on core: %d, siblings: %2d, levelType: 0x%x = %s, id: 0x%x\n", ((CoreDesc *) p)->id, siblings, levelType, levelType == 1 ? " SMT" : "Core", id);
        //}

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

/*             const std::uint32_t numCores = ((leafs[4].eax & 0xFC000000) >> 26) + 1;
            printf("num cores: %d\n", numCores); */

            thread *threads = new thread[maxNumIds];
            cpu_set_t *cpusetp = CPU_ALLOC(maxNumIds);
            std::size_t size = CPU_ALLOC_SIZE(maxNumIds);

            cpu.numNodes = maxNumIds;
            cpu.nodes = new CpuNode[maxNumIds];
            for (int i = 0; i < cpu.numNodes; ++i) {
                thread &th = threads[i];
                //cpu.nodes[i].id = i;

                pthread_attr_init(&th.attr);

                CPU_ZERO_S(size, cpusetp);
                CPU_SET_S(i, size, cpusetp);

                pthread_attr_setaffinity_np(&th.attr, size, cpusetp);

                pthread_create(&th.t, &th.attr, threadRoutine, &cpu);

                pthread_join(th.t, nullptr);
            }

            CPU_FREE(cpusetp);
            
            if (nleafs >= 0x0000001F) {
                puts("0x1F exists.");

            }

            // logical cpu
            // core within chip
            // chip within NUMA domain
            // NUMA domain

#if 0

            if (nleafs >= 0x0000000B && leafs[0x0B].ebx) {
                puts("0x0B exists.");

                std::uint32_t level = 0;
                while (level < 2) {
                    Regs leaf;
                    __get_cpuid_count(0xB, level, &leaf.eax, &leaf.ebx, &leaf.ecx, &leaf.edx);

                    dumpRegs(leaf);

                    std::uint32_t bitShift = leaf.eax & 0x0F;
                    std::uint32_t numCpus = leaf.ebx & 0xFF;
                    std::uint32_t levelType = (leaf.ecx & 0xFF00) >> 8;
                    std::uint32_t id = leaf.edx;

                    printf("bits to shift: 0x%x\n", bitShift);
                    printf("logical cpus at this level: 0x%x\n", numCpus);
                    printf("level type: 0x%x\n", levelType);
                    printf("x2APIC ID: 0x%x\n", id);

                    if (levelType == 1) {
                        printf("smt id: %d\n", id >> bitShift);
                    } else if ( levelType == 2) {
                        printf("core id: %d\n", id >> bitShift);
                    }

                    puts("\n");

                    level++;
                }
            }
#endif
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

static const CPUInfo cpu;

}

int main() {

    puts((char*)sys::cpu.vendorId);
    puts((char*)sys::cpu.brand);

    printf("INTEL: %s\n", sys::cpu.isIntel ? "true" : "false");
    printf("AMD: %s\n", sys::cpu.isAMD ? "true" : "false");
    printf("SSE: %s\n", sys::cpu.hasSSE() ? "true" : "false");
    printf("SSE3: %s\n", sys::cpu.hasSSE3() ? "true" : "false");
    printf("AVX: %s\n", sys::cpu.hasAVX() ? "true" : "false");
    printf("HYBRID: %s\n", sys::cpu.hasHYBRID() ? "true" : "false");

    return 0;
}