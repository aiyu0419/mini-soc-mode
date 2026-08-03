#include <systemc>

#include "memory.h"

#include <array>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>

class MemoryTest : public sc_core::sc_module {
public:
    Memory& memory;

    MemoryTest(
        sc_core::sc_module_name name,
        Memory& memory_reference
    )
        : sc_core::sc_module{name},
          memory{memory_reference} {

        SC_THREAD(run);
    }

private:
    void run() {
        constexpr Memory::Address test_address = 0x20;

        const std::array<std::uint8_t, 4> write_data{
            0x11,
            0x22,
            0x33,
            0x44
        };

        std::array<std::uint8_t, 4> read_data{};

        std::cout
            << "[" << sc_core::sc_time_stamp() << "] "
            << "Starting memory test\n";

        memory.write(
            test_address,
            write_data.data(),
            write_data.size()
        );

        memory.read(
            test_address,
            read_data.data(),
            read_data.size()
        );

        if (read_data != write_data) {
            std::cerr
                << "[" << sc_core::sc_time_stamp() << "] "
                << "Memory test FAILED\n";

            sc_core::sc_stop();
            return;
        }

        std::cout
            << "[" << sc_core::sc_time_stamp() << "] "
            << "Memory test PASSED: ";

        for (const auto byte : read_data) {
            std::cout
                << "0x"
                << std::hex
                << std::setw(2)
                << std::setfill('0')
                << static_cast<unsigned int>(byte)
                << ' ';
        }

        std::cout << std::dec << '\n';

        test_invalid_access();

        sc_core::sc_stop();
    }

    void test_invalid_access() {
        std::array<std::uint8_t, 8> buffer{};

        try {
            /*
             * Memory contains 256 bytes:
             *
             * Valid addresses: 0x00 through 0xFF
             *
             * Starting at 0xFC and reading eight bytes would require
             * addresses 0xFC through 0x103, so the request is invalid.
             */
            memory.read(
                0xFC,
                buffer.data(),
                buffer.size()
            );

            std::cerr
                << "Bounds-check test FAILED: "
                << "invalid access was accepted\n";
        } catch (const std::out_of_range& error) {
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << "Bounds-check test PASSED: "
                << error.what()
                << '\n';
        }
    }
};

int sc_main(int argc, char* argv[]) {
    static_cast<void>(argc);
    static_cast<void>(argv);

    constexpr std::size_t memory_size = 256;

    Memory memory{
        "memory",
        memory_size,
        sc_core::sc_time{10, sc_core::SC_NS}
    };

    MemoryTest test{
        "memory_test",
        memory
    };

    sc_core::sc_start();

    return 0;
}