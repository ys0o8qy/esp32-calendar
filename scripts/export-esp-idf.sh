#!/usr/bin/env bash
set -euo pipefail

IDF_PATH="${IDF_PATH:-/Users/nspzoow/.espressif/v6.0.1/esp-idf}"
IDF_TOOLS_PATH="${IDF_TOOLS_PATH:-/Users/nspzoow/.espressif}"
IDF_PYTHON_ENV_PATH="${IDF_PYTHON_ENV_PATH:-/Users/nspzoow/.espressif/python_env/idf6.0_py3.11_env}"

if [[ ! -f "$IDF_PATH/export.sh" ]]; then
  echo "ESP-IDF v6.0.1 was not found at: $IDF_PATH" >&2
  exit 1
fi

export IDF_PATH
export IDF_TOOLS_PATH
export IDF_PYTHON_ENV_PATH
source "$IDF_PATH/export.sh"
