//
// Created by samuel-gebhardt on 23.08.26.
//
#include <gtest/gtest.h>

#include <array>
#include <cstddef>   // std::byte
#include <cstdint>

#include "../include/itch/messages.hpp"

TEST(MessageParsing, SystemEvent) {
    // Message Type:    'S'          (1 byte)
    // Stock Locate:    0            (2 bytes, BE)
    // Tracking Number: 0            (2 bytes, BE)
    // Timestamp:       164433218    (6 bytes, BE)
    // Event Code:      'O'          (1 byte)  // "Start of Messages"

    std::array<std::byte, 12> buffer{
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
        std::byte{'O'}
    };

    auto msg = data_feed::parse_system_event(buffer);

    EXPECT_EQ(msg.locate, 0);
    EXPECT_EQ(msg.tracking, 0);
    EXPECT_EQ(msg.timestamp, 164433218u);
    EXPECT_EQ(msg.event_code, 'O');

}


TEST(MessageParsing, StockDirectory) {
    // Message Type:    'R'          (1 byte)
    // Stock Locate:    1            (2 bytes, BE)
    // Tracking Number: 0            (2 bytes, BE)
    // Timestamp:       1516151616   (6 bytes, BE)
    // Stock:           'AAPL'       (8 bytes, BE)
    // Market Category: Q            (1 byte)
    // FinStatInd:      'N'          (1 byte)
    // Round Lot Size:  100          (4 bytes, BE)
    // Round Lots Only: 'Y'          (1 byte)
    // Issue Class.:    'C'          (1 byte)
    // Issue Sub-Type:  'RT'         (2 bytes, BE)
    // Authenticity:    'P'          (1 byte)
    // IPO Flag:        'N'          (1 byte)
    //                  0             (7 byte)


    std::array<std::byte, 38> buffer{
        std::byte{'R'},

        // Stock Locate = 0
        std::byte{0x00}, std::byte{0x01},

        // Tracking Number = 0
        std::byte{0x00}, std::byte{0x00},

        // Timestamp
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x5A}, std::byte{0x5E},
        std::byte{0xA3}, std::byte{0x40},

        // Stock
        std::byte{'A'}, std::byte{'A'},
        std::byte{'P'}, std::byte{'L'},
        std::byte{' '}, std::byte{' '},
        std::byte{' '}, std::byte{' '},

        // Market Category
        std::byte{'Q'},

        // Financial Status Indicator
        std::byte{'R'},

        // Round Lot Size
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x64},

        // Round Lots Only
        std::byte{'Y'},

        // Issue Class
        std::byte{'C'},

        // Issue Sub-Type
        std::byte{'R'}, std::byte{'T'},

        // Authenticity
        std::byte{'P'},

        // IPO Flag
        std::byte{'N'},

        // trailing 0
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}
    };

    auto msg = data_feed::parse_stock_directory(buffer);


    std::array<char, 8> expected_stock = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '};

    EXPECT_EQ(msg.locate, 1);
    EXPECT_EQ(msg.tracking, 0);
    EXPECT_EQ(msg.timestamp, 1516151616u);
    EXPECT_EQ(msg.round_lot_size, 100);
    EXPECT_EQ(msg.stock, expected_stock);
    EXPECT_EQ(msg.market_category, 'Q');
    EXPECT_EQ(msg.financial_status, 'R');
    EXPECT_EQ(msg.round_lots_only, 'Y');

}


