#pragma once

#include <systemc>

#include "type.h"
#include "target.h"
#include <cstddef>
#include <cstdint>
#include <vector>


/*Design the first convolution accelerator as a single-layer, single-channel, blocking compute engine with local input, weight, and output buffers.
-The cpu can configure the accelerator by writing to its registers, which will trigger the accelerator to start a convolution operation.
-DMA can move data between the main memory and the accelerator's local buffers.
-The accelerator will read the input data and weights from its local buffers, perform the convolution operation, and write the output data back to its local buffer.
-The accelerator will then signal the CPU that the operation is complete by writing to a status register.
-compute latency changes with architectural parameters such as MAC parallelism, buffer sizes, and clock frequency. 
-The accelerator's performance can be evaluated by measuring the time taken to complete a convolution operation for a given input size and kernel size.
*/
class ConvolutionAccelerator final: public sc_core::sc_module, public Target {
public:
    ConvolutionAccelerator(
        sc_core::sc_module_name name,
        sc_core::sc_time register_delay,
        sc_core::sc_time clock_period,
        std::size_t input_size,
        std::size_t output_size,
        std::size_t weight_size
    );

//cpu write and read registers to configure the accelerator and check its status.
// The registers are memory-mapped to specific addresses in the accelerator's address space. 
void write(
    Address local_address, 
    const std::uint8_t* source, 
    std::size_t size 
) override;

 void read(
    Address local_address, 
    std::uint8_t* destination, 
    std::size_t size
) override;

[[nodiscard]]
const sc_core::sc_event& completion_event() const noexcept;

private:

enum class Register : Address {
    Control = 0x00,
    Status = 0x04,
    InputHeight= 0x08,
    InputWidth = 0x0C,
    WeightHeight = 0x10,
    WeightWidth = 0x14,
    Stride = 0x18,
    MacUnits = 0x1C,
    OutputHeight = 0x20,
    OutputWidth = 0x24,
    ComputeCyclesLow = 0x28,
    ComputeCyclesHigh = 0x2C
};

static constexpr Address REGISTER_END = 0x1000;
static constexpr Address INPUT_BUFFER_BASE = 0x1000;
static constexpr Address WEIGHT_BUFFER_BASE = 0x20000;
static constexpr Address OUTPUT_BUFFER_BASE = 0x30000;

static constexpr std::uint32_t CONTROL_START = 1U << 0U;
static constexpr std::uint32_t CONTROL_CLEAR_DONE = 1U << 1U;
static constexpr std::uint32_t STATUS_BUSY = 1U << 0U;
static constexpr std::uint32_t STATUS_DONE = 1U << 1U;  
static constexpr std::uint32_t STATUS_ERROR = 1U << 2U;

void worker();
void start_compute();
void validate_configuration() const;
void perform_convolution();

/*compute_cycles=roundup(output_width*output_height*kernel_width*kernel_height/mac_units)
- The compute_cycles() function calculates the total number of multiply-accumulate (MAC) operations required for the convolution and divides it by the number of MAC units to determine the number of cycles needed to complete the operation.
-The result is rounded up to the nearest integer to account for any remaining MAC operations that cannot be completed in a full cycle.
-The compute_cycles() function is marked as [[nodiscard]] to indicate that the return value should not be ignored, as it is an important metric for evaluating the performance of the accelerator.
-The function is also marked as const to indicate that it does not modify the state of the ConvolutionAccelerator object.
-The function is marked as noexcept to indicate that it does not throw any exceptions, which is important for performance-critical code.
-That's a performance model, not necessarily what real convolution hardware does. A real architecture may have pipeline fill/drain, memory stalls, tiling, SIMD lanes, PE arrays, accumulation latency, etc.
*/
[[nodiscard]]
std::uint64_t compute_cycles() const noexcept;

[[nodiscard]]
std::uint32_t status_value() const noexcept;

sc_core::sc_time register_delay_;
sc_core::sc_time clock_period_;
// Buffers for input, weight, and output data
std::vector<std::uint8_t> input_buffer_;
std::vector<std::uint8_t> weight_buffer_;
std::vector<std::uint8_t> output_buffer_;

void write_buffer(
    Address buffer_base,
    const std::uint8_t* source,
    std::size_t size_bytes
);
void read_buffer(
    Address buffer_base,
    std::uint8_t* destination,
    std::size_t size_bytes
);



std::uint32_t input_height_{0};
std::uint32_t input_width_{0};
std::uint32_t weight_height_{0};
std::uint32_t weight_width_{0};
std::uint32_t stride_{1};
std::uint32_t mac_units_{1};
std::uint32_t output_height_{0};
std::uint32_t output_width_{0}; 

std::uint64_t compute_cycles_{0};

bool busy_{false};
bool done_{false};
bool error_{false};

sc_core::sc_event start_event_;
sc_core::sc_event completion_event_;
};