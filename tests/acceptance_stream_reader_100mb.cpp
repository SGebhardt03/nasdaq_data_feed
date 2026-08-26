// Abnahmetest: "Programm liest 100 MB durch, ohne zu crashen, und zaehlt
// Bytes korrekt."
//
// Liest die komplette Fixture ueber df::StreamReader::next() ein und
// vergleicht die dabei selbst gezaehlten Bytes/Nachrichten gegen eine extern
// (unabhaengig von diesem Code) ermittelte Ground Truth aus einer
// Begleitdatei. Siehe tests/fixtures/itch_100mb.expected fuer die Herkunft
// der Referenzwerte.
//
// Aufruf: acceptance_stream_reader_100mb <fixture.gz> <expected.txt>
// Exit 0 = bestanden, Exit 1 = kontrolliert fehlgeschlagen (kein Crash),
// Exit 2 = falscher Aufruf.

#include "itch/endian.hpp"
#include "../include/itch/stream_reader.hpp"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

constexpr std::uint64_t kMinDecompressedBytes = 100'000'000;

std::unordered_map<std::string, std::uint64_t> parse_expected(
    const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open expected file: " + path);

  std::unordered_map<std::string, std::uint64_t> values;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') continue;
    const auto eq = line.find('=');
    if (eq == std::string::npos) continue;
    values[line.substr(0, eq)] = std::stoull(line.substr(eq + 1));
  }
  return values;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "usage: " << argv[0] << " <fixture.gz> <expected.txt>\n";
    return 2;
  }
  const std::string fixture_path = argv[1];
  const std::string expected_path = argv[2];

  bool ok = true;

  try {
    const auto expected = parse_expected(expected_path);
    const auto expected_bytes = expected.at("decompressed_bytes");
    const auto expected_messages = expected.at("messages");

    const auto start = std::chrono::steady_clock::now();

    df::StreamReader reader(fixture_path);
    std::uint64_t own_bytes = 0;
    std::uint64_t own_messages = 0;

    for (;;) {
      const auto msg = reader.next();
      if (msg.empty()) break;
      own_bytes += df::StreamReader::kLenPrefix + msg.size();
      ++own_messages;
    }

    const auto elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();
    const double mib = static_cast<double>(own_bytes) / (1024.0 * 1024.0);

    std::cout << "gelesen:     " << own_bytes << " Bytes, " << own_messages
              << " Nachrichten in " << elapsed << " s ("
              << (mib / elapsed) << " MiB/s)\n";

    auto check = [&](const char* what, std::uint64_t got, std::uint64_t want) {
      if (got != want) {
        std::cerr << "FEHLER: " << what << " stimmt nicht: gelesen=" << got
                  << " erwartet(ground truth)=" << want << "\n";
        ok = false;
      }
    };

    // Interner Selbstcheck: eigene Zaehlung muss mit dem vom Reader
    // gefuehrten Byte-Offset uebereinstimmen.
    check("eigene Bytezaehlung vs. reader.offset()", own_bytes,
          reader.offset());

    // Externer Cross-Check gegen die unabhaengig ermittelte Ground Truth.
    check("dekomprimierte Bytes vs. Ground Truth", own_bytes, expected_bytes);
    check("Nachrichtenanzahl vs. Ground Truth", own_messages,
          expected_messages);

    if (own_bytes < kMinDecompressedBytes) {
      std::cerr << "FEHLER: nur " << own_bytes << " Bytes gelesen, "
                << "Abnahmekriterium verlangt >= " << kMinDecompressedBytes
                << " Bytes\n";
      ok = false;
    }
  } catch (const std::exception& e) {
    std::cerr << "FEHLGESCHLAGEN (kontrolliert abgefangen, kein Crash): "
              << e.what() << "\n";
    return 1;
  }

  std::cout << (ok ? "PASS" : "FAIL") << "\n";
  return ok ? 0 : 1;
}
