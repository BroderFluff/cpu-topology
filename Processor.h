#pragma once
#ifndef SYS_PROCESSOR_H
#define SYS_PROCESSOR_H

#include <cstdint>
#include <cstring>

namespace sys {

class Processor {
public:
    Processor() noexcept;
    ~Processor();

    const char *getVendorId() const noexcept {
        const char *p;
        std::memcpy( p, vendorId, sizeof (char *));
        return p;        
    }

private:
    std::uint32_t vendorId[4];
    std::uint32_t brand[12];
};

}

#endif // SYS_PROCESSOR_H

