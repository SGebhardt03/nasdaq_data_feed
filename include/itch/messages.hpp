//
// Created by samuel-gebhardt on 22.08.26.
//
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>

#include "endian.hpp"
// #include "data_feed/endian.hpp"

namespace data_feed {

    // ---------------------------------------------------------------------------
    // Common header of all messages
    //
    //   Offset 0 : Message Type      1 byte  (ASCII)
    //   Offset 1 : Stock Locate      2 byte
    //   Offset 3 : Tracking Number   2 byte
    //   Offset 5 : Timestamp         6 byte  (ns since midnight)
    //   Offset 11: Payload
    // ---------------------------------------------------------------------------

    inline constexpr std::size_t kPayloadOffset = 11;

    struct MessageHeader {
        std::uint64_t timestamp;
        std::uint16_t locate;
        std::uint16_t tracking;
        char          type;
    };

    inline MessageHeader parse_header(std::span<const std::byte> b) {
        return MessageHeader{
            .timestamp = df::read_be48(b.subspan(5, 6)),
            .locate    = df::read_be<std::uint16_t>(b.subspan(1, 2)),
            .tracking  = df::read_be<std::uint16_t>(b.subspan(3, 2)),
            .type      = static_cast<char>(b[0]),
        };
    }

    using Ticker = std::array<char, 8>;   // right space padding
    using Mpid = std::array<char, 4>;

    inline Ticker read_ticker(std::span<const std::byte> b, std::size_t off) {
        Ticker t{};
        std::memcpy(t.data(), b.data() + off, t.size());
        return t;
    }

    // ---------------------------------------------------------------------------
    // 'R' — Stock Directory
    // ---------------------------------------------------------------------------

    struct StockDirectory {
        std::uint64_t timestamp;
        std::uint32_t round_lot_size;
        std::uint16_t locate;
        std::uint16_t tracking;
        Ticker        stock;
        char          market_category;
        char          financial_status;
        char          round_lots_only;
    };

    // Parsing only until byte 25
    inline StockDirectory parse_stock_directory(std::span<const std::byte> b) {
        const auto h = parse_header(b);
        return StockDirectory{
            .timestamp        = h.timestamp,
            .round_lot_size   = df::read_be<std::uint32_t>(b.subspan(21, 4)),
            .locate           = h.locate,
            .tracking         = h.tracking,
            .stock            = read_ticker(b, kPayloadOffset),
            .market_category  = static_cast<char>(b[19]),
            .financial_status = static_cast<char>(b[20]),
            .round_lots_only  = static_cast<char>(b[25]),
        };
    }

    struct SystemEvent {
        std::uint64_t timestamp;
        std::uint16_t locate; // Always 0
        std::uint16_t tracking;
        char          event_code;
    };

    inline SystemEvent parse_system_event(std::span<const std::byte> b) {
        const auto h = parse_header(b);
        return SystemEvent{
            .timestamp        = h.timestamp,
            .locate          = h.locate,
            .tracking        = h.tracking,
            .event_code      = static_cast<char>(b[11]),
        };
    }

    struct TradingAction {
        std::uint64_t           timestamp;
        std::uint16_t           locate;
        std::uint16_t           tracking;
        Ticker                  stock;
        char                    trading_state;
        char                    reserved;
        std::array<char, 4>     reason;

    };

    inline TradingAction parse_trading_action(std::span<const std::byte> b) {
        const auto h = parse_header(b);
        return TradingAction{
            .timestamp        = h.timestamp,
            .locate          = h.locate,
            .tracking        = h.tracking,
            .stock            = read_ticker(b, kPayloadOffset),
            .trading_state   = static_cast<char>(b[19]),
            .reserved          = static_cast<char>(b[20]),
            .reason = { static_cast<char>(b[21]), static_cast<char>(b[22]),
            static_cast<char>(b[23]), static_cast<char>(b[24]) },
        };
    }

    enum class Side : char { Buy = 'B', Sell = 'S' };

