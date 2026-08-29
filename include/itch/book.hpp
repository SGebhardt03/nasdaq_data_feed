//
// Created by samuel-gebhardt on 27.08.26.
//
#pragma once


#include <cstdint>
#include <map>
#include <functional>
#include <unordered_map>
#include <vector>
#include <optional>

#include <messages.hpp>



namespace data_feed {
    struct Order {
        int64_t price;
        uint32_t shares;
        uint16_t locate;
        Side side;
    };

    struct Level {
        int64_t  price_ticks{};
        uint32_t shares{};
    };

    struct Book {
        Book() = default;
        Book(const Book&) = delete;
        Book& operator=(const Book&) = delete;
        Book(Book&&) = default;
        Book& operator=(Book&&) = default;

        std::map<int64_t, uint32_t, std::greater<>> bids;
        std::map<int64_t, uint32_t, std::less<>> asks;

        std::optional<Level> best_bid() const {
            if (bids.empty()) return std::nullopt;
            const auto it = bids.begin();
            return Level{.price_ticks = it->first, .shares = it->second};
        }
        std::optional<Level> best_ask() const {
            if (asks.empty()) return std::nullopt;
            const auto it = asks.begin();
            return Level{.price_ticks = it->first, .shares = it->second};
        }
        bool empty() const { return bids.empty() && asks.empty(); }
    };


    struct BookDelta {
        bool top_changed;
        uint16_t locate;
    };

    struct BookStats {
        uint64_t duplicate_ref      = 0; // add ref on already existing ref
        uint64_t unknown_ref        = 0; // reduce, delete, replace on unknown ref
        uint64_t underflow_clamped  = 0; // reduce by more shares than quoted
        uint64_t level_missing      = 0; // order exists, price level does not
        uint64_t book_desync        = 0;
        uint64_t zero_shares        = 0;
    };

    class BookEngine {
    public:
        explicit BookEngine(size_t expected_orders = 1'000'000) : books_(1u << 16) {
            orders_.reserve(expected_orders);
        }

        const BookStats& stats() const { return stats_; }

        BookEngine(const BookEngine&) = delete;
        BookEngine& operator=(const BookEngine&) = delete;
        BookEngine(BookEngine&&) = default;
        BookEngine& operator=(BookEngine&&) = default;

        // Messages A, F
        void add_order(const uint64_t ref, const uint16_t locate, const Side side,
                       const int64_t price, const uint32_t shares) {

            if (shares == 0) [[unlikely]] { ++stats_.zero_shares; return; }

            if (auto [it, inserted] = orders_.try_emplace(ref, Order{.price = price, .shares = shares, .locate = locate, .side = side}); !inserted) {
                ++stats_.duplicate_ref;
                return;
            }
            auto& book = books_[locate];

            if (side == Side::Buy) {
                book.bids[price] += shares;
            }
            else {
                book.asks[price] += shares;
            }
        }

        // Messages X, E, C
        void reduce_order(const uint64_t ref, const uint64_t shares) {

            const auto it = orders_.find(ref);
            if (it == orders_.end()) {
                stats_.unknown_ref++;
                return;
            }

            Order& order = it->second;

            if (shares < order.shares) {
                order.shares -= shares;
            } else if (shares == order.shares) {
                remove_order(it);
                return;
            } else {
                stats_.underflow_clamped++;
                remove_order(it); // implicit clamping using order.shares
                return;
            }

            auto& book = books_[order.locate];
            if (order.side == Side::Buy) {
                decrement_level(book.bids, order.price, shares);
            } else {
                decrement_level(book.asks, order.price, shares);
            }

        }


        // Messages D
        void delete_order(const uint64_t ref) {
            const auto it = orders_.find(ref);
            if (it == orders_.end()) {
                stats_.unknown_ref++;
                return;
            }

            remove_order(it);
        }

        // Messages U
        void replace_order(const uint64_t old_ref, const uint64_t new_ref,
                           const int64_t new_price, const uint32_t new_shares) {

            const auto it = orders_.find(old_ref);
            if (it == orders_.end()) {
                stats_.unknown_ref++;
                return;
            }

            const Order& order = it->second;

            const auto side = order.side;
            const auto locate = order.locate;
            remove_order(it);
            add_order(new_ref, locate, side, new_price, new_shares);
        }
    private:
        std::unordered_map<uint64_t, Order> orders_;
        std::vector<Book> books_;
        BookStats stats_;

        template <typename Levels>
        void decrement_level(Levels& levels, int64_t price, uint32_t shares) {
            const auto lit = levels.find(price);
            if (lit == levels.end()) [[unlikely]] { ++stats_.level_missing; return; }
            if (lit->second < shares)  [[unlikely]] { ++stats_.book_desync; levels.erase(lit); return; }
            if (lit->second == shares) { levels.erase(lit); return; }
            lit->second -= shares;
        }

        void remove_order(const std::unordered_map<uint64_t, Order>::iterator it) {
            auto&[price, shares, locate, side] = it->second;

            auto& book = books_[locate];
            if (side == Side::Buy) {
                decrement_level(book.bids, price, shares);
            } else {
                decrement_level(book.asks, price, shares);
            }

            orders_.erase(it);
        }

    };
}