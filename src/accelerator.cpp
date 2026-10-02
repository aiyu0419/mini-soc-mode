
#include "accelerator.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include "common/bytes_utils.h"

ConvolutionAccelerator::ConvolutionAccelerator(
    sc_core::sc_module_name name,
    sc_core::sc_time register_delay,
    sc_core::sc_time clock_period,
    std::size_t input_size,
    std::size_t weight_size,
    std::size_t output_size   
)
    : sc_core::sc_module{name},
      register_delay_{register_delay},
      clock_period_{clock_period},
      input_buffer_(input_size, 0),
      weight_buffer_(weight_size, 0),
      output_buffer_(output_size, 0)
      
       {
    
    if (input_size == 0U) {
        throw std::invalid_argument(
            "Input size must be greater than zero"
        );
    }

    if (output_size == 0U) {
        throw std::invalid_argument(
            "Output size must be greater than zero"
        );
    }

    if (weight_size == 0U) {
        throw std::invalid_argument(
            "Weight size must be greater than zero"
        );
    }
    if (register_delay < sc_core::SC_ZERO_TIME) {
        throw std::invalid_argument(
            "Register delay cannot be negative"
        );
    }
    if (clock_period < sc_core::SC_ZERO_TIME) {
        throw std::invalid_argument(
            "Clock period cannot be negative"
        );
    }
    SC_THREAD(worker);

    std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << this->name()
        << ": initialized, register delay="
        << register_delay_
        << ", clock period="
        << clock_period_
        << ", input size="
        << input_size
        << ", output size="
        << output_size
        << ", weight size="
        << weight_size
        <<", output size="
        << output_size
        << '\n';

}

void ConvolutionAccelerator::write(
    Address local_address,
    const std::uint8_t* source,
    std::size_t size
) {
    if (source == nullptr) {
        throw std::invalid_argument(
            "ConvolutionAccelerator write received a null source"
        );
    }
    if (size == 0U) {
        throw std::invalid_argument(
            "ConvolutionAccelerator write received a zero size"
        );
    }

    //write to the register or buffer based on the local address
    if (local_address  < REGISTER_END) {
        if (size != sizeof(std::uint32_t)) {
        throw std::invalid_argument(
            "ConvolutionAccelerator: registers require 32-bit accesses"
        );
        }
        if ((local_address % 4U) != 0U) {
        throw std::invalid_argument(
            "ConvolutionAccelerator: register access is not 32-bit aligned"
        );

        }
        //convert the 4 bytes of data into a 32-bit value, assuming little-endian byte order
        const std::uint32_t value = byte_utils::load_u32_le(source);

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
                start_compute();
            }
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name() << ": Control register WRITE completed"
                << '\n';

            break;
        case Register::InputHeight:
            input_height_ = value;
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name() << ": inputHeight register WRITE completed"
                << '\n';
            break;
        case Register::InputWidth:
            input_width_ = value;
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name() << ": inputWidth register WRITE completed"
                << '\n';
            break;
        case Register::WeightHeight:
            weight_height_ = value;
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name() << ": weightHeight register WRITE completed"
                << '\n';
            break;
        case Register::WeightWidth:
            weight_width_ = value;
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name() << ": weightWidth register WRITE completed"
                << '\n';
            break;
        case Register::Stride:
            stride_ = value;
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name() << ": stride register WRITE completed"
                << '\n';
            break;
        case Register::MacUnits:
            mac_units_ = value;
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name() << ": macUnits register WRITE completed"
                << '\n';
            break;
       
        /*
         * These registers are read-only from the software side.
         */
        case Register::OutputHeight:
        case Register::OutputWidth:
        case Register::Status:
        case Register::ComputeCyclesLow:
        case Register::ComputeCyclesHigh:
            throw std::invalid_argument(
                "ConvolutionAccelerator: attempt to write to a read-only register"
            );
        default:
            throw std::out_of_range(
                "ConvolutionAccelerator: attempt to write to an unknown register"
            );
        }

        return;
    }
    // If the local address is beyond the register space, treat it as a buffer write
    write_buffer(
        local_address,
        source,
        size
    );

}

