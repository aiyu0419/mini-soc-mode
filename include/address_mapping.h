#pragma once

#include "target.h"

#include <cstddef>

namespace address_map {

//main memory occupies
//0x0000_0000 - 0x000F_FFFF
constexpr Address MEMORY_BASE = 0x00000000;
constexpr std::size_t MEMORY_SIZE = 0x00100000;// 1 MiB

// DMA MMIO
constexpr Address DMA_BASE = 0x10000000;
constexpr std::size_t DMA_SIZE = 0x00000100;

// ACCELERATOR MMIO and buffer space
constexpr Address ACCELERATOR_BASE = 0x20000000;
constexpr std::size_t ACCELERATOR_SIZE = 0x40000;

constexpr Address INTERRUPT_BASE = 0x30000000;
constexpr std::size_t INTERRUPT_SIZE = 0x00000100;

}  // namespace address_map