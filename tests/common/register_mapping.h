#pragma once
#include <cstdint>
#include"common/type.h"
namespace register_mapping {
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

namespace accelerator_register {

constexpr Address CONTROL       = 0x00;
constexpr Address STATUS        = 0x04;
constexpr Address INPUT_HEIGHT  = 0x08;
constexpr Address INPUT_WIDTH   = 0x0C;
constexpr Address KERNEL_HEIGHT = 0x10;
constexpr Address KERNEL_WIDTH  = 0x14;
constexpr Address STRIDE        = 0x18;
constexpr Address MAC_UNITS     = 0x1C;
constexpr Address OUTPUT_HEIGHT = 0x20;
constexpr Address OUTPUT_WIDTH  = 0x24;
constexpr Address CYCLES_LOW    = 0x28;
constexpr Address CYCLES_HIGH   = 0x2C;

constexpr Address INPUT_BUFFER_BASE  = 0x1000;
constexpr Address WEIGHT_BUFFER_BASE = 0x20000;
constexpr Address OUTPUT_BUFFER_BASE = 0x30000;

constexpr std::uint32_t START = 1U << 0U;

constexpr std::uint32_t STATUS_BUSY  = 1U << 0U;
constexpr std::uint32_t STATUS_DONE  = 1U << 1U;
constexpr std::uint32_t STATUS_ERROR = 1U << 2U;

}  // namespace accelerator_register

}