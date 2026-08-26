//
// Created by samuel-gebhardt on 26.08.26.
//
#include <gtest/gtest.h>

#include <array>
#include <cstddef>   // std::byte
#include <cstdint>

#include "../include/itch/stream_reader.hpp"
#include "../include/itch/messages.hpp"


TEST(BufferBoundary, withinMessage) {

    std::vector<std::byte> data = {
        // Message 1

        // Length 12
        std::byte{0x00}, std::byte{0x0C},

        std::byte{'S'},

        // Stock Locate = 0
        std::byte{0x00}, std::byte{0x00},

        // Tracking Number = 0
        std::byte{0x00}, std::byte{0x00},

        // Timestamp
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x09}, std::byte{0xCD},
        std::byte{0x0D}, std::byte{0x42},

        // Event Code = 'O'
        std::byte{'O'},

        // Message 2

        // Legnth 25
        std::byte{0x00}, std::byte{0x19},

        std::byte{'H'},

        // Stock Locate = 0
        std::byte{0x00}, std::byte{0x00},

        // Tracking Number = 0
        std::byte{0x00}, std::byte{0x00},

        // Timestamp
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x09}, std::byte{0xCD},
        std::byte{0x0D}, std::byte{0x42},

        // Stock
        std::byte{'A'}, std::byte{'A'},
        std::byte{'P'}, std::byte{'L'},
        std::byte{' '}, std::byte{' '},
        std::byte{' '}, std::byte{' '},

        // Trading State
        std::byte{'T'},

        // Reserved
        std::byte{'A'},

        // Reason
        std::byte{'L'}, std::byte{'U'},
        std::byte{'D'}, std::byte{'P'},
    };

    df::StreamReader reader = df::StreamReader{data,  28};

    auto msg = reader.next();
    data_feed::SystemEvent system_event =  data_feed::parse_system_event(msg);

    EXPECT_EQ(system_event.locate, 0);
    EXPECT_EQ(system_event.tracking, 0);
    EXPECT_EQ(system_event.timestamp, 164433218u);
    EXPECT_EQ(system_event.event_code, 'O');

    msg = reader.next();
    data_feed::TradingAction trading_action = data_feed::parse_trading_action(msg);

    std::array<char, 8> expected_stock = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '};
    std::array<char, 4> expected_reason = {'L', 'U', 'D', 'P'};

    EXPECT_EQ(trading_action.locate, 0);
    EXPECT_EQ(trading_action.tracking, 0);
    EXPECT_EQ(trading_action.timestamp, 164433218u);
    EXPECT_EQ(trading_action.stock, expected_stock);
    EXPECT_EQ(trading_action.trading_state, 'T');
    EXPECT_EQ(trading_action.reserved, 'A');
    EXPECT_EQ(trading_action.reason, expected_reason);

}