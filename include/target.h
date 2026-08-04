
#pragma once

#include <cstddef>
#include <cstdint>

#include "type.h"

/*The interconnect must be able to communicate with different target types:
- Memory
- DMA registers
- accelerator registers
- interrupt controller registers
target has a pure virtual interface that all targets must implement.
all targets must implement read and write methods that take a local address, a source/destination buffer, and a size in bytes.
*/
class Target {
public:
    virtual ~Target() = default;

    virtual void write(
        Address local_address,
        const std::uint8_t* source,
        std::size_t size_bytes
    ) = 0;

    virtual void read(
        Address local_address,
        std::uint8_t* destination,
        std::size_t size_bytes
    ) = 0;
};