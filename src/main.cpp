#include <systemc>

#include "accelerator.h"
#include "common/type.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "common/bytes_utils.h"


class ConvolutionTest final : public sc_core::sc_module {
public:

    ConvolutionTest(
        sc_core::sc_module_name name,
        ConvolutionAccelerator& accelerator
    )
        : sc_core::sc_module(name),
          accelerator_(accelerator) {

        SC_THREAD(run);
    }

private:
    /*
     * These are accelerator-local addresses.
     *
     * They currently match accelerator.h.
     */
    static constexpr Address REG_CONTROL       = 0x00;
    static constexpr Address REG_STATUS        = 0x04;
    static constexpr Address REG_INPUT_HEIGHT  = 0x08;
    static constexpr Address REG_INPUT_WIDTH   = 0x0C;
    static constexpr Address REG_KERNEL_HEIGHT = 0x10;
    static constexpr Address REG_KERNEL_WIDTH  = 0x14;
    static constexpr Address REG_STRIDE        = 0x18;
    static constexpr Address REG_MAC_UNITS     = 0x1C;
    static constexpr Address REG_OUTPUT_HEIGHT = 0x20;
    static constexpr Address REG_OUTPUT_WIDTH  = 0x24;
    static constexpr Address REG_CYCLES_LOW    = 0x28;
    static constexpr Address REG_CYCLES_HIGH   = 0x2C;

    static constexpr Address INPUT_BUFFER_BASE  = 0x1000;
    static constexpr Address WEIGHT_BUFFER_BASE = 0x20000;
    static constexpr Address OUTPUT_BUFFER_BASE = 0x30000;

    static constexpr std::uint32_t CONTROL_START =
        1U << 0U;

    

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
            result[i] = byte_utils::load_i32_le(
                bytes.data()
                    + i * sizeof(std::int32_t)
            );
        }

        return result;
    }

    void run() {
        std::cout
            << "[" << sc_core::sc_time_stamp() << "] "
            << "Convolution test started\n";

        /*
         * Input:
         *
         *  1   2   3   4
         *  5   6   7   8
         *  9  10  11  12
         * 13  14  15  16
         */
        const std::vector<std::int32_t> input = {
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
        const std::vector<std::int32_t> weights = {
            1, 1, 1,
            1, 1, 1,
            1, 1, 1
        };

        /*
         * Expected output:
         *
         * 54  63
         * 90  99
         */
        const std::vector<std::int32_t> expected = {
            54, 63,
            90, 99
        };

        /*
         * ------------------------------------------------
         * Step 1: Configure accelerator registers
         * ------------------------------------------------
         */

        write_register(
            REG_INPUT_HEIGHT,
            4
        );

        write_register(
            REG_INPUT_WIDTH,
            4
        );

        write_register(
            REG_KERNEL_HEIGHT,
            3
        );

        write_register(
            REG_KERNEL_WIDTH,
            3
        );

        write_register(
            REG_STRIDE,
            1
        );

        write_register(
            REG_MAC_UNITS,
            4
        );

        /*
         * ------------------------------------------------
         * Step 2: Transfer data into accelerator buffers
         * ------------------------------------------------
         *
         * Right now this directly calls accelerator.write().
         *
         * Later DMA will perform these writes through
         * the interconnect.
         */

        write_i32_buffer(
            INPUT_BUFFER_BASE,
            input
        );

        write_i32_buffer(
            WEIGHT_BUFFER_BASE,
            weights
        );

        std::cout
            << "[" << sc_core::sc_time_stamp() << "] "
            << "Configuration and buffers loaded\n";

        /*
         * ------------------------------------------------
         * Step 3: Start accelerator
         * ------------------------------------------------
         */

        const sc_core::sc_time compute_start =
            sc_core::sc_time_stamp();

        write_register(
            REG_CONTROL,
            CONTROL_START
        );

        std::cout
            << "[" << sc_core::sc_time_stamp() << "] "
            << "START written\n";

        /*
         * CPU/testbench now waits.
         *
         * Accelerator worker runs independently.
         */
        wait(
            accelerator_.completion_event()
        );

        const sc_core::sc_time compute_end =
            sc_core::sc_time_stamp();

        std::cout
            << "[" << sc_core::sc_time_stamp() << "] "
            << "Accelerator completion event received\n";

        /*
         * ------------------------------------------------
         * Step 4: Read status and performance registers
         * ------------------------------------------------
         */

        const std::uint32_t status =
            read_register(REG_STATUS);

        const std::uint32_t output_height =
            read_register(REG_OUTPUT_HEIGHT);

        const std::uint32_t output_width =
            read_register(REG_OUTPUT_WIDTH);

        const std::uint32_t cycles_low =
            read_register(REG_CYCLES_LOW);

        const std::uint32_t cycles_high =
            read_register(REG_CYCLES_HIGH);

        const std::uint64_t compute_cycles =
            static_cast<std::uint64_t>(cycles_low)
            |
            (
                static_cast<std::uint64_t>(cycles_high)
                << 32U
            );

        /*
         * ------------------------------------------------
         * Step 5: Read accelerator output buffer
         * ------------------------------------------------
         */

        const auto output =
            read_i32_buffer(
                OUTPUT_BUFFER_BASE,
                expected.size()
            );

        /*
         * ------------------------------------------------
         * Step 6: Print results
         * ------------------------------------------------
         */

        std::cout << "\n";
        std::cout << "=== Accelerator Results ===\n";

        std::cout
            << "Status: 0x"
            << std::hex
            << status
            << std::dec
            << '\n';

        std::cout
            << "Output dimensions: "
            << output_height
            << " x "
            << output_width
            << '\n';

        std::cout
            << "Reported compute cycles: "
            << compute_cycles
            << '\n';

        std::cout
            << "Observed elapsed simulation time: "
            << compute_end - compute_start
            << '\n';

        std::cout << "Output:\n";

        for (std::size_t i = 0; i < output.size(); ++i) {
            std::cout
                << output[i]
                << ' ';

            if ((i + 1U) % output_width == 0U) {
                std::cout << '\n';
            }
        }

        /*
         * ------------------------------------------------
         * Step 7: Verify result
         * ------------------------------------------------
         */

        if (output_height != 2U ||
            output_width != 2U) {

            throw std::runtime_error(
                "Incorrect output dimensions"
            );
        }

        if (compute_cycles != 9U) {
            throw std::runtime_error(
                "Incorrect compute cycle count"
            );
        }

        if (output != expected) {
            std::cerr << "Expected:\n";

            for (const auto value : expected) {
                std::cerr
                    << value
                    << ' ';
            }

            std::cerr << '\n';

            throw std::runtime_error(
                "Convolution output mismatch"
            );
        }

        std::cout << "\n";
        std::cout << "CONVOLUTION TEST PASSED\n";

        sc_core::sc_stop();
    }

    ConvolutionAccelerator& accelerator_;
};


int sc_main(
    int argc,
    char* argv[]
) {
    (void)argc;
    (void)argv;

    /*
     * MMIO register access = 5 ns
     * Accelerator clock    = 1 ns
     *
     * Buffer accesses currently add no accelerator-local delay.
     */
    ConvolutionAccelerator accelerator(
        "accelerator",
        sc_core::sc_time(5, sc_core::SC_NS),
        sc_core::sc_time(1, sc_core::SC_NS),
        64,     // input buffer
        36,     // weight buffer
        16      // output buffer
    );

    ConvolutionTest test(
        "test",
        accelerator
    );

    sc_core::sc_start();

    return 0;
}