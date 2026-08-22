// itch_stat: liest eine ITCH-5.0-Datei (gzip) ueber df::StreamReader und
// ermittelt mit StatsHandler in zwei Durchlaufen (Directory, dann gefiltert
// nach Watchlist) eine Message-Typ -> Count Tabelle.

#include "itch/stats_handler.hpp"
#include "stream_reader.cpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

// Liest die Datei einmal komplett per StreamReader und speist jede
// Nachricht in den Handler ein. Eine Dev-Sample-Datei (z. B.
// data/sample/head100mb.gz) ist per Konstruktion mitten im gzip-Stream
// abgeschnitten -- StreamReader wirft dann beim letzten, unvollstaendigen
// Frame. Das ist hier kein Fehler, sondern das erwartete Ende der Probe.
void run_pass(const std::string& path, data_feed::StatsHandler& h) {
    df::StreamReader reader(path);
    try {
        for (;;) {
            const auto msg = reader.next();
            if (msg.empty()) break;
            h.on_message(msg);
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "Hinweis: " << e.what()
                  << " -- vermutlich abgeschnittene Dev-Sample-Datei; "
                     "Statistik basiert auf den bis dahin gelesenen Nachrichten.\n";
    }
}

void print_type_counts(const data_feed::StatsHandler& h) {
    std::cout << "Message-Typ  Count\n";
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
