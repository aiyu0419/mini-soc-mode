#include <systemc>

#include "memory.h"


#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>

class MemoryTest final : public sc_core::sc_module {
public:

    MemoryTest(
        sc_core::sc_module_name name,
        Memory& memory
    )
        : sc_core::sc_module{name},
          memory_{memory} {

        SC_THREAD(run);
    }

private:
    Memory& memory_;

    void run() {
        test_read_write();
        test_last_valid_byte();
        test_out_of_bounds_read();
        test_out_of_bounds_write();

        sc_core::sc_stop();
    }

    void test_read_write() {
        constexpr Address test_address = 0x20;

        const std::array<std::uint8_t, 4> write_data{
            0x11,
            0x22,
            0x33,
            0x44
        };

        std::array<std::uint8_t, 4> read_data{};

        memory_.write(
            test_address,
            write_data.data(),
            write_data.size()
        );

        memory_.read(
            test_address,
            read_data.data(),
            read_data.size()
        );

        if (read_data != write_data) {
            throw std::runtime_error{
                "test_read_write: read data does not match written data"
            };
        }

        std::cout << "test_read_write PASSED\n";
    }

    void test_last_valid_byte() {
        constexpr Address address = 0xFF;

        const std::array<std::uint8_t, 1> write_data{
            0xAB
        };

        std::array<std::uint8_t, 1> read_data{};

        memory_.write(
            address,
            write_data.data(),
            write_data.size()
        );

        memory_.read(
            address,
            read_data.data(),
            read_data.size()
        );

        if (read_data != write_data) {
            throw std::runtime_error{
                "test_last_valid_byte: last valid byte read/write failed"
            };
        }

        std::cout << "test_last_valid_byte PASSED\n";
    }

    void test_out_of_bounds_read() {
        std::array<std::uint8_t, 8> buffer{};

        try {
            /*
             * Memory size = 256 bytes.
             *
             * Valid addresses:
             *     0x00 ... 0xFF
             *
             * Reading 8 bytes starting from 0xFC would access:
             *
             *     0xFC ... 0x103
             *
             * Therefore Memory::read() must reject this request.
             */
            memory_.read(
                0xFC,
                buffer.data(),
                buffer.size()
            );
        } catch (const std::out_of_range&) {
            std::cout << "test_out_of_bounds_read PASSED\n";
            return;
        }

        throw std::runtime_error{
            "test_out_of_bounds_read: invalid read was accepted"
        };
    }

    void test_out_of_bounds_write() {
        const std::array<std::uint8_t, 8> buffer{
            0x01,
            0x02,
            0x03,
            0x04,
            0x05,
            0x06,
            0x07,
            0x08
        };

        try {
            memory_.write(
                0xFC,
                buffer.data(),
                buffer.size()
            );
        } catch (const std::out_of_range&) {
            std::cout << "test_out_of_bounds_write PASSED\n";
            return;
        }

        throw std::runtime_error{
            "test_out_of_bounds_write: invalid write was accepted"
        };
    }
};


int sc_main(int argc, char* argv[]) {
    static_cast<void>(argc);
    static_cast<void>(argv);

    try {
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

        std::cout << "All memory tests PASSED\n";

        return 0;

    } catch (const std::exception& error) {
        std::cerr
            << "Memory test FAILED: "
            << error.what()
            << '\n';

        return 1;
    }
}