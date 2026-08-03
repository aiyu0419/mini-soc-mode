#include "memory.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
/*
 * Memory model implementation.
 *
 * This is a simple memory model that provides read and write access to a
 * storage area. It includes bounds checking to ensure that memory accesses
 * are valid. The memory model also simulates access delays for read and write
 * operations.
 *
 * The memory model is implemented as a SystemC module, allowing it to be
 * integrated into a larger SystemC simulation environment.
 */ 

//initialize memory with size and access delay, throw exception if size is 0 or access delay is negative
Memory::Memory(
    sc_core::sc_module_name name,
    std::size_t size_bytes,
    sc_core::sc_time access_delay
)
    : sc_core::sc_module{name},
      storage_(size_bytes, 0),
      access_delay_{access_delay} {

    if (size_bytes == 0) {
        throw std::invalid_argument(
            "Memory size must be greater than zero"
        );
    }

    if (access_delay < sc_core::SC_ZERO_TIME) {
        throw std::invalid_argument(
            "Memory access delay cannot be negative"
        );
    }

    std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << name << ": initialized with "
        << storage_.size() << " bytes, access delay "
        << access_delay_ << '\n';
}

void Memory::write(
    Address address,
    const std::uint8_t* source,
    std::size_t size_bytes
) {
    if (source == nullptr && size_bytes != 0) {
        throw std::invalid_argument(
            "Memory::write received a null source pointer"
        );
    }

    check_bounds(address, size_bytes);

    std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << name() << ": WRITE request"
        << " address=0x"
        << std::hex << address
        << std::dec
        << " size=" << size_bytes
        << " bytes\n";

    wait(access_delay_);

    const auto offset = static_cast<std::size_t>(address);

    std::copy_n(
        source,
        size_bytes,
        storage_.begin() + offset
    );

    std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << name() << ": WRITE completed"
        << " address=0x"
        << std::hex << address
        << std::dec
        << " size=" << size_bytes
        << " bytes\n";
}

void Memory::read(
    Address address,
    std::uint8_t* destination,
    std::size_t size_bytes
) {
    if (destination == nullptr && size_bytes != 0) {
        throw std::invalid_argument(
            "Memory::read received a null destination pointer"
        );
    }

    check_bounds(address, size_bytes);

    std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << name() << ": READ request"
        << " address=0x"
        << std::hex << address
        << std::dec
        << " size=" << size_bytes
        << " bytes\n";

    wait(access_delay_);

    const auto offset = static_cast<std::size_t>(address);

    std::copy_n(
        storage_.begin() + offset,
        size_bytes,
        destination
    );

    std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << name() << ": READ completed"
        << " address=0x"
        << std::hex << address
        << std::dec
        << " size=" << size_bytes
        << " bytes\n";
}

std::size_t Memory::size_bytes() const noexcept {
    return storage_.size();
}

void Memory::check_bounds(
    Address address,
    std::size_t transfer_size
) const {
    /*
     * Avoid checking:
     *
     *     address + transfer_size > storage_.size()
     *
     * because address + transfer_size could overflow.
     */

    if (address > storage_.size()) {
        std::ostringstream message;

        message
            << "Memory address 0x"
            << std::hex << address
            << std::dec
            << " is outside memory capacity "
            << storage_.size();

        throw std::out_of_range(message.str());
    }

    const auto offset = static_cast<std::size_t>(address);
    const auto remaining_bytes = storage_.size() - offset;

    if (transfer_size > remaining_bytes) {
        std::ostringstream message;

        message
            << "Memory transaction is out of bounds: "
            << "address=0x"
            << std::hex << address
            << std::dec
            << ", size=" << transfer_size
            << ", capacity=" << storage_.size();

        throw std::out_of_range(message.str());
    }
}