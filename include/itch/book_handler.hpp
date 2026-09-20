//
// Created by samuel-gebhardt on 28.08.26.
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
#include "book.hpp"

namespace data_feed {

    struct HandlerStats {
        uint64_t malformed_messages = 0;
        uint64_t unknown_references = 0;
    };

    class BookHandler : public HandlerDefaults {
    public:
        explicit BookHandler(std::vector<std::uint8_t> watch) : watch_(std::move(watch)) {}

        void on_add_order(const std::span<const std::byte> body) {
            const auto order = parse_add_order(body);
            if (!order) [[unlikely]] {
                ++stats_.malformed_messages;
                return;
            }
            engine_.add_order(order->order_reference_number, order->locate,
                              order->side, order->price, order->shares);
        }
        void on_executed(const std::span<const std::byte> body) {
            const auto executed_order = parse_executed_order(body);
            engine_.reduce_order(executed_order.order_reference_number,
                    executed_order.executed_shares);
            }

        void on_cancel(const std::span<const std::byte> body) {
            const auto cancel_order = parse_order_cancel(body);
            engine_.reduce_order(cancel_order.order_reference_number,
                cancel_order.cancelled_shares);
        }

        void on_delete(const std::span<const std::byte> body) {
            const auto delete_order = parse_order_delete(body);
            engine_.delete_order(delete_order.order_reference_number);
        }

        void on_replace(const std::span<const std::byte> body) {
            const auto replace_order = parse_order_replace(body);
            engine_.replace_order(replace_order.old_order_reference_number,
                replace_order.new_order_reference_number, replace_order.price,
                replace_order.shares);
        }

        void on_trading_action(const std::span<const std::byte> body) {
            const auto system_event = parse_system_event(body);
            phase_ = system_event.event_code;
        }

        [[nodiscard]] std::vector<std::uint8_t> watch() const {
            return watch_;
        }

        [[nodiscard]] std::vector<char> state() const {
            return state_;
        }

        [[nodiscard]] char phase() const {
            return phase_;
        }

    private:

        static std::string_view trim(const Ticker& t) {
            std::size_t n = t.size();
            while (n > 0 && t[n - 1] == ' ') --n;
            return std::string_view(t.data(), n);
        }

        BookEngine engine_;
        std::vector<std::uint8_t> watch_;
        std::vector<char> state_ = std::vector<char>(65536, 'T');
        char phase_ = 'O';
        HandlerStats stats_;
    };
}

