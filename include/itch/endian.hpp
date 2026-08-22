#pragma once

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace df {
    namespace detail {

        template <std::unsigned_integral T, std::size_t N>
        [[nodiscard]] constexpr T read_be_n(std::span<const std::byte> src,
                                            std::size_t offset) noexcept {
            static_assert(N <= sizeof(T));
            assert(offset + N <= src.size());
            T value = 0;
            for (std::size_t i = 0; i < N; ++i) {
                value = static_cast<T>(value << 8)
                      | static_cast<T>(std::to_integer<unsigned char>(src[offset + i]));
            }
            return value;
        }

    }  // namespace detail

    template <std::unsigned_integral T>
    [[nodiscard]] constexpr T read_be(std::span<const std::byte> src,
                                      std::size_t offset = 0) noexcept {
        return detail::read_be_n<T, sizeof(T)>(src, offset);
    }

    [[nodiscard]] constexpr std::uint64_t read_be48(std::span<const std::byte> src,
                                                    std::size_t offset = 0) noexcept {
        return detail::read_be_n<std::uint64_t, 6>(src, offset);
    }

}  // namespace df