TEST(MessageParsing, TradingAction) {
    // Message Type:    'H'          (1 byte)
    // Stock Locate:    0            (2 bytes, BE)
    // Tracking Number: 0            (2 bytes, BE)
    // Timestamp:       164433218    (6 bytes, BE)
    // Stock:           "AAPL"       (8 bytes, BE)
    // Trading State:   'T'          (1 byte)
    // Reserved:        0            (1 byte)
    // Reason:          "LUDP"       (4 bytes, BE)

    std::array<std::byte, 25> buffer{
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

    auto msg = data_feed::parse_trading_action(buffer);


    std::array<char, 8> expected_stock = {'A', 'A', 'P', 'L', ' ', ' ', ' ', ' '};
    std::array<char, 4> expected_reason = {'L', 'U', 'D', 'P'};

    EXPECT_EQ(msg.locate, 0);
    EXPECT_EQ(msg.tracking, 0);
    EXPECT_EQ(msg.timestamp, 164433218u);
    EXPECT_EQ(msg.stock, expected_stock);
    EXPECT_EQ(msg.trading_state, 'T');
    EXPECT_EQ(msg.reserved, 'A');
    EXPECT_EQ(msg.reason, expected_reason);


}


TEST(MessageParsing, AddOrder) {
    // Message Type:            'A'          (1 byte)
    // Stock Locate:            5            (2 bytes, BE)
    // Tracking Number:         7            (2 bytes, BE)
    // Timestamp:               164433218    (6 bytes, BE)
    // Order Reference Number:  42           (8 bytes, BE)
    // Buy/Sell Indicator:      'B'          (1 byte)
    // Shares:                  100          (4 bytes, BE)
    // Stock:                   'GOOG'       (8 bytes, BE)
    // Price:                   123456       (4 bytes, BE)

    std::array<std::byte, 36> buffer{
        std::byte{'A'},

        // Stock Locate = 5
        std::byte{0x00}, std::byte{0x05},

        // Tracking Number = 7
        std::byte{0x00}, std::byte{0x07},

        // Timestamp
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x09}, std::byte{0xCD},
        std::byte{0x0D}, std::byte{0x42},

        // Order Reference Number = 42
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x2A},

        // Buy/Sell Indicator
        std::byte{'B'},

        // Shares = 100
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x64},

        // Stock
        std::byte{'G'}, std::byte{'O'},
        std::byte{'O'}, std::byte{'G'},
        std::byte{' '}, std::byte{' '},
        std::byte{' '}, std::byte{' '},

        // Price = 123456
        std::byte{0x00}, std::byte{0x01},
        std::byte{0xE2}, std::byte{0x40},
    };

    auto msg = data_feed::parse_add_order(buffer);
    ASSERT_TRUE(msg.has_value());

    std::array<char, 8> expected_stock = {'G', 'O', 'O', 'G', ' ', ' ', ' ', ' '};

    EXPECT_EQ(msg->locate, 5);
    EXPECT_EQ(msg->tracking, 7);
    EXPECT_EQ(msg->timestamp, 164433218u);
    EXPECT_EQ(msg->order_reference_number, 42u);
    EXPECT_EQ(msg->side, data_feed::Side::Buy);
    EXPECT_EQ(msg->shares, 100u);
    EXPECT_EQ(msg->stock, expected_stock);
    EXPECT_EQ(msg->price, 123456);
}


TEST(MessageParsing, AddOrderWithMpid) {
    // Same layout as AddOrder, plus:
    // MPID:                    'EDGX'       (4 bytes)

    std::array<std::byte, 40> buffer{
        std::byte{'F'},

        // Stock Locate = 5
        std::byte{0x00}, std::byte{0x05},

        // Tracking Number = 7
        std::byte{0x00}, std::byte{0x07},

        // Timestamp
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x09}, std::byte{0xCD},
        std::byte{0x0D}, std::byte{0x42},

        // Order Reference Number = 42
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x2A},

        // Buy/Sell Indicator
        std::byte{'B'},

        // Shares = 100
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x64},

        // Stock
        std::byte{'G'}, std::byte{'O'},
        std::byte{'O'}, std::byte{'G'},
        std::byte{' '}, std::byte{' '},
        std::byte{' '}, std::byte{' '},

        // Price = 123456
        std::byte{0x00}, std::byte{0x01},
        std::byte{0xE2}, std::byte{0x40},

        // MPID
        std::byte{'E'}, std::byte{'D'},
        std::byte{'G'}, std::byte{'X'},
    };

    auto msg = data_feed::parse_add_order_with_mpid(buffer);
    ASSERT_TRUE(msg.has_value());

    std::array<char, 8> expected_stock = {'G', 'O', 'O', 'G', ' ', ' ', ' ', ' '};
    std::array<char, 4> expected_mpid = {'E', 'D', 'G', 'X'};

    EXPECT_EQ(msg->base.locate, 5);
    EXPECT_EQ(msg->base.tracking, 7);
    EXPECT_EQ(msg->base.timestamp, 164433218u);
    EXPECT_EQ(msg->base.order_reference_number, 42u);
    EXPECT_EQ(msg->base.side, data_feed::Side::Buy);
    EXPECT_EQ(msg->base.shares, 100u);
    EXPECT_EQ(msg->base.stock, expected_stock);
    EXPECT_EQ(msg->base.price, 123456);
    EXPECT_EQ(msg->mpid, expected_mpid);
}


