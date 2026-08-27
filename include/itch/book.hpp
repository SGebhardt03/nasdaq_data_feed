//
// Created by samuel-gebhardt on 27.08.26.
//
#pragma once


#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <messages.hpp>

struct Order {
    int64_t price;
    uint32_t shares;
    uint16_t locate;
    data_feed::Side side;
};

struct Level {
    int32_t  price_ticks{};
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
        auto it = bids.begin();
        return Level{it->first, it->second};
    }
    std::optional<Level> best_ask() const {
        if (asks.empty()) return std::nullopt;
        auto it = asks.begin();
        return Level{it->first, it->second};
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
    uint64_t locate_mismatch    = 0;
    uint64_t underflow_clamped  = 0; // reduce by more shares than quoted
    uint64_t level_missing      = 0; // order exists, price level does not
};

class BookEngine {
public:
    const BookStats& stats() const { return stats_; }

    void add_order(const uint64_t ref, const uint16_t locate, const data_feed::Side side,
                   const int64_t price, const uint32_t shares) {
        if (auto [it, inserted] = orders_.try_emplace(ref, Order{.price = price, .shares = shares, .locate = locate, .side = side}); !inserted) {
            ++stats_.duplicate_ref; return;
        }
        auto& book = books_[locate];

        if (side == data_feed::Side::Buy) {
            book.bids[price] += shares;
        }
        else {
            book.asks[price] += shares;
        }
    }
    void reduce_order(uint64_t ref, uint32_t shares);   // E, C, X
    void delete_order(uint64_t ref);                    // D
    void replace_order(uint64_t old_ref, uint64_t new_ref,
                       int32_t new_price, uint32_t new_shares); // U
private:
    std::unordered_map<uint64_t, Order> orders_;
    std::vector<Book> books_;
    BookStats stats_;
};