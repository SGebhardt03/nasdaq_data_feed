# data_feed

> **Work in progress.** File formats and the CLI are still changing; nothing here should be considered stable yet.

A C++20 parser and order-book reconstructor for the NASDAQ TotalView-ITCH 5.0 binary market-data feed. It streams a gzip-compressed ITCH file, decodes messages, and feeds them into pluggable handlers for statistics, order-book reconstruction, and top-of-book (L1) export.

## Features

- Streaming gzip reader (`StreamReader`) that decodes ITCH's length-prefixed message framing without loading the whole file into memory.
- Message parsing for the core ITCH 5.0 message types (System Event, Stock Directory, Trading Action, Add Order (+ MPID), Order Executed (+ Price), Order Cancel/Delete/Replace, Trade, Cross Trade).
- In-memory order book reconstruction (`BookHandler`) with basic consistency checks (duplicate/unknown references, underflow clamping, crossed-book detection).
- `StatsHandler` for per-message-type counts, optionally filtered by a symbol watchlist.
- `L1Writer` to export top-of-book changes as buffered per-symbol CSV files (one file per stock locate).
- `research/plot_l1.py` to plot the exported bid/ask series with matplotlib.
- An `itch_stat` CLI for quick stats/book/L1-export runs over a sample file.

## Building

Requirements: CMake ≥ 3.20, a C++20 compiler, zlib, GoogleTest.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build
```

## Usage

```sh
./build/itch_stat [stats|book] [path] [output_dir]
```

| Argument     | Default                          | Meaning                                   |
|--------------|----------------------------------|-------------------------------------------|
| `mode`       | `book`                           | `stats` or `book`                         |
| `path`       | `data/raw/S112825-v50.txt.gz`    | gzip-compressed ITCH 5.0 file             |
| `output_dir` | none                             | if given, L1 CSV files are written here (created if missing) |

```sh
./build/itch_stat stats data/raw/S112825-v50.txt.gz
./build/itch_stat book  data/raw/S112825-v50.txt.gz            # counters only
./build/itch_stat book  data/raw/S112825-v50.txt.gz output/     # plus L1 CSVs
```

- `stats` prints a message-type count table, filtered by a symbol watchlist.
- `book` reconstructs the order book and prints handler/book consistency counters. With `output_dir`, it also writes top-of-book changes there.

The watchlist is currently hard-coded in `src/main.cpp` (`AAPL` only for `book`); an empty list means all symbols.

### L1 output

`L1Writer` writes one headerless CSV per stock locate, named `<locate>.csv` (e.g. `output/24.csv`), with one row per top-of-book change:

```
ts_ns,locate,bid_px,bid_sz,ask_px,ask_sz
```

Prices are raw ITCH ticks (dollars × 10,000). If a side of the book is empty, its price and size fields are left blank.

### Plotting

Requires Python 3 with `matplotlib`.

```sh
python3 research/plot_l1.py output/24.csv                    # show interactively
python3 research/plot_l1.py output/24.csv -o output/24.png   # save to file
```

### Sample data

```sh
scripts/fetch_data.sh
```

## Layout

```
include/itch/   Core library (streaming, framing, parsing, book, handlers, L1 writer)
src/            itch_stat CLI
tests/          Unit tests and a 100 MB streaming acceptance test
scripts/        Data-fetching helpers
research/       Analysis scripts (`plot_l1.py` works; `load.py`, `signals.py`, `evaluate.py` are empty placeholders)
bench/          Benchmark notes/results (empty for now)
output/         Generated L1 CSVs (not part of the library)
```

## Status

Core streaming, parsing, and order-book reconstruction work and are covered by tests. L1 export and plotting work end to end but have no tests yet. Not yet done: signal/evaluation research scripts, benchmarking, a configurable watchlist, and a stable CLI/output format.
