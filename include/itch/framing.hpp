//
// Created by samuel-gebhardt on 22.08.26.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "data_feed/endian.hpp"

namespace data_feed {
    template <class Handler>
    void for_each_message(std::span<const std::byte> file, Handler& h) {
        std::size_t off = 0;
        while (off + 2 <= file.size()) {
            const auto len = read_be<std::uint16_t>(file.subspan(off, 2));
            off += 2;
            if (len == 0 || off + len > file.size()) break;
            h.on_message(file.subspan(off, len));
            off += len;
        }
    }
}