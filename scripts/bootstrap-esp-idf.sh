#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IDF_PATH_GLOBAL="${IDF_PATH_GLOBAL:-/Users/nspzoow/.espressif/v6.0.1/esp-idf}"
IDF_TOOLS_PATH_GLOBAL="${IDF_TOOLS_PATH_GLOBAL:-/Users/nspzoow/.espressif}"
IDF_PYTHON_ENV_PATH_GLOBAL="${IDF_PYTHON_ENV_PATH_GLOBAL:-/Users/nspzoow/.espressif/python_env/idf6.0_py3.11_env}"

if [[ ! -f "$IDF_PATH_GLOBAL/export.sh" ]]; then
  echo "ESP-IDF v6.0.1 was not found at: $IDF_PATH_GLOBAL" >&2
  echo "Install ESP-IDF v6.0.1 first, then rerun this script." >&2
  exit 1
fi

cat > "$ROOT/scripts/export-esp-idf.sh" <<EOF
#!/usr/bin/env bash
set -euo pipefail

IDF_PATH="\${IDF_PATH:-$IDF_PATH_GLOBAL}"
IDF_TOOLS_PATH="\${IDF_TOOLS_PATH:-$IDF_TOOLS_PATH_GLOBAL}"
IDF_PYTHON_ENV_PATH="\${IDF_PYTHON_ENV_PATH:-$IDF_PYTHON_ENV_PATH_GLOBAL}"

if [[ ! -f "\$IDF_PATH/export.sh" ]]; then
  echo "ESP-IDF v6.0.1 was not found at: \$IDF_PATH" >&2
  exit 1
fi

export IDF_PATH
export IDF_TOOLS_PATH
export IDF_PYTHON_ENV_PATH
source "\$IDF_PATH/export.sh"
EOF
chmod +x "$ROOT/scripts/export-esp-idf.sh"

echo "Configured ESP-IDF v6.0.1. Use: source scripts/export-esp-idf.sh"
