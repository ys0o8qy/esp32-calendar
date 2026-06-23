#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

run_esp32=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --esp32)
      run_esp32=1
      shift
      ;;
    -h|--help)
      echo "Usage: $0 [--esp32]" >&2
      exit 0
      ;;
    *)
      echo "Usage: $0 [--esp32]" >&2
      exit 2
      ;;
  esac
done

python3 -m unittest tests/test_render_png_check.py tests/test_rlcd_log_to_png.py
cmake -S tests -B build-tests
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure

./scripts/render-check.sh build-sim/calendar-render.png

if [[ "$run_esp32" == "1" ]]; then
  ./scripts/build.sh esp32
fi

cat <<'EOF'
Render verification exported: build-sim/calendar-render.png
Next required step: inspect the PNG visually with the $rlcd-render-check workflow.
EOF
