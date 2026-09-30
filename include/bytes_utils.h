#pragma once

#include <cstdint>

namespace byte_utils {

inline std::uint32_t load_u32_le(
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


inline void store_u32_le(
    std::uint32_t value,
    std::uint8_t* data
) {
    data[0] = static_cast<std::uint8_t>(
        value & 0xFFU
    );

    data[1] = static_cast<std::uint8_t>(
        (value >> 8U) & 0xFFU
    );

    data[2] = static_cast<std::uint8_t>(
        (value >> 16U) & 0xFFU
    );

    data[3] = static_cast<std::uint8_t>(
        (value >> 24U) & 0xFFU
    );
}


inline std::int32_t load_i32_le(
    const std::uint8_t* data
) {
    return static_cast<std::int32_t>(
        load_u32_le(data)
    );
}


inline void store_i32_le(
    std::int32_t value,
    std::uint8_t* data
) {
    store_u32_le(
        static_cast<std::uint32_t>(value),
        data
    );
}

} // namespace byte_utils