// itch_stat: reads an ITCH-5.0 file (gzip) via df::StreamReader.
//
// Usage: itch_stat [stats|book] [path]
//   stats: StatsHandler, two passes (directory, then filtered by
//          watchlist) -> message-type -> count table.
//   book:  BookHandler, one pass over all symbols; at the end the
//          handler and book statistics are printed.

#include "itch/book_handler.hpp"
#include "itch/dispatch.hpp"
#include "itch/stats_handler.hpp"

#include <iostream>
#include <string>

namespace {

void print_type_counts(const data_feed::StatsHandler& h) {
    std::cout << "Message-type  Count\n";
    for (int t = 0; t < 256; ++t) {
        const auto n = h.counts()[static_cast<std::size_t>(t)];
        if (n == 0) continue;
        const char c = static_cast<char>(t);
        std::cout << "     " << c << "        " << n << "\n";
    }
}

void run_stats(const std::string& path) {
    data_feed::StatsHandler h{{"AAPL", "MSFT", "SPY", "XYZ"}};

    data_feed::run_pass(path, h);

    print_type_counts(h);
    std::cout << "watched symbols from watchlist: " << h.watched_count()
              << " / " << h.directory().size() << " locates\n";
}

void print_book_stats(const data_feed::BookHandler& h) {
    const auto& hs = h.stats();
    const auto& bs = h.book_stats();
    std::cout << "Handler statistics\n"
              << "  malformed_messages:  " << hs.malformed_messages << "\n"
              << "  unknown_references:  " << hs.unknown_references << "\n"
              << "Book statistics\n"
              << "  duplicate_ref:       " << bs.duplicate_ref << "\n"
              << "  unknown_ref:         " << bs.unknown_ref << "\n"
              << "  underflow_clamped:   " << bs.underflow_clamped << "\n"
              << "  level_missing:       " << bs.level_missing << "\n"
              << "  book_desync:         " << bs.book_desync << "\n"
              << "  zero_shares:         " << bs.zero_shares << "\n"
              << "  book_crossed:        " << bs.book_crossed << "\n";
}

void run_book(const std::string& path) {
    data_feed::BookHandler h{{"AAPL"}};//, "MSFT", "SPY", "XYZ"}};   // empty watch list = all locates

    data_feed::run_pass(path, h);

    print_book_stats(h);
}

}  // namespace

void run_writer(const std::string& path, const std::string& output_path) {

    data_feed::L1Writer writer{output_path};
    data_feed::BookHandler h{{"AAPL"}};//, "MSFT", "SPY", "XYZ"}};   // empty watch list = all locates

    h.set_writer(writer);

    data_feed::run_pass(path, h);
    print_book_stats(h);
}

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "book";
    const std::string path = argc > 2 ? argv[2] : "data/raw/S112825-v50.txt.gz";
    const std::string output_path = argc > 3 ? argv[3] : "output/";

    if (mode != "stats" && mode != "book") {
        std::cerr << "usage: " << argv[0] << " [stats|book] [path]\n";
        return 2;
    }

    try {
        if (mode == "stats") run_stats(path);
        if (mode == "books" && argc == 2) run_book(path);
        else                 run_writer(path, output_path);
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