TEST(MessageParsing, ExecutedOrder) {
    // Message Type:            'E'          (1 byte)
    // Stock Locate:            5            (2 bytes, BE)
    // Tracking Number:         7            (2 bytes, BE)
    // Timestamp:               164433218    (6 bytes, BE)
    // Order Reference Number:  42           (8 bytes, BE)
    // Executed Shares:         500          (4 bytes, BE)
    // Match Number:            99999        (8 bytes, BE)

    std::array<std::byte, 31> buffer{
        std::byte{'E'},

        // Stock Locate = 5
        std::byte{0x00}, std::byte{0x05},

        // Tracking Number = 7
        std::byte{0x00}, std::byte{0x07},

        // Timestamp
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x09}, std::byte{0xCD},
        std::byte{0x0D}, std::byte{0x42},

        // Order Reference Number = 42
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x2A},

        // Executed Shares = 500
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x01}, std::byte{0xF4},

        // Match Number = 99999
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x01},
        std::byte{0x86}, std::byte{0x9F},
    };

    auto msg = data_feed::parse_executed_order(buffer);

    EXPECT_EQ(msg.locate, 5);
    EXPECT_EQ(msg.tracking, 7);
    EXPECT_EQ(msg.timestamp, 164433218u);
    EXPECT_EQ(msg.order_reference_number, 42u);
    EXPECT_EQ(msg.executed_shares, 500u);
    EXPECT_EQ(msg.match_number, 99999u);
}


TEST(MessageParsing, ExecutedOrderWithPrice) {
    // Message Type:            'C'          (1 byte)
    // Stock Locate:            5            (2 bytes, BE)
    // Tracking Number:         7            (2 bytes, BE)
    // Timestamp:               164433218    (6 bytes, BE)
    // Order Reference Number:  42           (8 bytes, BE)
    // Executed Shares:         500          (4 bytes, BE)
    // Match Number:            99999        (8 bytes, BE)
    // Printable:               'Y'          (1 byte)
    // Execution Price:         250000       (4 bytes, BE)

    std::array<std::byte, 36> buffer{
        std::byte{'C'},

        // Stock Locate = 5
        std::byte{0x00}, std::byte{0x05},

        // Tracking Number = 7
        std::byte{0x00}, std::byte{0x07},

        // Timestamp
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x09}, std::byte{0xCD},
        std::byte{0x0D}, std::byte{0x42},

        // Order Reference Number = 42
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x2A},

        // Executed Shares = 500
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x01}, std::byte{0xF4},

        // Match Number = 99999
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x01},
        std::byte{0x86}, std::byte{0x9F},

        // Printable
        std::byte{'Y'},

        // Execution Price = 250000
        std::byte{0x00}, std::byte{0x03},
        std::byte{0xD0}, std::byte{0x90},
    };

    auto msg = data_feed::parse_executed_order_with_price(buffer);
    ASSERT_TRUE(msg.has_value());

    EXPECT_EQ(msg->locate, 5);
    EXPECT_EQ(msg->tracking, 7);
    EXPECT_EQ(msg->timestamp, 164433218u);
    EXPECT_EQ(msg->order_reference_number, 42u);
    EXPECT_EQ(msg->executed_shares, 500u);
    EXPECT_EQ(msg->match_number, 99999u);
    EXPECT_EQ(msg->printable, data_feed::Printable::Yes);
    EXPECT_EQ(msg->price, 250000u);
}


