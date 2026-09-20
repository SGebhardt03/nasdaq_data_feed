//
// Created by samuel-gebhardt on 22.08.26.
//

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "messages.hpp"
#include "dispatch.hpp"

namespace data_feed {

class StatsHandler : public HandlerDefaults {
public:
    explicit StatsHandler(std::vector<std::string> watchlist)
    : wanted_(std::move(watchlist)) {}

    [[nodiscard]] const std::array<std::uint64_t, 256>& counts() const { return counts_; }

    // locate -> Ticker, Index 0 unused (locates start at 1)
    // Empty ticker possible
    [[nodiscard]] const std::vector<Ticker>& directory() const { return locate_to_ticker_; }


    [[nodiscard]] std::size_t watched_count() const {
        std::size_t n = 0;
        for (auto w : watch_) n += w;
        return n;
    }

    void on_any(char type) {
        ++counts_[static_cast<unsigned char>(type)];
    }

    void on_stock_directory(std::span<const std::byte> body) {
        const auto sd = parse_stock_directory(body);
        if (sd.locate >= locate_to_ticker_.size())
            locate_to_ticker_.resize(sd.locate + 1);
        locate_to_ticker_[sd.locate] = sd.stock;
        watch_[sd.locate] = static_cast<std::uint8_t>(is_wanted(sd.stock));
    }

    void on_add_order(std::span<const std::byte> body) {
        const auto order = parse_add_order(body);
    }

    void on_executed(std::span<const std::byte> body) {
        const auto executed_order = parse_executed_order(body);
    }

    void on_cancel(std::span<const std::byte> body) {
        const auto cancel_order = parse_order_cancel(body);
    }

    void on_delete(std::span<const std::byte> body) {
        const auto delete_order = parse_order_delete(body);
    }

    void on_replace(std::span<const std::byte> body) {
        const auto replace_order = parse_order_replace(body);
    }

    [[nodiscard]] const std::array<std::uint8_t, 65536>& watch() const {
        return watch_;
    }

private:

    [[nodiscard]] bool is_wanted(const Ticker& t) const {
        const std::string_view sv = trim(t);
        for (const auto& w : wanted_)
            if (w == sv) return true;
        return false;
    }

    static std::string_view trim(const Ticker& t) {
        std::size_t n = t.size();
        while (n > 0 && t[n - 1] == ' ') --n;
        return std::string_view(t.data(), n);
    }

    std::array<std::uint64_t, 256>  counts_{};
    std::vector<Ticker>             locate_to_ticker_;   // Index = stock_locate
    std::array<std::uint8_t, 65536> watch_{};           // Index = stock_locate
    std::vector<std::string>        wanted_;
};

}  // namespace data_feed