//
// Created by samuel-gebhardt on 28.08.26.
//
#pragma once

#include <cstddef>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>

#include "stream_reader.hpp"

namespace data_feed {
    template <class H>
    void dispatch(std::span<const std::byte> body, H& handler) {
        const char type = static_cast<char>(body[0]);
        handler.on_any(type);
        if (type == 'R') { handler.on_stock_directory(body); return; }
        if (type == 'S') { handler.on_system_event(body); return; }

        const auto& w = handler.watch();
        if (!w.empty()) {
            const auto loc = df::read_be<std::uint16_t>(body.subspan(1, 2));
            if (loc >= w.size() || !w[loc]) return;
        }

        switch (type) {
            case 'A': case 'F': handler.on_add_order(body);      break;
            case 'E':           handler.on_executed(body);       break;
            case 'C':           handler.on_executed(body); break;
            case 'X':           handler.on_cancel(body);         break;
            case 'D':           handler.on_delete(body);         break;
            case 'U':           handler.on_replace(body);        break;
            case 'H':           break;
            case 'P': case 'Q': handler.on_trade(body);          break;
            default: break;
        }
        handler.after_message();
    }


    // Liest die Datei einmal komplett per StreamReader und speist jede
    // Nachricht in den Handler ein. Eine Dev-Sample-Datei (z. B.
    // data/sample/head100mb.gz) ist per Konstruktion mitten im gzip-Stream
    // abgeschnitten -- StreamReader wirft dann beim letzten, unvollstaendigen
    // Frame. Das ist hier kein Fehler, sondern das erwartete Ende der Probe.
    template <class H>
    void run_pass(const std::string& path, H& handler) {
        df::StreamReader reader(path);
        try {
            for (;;) {
                const auto msg = reader.next();
                if (msg.empty()) break;
                dispatch(msg, handler);
            }
        } catch (const std::runtime_error& e) {
            std::cerr << "Hinweis: " << e.what()
                      << " -- vermutlich abgeschnittene Dev-Sample-Datei; "
                         "Statistik basiert auf den bis dahin gelesenen Nachrichten.\n";
        }
    }

    struct HandlerDefaults {
        void on_stock_directory(std::span<const std::byte>) {}
        void on_system_event(std::span<const std::byte>) {}
        void on_trade(std::span<const std::byte>) {}
        void after_message() {}
        void on_any(char) {}
    };
}