TEST(MessageParsing, OrderCancel) {
    // Message Type:            'X'          (1 byte)
    // Stock Locate:            5            (2 bytes, BE)
    // Tracking Number:         7            (2 bytes, BE)
    // Timestamp:               164433218    (6 bytes, BE)
    // Order Reference Number:  42           (8 bytes, BE)
    // Cancelled Shares:        300          (4 bytes, BE)

    std::array<std::byte, 23> buffer{
        std::byte{'X'},

        // Stock Locate = 5
        std::byte{0x00}, std::byte{0x05},

        // Tracking Number = 7
        std::byte{0x00}, std::byte{0x07},

        // Timestamp
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x09}, std::byte{0xCD},
        std::byte{0x0D}, std::byte{0x42},

        // Order Reference Number = 42
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x2A},

        // Cancelled Shares = 300
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x01}, std::byte{0x2C},
    };

    auto msg = data_feed::parse_order_cancel(buffer);

    EXPECT_EQ(msg.locate, 5);
    EXPECT_EQ(msg.tracking, 7);
    EXPECT_EQ(msg.timestamp, 164433218u);
    EXPECT_EQ(msg.order_reference_number, 42u);
    EXPECT_EQ(msg.cancelled_shares, 300u);
}


TEST(MessageParsing, OrderDelete) {
    // Message Type:            'D'          (1 byte)
    // Stock Locate:            5            (2 bytes, BE)
    // Tracking Number:         7            (2 bytes, BE)
    // Timestamp:               164433218    (6 bytes, BE)
    // Order Reference Number:  42           (8 bytes, BE)

    std::array<std::byte, 19> buffer{
        std::byte{'D'},

        // Stock Locate = 5
        std::byte{0x00}, std::byte{0x05},

        // Tracking Number = 7
        std::byte{0x00}, std::byte{0x07},

        // Timestamp
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x09}, std::byte{0xCD},
        std::byte{0x0D}, std::byte{0x42},

        // Order Reference Number = 42
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x2A},
    };

    auto msg = data_feed::parse_order_delete(buffer);

    EXPECT_EQ(msg.locate, 5);
    EXPECT_EQ(msg.tracking, 7);
    EXPECT_EQ(msg.timestamp, 164433218u);
    EXPECT_EQ(msg.order_reference_number, 42u);
}


TEST(MessageParsing, OrderReplace) {
    // Message Type:                'U'          (1 byte)
    // Stock Locate:                5            (2 bytes, BE)
    // Tracking Number:             7            (2 bytes, BE)
    // Timestamp:                   164433218    (6 bytes, BE)
    // Old Order Reference Number:  42           (8 bytes, BE)
    // New Order Reference Number:  43           (8 bytes, BE)
    // Shares:                      200          (4 bytes, BE)
    // Price:                       150000       (4 bytes, BE)

    std::array<std::byte, 35> buffer{
        std::byte{'U'},

        // Stock Locate = 5
        std::byte{0x00}, std::byte{0x05},

        // Tracking Number = 7
        std::byte{0x00}, std::byte{0x07},

        // Timestamp
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x09}, std::byte{0xCD},
        std::byte{0x0D}, std::byte{0x42},

        // Old Order Reference Number = 42
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x2A},

        // New Order Reference Number = 43
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x2B},

        // Shares = 200
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0xC8},

        // Price = 150000
        std::byte{0x00}, std::byte{0x02},
        std::byte{0x49}, std::byte{0xF0},
    };

    auto msg = data_feed::parse_order_replace(buffer);

    EXPECT_EQ(msg.locate, 5);
    EXPECT_EQ(msg.tracking, 7);
    EXPECT_EQ(msg.timestamp, 164433218u);
    EXPECT_EQ(msg.old_order_reference_number, 42u);
    EXPECT_EQ(msg.new_order_reference_number, 43u);
    EXPECT_EQ(msg.shares, 200u);
    EXPECT_EQ(msg.price, 150000u);
}


