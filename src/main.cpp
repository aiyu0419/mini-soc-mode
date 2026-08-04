#include <systemc>

#include "address_mapping.h"
#include "interconnect.h"
#include "memory.h"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>

class InterconnectTest : public sc_core::sc_module {
public:
    InterconnectTest(
        sc_core::sc_module_name name,
        Interconnect& interconnect
    )
        : sc_core::sc_module{name},
          interconnect_{interconnect} {

        SC_THREAD(run);
    }

private:
    void run() {
        test_valid_access();
        test_unmapped_access();
        test_boundary_crossing();

        sc_core::sc_stop();
    }

    void test_valid_access() {
        constexpr Address address =
            address_map::MEMORY_BASE + 0x20;

        const std::array<std::uint8_t, 4> write_data{
            0x11,
            0x22,
            0x33,
            0x44
        };

        std::array<std::uint8_t, 4> read_data{};

        interconnect_.write(
            address,
            write_data.data(),
            write_data.size()
        );

        interconnect_.read(
            address,
            read_data.data(),
            read_data.size()
        );

        if (read_data != write_data) {
            throw std::runtime_error(
                "Valid interconnect access failed"
            );
        }

        std::cout
            << "[" << sc_core::sc_time_stamp() << "] "
            << "Valid interconnect test PASSED\n";
    }

    void test_unmapped_access() {
        std::array<std::uint8_t, 4> buffer{};

        try {
            interconnect_.read(
                0x50000000,
                buffer.data(),
                buffer.size()
            );

            throw std::runtime_error(
                "Unmapped address was incorrectly accepted"
            );

        } catch (const std::out_of_range& error) {
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << "Unmapped-address test PASSED: "
                << error.what()
                << '\n';
        }
    }

    void test_boundary_crossing() {
        std::array<std::uint8_t, 8> buffer{};

        try {
            /*
             * Memory range:
             *
             * 0x00000000–0x000000FF
             *
             * Starting at 0xFC and reading eight bytes would
             * cross beyond the mapped range.
             */
            interconnect_.read(
                address_map::MEMORY_BASE + 0xFC,
                buffer.data(),
                buffer.size()
            );

            throw std::runtime_error(
                "Boundary-crossing transaction was accepted"
            );

        } catch (const std::out_of_range& error) {
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << "Boundary-crossing test PASSED: "
                << error.what()
                << '\n';
        }
    }

    Interconnect& interconnect_;
};

int sc_main(int argc, char* argv[]) {
    static_cast<void>(argc);
    static_cast<void>(argv);

    Memory memory{
        "main_memory",
        address_map::MEMORY_SIZE,
        sc_core::sc_time{10, sc_core::SC_NS}
    };

    Interconnect interconnect{
        "interconnect",
        sc_core::sc_time{2, sc_core::SC_NS}
    };

    interconnect.map_target(
        address_map::MEMORY_BASE,
        address_map::MEMORY_SIZE,
        memory,
        "main_memory"
    );

    InterconnectTest test{
        "interconnect_test",
        interconnect
    };

    sc_core::sc_start();

    return 0;
}