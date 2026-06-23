#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IDF_VERSION="${IDF_VERSION:-v5.5.1}"
IDF_PATH_LOCAL="$ROOT/.tools/esp-idf"
IDF_TOOLS_PATH_LOCAL="$ROOT/.tools/espressif"

mkdir -p "$ROOT/.tools"

if [[ ! -d "$IDF_PATH_LOCAL/.git" ]]; then
  echo "Cloning ESP-IDF $IDF_VERSION into $IDF_PATH_LOCAL ..."
  git clone --depth 1 --branch "$IDF_VERSION" --recursive https://github.com/espressif/esp-idf.git "$IDF_PATH_LOCAL"
else
  echo "ESP-IDF already exists at $IDF_PATH_LOCAL"
fi

export IDF_TOOLS_PATH="$IDF_TOOLS_PATH_LOCAL"
"$IDF_PATH_LOCAL/install.sh" esp32s3

cat > "$ROOT/scripts/export-esp-idf.sh" <<EOF
#!/usr/bin/env bash
export IDF_TOOLS_PATH="$IDF_TOOLS_PATH_LOCAL"
source "$IDF_PATH_LOCAL/export.sh"
EOF
chmod +x "$ROOT/scripts/export-esp-idf.sh"

echo "Done. Use: source scripts/export-esp-idf.sh"
