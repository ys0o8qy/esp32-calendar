#!/usr/bin/env bash
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export IDF_TOOLS_PATH="$ROOT/.tools/espressif"
export IDF_PYTHON_ENV_PATH="$ROOT/.tools/espressif/python_env/idf5.5_py3.9_env"
export ESP_PYTHON="/usr/bin/python3"
source "$ROOT/.tools/esp-idf/export.sh"
