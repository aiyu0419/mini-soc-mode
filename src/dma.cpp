#include "dma.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

DMA::DMA(
    sc_core::sc_module_name name,
    Interconnect& interconnect,
    sc_core::sc_time register_delay
)
    : sc_core::sc_module{name},
      interconnect_{interconnect},
      register_delay_{register_delay} {

    if (register_delay < sc_core::SC_ZERO_TIME) {
        throw std::invalid_argument(
            "DMA register delay cannot be negative"
        );
    }

    SC_THREAD(worker);

    std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << this->name()
        << ": initialized, register delay="
        << register_delay_
        << '\n';
}

void DMA::write(
    Address local_address,
    const std::uint8_t* source,
    std::size_t size_bytes
) {
    if (source == nullptr) {
        throw std::invalid_argument(
            "DMA write received a null source"
        );
    }

    if (size_bytes != sizeof(std::uint32_t)) {
        throw std::invalid_argument(
            "DMA registers require 32-bit accesses"
        );
    }

    if ((local_address % 4U) != 0U) {
        throw std::invalid_argument(
            "DMA register access is not 32-bit aligned"
        );
    }

    //convert the 4 bytes of data into a 32-bit value, assuming little-endian byte order
    const std::uint32_t value = load_u32_le(source);

    std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << name()
        << ": register WRITE request"
        << " offset=0x" << std::hex << local_address
        << " value=0x" << value
        << std::dec << '\n';

    wait(register_delay_);

    switch (static_cast<Register>(local_address)) {
        case Register::Control:
            if ((value & CONTROL_CLEAR_DONE) != 0U) {
                done_ = false;
                error_ = false;
            }

            if ((value & CONTROL_START) != 0U) {
                start_transfer();
            }
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name() << ": Control register WRITE completed"
                << '\n';

            break;

        case Register::SourceLow:
            source_low_ = value;
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name() << ": Source Low register WRITE completed"
                << '\n';
            break;

        case Register::SourceHigh:
            source_high_ = value;
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name() << ": Source High register WRITE completed"
                << '\n';
            break;

        case Register::DestinationLow:
            destination_low_ = value;
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name() << ": Destination Low register WRITE completed"
                << '\n';
            break;

        case Register::DestinationHigh:
            destination_high_ = value;
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name() << ": Destination High register WRITE completed"
                << '\n';
            break;

        case Register::LengthBytes:
            length_bytes_ = value;
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name() << ": Length Bytes register WRITE completed"
                << '\n';
            break;

        case Register::BurstBytes:
            burst_bytes_ = value;
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name() << ": Burst Bytes register WRITE completed"
                << '\n';
            break;

        case Register::Status:
            throw std::invalid_argument(
                "DMA STATUS register is read-only"
            );

        default:
            throw std::out_of_range(
                "Unknown DMA register offset"
            );
    }
}

void DMA::read(
    Address local_address,
    std::uint8_t* destination,
    std::size_t size_bytes
) {
    if (destination == nullptr) {
        throw std::invalid_argument(
            "DMA read received a null destination"
        );
    }

    if (size_bytes != sizeof(std::uint32_t)) {
        throw std::invalid_argument(
            "DMA registers require 32-bit accesses"
        );
    }

    if ((local_address % 4U) != 0U) {
        throw std::invalid_argument(
            "DMA register access is not 32-bit aligned"
        );
    }
    std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << name()
        << ": register READ request"
        << " offset=0x" << std::hex << local_address
        << std::dec << '\n';

    wait(register_delay_);

    std::uint32_t value = 0;

    switch (static_cast<Register>(local_address)) {
        case Register::Status:
            value = status_value();
            break;

        case Register::SourceLow:
            value = source_low_;
            break;

        case Register::SourceHigh:
            value = source_high_; 
            break;

        case Register::DestinationLow:
            value = destination_low_;
            break;

        case Register::DestinationHigh:
            value = destination_high_;
            break;

        case Register::LengthBytes:
            value = length_bytes_;
            break;

        case Register::BurstBytes:
            value = burst_bytes_;
            break;

        case Register::Control:
            value = 0;
            break;

        default:
            throw std::out_of_range(
                "Unknown DMA register offset"
            );
    }
    //convert the 32-bit value into 4 bytes of data, assuming little-endian byte order
    store_u32_le(value, destination);

    std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << name()
        << ": register READ Completed"
        << " offset=0x" << std::hex << local_address
        << " value=0x" << value
        << std::dec << '\n';
}

