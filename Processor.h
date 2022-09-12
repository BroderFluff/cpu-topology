#pragma once
#ifndef SYS_PROCESSOR_H
#define SYS_PROCESSOR_H

#include <cstdint>

namespace sys {

struct Processor {
    std::uint32_t vendorId[4];
    std::uint32_t brand[12];

    Processor() noexcept;
    ~Processor();
};

}

#endif // SYS_PROCESSOR_H

