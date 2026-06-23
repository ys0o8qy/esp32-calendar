#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
if [[ -f scripts/export-esp-idf.sh ]]; then
  source scripts/export-esp-idf.sh
fi
idf.py set-target esp32s3
idf.py build