void DMA::start_transfer() {
    if (busy_) {
        error_ = true;

        std::cerr
            << "[" << sc_core::sc_time_stamp() << "] "
            << name()
            << ": START ignored because DMA is busy\n";

        return;
    }

    done_ = false;
    error_ = false;

    /*
     * Schedule worker() for the next delta cycle.
     *
     * The CPU's register write can return before the DMA transfer
     * itself executes.
     */
    start_event_.notify(sc_core::SC_ZERO_TIME);
}

void DMA::worker() {
    while (true) {
        wait(start_event_);

        busy_ = true;
        done_ = false;
        error_ = false;

        try {
            const Address source = source_address();
            const Address destination = destination_address();

            if (length_bytes_ == 0U) {
                throw std::invalid_argument(
                    "DMA transfer length is zero"
                );
            }

            if (burst_bytes_ == 0U) {
                throw std::invalid_argument(
                    "DMA burst size is zero"
                );
            }

            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name()
                << ": transfer started"
                << " source=0x" << std::hex << source
                << " destination=0x" << destination
                << std::dec
                << " length=" << length_bytes_
                << " burst=" << burst_bytes_
                << '\n';

            std::vector<std::uint8_t> burst_buffer(
                burst_bytes_,
                0
            );

            std::size_t bytes_transferred = 0;

            while (bytes_transferred < length_bytes_) {
                const std::size_t remaining =
                    static_cast<std::size_t>(length_bytes_)
                    - bytes_transferred;

                const std::size_t current_burst =
                    std::min<std::size_t>(
                        burst_bytes_,
                        remaining
                    );

                const Address source_address =
                    source
                    + static_cast<Address>(bytes_transferred);

                const Address destination_address =
                    destination
                    + static_cast<Address>(bytes_transferred);

                interconnect_.read(
                    source_address,
                    burst_buffer.data(),
                    current_burst
                );

                interconnect_.write(
                    destination_address,
                    burst_buffer.data(),
                    current_burst
                );

                bytes_transferred += current_burst;

                std::cout
                    << "[" << sc_core::sc_time_stamp() << "] "
                    << name()
                    << ": transferred "
                    << bytes_transferred
                    << "/"
                    << length_bytes_
                    << " bytes\n";
            }

            done_ = true;

            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name()
                << ": transfer completed\n";

        } catch (const std::exception& error) {
            error_ = true;

            std::cerr
                << "[" << sc_core::sc_time_stamp() << "] "
                << name()
                << ": transfer failed: "
                << error.what()
                << '\n';
        }

        busy_ = false;
        completion_event_.notify(sc_core::SC_ZERO_TIME);
    }
}

std::uint32_t DMA::status_value() const noexcept {
    std::uint32_t status = 0;

    if (busy_) {
        status |= STATUS_BUSY;
    }

    if (done_) {
        status |= STATUS_DONE;
    }

    if (error_) {
        status |= STATUS_ERROR;
    }

    return status;
}

const sc_core::sc_event&
DMA::completion_event() const noexcept {
    return completion_event_;
}

Address DMA::source_address() const noexcept {
    const std::uint64_t address =
        (static_cast<std::uint64_t>(source_high_) << 32U)
        | source_low_;

    return static_cast<Address>(address);
}

Address DMA::destination_address() const noexcept {
    const std::uint64_t address =
        (static_cast<std::uint64_t>(destination_high_) << 32U)
        | destination_low_;

    return static_cast<Address>(address);
}

std::uint32_t DMA::load_u32_le(
    const std::uint8_t* data
) {
    return
        static_cast<std::uint32_t>(data[0])
        |
        (static_cast<std::uint32_t>(data[1]) << 8U)
        |
        (static_cast<std::uint32_t>(data[2]) << 16U)
        |
        (static_cast<std::uint32_t>(data[3]) << 24U);
}

void DMA::store_u32_le(
    std::uint32_t value,
    std::uint8_t* data
) {
    data[0] = static_cast<std::uint8_t>(value);
    data[1] = static_cast<std::uint8_t>(value >> 8U);
    data[2] = static_cast<std::uint8_t>(value >> 16U);
    data[3] = static_cast<std::uint8_t>(value >> 24U);
}