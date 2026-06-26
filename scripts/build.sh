#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

target="${1:-esp32}"

case "$target" in
  esp32)
    if [[ -f scripts/export-esp-idf.sh ]]; then
      source scripts/export-esp-idf.sh
    fi
    bmgr_generator="managed_components/espressif__esp_board_manager/gen_bmgr_config_codes.py"
    if [[ ! -f "$bmgr_generator" ]]; then
      idf.py reconfigure
    fi
    rm -f sdkconfig sdkconfig.old
    rm -rf build
    idf.py set-target esp32s3
    python "$bmgr_generator" -c ./boards -b waveshare_ESP32_S3_RLCD_4_2 --project-dir .
    rm -f sdkconfig sdkconfig.old
    rm -rf build
    export SDKCONFIG_DEFAULTS="$PWD/sdkconfig.defaults;$PWD/components/gen_bmgr_codes/board_manager.defaults"
    idf.py set-target esp32s3
    idf.py build
    ;;
  sim)
    exec scripts/build-sim.sh
    ;;
  all)
    scripts/build.sh esp32
    scripts/build.sh sim
    ;;
  *)
    echo "Usage: $0 [esp32|sim|all]" >&2
    exit 2
    ;;
esac