    struct AddOrder {
        std::uint64_t timestamp;
        std::uint16_t locate;
        std::uint16_t tracking;
        std::uint64_t order_reference_number;
        Side          side;
        std::uint32_t shares;
        Ticker        stock;
        std::int64_t  price;


    };

    constexpr std::optional<Side> to_side(std::byte b) noexcept {
        switch (static_cast<char>(b)) {
            case 'B': return Side::Buy;
            case 'S': return Side::Sell;
            default:  return std::nullopt;
        }
    }

    inline std::optional<AddOrder> parse_add_order(std::span<const std::byte> b) {
        const auto side = to_side(b[19]);
        if (!side) return std::nullopt;

        const auto h = parse_header(b);
        return AddOrder{
            .timestamp              = h.timestamp,
            .locate                 = h.locate,
            .tracking               = h.tracking,
            .order_reference_number = df::read_be<std::uint64_t>(b.subspan(11, 8)),
            .side                   = *side,
            .shares                 = df::read_be<std::uint32_t>(b.subspan(20, 4)),
            .stock                  = read_ticker(b, 24),
            .price                  = df::read_be<std::uint32_t>(b.subspan(32, 4)),
        };
    }

    struct AddOrderWithMpid {
        AddOrder base;
        Mpid     mpid;          // std::array<char, 4>
    };

    inline std::optional<AddOrderWithMpid> parse_add_order_with_mpid(std::span<const std::byte> b) {
        auto base = parse_add_order(b);
        if (!base) return std::nullopt;
        AddOrderWithMpid m{.base = *base, .mpid = {}};
        std::memcpy(m.mpid.data(), b.data() + 36, m.mpid.size());
        return m;
    }

    struct ExecutedOrder {
        std::uint64_t timestamp;
        std::uint16_t locate;
        std::uint16_t tracking;
        std::uint64_t order_reference_number;
        std::uint64_t executed_shares;
        std::uint64_t match_number;
    };

    inline ExecutedOrder parse_executed_order(std::span<const std::byte> b) {
        const auto h = parse_header(b);
        return ExecutedOrder{
            .timestamp        = h.timestamp,
            .locate           = h.locate,
            .tracking         = h.tracking,
            .order_reference_number = df::read_be<std::uint64_t>(b.subspan(11, 8)),
            .executed_shares   = df::read_be<std::uint32_t>(b.subspan(19, 4)),
            .match_number      = df::read_be<std::uint64_t>(b.subspan(23, 8)),
        };
    }

    enum class Printable : char { Yes = 'Y', No = 'N' };

    constexpr std::optional<Printable> to_printable(std::byte b) noexcept {
        switch (static_cast<char>(b)) {
            case 'Y': return Printable::Yes;
            case 'N': return Printable::No;
            default:  return std::nullopt;
        }
    }

    struct ExecutedOrderWithPrice {
        std::uint64_t timestamp;
        std::uint16_t locate;
        std::uint16_t tracking;
        std::uint64_t order_reference_number;
        std::uint64_t executed_shares;
        std::uint64_t match_number;
        Printable     printable;
        std::uint64_t price;
    };

    inline std::optional<ExecutedOrderWithPrice> parse_executed_order_with_price(std::span<const std::byte> b) {
        const auto printable = to_printable(b[31]);
        if (!printable) return std::nullopt;

        const auto h = parse_header(b);
        return ExecutedOrderWithPrice{
            .timestamp              = h.timestamp,
            .locate                 = h.locate,
            .tracking               = h.tracking,
            .order_reference_number = df::read_be<std::uint64_t>(b.subspan(11, 8)),
            .executed_shares        = df::read_be<std::uint32_t>(b.subspan(19, 4)),
            .match_number           = df::read_be<std::uint64_t>(b.subspan(23, 8)),
            .printable              = *printable,
            .price                  = df::read_be<std::uint32_t>(b.subspan(32, 4)),
        };
    }

