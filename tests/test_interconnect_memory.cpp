#include <systemc>

#include "address_mapping.h"
#include "interconnect.h"
#include "memory.h"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>

class InterconnectMemoryTest final : public sc_core::sc_module {
public:

    InterconnectMemoryTest(
        sc_core::sc_module_name name,
        Interconnect& interconnect
    )
        : sc_core::sc_module{name},
          interconnect_{interconnect} {

        SC_THREAD(run);
    }

private:
    Interconnect& interconnect_;

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
            throw std::runtime_error{
                "test_valid_access: read data does not match written data"
            };
        }

        std::cout
            << "[" << sc_core::sc_time_stamp() << "] "
            << "test_valid_access PASSED\n";
    }

    void test_unmapped_access() {
        std::array<std::uint8_t, 4> buffer{};

        try {
            interconnect_.read(
                0x50000000,
                buffer.data(),
                buffer.size()
            );
        } catch (const std::out_of_range&) {
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << "test_unmapped_access PASSED\n";

            return;
        }

        throw std::runtime_error{
            "test_unmapped_access: unmapped address was accepted"
        };
    }

    void test_boundary_crossing() {
        std::array<std::uint8_t, 8> buffer{};

        try {
            /*
             * Main-memory range:
             *
             * MEMORY_BASE
             *     ...
             * MEMORY_BASE + MEMORY_SIZE - 1
             *
             * Starting at offset 0xFC and reading 8 bytes
             * crosses the end of a 256-byte mapping.
             */
            interconnect_.read(
                address_map::MEMORY_BASE + 0xFFFFC,
                buffer.data(),
                buffer.size()
            );
        } catch (const std::out_of_range&) {
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << "test_boundary_crossing PASSED\n";

            return;
        }

        throw std::runtime_error{
            "test_boundary_crossing: transaction crossing mapping boundary "
            "was accepted"
        };
    }
};


int sc_main(int argc, char* argv[]) {
    static_cast<void>(argc);
    static_cast<void>(argv);

    try {
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

        InterconnectMemoryTest test{
            "interconnect_memory_test",
            interconnect
        };

        sc_core::sc_start();

        std::cout << "All interconnect-memory tests PASSED\n";

        return 0;

    } catch (const std::exception& error) {
        std::cerr
            << "Interconnect-memory test FAILED: "
            << error.what()
            << '\n';

        return 1;
    }
}