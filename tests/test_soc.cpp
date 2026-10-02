#include <systemc>

#include "accelerator.h"
#include "address_mapping.h"
#include "common/bytes_utils.h"
#include "dma.h"
#include "interconnect.h"
#include "memory.h"
#include "common/register_mapping.h"
#include "common/test_util.h"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>


class SoCTest final : public sc_core::sc_module {
public:

    SoCTest(
        sc_core::sc_module_name name,
        Interconnect& interconnect,
        DMA& dma,
        ConvolutionAccelerator& accelerator
    )
        : sc_core::sc_module{name},
          interconnect_{interconnect},
          dma_{dma},
          accelerator_{accelerator} {

        SC_THREAD(run);
    }

private:
    Interconnect& interconnect_;
    DMA& dma_;
    ConvolutionAccelerator& accelerator_;

    void run() {
        test_end_to_end_convolution();

        sc_core::sc_stop();
    }
    void dma_transfer(
            Address source,
            Address destination,
            std::size_t length,
            std::uint32_t burst_size = 8
    ) {
            test_util::write_u32(
            interconnect_,
            address_map::DMA_BASE
                + register_mapping::dma_register::SOURCE_LOW,
            static_cast<std::uint32_t>(source)
        );

        test_util::write_u32(
            interconnect_,
            address_map::DMA_BASE
                + register_mapping::dma_register::SOURCE_HIGH,
            0
    );

        test_util::write_u32(
            interconnect_,
            address_map::DMA_BASE
                + register_mapping::dma_register::DESTINATION_LOW,
            static_cast<std::uint32_t>(destination)
        );

        test_util::write_u32(
            interconnect_,
            address_map::DMA_BASE
                + register_mapping::dma_register::DESTINATION_HIGH,
            0
        );

        test_util::write_u32(
            interconnect_,
            address_map::DMA_BASE
                + register_mapping::dma_register::LENGTH_BYTES,
            static_cast<std::uint32_t>(length)
        );

        test_util::write_u32(
            interconnect_,
            address_map::DMA_BASE
                + register_mapping::dma_register::BURST_BYTES,
            burst_size
        );

        test_util::write_u32(
            interconnect_,
            address_map::DMA_BASE
                + register_mapping::dma_register::CONTROL,
            register_mapping::dma_register::START
        );

        wait(dma_.completion_event());

        const std::uint32_t status =
            test_util::read_u32(
                interconnect_,
                address_map::DMA_BASE
                    + register_mapping::dma_register::STATUS
            );

        if ((status & register_mapping::dma_register::STATUS_ERROR) != 0U) {
            throw std::runtime_error{
                "DMA transfer reported ERROR"
            };
        }
    

        if ((status & register_mapping::dma_register::STATUS_DONE) == 0U) {
            throw std::runtime_error{
                "DMA transfer did not set DONE"
            };
        }
    

        if ((status & register_mapping::dma_register::STATUS_BUSY) != 0U) {
            throw std::runtime_error{
                "DMA remained BUSY after completion"
            };
        }
    }
    void test_end_to_end_convolution() {
       /*
        * Main-memory layout used by this test.
        */
        constexpr Address input_memory_address =
            address_map::MEMORY_BASE + 0x0000;

        constexpr Address weight_memory_address =
            address_map::MEMORY_BASE + 0x0100;

        constexpr Address output_memory_address =
            address_map::MEMORY_BASE + 0x0200;

       /*
        * Accelerator system addresses.
        *
        * These are NOT accelerator-local addresses.
        */
        constexpr Address accelerator_input_address =
        address_map::ACCELERATOR_BASE
        + register_mapping::accelerator_register::INPUT_BUFFER_BASE;

        constexpr Address accelerator_weight_address =
        address_map::ACCELERATOR_BASE
        + register_mapping::accelerator_register::WEIGHT_BUFFER_BASE;

        constexpr Address accelerator_output_address =
        address_map::ACCELERATOR_BASE
        + register_mapping::accelerator_register::OUTPUT_BUFFER_BASE;


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
        * Expected convolution result:
        *
        * 54 63
        * 90 99
        */
        const std::vector<std::int32_t> expected{
            54, 63,
            90, 99
        };


   /*
    * ------------------------------------------------
    * Step 1:
    * CPU/test driver prepares data in main memory.
    * ------------------------------------------------
    */
        test_util::write_i32_buffer(
            interconnect_,
            input_memory_address,
            input
        );

        test_util::write_i32_buffer(
            interconnect_,
            weight_memory_address,
            weights
        );


   /*
    * ------------------------------------------------
    * Step 2:
    * DMA transfers input:
    *
    * Memory -> Interconnect -> Accelerator input buffer
    * ------------------------------------------------
    */
        dma_transfer(
        input_memory_address,
        accelerator_input_address,
        input.size() * sizeof(std::int32_t)
    );


    /*
     * ------------------------------------------------
     * Step 3:
     * DMA transfers weights:
     *
     * Memory -> Interconnect -> Accelerator weight buffer
     * ------------------------------------------------
     */
        dma_transfer(
            weight_memory_address,
            accelerator_weight_address,
            weights.size() * sizeof(std::int32_t)
        );
        
    

    /*
     * ------------------------------------------------
     * Step 4:
     * CPU/test driver configures accelerator through
     * system MMIO addresses.
     * ------------------------------------------------
     */
        test_util::write_u32(
            interconnect_,
            address_map::ACCELERATOR_BASE
                + register_mapping::accelerator_register::INPUT_HEIGHT,
            4
    );

        test_util::write_u32(
            interconnect_,    
            address_map::ACCELERATOR_BASE
                + register_mapping::accelerator_register::INPUT_WIDTH,
            4
    );

        test_util::write_u32(
        interconnect_,
        address_map::ACCELERATOR_BASE
            + register_mapping::accelerator_register::KERNEL_HEIGHT,
        3
    );

    test_util::write_u32(
        interconnect_,
        address_map::ACCELERATOR_BASE
            + register_mapping::accelerator_register::KERNEL_WIDTH,
        3
    );

    test_util::write_u32(
        interconnect_,
        address_map::ACCELERATOR_BASE
            + register_mapping::accelerator_register::STRIDE,
        1
    );

    test_util::write_u32(
        interconnect_,
        address_map::ACCELERATOR_BASE
            + register_mapping::accelerator_register::MAC_UNITS,
        4
    );


    /*
     * ------------------------------------------------
     * Step 5:
     * Start accelerator through MMIO.
     * ------------------------------------------------
     */
    test_util::write_u32(
        interconnect_,
        address_map::ACCELERATOR_BASE
            + register_mapping::accelerator_register::CONTROL,
        register_mapping::accelerator_register::START
    );


    /*
     * ------------------------------------------------
     * Step 6:
     * CPU waits while accelerator computes.
     * ------------------------------------------------
     */
    wait(
        accelerator_.completion_event()
    );


    /*
     * Verify accelerator status.
     */
    const std::uint32_t accelerator_status =
        test_util::read_u32(
            interconnect_,
            address_map::ACCELERATOR_BASE
                + register_mapping::accelerator_register::STATUS
        );

    if ((accelerator_status &
         register_mapping::accelerator_register::STATUS_ERROR) != 0U) {

        throw std::runtime_error{
            "Accelerator reported ERROR"
        };
    }

    if ((accelerator_status &
         register_mapping::accelerator_register::STATUS_DONE) == 0U) {

        throw std::runtime_error{
            "Accelerator did not set DONE"
        };
    }

    if ((accelerator_status &
         register_mapping::accelerator_register::STATUS_BUSY) != 0U) {

        throw std::runtime_error{
            "Accelerator remained BUSY after completion"
        };
    }


    /*
     * ------------------------------------------------
     * Step 7:
     * Verify accelerator-generated dimensions.
     * ------------------------------------------------
     */
    const std::uint32_t output_height =
        test_util::read_u32(
            interconnect_,
            address_map::ACCELERATOR_BASE
                + register_mapping::accelerator_register::OUTPUT_HEIGHT
        );

    const std::uint32_t output_width =
        test_util::read_u32(
            interconnect_,
            address_map::ACCELERATOR_BASE
                + register_mapping::accelerator_register::OUTPUT_WIDTH
        );

    if (output_height != 2U ||
        output_width != 2U) {

        throw std::runtime_error{
            "Incorrect accelerator output dimensions"
        };
    }


    /*
     * ------------------------------------------------
     * Step 8:
     * DMA transfers result back:
     *
     * Accelerator output
     *      -> Interconnect
     *      -> Main memory
     * ------------------------------------------------
     */
    dma_transfer(
        accelerator_output_address,
        output_memory_address,
        expected.size() * sizeof(std::int32_t)
    );


    /*
     * ------------------------------------------------
     * Step 9:
     * CPU/test driver reads final result from memory.
     * ------------------------------------------------
     */
    const auto output =
        test_util::read_i32_buffer(
            interconnect_,
            output_memory_address,
            expected.size()
        );


    /*
     * ------------------------------------------------
     * Step 10:
     * Verify final end-to-end result.
     * ------------------------------------------------
     */
    if (output != expected) {
        std::cerr << "Expected: ";

        for (const auto value : expected) {
            std::cerr << value << ' ';
        }

        std::cerr << "\nActual:   ";

        for (const auto value : output) {
            std::cerr << value << ' ';
        }

        std::cerr << '\n';

        throw std::runtime_error{
            "End-to-end convolution output mismatch"
        };
    }


    /*
     * Optional performance-model verification.
     */
    const std::uint32_t cycles_low =
        test_util::read_u32(
            interconnect_,
            address_map::ACCELERATOR_BASE
                + register_mapping::accelerator_register::CYCLES_LOW
        );

    const std::uint32_t cycles_high =
        test_util::read_u32(
            interconnect_,
            address_map::ACCELERATOR_BASE
                + register_mapping::accelerator_register::CYCLES_HIGH
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
            "Incorrect accelerator compute cycle count"
        };
    }


    std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << "test_end_to_end_convolution PASSED\n";   
}};

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

        DMA dma{
            "dma",
            interconnect,
            sc_core::sc_time{1, sc_core::SC_NS}
        };

        ConvolutionAccelerator accelerator{
            "accelerator",
            sc_core::sc_time{5, sc_core::SC_NS},
            sc_core::sc_time{1, sc_core::SC_NS},
            64,
            36,
            16
        };

        /*
         * System address map.
         */
        interconnect.map_target(
            address_map::MEMORY_BASE,
            address_map::MEMORY_SIZE,
            memory,
            "main_memory"
        );

        interconnect.map_target(
            address_map::DMA_BASE,
            address_map::DMA_SIZE,
            dma,
            "dma"
        );

        interconnect.map_target(
            address_map::ACCELERATOR_BASE,
            address_map::ACCELERATOR_SIZE,
            accelerator,
            "accelerator"
        );

        SoCTest test{
            "soc_test",
            interconnect,
            dma,
            accelerator
        };

        sc_core::sc_start();

        std::cout
            << "SoC integration test PASSED\n";

        return 0;

    } catch (const std::exception& error) {
        std::cerr
            << "SoC integration test FAILED: "
            << error.what()
            << '\n';

        return 1;
    }
}