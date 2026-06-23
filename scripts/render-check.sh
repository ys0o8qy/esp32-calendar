#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

OUT="${1:-build-sim/calendar-render.png}"

scripts/build-sim.sh
mkdir -p "$(dirname "$OUT")"
SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-dummy}" ./build-sim/calendar_sim --dump-png "$OUT"
python3 scripts/check-render-png.py "$OUT"
echo "Render PNG: $OUT"
