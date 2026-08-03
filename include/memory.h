/*The first memory model only needs:
 * - A storage area to hold the memory contents
 * - Access methods for reading and writing data
 * - Bounds checking to ensure valid memory accesses
*/
#pragma once

#include <systemc>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class Memory : public sc_core::sc_module {
public:
    using Address = std::uint64_t;

    Memory(
        sc_core::sc_module_name name,
        std::size_t size_bytes,
        sc_core::sc_time access_delay
    );

    void write(
        Address address,
        const std::uint8_t* source,
        std::size_t size_bytes
    );

    void read(
        Address address,
        std::uint8_t* destination,
        std::size_t size_bytes
    );

    [[nodiscard]]
    std::size_t size_bytes() const noexcept;

private:
    void check_bounds(
        Address address,
        std::size_t transfer_size
    ) const;

    std::vector<std::uint8_t> storage_;
    sc_core::sc_time access_delay_;
};