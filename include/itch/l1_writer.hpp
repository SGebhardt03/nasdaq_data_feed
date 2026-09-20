//
// Created by samuel-gebhardt on 21.09.26.
//
#pragma once

#include <array>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "book.hpp"

namespace data_feed {

    // Schreibt bei jeder Top-of-Book-Aenderung eine CSV-Zeile
    // (ts_ns,locate,bid_px,bid_sz,ask_px,ask_sz) in eine Datei pro Locate.
    // Preise sind rohe Ticks; eine fehlende Seite (kein Bid/Ask) ergibt
    // ein leeres Feld statt einer Zahl.
    class L1Writer {
    public:
        explicit L1Writer(std::filesystem::path dir, std::size_t flush_threshold = 1u << 16)
            : dir_(std::move(dir)), flush_threshold_(flush_threshold) {
            std::filesystem::create_directories(dir_);
        }

        ~L1Writer() { flush_all(); }

        L1Writer(const L1Writer&) = delete;
        L1Writer& operator=(const L1Writer&) = delete;

        void emit(uint64_t ts_ns, uint16_t locate, const TopOfBook& tob) {
            PerSymbolFile& file = file_for(locate);
            std::string& buf = file.buffer;

            append_uint(buf, ts_ns);
            buf += ',';
            append_uint(buf, locate);
            buf += ',';
            if (tob.bid) append_int(buf, tob.bid->price_ticks);
            buf += ',';
            if (tob.bid) append_uint(buf, tob.bid->shares);
            buf += ',';
            if (tob.ask) append_int(buf, tob.ask->price_ticks);
            buf += ',';
            if (tob.ask) append_uint(buf, tob.ask->shares);
            buf += '\n';

            if (buf.size() >= flush_threshold_) flush(file);
        }

        void flush_all() {
            for (auto& [locate, file] : files_) flush(file);
        }

    private:
        struct PerSymbolFile {
            std::ofstream stream;
            std::string   buffer;
        };

        static void append_uint(std::string& out, uint64_t v) {
            std::array<char, 24> tmp{};
            const auto res = std::to_chars(tmp.data(), tmp.data() + tmp.size(), v);
            out.append(tmp.data(), res.ptr);
        }

        static void append_int(std::string& out, int64_t v) {
            std::array<char, 24> tmp{};
            const auto res = std::to_chars(tmp.data(), tmp.data() + tmp.size(), v);
            out.append(tmp.data(), res.ptr);
        }

        PerSymbolFile& file_for(uint16_t locate) {
            const auto it = files_.find(locate);
            if (it != files_.end()) return it->second;

            PerSymbolFile file;
            file.stream.open(dir_ / (std::to_string(locate) + ".csv"),
                              std::ios::out | std::ios::trunc);
            if (!file.stream) {
                throw std::runtime_error(
                    "L1Writer: kann Datei fuer locate " + std::to_string(locate) + " nicht oeffnen");
            }
            file.buffer.reserve(flush_threshold_ + 256);
            return files_.emplace(locate, std::move(file)).first->second;
        }

        static void flush(PerSymbolFile& file) {
            if (file.buffer.empty()) return;
            file.stream.write(file.buffer.data(), static_cast<std::streamsize>(file.buffer.size()));
            file.buffer.clear();
        }

        std::filesystem::path dir_;
        std::size_t flush_threshold_;
        std::unordered_map<uint16_t, PerSymbolFile> files_;
    };

}  // namespace data_feed
