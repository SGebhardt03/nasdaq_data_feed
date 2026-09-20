# data_feed

> **Work in progress.** APIs, file formats, and the CLI are still changing; nothing here should be considered stable yet.

A C++20 parser and order-book reconstructor for the NASDAQ TotalView-ITCH 5.0 binary market-data feed. It streams a gzip-compressed ITCH file, decodes messages, and feeds them into pluggable handlers for statistics, order-book reconstruction, and top-of-book (L1) export.

## Features

- Streaming gzip reader (`StreamReader`) that decodes ITCH's length-prefixed message framing without loading the whole file into memory.
- Message parsing for the core ITCH 5.0 message types (System Event, Stock Directory, Trading Action, Add Order (+ MPID), Order Executed (+ Price), Order Cancel/Delete/Replace, Trade, Cross Trade).
- In-memory order book reconstruction (`BookHandler`) with basic consistency checks (duplicate/unknown references, underflow clamping, crossed-book detection).
- `StatsHandler` for per-message-type counts, optionally filtered by a symbol watchlist.
- `L1Writer` to export top-of-book changes as per-symbol CSV files.
- A `itch_stat` CLI for quick stats/book runs over a sample file.

## Building

Requirements: CMake ≥ 3.20, a C++20 compiler, zlib, GoogleTest.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build
```

## Usage

```sh
./build/itch_stat stats data/raw/S112825-v50.txt.gz
./build/itch_stat book  data/raw/S112825-v50.txt.gz
```

Sample data can be fetched with:

```sh
scripts/fetch_data.sh
```

## Layout

```
include/itch/   Core library (streaming, framing, parsing, book, handlers)
src/            itch_stat CLI
tests/          Unit tests and a 100 MB streaming acceptance test
scripts/        Data-fetching helpers
research/       Exploratory analysis scripts (early stage, mostly empty for now)
bench/          Benchmark notes/results
```

## Status

Core streaming, parsing, and order-book reconstruction work and are covered by tests. Not yet done: the research/analysis scripts, benchmarking, and a stable CLI/output format.
