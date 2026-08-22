//
// Created by samuel-gebhardt on 22.08.26.
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

namespace data_feed {

class StatsHandler {
public:
    explicit StatsHandler(std::vector<std::string> watchlist)
    : wanted_(std::move(watchlist)) {}


    void on_message(std::span<const std::byte> body) {
        const char type = static_cast<char>(body[0]);
        // Nur im ersten Durchlauf zaehlen -- sonst wuerde der gefilterte
        // zweite Durchlauf (der ebenfalls ueber alle Nachrichten laeuft)
        // jeden Typ ein zweites Mal zaehlen.
        if (!directory_ready_) ++counts_[static_cast<unsigned char>(type)];

        if (type == 'R') {
            on_stock_directory(body);
            return;
        }

        const auto loc = df::read_be<std::uint16_t>(body.subspan(1, 2));
        if (loc >= watch_.size() || !watch_[loc]) return;
        switch (type) {
            // case 'A': case 'F': on_add_order(body);  break;   // TODO 1.2
            // case 'E': case 'C': on_executed(body);   break;
            // case 'X':           on_cancel(body);     break;
            // case 'D':           on_delete(body);     break;
            // case 'U':           on_replace(body);    break;
            // case 'P': case 'Q': on_trade(body);      break;
            default: break;
        }
    }

    // Zwischen den beiden Durchlaeufen aufzurufen, wenn alle 'R' durch sind.
    void finalize_directory() {
        watch_.assign(locate_to_ticker_.size(), 0);
        for (std::size_t i = 0; i < locate_to_ticker_.size(); ++i)
            watch_[i] = static_cast<std::uint8_t>(is_wanted(locate_to_ticker_[i]));
        directory_ready_ = true;
    }


    const std::array<std::uint64_t, 256>& counts() const { return counts_; }

    // locate -> Ticker, Index 0 ungenutzt (Locates starten bei 1).
    // Luecken sind moeglich und enthalten dann leere Ticker.
    const std::vector<Ticker>& directory() const { return locate_to_ticker_; }


    std::size_t watched_count() const {
        std::size_t n = 0;
        for (auto w : watch_) n += w;
        return n;
    }

private:
    // 'R' Stock Directory. parse_stock_directory aufrufen, locate_to_ticker_
    // bei Bedarf auf locate+1 vergroessern (resize, nicht reserve) und den
    // Ticker ablegen. Kein Filter -- die Tabelle braucht alle Symbole,
    // nicht nur die der Watchlist.
    void on_stock_directory(std::span<const std::byte> body) {
        const auto sd = parse_stock_directory(body);
        if (sd.locate >= locate_to_ticker_.size())
            locate_to_ticker_.resize(sd.locate + 1);
        locate_to_ticker_[sd.locate] = sd.stock;
    }

    // TODO 1.2/1.3: on_add_order, on_executed, on_cancel,
    //               on_delete, on_replace, on_trade
    //               -- jeweils parse_* aufrufen, Ergebnis vorerst verwerfen.
    //               Zusaetzlich Min/Max von price und shares mitfuehren:
    //               absurde Werte verraten einen Offset-Fehler sofort.


    bool is_wanted(const Ticker& t) const {
        const std::string_view sv = trim(t);
        for (const auto& w : wanted_)
            if (w == sv) return true;
        return false;
    }

    static std::string_view trim(const Ticker& t) {
        std::size_t n = t.size();
        while (n > 0 && t[n - 1] == ' ') --n;
        return std::string_view(t.data(), n);
    }

    std::array<std::uint64_t, 256> counts_{};
    std::vector<Ticker>            locate_to_ticker_;   // Index = stock_locate
    std::vector<std::uint8_t>      watch_;              // Index = stock_locate
    std::vector<std::string>       wanted_;
    bool                           directory_ready_ = false;
};

}  // namespace data_feed