    struct OrderCancel {
        std::uint64_t timestamp;
        std::uint16_t locate;
        std::uint16_t tracking;
        std::uint64_t order_reference_number;
        std::uint32_t cancelled_shares;
    };

    inline OrderCancel parse_order_cancel(std::span<const std::byte> b) {
        const auto h = parse_header(b);
        return OrderCancel{
            .timestamp              = h.timestamp,
            .locate                 = h.locate,
            .tracking               = h.tracking,
            .order_reference_number = df::read_be<std::uint64_t>(b.subspan(11, 8)),
            .cancelled_shares       = df::read_be<std::uint32_t>(b.subspan(19, 4)),
        };
    }

    struct OrderDelete {
        std::uint64_t timestamp;
        std::uint16_t locate;
        std::uint16_t tracking;
        std::uint64_t order_reference_number;
    };

    inline OrderDelete parse_order_delete(std::span<const std::byte> b) {
        const auto h = parse_header(b);
        return OrderDelete{
            .timestamp              = h.timestamp,
            .locate                 = h.locate,
            .tracking               = h.tracking,
            .order_reference_number = df::read_be<std::uint64_t>(b.subspan(11, 8)),
        };
    }

    struct OrderReplace {
        std::uint64_t timestamp;
        std::uint16_t locate;
        std::uint16_t tracking;
        std::uint64_t old_order_reference_number;
        std::uint64_t new_order_reference_number;
        std::uint64_t shares;
        std::uint64_t price;
    };

    inline OrderReplace parse_order_replace(std::span<const std::byte> b) {
        const auto h = parse_header(b);
        return OrderReplace{
            .timestamp                  = h.timestamp,
            .locate                     = h.locate,
            .tracking                   = h.tracking,
            .old_order_reference_number = df::read_be<std::uint64_t>(b.subspan(11, 8)),
            .new_order_reference_number = df::read_be<std::uint64_t>(b.subspan(19, 8)),
            .shares                     = df::read_be<std::uint32_t>(b.subspan(27, 4)),
            .price                      = df::read_be<std::uint32_t>(b.subspan(31, 4)),
        };
    }

    struct Trade {
        std::uint64_t timestamp;
        std::uint16_t locate;
        std::uint16_t tracking;
        std::uint64_t order_reference_number;
        std::uint64_t shares;
        Ticker        stock;
        std::uint64_t price;
        std::uint64_t match_number;
    };

    inline Trade parse_trade(std::span<const std::byte> b) {
        const auto h = parse_header(b);
        return Trade{
            .timestamp              = h.timestamp,
            .locate                 = h.locate,
            .tracking               = h.tracking,
            .order_reference_number = df::read_be<std::uint64_t>(b.subspan(11, 8)),
            .shares                 = df::read_be<std::uint32_t>(b.subspan(20, 4)),
            .stock                  = read_ticker(b, 24),
            .price                  = df::read_be<std::uint32_t>(b.subspan(32, 4)),
            .match_number           = df::read_be<std::uint64_t>(b.subspan(36, 8)),
        };
    }

    struct CrossTrade {
        std::uint64_t timestamp;
        std::uint16_t locate;
        std::uint16_t tracking;
        std::uint64_t shares;
        Ticker        stock;
        std::uint64_t cross_price;
        std::uint64_t match_number;
        char          cross_type;
    };

    inline CrossTrade parse_cross_ticker(std::span<const std::byte> b) {
        const auto h = parse_header(b);
        return CrossTrade{
            .timestamp              = h.timestamp,
            .locate                 = h.locate,
            .tracking               = h.tracking,
            .shares                 = df::read_be<std::uint64_t>(b.subspan(11, 8)),
            .stock                  = read_ticker(b, 19),
            .cross_price            = df::read_be<std::uint32_t>(b.subspan(27, 4)),
            .match_number           = df::read_be<std::uint64_t>(b.subspan(31, 8)),
            .cross_type             = static_cast<char>(b[39]),
        };
    }

}  // namespace data_feed