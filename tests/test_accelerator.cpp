#include <systemc>

#include "accelerator.h"
#include "common/bytes_utils.h"
#include "common/type.h"
#include "common/register_mapping.h"

#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>



class AcceleratorTest final : public sc_core::sc_module {
public:

    AcceleratorTest(
        sc_core::sc_module_name name,
        ConvolutionAccelerator& accelerator
    )
        : sc_core::sc_module{name},
          accelerator_{accelerator} {

        SC_THREAD(run);
    }

private:
    ConvolutionAccelerator& accelerator_;

    void run() {
        test_basic_convolution();

        sc_core::sc_stop();
    }


    void write_register(
        Address address,
        std::uint32_t value
    ) {
        std::uint8_t bytes[4];

        byte_utils::store_u32_le(
            value,
            bytes
        );

        accelerator_.write(
            address,
            bytes,
            sizeof(bytes)
        );
    }


    std::uint32_t read_register(
        Address address
    ) {
        std::uint8_t bytes[4]{};

        accelerator_.read(
            address,
            bytes,
            sizeof(bytes)
        );

        return byte_utils::load_u32_le(bytes);
    }


    void write_i32_buffer(
        Address address,
        const std::vector<std::int32_t>& values
    ) {
        std::vector<std::uint8_t> bytes(
            values.size() * sizeof(std::int32_t)
        );

        for (std::size_t i = 0; i < values.size(); ++i) {
            byte_utils::store_i32_le(
                values[i],
                bytes.data()
                    + i * sizeof(std::int32_t)
            );
        }

        accelerator_.write(
            address,
            bytes.data(),
            bytes.size()
        );
    }


    std::vector<std::int32_t> read_i32_buffer(
        Address address,
        std::size_t count
    ) {
        std::vector<std::uint8_t> bytes(
            count * sizeof(std::int32_t)
        );

        accelerator_.read(
            address,
            bytes.data(),
            bytes.size()
        );

        std::vector<std::int32_t> result(count);

        for (std::size_t i = 0; i < count; ++i) {
            result[i] =
                byte_utils::load_i32_le(
                    bytes.data()
                        + i * sizeof(std::int32_t)
                );
        }

        return result;
    }


    void test_basic_convolution() {
        /*
         * Input:
         *
         *  1   2   3   4
         *  5   6   7   8
         *  9  10  11  12
         * 13  14  15  16
         */
        const std::vector<std::int32_t> input{
             1,  2,  3,  4,
             5,  6,  7,  8,
             9, 10, 11, 12,
            13, 14, 15, 16
        };

        /*
         * Kernel:
         *
         * 1 1 1
         * 1 1 1
         * 1 1 1
         */
        const std::vector<std::int32_t> weights{
            1, 1, 1,
            1, 1, 1,
            1, 1, 1
        };

        /*
         * Expected:
         *
         * 54  63
         * 90  99
         */
        const std::vector<std::int32_t> expected{
            54, 63,
            90, 99
        };

        /*
         * Configure accelerator.
         */
        write_register(
            register_mapping::accelerator_register::INPUT_HEIGHT,
            4
        );

        write_register(
            register_mapping::accelerator_register::INPUT_WIDTH,
            4
        );

        write_register(
            register_mapping::accelerator_register::KERNEL_HEIGHT,
            3
        );

        write_register(
            register_mapping::accelerator_register::KERNEL_WIDTH,
            3
        );

        write_register(
            register_mapping::accelerator_register::STRIDE,
            1
        );

        write_register(
            register_mapping::accelerator_register::MAC_UNITS,
            4
        );

        /*
         * Load accelerator-local buffers directly.
         *
         * This is intentionally NOT using DMA or Interconnect.
         */
        write_i32_buffer(
            register_mapping::accelerator_register::INPUT_BUFFER_BASE,
            input
        );

        write_i32_buffer(
            register_mapping::accelerator_register::WEIGHT_BUFFER_BASE,
            weights
        );

        /*
         * Start accelerator.
         */
        write_register(
            register_mapping::accelerator_register::CONTROL,
            register_mapping::accelerator_register::START
        );

        /*
         * Wait for asynchronous computation.
         */
        wait(accelerator_.completion_event());

        /*
         * Verify final status.
         */
        const std::uint32_t status =
            read_register(
                register_mapping::accelerator_register::STATUS
            );

        if ((status &
             register_mapping::accelerator_register::STATUS_ERROR) != 0U) {

            throw std::runtime_error{
                "test_basic_convolution: "
                "accelerator reported ERROR"
            };
        }

        if ((status &
             register_mapping::accelerator_register::STATUS_DONE) == 0U) {

            throw std::runtime_error{
                "test_basic_convolution: "
                "accelerator did not set DONE"
            };
        }

        if ((status &
             register_mapping::accelerator_register::STATUS_BUSY) != 0U) {

            throw std::runtime_error{
                "test_basic_convolution: "
                "accelerator remained BUSY"
            };
        }

        /*
         * Verify output dimensions.
         */
        const std::uint32_t output_height =
            read_register(
                register_mapping::accelerator_register::OUTPUT_HEIGHT
            );

        const std::uint32_t output_width =
            read_register(
                register_mapping::accelerator_register::OUTPUT_WIDTH
            );

        if (output_height != 2U ||
            output_width != 2U) {

            throw std::runtime_error{
                "test_basic_convolution: "
                "incorrect output dimensions"
            };
        }

        /*
         * Verify modeled compute cycles.
         */
        const std::uint32_t cycles_low =
            read_register(
                register_mapping::accelerator_register::CYCLES_LOW
            );

        const std::uint32_t cycles_high =
            read_register(
                register_mapping::accelerator_register::CYCLES_HIGH
            );

        const std::uint64_t compute_cycles =
            static_cast<std::uint64_t>(cycles_low)
            |
            (
                static_cast<std::uint64_t>(cycles_high)
                << 32U
            );

        if (compute_cycles != 9U) {
            throw std::runtime_error{
                "test_basic_convolution: "
                "incorrect compute cycle count"
            };
        }

        /*
         * Verify convolution output.
         */
        const auto output =
            read_i32_buffer(
                register_mapping::accelerator_register::OUTPUT_BUFFER_BASE,
                expected.size()
            );

        if (output != expected) {
            throw std::runtime_error{
                "test_basic_convolution: "
                "convolution output mismatch"
            };
        }

        std::cout
            << "[" << sc_core::sc_time_stamp() << "] "
            << "test_basic_convolution PASSED\n";
    }
};


int sc_main(int argc, char* argv[]) {
    static_cast<void>(argc);
    static_cast<void>(argv);

    try {
        /*
         * MMIO access latency = 5 ns
         * Accelerator clock   = 1 ns
         */
        ConvolutionAccelerator accelerator{
            "accelerator",
            sc_core::sc_time{5, sc_core::SC_NS},
            sc_core::sc_time{1, sc_core::SC_NS},
            64,  // input buffer bytes
            36,  // weight buffer bytes
            16   // output buffer bytes
        };

        AcceleratorTest test{
            "accelerator_test",
            accelerator
        };

        sc_core::sc_start();

        std::cout
            << "All accelerator tests PASSED\n";

        return 0;

    } catch (const std::exception& error) {
        std::cerr
            << "Accelerator test FAILED: "
            << error.what()
            << '\n';

        return 1;
    }
}