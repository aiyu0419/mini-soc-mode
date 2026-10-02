#pragma once
#include <cstdint>
#include "common/bytes_utils.h"
#include "common/type.h"

#include "interconnect.h"
#include <vector>

namespace test_util {
    inline void write_u32(
        Interconnect& interconnect,
        Address address,
        std::uint32_t value
    ) {
        std::uint8_t bytes[4];

        byte_utils::store_u32_le(
            value,
            bytes
        );

        interconnect.write(
            address,
            bytes,
            sizeof(bytes)
        );
    }

    inline std::uint32_t read_u32(
        Interconnect& interconnect,
        Address address
    ) {
        std::uint8_t bytes[4]{};

        interconnect.read(
            address,
            bytes,
            sizeof(bytes)
        );

        return byte_utils::load_u32_le(bytes);
    }

    inline void write_i32_buffer(
    Interconnect& interconnect,
    Address address,
    const std::vector<std::int32_t>& values
) {
    std::vector<std::uint8_t> bytes(
        values.size() * sizeof(std::int32_t)
    );

    for (std::size_t i = 0; i < values.size(); ++i) {
        byte_utils::store_i32_le(
            values[i],
            bytes.data()
                + i * sizeof(std::int32_t)
        );
    }

    interconnect.write(
        address,
        bytes.data(),
        bytes.size()
    );
}


    inline std::vector<std::int32_t> read_i32_buffer(
    Interconnect& interconnect,
    Address address,
    std::size_t count
) {
    std::vector<std::uint8_t> bytes(
        count * sizeof(std::int32_t)
    );

    interconnect.read(
        address,
        bytes.data(),
        bytes.size()
    );

    std::vector<std::int32_t> result(count);

    for (std::size_t i = 0; i < count; ++i) {
        result[i] =
            byte_utils::load_i32_le(
                bytes.data()
                    + i * sizeof(std::int32_t)
            );
    }

    return result;
}

}//namespace test_util