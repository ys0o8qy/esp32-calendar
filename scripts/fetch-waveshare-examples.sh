#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
mkdir -p vendor
if [[ ! -d vendor/waveshare-esp32-s3-rlcd-4.2/.git ]]; then
  git clone --depth 1 --filter=blob:none https://github.com/waveshareteam/ESP32-S3-RLCD-4.2 vendor/waveshare-esp32-s3-rlcd-4.2
else
  git -C vendor/waveshare-esp32-s3-rlcd-4.2 pull --ff-only
fi
