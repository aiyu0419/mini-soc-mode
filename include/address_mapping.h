#pragma once

#include "target.h"

#include <cstddef>

namespace address_map {
//For the current test, memory is only 256 bytes
//Later we can increase it.
constexpr Address MEMORY_BASE = 0x00000000;
constexpr std::size_t MEMORY_SIZE = 0x00000100;

constexpr Address DMA_BASE = 0x10000000;
constexpr std::size_t DMA_SIZE = 0x00000100;

constexpr Address ACCELERATOR_BASE = 0x20000000;
constexpr std::size_t ACCELERATOR_SIZE = 0x00000100;

constexpr Address INTERRUPT_BASE = 0x30000000;
constexpr std::size_t INTERRUPT_SIZE = 0x00000100;

}  // namespace address_map