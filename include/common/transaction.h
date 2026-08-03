/*At first, this can be simplified transaction structure.
 Later, we may replace or supplement it with TLM-2.0 generic payloads.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

enum class Command {
    Read,
    Write
};

struct Transaction {
    Command command;
    std::uint64_t address;
    std::vector<std::uint8_t> data;
    std::size_t size_bytes;
};