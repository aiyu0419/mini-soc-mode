#pragma once

#include <systemc>
#include "type.h"
#include "target.h"
#include "interconnect.h"

#include <cstdint>
#include <cstddef>
#include <vector>

//Build the first DMA as a single-channel, blocking, memory-to-memory DMA engine.
class DMA final: public sc_core::sc_module, public Target {
public:
     DMA(sc_core::sc_module_name name, 
        Interconnect& interconnect, 
        sc_core::sc_time register_delay);

    //allow cpu to write to the DMA registers, which will trigger the DMA engine to start a transfer.
    void write(
        Address local_address,
        const std::uint8_t* source,
        std::size_t size_bytes
    ) override;

    //allow cpu to read from the DMA registers, which will return the current status of the DMA engine.
    void read(
        Address local_address,
        std::uint8_t* destination,
        std::size_t size_bytes
    ) override;

    [[nodiscard]] const sc_core::sc_event& completion_event() const noexcept;

    private:
        enum class Register : Address {
            Control = 0x00,
            Status = 0x04,
            SourceLow = 0x08,
            SourceHigh = 0x0C,
            DestinationLow = 0x10,
            DestinationHigh = 0x14,
            LengthBytes = 0x18,
            BurstBytes = 0x1C,
        };
        static constexpr std::uint32_t CONTROL_START = 1U << 0U;
        static constexpr std::uint32_t CONTROL_CLEAR_DONE = 1U << 1U;

        static constexpr std::uint32_t STATUS_BUSY = 1U << 0U;
        static constexpr std::uint32_t STATUS_DONE = 1U << 1U;
        static constexpr std::uint32_t STATUS_ERROR = 1U << 2U;

        void worker();
        void start_transfer();

        [[nodiscard]] 
        std::uint32_t status_value() const noexcept;

        [[nodiscard]]
        Address source_address() const noexcept;

        [[nodiscard]]
        Address destination_address() const noexcept;

        [[nodiscard]]
        static std::uint32_t load_u32_le(const std::uint8_t* data);

        [[nodiscard]]
        static void store_u32_le(std::uint32_t value,std::uint8_t* data);

        bool busy_{false};
        bool done_{false};
        bool error_{false};

        std::uint32_t source_low_{0};
        std::uint32_t source_high_{0};

        std::uint32_t destination_low_{0};
        std::uint32_t destination_high_{0};

        std::uint32_t length_bytes_{0};
        std::uint32_t burst_bytes_{16};

        Interconnect& interconnect_;
        sc_core::sc_time register_delay_;   

        sc_core::sc_event completion_event_;
        sc_core::sc_event start_event_;


    };