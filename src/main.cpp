// itch_stat: liest eine ITCH-5.0-Datei (gzip) ueber df::StreamReader und
// ermittelt mit StatsHandler in zwei Durchlaufen (Directory, dann gefiltert
// nach Watchlist) eine Message-Typ -> Count Tabelle.

#include "itch/dispatch.hpp"
#include "itch/stats_handler.hpp"
#include "../include/itch/stream_reader.hpp"

#include <iostream>
#include <stdexcept>
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

}  // namespace

int main(int argc, char** argv) {
    const std::string path = argc > 1 ? argv[1] : "data/sample/head100mb.gz";

    try {
        data_feed::StatsHandler h{{"AAPL", "MSFT", "SPY", "XYZ"}};

        run_pass(path, h);          // Durchlauf 1: Directory
        h.finalize_directory();
        run_pass(path, h);          // Durchlauf 2: gefiltert

        print_type_counts(h);
        std::cout << "beobachtete Symbole aus Watchlist: " << h.watched_count()
                  << " / " << h.directory().size() << " Locates\n";
    } catch (const std::exception& e) {
        std::cerr << "FEHLER: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
