#include "interconnect.h"

#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

Interconnect::Interconnect(
    sc_core::sc_module_name name,
    sc_core::sc_time routing_delay
)
    : sc_core::sc_module{name},
      routing_delay_{routing_delay} {

    if (routing_delay < sc_core::SC_ZERO_TIME) {
        throw std::invalid_argument(
            "Interconnect routing delay cannot be negative"
        );
    }

    std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << this->name()
        << ": initialized, routing delay="
        << routing_delay_
        << '\n';
}

void Interconnect::map_target(
    Address base_address,
    std::size_t range_size,
    Target& target,
    std::string target_name
) {
    if (range_size == 0) {
        throw std::invalid_argument(
            "Interconnect mapping size must be greater than zero"
        );
    }

    /*
     * Check that:
     *
     * base_address + range_size - 1
     *
     * fits inside the 32-bit address space.
     */
    const auto max_address =
        std::numeric_limits<Address>::max();

    if (range_size - 1 >
        static_cast<std::size_t>(max_address - base_address)) {

        throw std::out_of_range(
            "Interconnect mapping exceeds address space"
        );
    }

    for (const auto& existing : mappings_) {
        if (ranges_overlap(
                base_address,
                range_size,
                existing.base_address,
                existing.range_size
            )) {

            std::ostringstream message;

            message
                << "Address mapping for "
                << target_name
                << " overlaps mapping for "
                << existing.target_name;

            throw std::invalid_argument(message.str());
        }
    }

    mappings_.push_back(
        Mapping{
            base_address,
            range_size,
            &target,
            std::move(target_name)
        }
    );

    const auto final_address =
        static_cast<Address>(
            base_address + range_size - 1
        );

    std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << name()
        << ": mapped "
        << mappings_.back().target_name
        << " at [0x"
        << std::hex
        << std::setw(8)
        << std::setfill('0')
        << base_address
        << ", 0x"
        << std::setw(8)
        << final_address
        << "]"
        << std::dec
        << '\n';
}

void Interconnect::write(
    Address system_address,
    const std::uint8_t* source,
    std::size_t size_bytes
) {
    if (source == nullptr && size_bytes != 0) {
        throw std::invalid_argument(
            "Interconnect::write received null source"
        );
    }

    const auto decoded =
        decode(system_address, size_bytes);

    std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << name()
        << ": WRITE route"
        << " system_address=0x"
        << std::hex
        << system_address
        << " target="
        << decoded.target_name
        << " local_address=0x"
        << decoded.local_address
        << std::dec
        << " size="
        << size_bytes
        << '\n';

    wait(routing_delay_);

    decoded.target.write(
        decoded.local_address,
        source,
        size_bytes
    );
}

void Interconnect::read(
    Address system_address,
    std::uint8_t* destination,
    std::size_t size_bytes
) {
    if (destination == nullptr && size_bytes != 0) {
        throw std::invalid_argument(
            "Interconnect::read received null destination"
        );
    }

    const auto decoded =
        decode(system_address, size_bytes);

    std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << name()
        << ": READ route"
        << " system_address=0x"
        << std::hex
        << system_address
        << " target="
        << decoded.target_name
        << " local_address=0x"
        << decoded.local_address
        << std::dec
        << " size="
        << size_bytes
        << '\n';

    wait(routing_delay_);

    decoded.target.read(
        decoded.local_address,
        destination,
        size_bytes
    );
}

Interconnect::DecodedTarget Interconnect::decode(
    Address system_address,
    std::size_t transfer_size
) const {
    for (const auto& mapping : mappings_) {
        const auto mapping_end =
            static_cast<Address>(
                mapping.base_address
                + mapping.range_size
            );

        if (system_address < mapping.base_address || system_address >= mapping_end) {
            continue;
        }

        
        if (transfer_size > static_cast<std::size_t>(mapping_end - system_address)) {
            std::ostringstream message;

            message
                << "Transaction crosses target boundary: "
                << "address=0x"
                << std::hex
                << system_address
                << std::dec
                << ", size="
                << transfer_size
                << ", target="
                << mapping.target_name;

            throw std::out_of_range(message.str());
        }
        Address offset = system_address - mapping.base_address;

        return DecodedTarget{
            *mapping.target,
            offset,
            mapping.target_name
        };
    }

    std::ostringstream message;

    message
        << "Unmapped system address: 0x"
        << std::hex
        << system_address;

    throw std::out_of_range(message.str());
}

bool Interconnect::ranges_overlap(
    Address first_base,
    std::size_t first_size,
    Address second_base,
    std::size_t second_size
) const {
    const auto first_end =
        static_cast<std::uint64_t>(first_base)
        + first_size;

    const auto second_end =
        static_cast<std::uint64_t>(second_base)
        + second_size;

    return
        static_cast<std::uint64_t>(first_base)
            < second_end
        &&
        static_cast<std::uint64_t>(second_base)
            < first_end;
}