void ConvolutionAccelerator::read(
    Address local_address,
    std::uint8_t* destination,
    std::size_t size
) {
    if (destination == nullptr) {
        throw std::invalid_argument(
            "ConvolutionAccelerator read received a null destination"
        );
    }
    if (size == 0U) {
        throw std::invalid_argument(
            "ConvolutionAccelerator read received a zero size"
        );
    }

    //read from the register or buffer based on the local address
    if (local_address < REGISTER_END) {
        if (size != sizeof(std::uint32_t)) {
            throw std::invalid_argument(
                "ConvolutionAccelerator: registers require 32-bit accesses"
            );
        }
        if ((local_address % 4U) != 0U) {
            throw std::invalid_argument(
                "ConvolutionAccelerator: register access is not 32-bit aligned"
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
        case Register::InputHeight:
            value = input_height_;
            break;
        case Register::InputWidth:
            value = input_width_;
            break;
        case Register::WeightHeight:
            value = weight_height_;
            break;
        case Register::WeightWidth:
            value = weight_width_;
            break;
        case Register::Stride:
            value = stride_;
            break;
        case Register::MacUnits:
            value = mac_units_;
            break;
        case Register::OutputHeight:
            value = output_height_;
            break;
        case Register::OutputWidth:
            value = output_width_;
            break;
        case Register::ComputeCyclesLow:
            value = static_cast<std::uint32_t>(compute_cycles_ & 0xFFFFFFFF);
            break;
        case Register::ComputeCyclesHigh:
            value = static_cast<std::uint32_t>((compute_cycles_ >> 32U) & 0xFFFFFFFF);
            break;
        case Register::Control:
            value = 0;
            break;
        default:
            throw std::out_of_range(
                "ConvolutionAccelerator: attempt to read from an unknown register"
            );
        }
        byte_utils::store_u32_le(value, destination);

        std::cout
        << "[" << sc_core::sc_time_stamp() << "] "
        << name()
        << ": register READ Completed"
        << " offset=0x" << std::hex << local_address
        << " value=0x" << value
        << std::dec << '\n';
        return;
        }
    read_buffer(
        local_address,
        destination,
        size
    );    

    }

const sc_core::sc_event&
ConvolutionAccelerator::completion_event() const noexcept {
    return completion_event_;
}

void ConvolutionAccelerator::start_compute() {
    if (busy_) {
        error_ = true;
        std::cerr
            << "[" << sc_core::sc_time_stamp() << "] "
            << name()
            << ": START ignored because DMA is busy\n";
        return;
    }

    /*
     * start_compute() itself does not perform convolution.
     *
     * It only wakes the accelerator worker process.
     *
     * SC_ZERO_TIME means:
     *
     *     schedule worker in the next delta cycle,
     *     without advancing simulated hardware time.
    */
    start_event_.notify(sc_core::SC_ZERO_TIME);
}

void ConvolutionAccelerator::worker() {
    while (true) {
        wait(start_event_);

        busy_ = true;
        done_ = false;
        error_ = false;

        try {
            validate_configuration();
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name()
                << ": convolution started\n";

            output_height_ = (input_height_ - weight_height_) / stride_ + 1U;
            output_width_ = (input_width_ - weight_width_) / stride_ + 1U;

            compute_cycles_ = compute_cycles();

            /*
             * This is the accelerator's timing model.
             *
             * The SystemC process is suspended while simulated hardware
             * executes.
             */
            wait(
                clock_period_
                * static_cast<double>(compute_cycles_)
            );
            /*
             * Functional model.
             *
             * We calculate the actual convolution result after the
             * simulated execution latency has elapsed.
             */
            perform_convolution();
            
            std::cout
                << "[" << sc_core::sc_time_stamp() << "] "
                << name()
                << ": convolution completed\n";

            done_ = true;
            busy_ = false;
            completion_event_.notify(sc_core::SC_ZERO_TIME);

        } catch (...) {
            error_ = true;
            done_ = false;
            busy_ = false;
            std::cerr
                << "[" << sc_core::sc_time_stamp() << "] "
                << name()
                << ": unknown accelerator error\n";
            completion_event_.notify(sc_core::SC_ZERO_TIME);
            throw;
        }

    }
}


void ConvolutionAccelerator::validate_configuration() const {
    if (input_height_==0U || input_width_==0U) {
        throw std::invalid_argument(
            "CONVOLUTION: Input dimensions must be greater than zero"
        );
    }

    if (weight_height_==0U || weight_width_==0U) {
        throw std::invalid_argument(
            "CONVOLUTION: Weight dimensions must be greater than zero"
        );
    }

    if (stride_==0U) {
        throw std::invalid_argument(
            "CONVOLUTION: Stride must be greater than zero"
        );
    }

    if (mac_units_==0U) {
        throw std::invalid_argument(
            "CONVOLUTION: MAC units must be greater than zero"
        );
    }

    if (input_height_ < weight_height_) {
        throw std::invalid_argument(
            "CONVOLUTION: Input height must be greater than or equal to weight height"
        );
    }

    if (input_width_ < weight_width_) {
        throw std::invalid_argument(
            "CONVOLUTION: Input width must be greater than or equal to weight width"
        );
    }
    const std::uint64_t input_elements = 
    static_cast<std::uint64_t>(input_height_) * input_width_;
    const std::uint64_t weight_elements =
    static_cast<std::uint64_t>(weight_height_) * weight_width_;

    const std::uint64_t output_height = static_cast<std::uint64_t>(input_height_ - weight_height_) / stride_ + 1U;
    const std::uint64_t output_width = static_cast<std::uint64_t>(input_width_ - weight_width_) / stride_ + 1U;
    const std::uint64_t output_elements =
    output_height * output_width;

    /*
     * This implementation assumes:
     *
     * input  = int32_t
     * weight = int32_t
     * output = int32_t
     */
    const std::uint64_t input_bytes = input_elements * sizeof(std::uint32_t);
    const std::uint64_t weight_bytes = weight_elements * sizeof(std::uint32_t);
    const std::uint64_t output_bytes = output_elements * sizeof(std::uint32_t);


if (input_bytes > input_buffer_.size()) {
        throw std::out_of_range(
            "ConvolutionAccelerator: input exceeds buffer capacity"
        );
    }   
if (weight_bytes > weight_buffer_.size()) {
        throw std::out_of_range(
            "ConvolutionAccelerator: weights exceed buffer capacity"
        );
    }
if (output_bytes > output_buffer_.size()) {
        throw std::out_of_range(
            "ConvolutionAccelerator: output exceeds buffer capacity"
        );
    }
}

std::uint64_t ConvolutionAccelerator::compute_cycles() const noexcept{
    std::uint64_t output_elements =
    static_cast <std::uint64_t> (output_height_) * output_width_;

    const std::uint64_t macs_per_output =
    static_cast<std::uint64_t>(weight_height_)* weight_width_;

    const std::uint64_t total_macs =
    output_elements * macs_per_output;

    /*
     * Assume every MAC unit can perform one multiply-accumulate
     * operation per accelerator clock cycle.
     *
     * cycles = ceil(total MAC operations / number of MAC units)
     */
    return (
        total_macs
        + static_cast<std::uint64_t>(mac_units_)
        - 1ULL
    ) / static_cast<std::uint64_t>(mac_units_);
}

void ConvolutionAccelerator::perform_convolution(){

    std::fill(output_buffer_.begin(), output_buffer_.end(),0);
    //(output_y, output_x) is coordinate of 2D output
    for (std::uint32_t output_y = 0; output_y < output_height_; ++output_y){ 
        for(std::uint32_t output_x = 0; output_x < output_width_; ++output_x){
            std::int64_t accumulator = 0;
            for (std::uint32_t weight_y = 0; weight_y < weight_height_; ++weight_y){
                for (std::uint32_t weight_x = 0; weight_x < weight_width_; ++weight_x){
                    //(input_y, input_x) is coordinate of input
                    std::uint32_t input_y = output_y * stride_ + weight_y;
                    std::uint32_t input_x = output_x * stride_ + weight_x;
                    //convert 2D coordinate into 1D buffer index
                    std::size_t input_index = input_y * input_width_ + input_x;
                    std::size_t weight_index = weight_y * weight_width_ + weight_x;
                    const auto input_value = byte_utils::load_i32_le(&input_buffer_[
                                input_index
                                * sizeof(std::int32_t)]);
                    const auto weight_value = byte_utils::load_i32_le(&weight_buffer_[
                                weight_index
                                * sizeof(std::int32_t)]);
                    accumulator += static_cast<std::int64_t>(input_value) * static_cast<std::int64_t>(weight_value);


                }
            }
            /*
             * Saturate to int32_t so overflow behavior is explicit
             * instead of implementation-dependent.
             */
            accumulator = std::clamp(
                accumulator,
                static_cast<std::int64_t>(
                    std::numeric_limits<std::int32_t>::min()
                ),
                static_cast<std::int64_t>(
                    std::numeric_limits<std::int32_t>::max()
                )
            );
            const std::size_t output_index =
                static_cast<std::size_t>(output_y)
                * output_width_
                + output_x;
            byte_utils::store_i32_le(
                static_cast<std::int32_t>(accumulator),
                &output_buffer_[output_index * sizeof(std::int32_t)]
            );

        }
    }

}

std::uint32_t
ConvolutionAccelerator::status_value() const noexcept {
    std::uint32_t value = 0;

    if (busy_) {
        value |= STATUS_BUSY;
    }

    if (done_) {
        value |= STATUS_DONE;
    }

    if (error_) {
        value |= STATUS_ERROR;
    }

    return value;
}

void ConvolutionAccelerator::write_buffer(
    Address local_address,
    const std::uint8_t* source,
    std::size_t size_bytes
) {
    /*
     * Input buffer
     */
    if (local_address >= INPUT_BUFFER_BASE &&
        local_address < WEIGHT_BUFFER_BASE) {

        const std::size_t offset =
            static_cast<std::size_t>(
                local_address - INPUT_BUFFER_BASE
            );

        if (offset > input_buffer_.size() ||
            size_bytes > input_buffer_.size() - offset) {

            throw std::out_of_range(
                "ConvolutionAccelerator: input buffer write out of range"
            );
        }

        std::memcpy(
            input_buffer_.data() + offset,
            source,
            size_bytes
        );

        return;
    }
    /*
     * Weight buffer
     */
    if (local_address >= WEIGHT_BUFFER_BASE &&
        local_address < OUTPUT_BUFFER_BASE) {

        const std::size_t offset =
            static_cast<std::size_t>(
                local_address - WEIGHT_BUFFER_BASE
            );

        if (offset > weight_buffer_.size() ||
            size_bytes > weight_buffer_.size() - offset) {

            throw std::out_of_range(
                "ConvolutionAccelerator: weight buffer write out of range"
            );
        }

        std::memcpy(
            weight_buffer_.data() + offset,
            source,
            size_bytes
        );

        return;
    }

    /*
     * Output buffer can be considered accelerator-owned.
     * I would not allow normal software/DMA writes to it.
     */
    if (local_address >= OUTPUT_BUFFER_BASE) {
        throw std::runtime_error(
            "ConvolutionAccelerator: output buffer is read-only"
        );
    }

    throw std::out_of_range(
        "ConvolutionAccelerator: invalid buffer write address"
    );
}

void ConvolutionAccelerator::read_buffer(
    Address local_address,
    std::uint8_t* destination,
    std::size_t size_bytes
) {
    if (local_address >= INPUT_BUFFER_BASE &&
        local_address < WEIGHT_BUFFER_BASE) {

        const std::size_t offset =
            static_cast<std::size_t>(
                local_address - INPUT_BUFFER_BASE
            );

        if (offset > input_buffer_.size() ||
            size_bytes > input_buffer_.size() - offset) {

            throw std::out_of_range(
                "ConvolutionAccelerator: input buffer read out of range"
            );
        }

        std::memcpy(
            destination,
            input_buffer_.data() + offset,
            size_bytes
        );

        return;
    }

    if (local_address >= WEIGHT_BUFFER_BASE &&
        local_address < OUTPUT_BUFFER_BASE) {

        const std::size_t offset =
            static_cast<std::size_t>(
                local_address - WEIGHT_BUFFER_BASE
            );

        if (offset > weight_buffer_.size() ||
            size_bytes > weight_buffer_.size() - offset) {

            throw std::out_of_range(
                "ConvolutionAccelerator: weight buffer read out of range"
            );
        }

        std::memcpy(
            destination,
            weight_buffer_.data() + offset,
            size_bytes
        );

        return;
    }

    if (local_address >= OUTPUT_BUFFER_BASE) {
        const std::size_t offset =
            static_cast<std::size_t>(
                local_address - OUTPUT_BUFFER_BASE
            );

        if (offset > output_buffer_.size() ||
            size_bytes > output_buffer_.size() - offset) {

            throw std::out_of_range(
                "ConvolutionAccelerator: output buffer read out of range"
            );
        }

        std::memcpy(
            destination,
            output_buffer_.data() + offset,
            size_bytes
        );

        return;
    }

    throw std::out_of_range(
        "ConvolutionAccelerator: invalid buffer read address"
    );
}