TEST(MessageParsing, Trade) {
    // Message Type:            'P'          (1 byte)
    // Stock Locate:            5            (2 bytes, BE)
    // Tracking Number:         7            (2 bytes, BE)
    // Timestamp:               164433218    (6 bytes, BE)
    // Order Reference Number:  42           (8 bytes, BE)
    // Buy/Sell Indicator:      0            (1 byte, unused by parser)
    // Shares:                  1000         (4 bytes, BE)
    // Stock:                   'MSFT'       (8 bytes, BE)
    // Price:                   300000       (4 bytes, BE)
    // Match Number:            77777        (8 bytes, BE)

    std::array<std::byte, 44> buffer{
        std::byte{'P'},

        // Stock Locate = 5
        std::byte{0x00}, std::byte{0x05},

        // Tracking Number = 7
        std::byte{0x00}, std::byte{0x07},

        // Timestamp
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x09}, std::byte{0xCD},
        std::byte{0x0D}, std::byte{0x42},

        // Order Reference Number = 42
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x2A},

        // Buy/Sell Indicator (unused by parser)
        std::byte{0x00},

        // Shares = 1000
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x03}, std::byte{0xE8},

        // Stock
        std::byte{'M'}, std::byte{'S'},
        std::byte{'F'}, std::byte{'T'},
        std::byte{' '}, std::byte{' '},
        std::byte{' '}, std::byte{' '},

        // Price = 300000
        std::byte{0x00}, std::byte{0x04},
        std::byte{0x93}, std::byte{0xE0},

        // Match Number = 77777
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x01},
        std::byte{0x2F}, std::byte{0xD1},
    };

    auto msg = data_feed::parse_trade(buffer);

    std::array<char, 8> expected_stock = {'M', 'S', 'F', 'T', ' ', ' ', ' ', ' '};

    EXPECT_EQ(msg.locate, 5);
    EXPECT_EQ(msg.tracking, 7);
    EXPECT_EQ(msg.timestamp, 164433218u);
    EXPECT_EQ(msg.order_reference_number, 42u);
    EXPECT_EQ(msg.shares, 1000u);
    EXPECT_EQ(msg.stock, expected_stock);
    EXPECT_EQ(msg.price, 300000u);
    EXPECT_EQ(msg.match_number, 77777u);
}


TEST(MessageParsing, CrossTrade) {
    // Message Type:    'Q'          (1 byte)
    // Stock Locate:    5            (2 bytes, BE)
    // Tracking Number: 7            (2 bytes, BE)
    // Timestamp:       164433218    (6 bytes, BE)
    // Shares:          5000         (8 bytes, BE)
    // Stock:           'IBM'        (8 bytes, BE)
    // Cross Price:     999900       (4 bytes, BE)
    // Match Number:    654321       (8 bytes, BE)
    // Cross Type:      'O'          (1 byte)

    std::array<std::byte, 40> buffer{
        std::byte{'Q'},

        // Stock Locate = 5
        std::byte{0x00}, std::byte{0x05},

        // Tracking Number = 7
        std::byte{0x00}, std::byte{0x07},

        // Timestamp
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x09}, std::byte{0xCD},
        std::byte{0x0D}, std::byte{0x42},

        // Shares = 5000
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x13}, std::byte{0x88},

        // Stock
        std::byte{'I'}, std::byte{'B'},
        std::byte{'M'}, std::byte{' '},
        std::byte{' '}, std::byte{' '},
        std::byte{' '}, std::byte{' '},

        // Cross Price = 999900
        std::byte{0x00}, std::byte{0x0F},
        std::byte{0x41}, std::byte{0xDC},

        // Match Number = 654321
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x09},
        std::byte{0xFB}, std::byte{0xF1},

        // Cross Type
        std::byte{'O'},
    };

    auto msg = data_feed::parse_cross_ticker(buffer);

    std::array<char, 8> expected_stock = {'I', 'B', 'M', ' ', ' ', ' ', ' ', ' '};

    EXPECT_EQ(msg.locate, 5);
    EXPECT_EQ(msg.tracking, 7);
    EXPECT_EQ(msg.timestamp, 164433218u);
    EXPECT_EQ(msg.shares, 5000u);
    EXPECT_EQ(msg.stock, expected_stock);
    EXPECT_EQ(msg.cross_price, 999900u);
    EXPECT_EQ(msg.match_number, 654321u);
    EXPECT_EQ(msg.cross_type, 'O');
}


