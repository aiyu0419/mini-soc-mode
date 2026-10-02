#include <systemc>

#include "address_mapping.h"
#include "dma.h"
#include "interconnect.h"
#include "memory.h"
#include "common/bytes_utils.h"
#include "common/register_mapping.h"
#include "common/test_util.h"

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>



class DmaMemoryTest final : public sc_core::sc_module {
public:

    DmaMemoryTest(
        sc_core::sc_module_name name,
        Interconnect& interconnect,
        DMA& dma
    )
        : sc_core::sc_module{name},
          interconnect_{interconnect},
          dma_{dma} {

        SC_THREAD(run);
    }

private:
    Interconnect& interconnect_;
    DMA& dma_;

    void run() {
        test_memory_to_memory_transfer();

        sc_core::sc_stop();
    }


    void test_memory_to_memory_transfer() {
        constexpr Address source_address =
            address_map::MEMORY_BASE + 0x20;

        constexpr Address destination_address =
            address_map::MEMORY_BASE + 0xA0;

        constexpr std::size_t transfer_size = 32;
        constexpr std::uint32_t burst_size = 8;

        std::array<std::uint8_t, transfer_size> input{};

        for (std::size_t index = 0;
             index < input.size();
             ++index) {

            input[index] =
                static_cast<std::uint8_t>(index + 1);
        }

        /*
         * CPU/test driver initializes source memory.
         */
        interconnect_.write(
            source_address,
            input.data(),
            input.size()
        );

        /*
         * CPU/test driver configures DMA through MMIO.
         */
        test_util::write_u32(
            interconnect_,
            address_map::DMA_BASE
                + register_mapping::dma_register::SOURCE_LOW,
            static_cast<std::uint32_t>(
                source_address
            )
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
            static_cast<std::uint32_t>(
                destination_address
            )
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
            static_cast<std::uint32_t>(
                transfer_size
            )
        );

        test_util::write_u32(
            interconnect_,
            address_map::DMA_BASE
                + register_mapping::dma_register::BURST_BYTES,
            burst_size
        );

        /*
         * Start DMA.
         */
        test_util::write_u32(
            interconnect_,
            address_map::DMA_BASE
                + register_mapping::dma_register::CONTROL,
            register_mapping::dma_register::START
        );

        /*
         * Wait for asynchronous DMA worker.
         */
        wait(dma_.completion_event());

        /*
         * Verify DMA status.
         */
        const std::uint32_t status =
            test_util::read_u32(
                interconnect_,
                address_map::DMA_BASE
                    + register_mapping::dma_register::STATUS
            );

        if ((status & register_mapping::dma_register::STATUS_ERROR) != 0U) {
            throw std::runtime_error{
                "test_memory_to_memory_transfer: "
                "DMA reported ERROR"
            };
        }

        if ((status & register_mapping::dma_register::STATUS_DONE) == 0U) {
            throw std::runtime_error{
                "test_memory_to_memory_transfer: "
                "DMA did not set DONE"
            };
        }

        if ((status & register_mapping::dma_register::STATUS_BUSY) != 0U) {
            throw std::runtime_error{
                "test_memory_to_memory_transfer: "
                "DMA remained BUSY after completion"
            };
        }

        /*
         * Verify destination memory.
         */
        std::array<std::uint8_t, transfer_size> output{};

        interconnect_.read(
            destination_address,
            output.data(),
            output.size()
        );

        if (output != input) {
            throw std::runtime_error{
                "test_memory_to_memory_transfer: "
                "destination data does not match source data"
            };
        }

        std::cout
            << "[" << sc_core::sc_time_stamp() << "] "
            << "test_memory_to_memory_transfer PASSED\n";
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

        DMA dma{
            "dma",
            interconnect,
            sc_core::sc_time{1, sc_core::SC_NS}
        };

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

        DmaMemoryTest test{
            "dma_memory_test",
            interconnect,
            dma
        };

        sc_core::sc_start();

        std::cout
            << "All DMA-memory tests PASSED\n";

        return 0;

    } catch (const std::exception& error) {
        std::cerr
            << "DMA-memory test FAILED: "
            << error.what()
            << '\n';

        return 1;
    }
}