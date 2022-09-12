#pragma once
#ifndef SYS_PROCESSOR_H
#define SYS_PROCESSOR_H

#include <cstdint>

namespace sys {

class Processor {
public:
    Processor() noexcept;
    ~Processor();

private:
    std::uint32_t vendorId[4];
    std::uint32_t brand[12];
};

}

#endif // SYS_PROCESSOR_H

