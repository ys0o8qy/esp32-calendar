# esp32-calendar

VS Code + ESP-IDF scaffold for the Waveshare ESP32-S3-RLCD-4.2 board.

## Reference

- Waveshare docs: https://docs.waveshare.net/ESP32-S3-RLCD-4.2/
- Waveshare example repo: https://github.com/waveshareteam/ESP32-S3-RLCD-4.2

## Chosen stack

This project intentionally uses the professional path from the Waveshare docs:

- VS Code
- Espressif ESP-IDF Extension
- ESP-IDF `v6.0.1`
- Target: `esp32s3`

Think of this project as the empty house with the wiring already done: ESP-IDF, target, flash/PSRAM assumptions, and editor integration are prepared, and the calendar UI can be built cleanly on top.

## Configure ESP-IDF

```bash
cd ~/Documents/personal/esp32-calendar
chmod +x scripts/*.sh
./scripts/bootstrap-esp-idf.sh
source ./scripts/export-esp-idf.sh
idf.py --version
```

This project uses the ESP-IDF v6.0.1 installation under:

```text
/Users/nspzoow/.espressif/v6.0.1/esp-idf
/Users/nspzoow/.espressif
```

The bootstrap script validates that installation and regenerates
`scripts/export-esp-idf.sh`; it does not install a project-local ESP-IDF copy.

## Build

```bash
./scripts/build.sh esp32
```

Or manually:

```bash
source ./scripts/export-esp-idf.sh
idf.py set-target esp32s3
idf.py build
```

Flash over USB Serial/JTAG:

```bash
source ./scripts/export-esp-idf.sh
idf.py -p /dev/cu.usbmodem101 flash
```

`scripts/build.sh` also supports:

```bash
./scripts/build.sh sim
./scripts/build.sh all
```

## Desktop LVGL simulator

The calendar UI is structured so the LVGL screen can run on a desktop SDL2
window before it is flashed to the ESP32 board. This keeps layout work fast and
isolates display/Wi-Fi/weather hardware code behind a platform boundary. The
desktop simulator and ESP32 firmware both use `src/app/calendar_ui.*`.

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
./scripts/build-sim.sh
SDL_VIDEODRIVER=dummy ./build-sim/calendar_sim --smoke-test
```

Run the interactive simulator:

```bash
./scripts/run-sim.sh
```

Dump the simulator render to a PNG and run the structural smoke check:

```bash
./scripts/render-check.sh
```

This writes `build-sim/calendar-render.png` by default. To choose the output
path:

```bash
./scripts/render-check.sh /tmp/calendar-render.png
```

For lower-level simulator usage:

```bash
SDL_VIDEODRIVER=dummy ./build-sim/calendar_sim --dump-png build-sim/calendar-render.png
python3 scripts/check-render-png.py build-sim/calendar-render.png
```

If LVGL is checked out elsewhere, pass it explicitly:

```bash
LVGL_ROOT=/absolute/path/to/lvgl ./scripts/build-sim.sh
```

Shared UI data lives in `src/app/calendar_model.*`. LVGL screen construction
lives in `src/app/calendar_ui.*`. The ESP32 display bridge lives in
`src/platform/esp32/calendar_display.*` and adapts LVGL RGB565 draw buffers to
the ST7305 monochrome RLCD buffer used by the Waveshare board. ESP32-specific
Wi-Fi, NTP, weather HTTP, RTC, and sensor code should feed the same model
through `src/platform/esp32/`.

The UI uses a generated 18px, 1bpp Simplified Chinese subset font at
`src/app/calendar_font_zh.*`. The generator reads the current UI string literals,
renders only the required non-ASCII glyphs from macOS
`/System/Library/Fonts/Hiragino Sans GB.ttc`, and leaves ASCII/digits to the
Montserrat fallback font. After changing Chinese UI text, regenerate and verify
the subset:

```bash
python3 scripts/generate-zh-font.py
python3 scripts/check-font-coverage.py
```

## Open in VS Code

```bash
code ~/Documents/personal/esp32-calendar
```

Then install the recommended `Espressif IDF` extension when prompted. The workspace settings point the extension at the ESP-IDF v6.0.1 install under `/Users/nspzoow/.espressif`.

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

`sdkconfig.defaults` enables ESP32-S3 target, 16MB flash, octal PSRAM, and the
LVGL v8 options needed by the shared calendar UI.

## Fetch Waveshare examples

```bash
./scripts/fetch-waveshare-examples.sh
```

Expected reference location:

```text
vendor/waveshare-esp32-s3-rlcd-4.2/02_Example/ESP-IDF
```

The ESP32 display port follows the official ESP-IDF `08_LVGL_V8_Test` example:
GPIO12 MOSI, GPIO11 SCK, GPIO5 DC, GPIO40 CS, GPIO41 reset, 400x300 landscape,
and the ST7305 initialization sequence from Waveshare.

For hardware render debugging, enable `CALENDAR_DUMP_RLCD_FRAME` in
`idf.py menuconfig` under `ESP32 Calendar`. The first LVGL flush will log
`CALENDAR_RLCD_FRAME_BEGIN`, `CALENDAR_RLCD_FRAME_HEX`, and
`CALENDAR_RLCD_FRAME_END` records containing the final ST7305 1bpp frame buffer
sent to the panel.
