#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if [[ -z "${LVGL_ROOT:-}" ]]; then
  LVGL_ROOT="$ROOT/third_party/lvgl"
fi

if [[ ! -f "$LVGL_ROOT/CMakeLists.txt" ]]; then
  echo "LVGL was not found at: $LVGL_ROOT" >&2
  echo "Fetch it with:" >&2
  echo "  git clone --depth 1 --branch v8.3.11 --single-branch https://github.com/lvgl/lvgl.git third_party/lvgl" >&2
  exit 1
fi

cmake -S sim -B build-sim -DLVGL_ROOT="$LVGL_ROOT"
cmake --build build-sim
