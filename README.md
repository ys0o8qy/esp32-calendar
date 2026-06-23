# esp32-calendar

VS Code + ESP-IDF scaffold for the Waveshare ESP32-S3-RLCD-4.2 board.

## Reference

- Waveshare docs: https://docs.waveshare.net/ESP32-S3-RLCD-4.2/
- Waveshare example repo: https://github.com/waveshareteam/ESP32-S3-RLCD-4.2

## Chosen stack

This project intentionally uses the professional path from the Waveshare docs:

- VS Code
- Espressif ESP-IDF Extension
- ESP-IDF `v5.5.x` or newer
- Target: `esp32s3`

Think of this project as the empty house with the wiring already done: ESP-IDF, target, flash/PSRAM assumptions, and editor integration are prepared, and the calendar UI can be built cleanly on top.

## Bootstrap ESP-IDF locally

```bash
cd ~/Documents/personal/esp32-calendar
chmod +x scripts/*.sh
./scripts/bootstrap-esp-idf.sh
source ./scripts/export-esp-idf.sh
idf.py --version
```

The bootstrap installs ESP-IDF under:

```text
.tools/esp-idf
.tools/espressif
```

So this project does not depend on a global ESP-IDF install.

## Build

```bash
./scripts/build.sh
```

Or manually:

```bash
source ./scripts/export-esp-idf.sh
idf.py set-target esp32s3
idf.py build
```

## Desktop LVGL simulator

The calendar UI is structured so the LVGL screen can run on a desktop SDL2
window before it is flashed to the ESP32 board. This keeps layout work fast and
isolates display/Wi-Fi/weather hardware code behind a platform boundary.

Install desktop prerequisites:

```bash
brew install sdl2 cmake
```

Fetch LVGL v8 locally. The checkout is intentionally ignored by Git:

```bash
git clone --depth 1 --branch v8.3.11 --single-branch https://github.com/lvgl/lvgl.git third_party/lvgl
```

Build and smoke-test the simulator:

```bash
cmake -S sim -B build-sim
cmake --build build-sim
SDL_VIDEODRIVER=dummy ./build-sim/calendar_sim --smoke-test
```

Run the interactive simulator:

```bash
./build-sim/calendar_sim
```

If LVGL is checked out elsewhere, pass it explicitly:

```bash
cmake -S sim -B build-sim -DLVGL_ROOT=/absolute/path/to/lvgl
```

Shared UI data lives in `src/app/calendar_model.*`. LVGL screen construction
lives in `src/app/calendar_ui.*`. ESP32-specific Wi-Fi, NTP, weather HTTP, RTC,
and sensor code should feed the same model through `src/platform/esp32/`.

## Open in VS Code

```bash
code ~/Documents/personal/esp32-calendar
```

Then install the recommended `Espressif IDF` extension when prompted. The workspace settings point the extension at the project-local ESP-IDF install.

## Board notes

Waveshare ESP32-S3-RLCD-4.2 uses:

- ESP32-S3-WROOM-1-N16R8
- 16MB Flash
- 8MB PSRAM
- 4.2-inch RLCD driven by ST7305
- ES8311 audio codec
- ES7210 ADC / dual mic path
- PCF85063 RTC
- SHTC3 temperature/humidity sensor

`sdkconfig.defaults` enables ESP32-S3 target, 16MB flash, and octal PSRAM.

## Fetch Waveshare examples

```bash
./scripts/fetch-waveshare-examples.sh
```

Expected reference location:

```text
vendor/waveshare-esp32-s3-rlcd-4.2/02_Example/ESP-IDF
```

## Next step for display/calendar UI

Use Waveshare's ESP-IDF LVGL/factory examples as the hardware reference. Calendar UI should start from their LVGL v8 example path because Waveshare marks LVGL `v8.3.11` as the stable dependency for several examples.
