#include <systemc>

#include "address_mapping.h"
#include "dma.h"
#include "interconnect.h"
#include "memory.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace dma_register {

constexpr Address CONTROL = 0x00;
constexpr Address STATUS = 0x04;
constexpr Address SOURCE_LOW = 0x08;
constexpr Address SOURCE_HIGH = 0x0C;
constexpr Address DESTINATION_LOW = 0x10;
constexpr Address DESTINATION_HIGH = 0x14;
constexpr Address LENGTH_BYTES = 0x18;
constexpr Address BURST_BYTES = 0x1C;

constexpr std::uint32_t START = 1U << 0U;

constexpr std::uint32_t STATUS_BUSY = 1U << 0U;
constexpr std::uint32_t STATUS_DONE = 1U << 1U;
constexpr std::uint32_t STATUS_ERROR = 1U << 2U;

}  // namespace dma_register

void write_u32(
    Interconnect& interconnect,
    Address address,
    std::uint32_t value
) {
    const std::array<std::uint8_t, 4> bytes{
        static_cast<std::uint8_t>(value),
        static_cast<std::uint8_t>(value >> 8U),
        static_cast<std::uint8_t>(value >> 16U),
        static_cast<std::uint8_t>(value >> 24U)
    };

    interconnect.write(
        address,
        bytes.data(),
        bytes.size()
    );
}

std::uint32_t read_u32(
    Interconnect& interconnect,
    Address address
) {
    std::array<std::uint8_t, 4> bytes{};

    interconnect.read(
        address,
        bytes.data(),
        bytes.size()
    );

    return
        static_cast<std::uint32_t>(bytes[0])
        |
        (static_cast<std::uint32_t>(bytes[1]) << 8U)
        |
        (static_cast<std::uint32_t>(bytes[2]) << 16U)
        |
        (static_cast<std::uint32_t>(bytes[3]) << 24U);
}

class DmaTest : public sc_core::sc_module {
public:
    DmaTest(
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
    void run() {
        constexpr Address source_address = 0x20;
        constexpr Address destination_address = 0xA0;
        constexpr std::size_t transfer_size = 32;

        std::array<std::uint8_t, transfer_size> input{};

        for (std::size_t index = 0;
             index < input.size();
             ++index) {

            input[index] =
                static_cast<std::uint8_t>(index + 1);
        }

        /*
         * Initialize source memory.
         */
        interconnect_.write(
            source_address,
            input.data(),
            input.size()
        );

        /*
         * Configure DMA through MMIO.
         */
        write_u32(
            interconnect_,
            address_map::DMA_BASE
                + dma_register::SOURCE_LOW,
            static_cast<std::uint32_t>(source_address)
        );

        write_u32(
            interconnect_,
            address_map::DMA_BASE
                + dma_register::SOURCE_HIGH,
            0
        );

        write_u32(
            interconnect_,
            address_map::DMA_BASE
                + dma_register::DESTINATION_LOW,
            static_cast<std::uint32_t>(
                destination_address
            )
        );

        write_u32(
            interconnect_,
            address_map::DMA_BASE
                + dma_register::DESTINATION_HIGH,
            0
        );

        write_u32(
            interconnect_,
            address_map::DMA_BASE
                + dma_register::LENGTH_BYTES,
            static_cast<std::uint32_t>(transfer_size)
        );

        write_u32(
            interconnect_,
            address_map::DMA_BASE
                + dma_register::BURST_BYTES,
            static_cast<std::uint32_t>(8)
        );

        /*
         * Start DMA.
         */
        write_u32(
            interconnect_,
            address_map::DMA_BASE
                + dma_register::CONTROL,
            dma_register::START
        );

        /*
         * Wait until the DMA worker completes.
         */
        wait(dma_.completion_event());

        const std::uint32_t status =
            read_u32(
                interconnect_,
                address_map::DMA_BASE
                    + dma_register::STATUS
            );

        if ((status & dma_register::STATUS_ERROR) != 0U) {
            throw std::runtime_error(
                "DMA reported an error"
            );
        }

        if ((status & dma_register::STATUS_DONE) == 0U) {
            throw std::runtime_error(
                "DMA did not set DONE"
            );
        }

        std::array<std::uint8_t, transfer_size> output{};

        interconnect_.read(
            destination_address,
            output.data(),
            output.size()
        );

        if (output != input) {
            throw std::runtime_error(
                "DMA output does not match input"
            );
        }

        std::cout
            << "[" << sc_core::sc_time_stamp() << "] "
            << "DMA transfer test PASSED\n";

        sc_core::sc_stop();
    }

    Interconnect& interconnect_;
    DMA& dma_;
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

    DmaTest test{
        "dma_test",
        interconnect,
        dma
    };

    sc_core::sc_start();

    return 0;
}