
#pragma once

#include <systemc>

#include "target.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/*Build the first interconnect as a blocking address router.
Its job is not yet to model a real AXI bus. It only needs to:
-Receive a system address.
-Find which target owns that address range.
-Convert the system address into a target-local address.
-Forward the read or write.
-Add routing delay and logs.
-Reject unmapped or overlapping address ranges.
*/

class Interconnect : public sc_core::sc_module {
public:
    Interconnect(
        sc_core::sc_module_name name,
        sc_core::sc_time routing_delay
    );

    void map_target(
        Address base_address,
        std::size_t range_size,
        Target& target,
        std::string target_name
    );

    void write(
        Address system_address,
        const std::uint8_t* source,
        std::size_t size_bytes
    );

    void read(
        Address system_address,
        std::uint8_t* destination,
        std::size_t size_bytes
    );

private:
    struct Mapping {
        Address base_address;
        std::size_t range_size;
        Target* target;
        std::string target_name;
    };

    struct DecodedTarget {
        Target& target;
        Address local_address;
        const std::string& target_name;
    };

    [[nodiscard]]
    DecodedTarget decode(
        Address system_address,
        std::size_t transfer_size
    ) const;

    [[nodiscard]]
    bool ranges_overlap(
        Address first_base,
        std::size_t first_size,
        Address second_base,
        std::size_t second_size
    ) const;

    std::vector<Mapping> mappings_;
    sc_core::sc_time routing_delay_;
};