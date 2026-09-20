// itch_stat: liest eine ITCH-5.0-Datei (gzip) ueber df::StreamReader.
//
// Aufruf: itch_stat [stats|book] [pfad]
//   stats: StatsHandler, zwei Durchlaeufe (Directory, dann gefiltert nach
//          Watchlist) -> Message-Typ -> Count Tabelle.
//   book:  BookHandler, ein Durchlauf ueber alle Symbole; am Ende werden
//          die Handler- und Book-Statistiken ausgegeben.

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
    std::cout << "beobachtete Symbole aus Watchlist: " << h.watched_count()
              << " / " << h.directory().size() << " Locates\n";
}

void print_book_stats(const data_feed::BookHandler& h) {
    const auto& hs = h.stats();
    const auto& bs = h.book_stats();
    std::cout << "Handler-Statistik\n"
              << "  malformed_messages:  " << hs.malformed_messages << "\n"
              << "  unknown_references:  " << hs.unknown_references << "\n"
              << "Book-Statistik\n"
              << "  duplicate_ref:       " << bs.duplicate_ref << "\n"
              << "  unknown_ref:         " << bs.unknown_ref << "\n"
              << "  underflow_clamped:   " << bs.underflow_clamped << "\n"
              << "  level_missing:       " << bs.level_missing << "\n"
              << "  book_desync:         " << bs.book_desync << "\n"
              << "  zero_shares:         " << bs.zero_shares << "\n"
              << "  book_crossed:        " << bs.book_crossed << "\n";
}

void run_book(const std::string& path) {
    data_feed::BookHandler h{{"AAPL"}};//, "MSFT", "SPY", "XYZ"}};   // leere Watch-Liste = alle Locates

    data_feed::run_pass(path, h);

    print_book_stats(h);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "stats";
    const std::string path = argc > 2 ? argv[2] : "data/sample/head100mb.gz";

    if (mode != "stats" && mode != "book") {
        std::cerr << "Aufruf: " << argv[0] << " [stats|book] [pfad]\n";
        return 2;
    }

    try {
        if (mode == "stats") run_stats(path);
        else                 run_book(path);
    } catch (const std::exception& e) {
        std::cerr << "FEHLER: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
