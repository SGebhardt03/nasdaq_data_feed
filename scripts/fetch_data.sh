#!/usr/bin/env bash
set -euo pipefail
FILE="S112825-v50.txt.gz"
URL="https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/${FILE}"
mkdir -p ../data/raw data/sample data/derived
[[ -f "data/raw/${FILE}" ]] || curl -C - -o "data/raw/${FILE}" "${URL}"
curl -r 0-100000000 -o "data/sample/head100mb.gz" "${URL}"   # Dev-Sample
sha256sum "data/raw/${FILE}" | tee data/raw/${FILE}.sha256
