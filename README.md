# esp32-calendar

ESP-Claw Edge Agent based calendar firmware for the Waveshare
ESP32-S3-RLCD-4.2 board, plus a desktop LVGL simulator used for 400x300 RLCD
render QA.

## Architecture

The firmware project entry is `application/edge_agent`, imported from
`espressif/esp-claw` and locked to upstream commit
`ac2a2ec0ca0b23d1270065e563c4ee4253e32e51` for this migration.

The hardware baseline is the ESP-Claw board support:

```text
application/edge_agent/boards/waveshare/waveshare_ESP32_S3_RLCD_4_2
```

That board support owns the RLCD custom panel, ES8311/ES7210 audio devices,
I2C/I2S peripherals, partitions, flash, and PSRAM defaults. The calendar does
not keep a separate firmware RLCD bridge.

Calendar-specific code lives in:

```text
application/edge_agent/components/calendar_home
```

The only app-level calendar entry is:

```c
esp_err_t calendar_home_start(void);
```

`calendar_home` is started by Edge Agent after board/display/config/filesystem
initialization. It uses the ESP-Claw `display_arbiter` as the default calendar
screen and pauses LVGL flushing when Lua takes display ownership.

## Retained Calendar Data

First migrated version keeps only local board data:

- system time from ESP-Claw runtime configuration/network stack
- PCF85063 RTC fallback
- SHTC3 indoor temperature and humidity

Wi-Fi provisioning, timezone, HTTP server, LLM/IM/MCP/Lua/Agent configuration,
event routing, and scheduler are provided by ESP-Claw. The old local
`voice_assistant_sdk`, `local_tts`, WebSocket assistant protocol, local wake
echo path, calendar Wi-Fi Kconfig, and local SNTP Kconfig have been removed.

## ESP-IDF

This project follows ESP-Claw's recommended ESP-IDF baseline:

```text
ESP-IDF v5.5.4
```

Default local script paths assume:

```text
/Users/nspzoow/.espressif/v5.5.4/esp-idf
/Users/nspzoow/.espressif
```

Configure or validate the local export script:

```bash
./scripts/bootstrap-esp-idf.sh
source ./scripts/export-esp-idf.sh
idf.py --version
```

## Firmware Build

Use the wrapper:

```bash
./scripts/build.sh esp32
```

Equivalent manual commands:

```bash
source ./scripts/export-esp-idf.sh
cd application/edge_agent
idf.py reconfigure
rm -f sdkconfig sdkconfig.old
idf.py set-target esp32s3
python managed_components/espressif__esp_board_manager/gen_bmgr_config_codes.py \
  -c ./boards -b waveshare_ESP32_S3_RLCD_4_2 --project-dir .
rm -f sdkconfig sdkconfig.old
export SDKCONFIG_DEFAULTS="$PWD/sdkconfig.defaults;$PWD/components/gen_bmgr_codes/board_manager.defaults"
idf.py set-target esp32s3
idf.py build
```

Flash over USB Serial/JTAG from `application/edge_agent`:

```bash
idf.py -p /dev/cu.usbmodem101 flash monitor
```

## Desktop LVGL Simulator

The simulator remains independent from the ESP-Claw firmware project so UI
layout work and PNG QA stay fast:

```bash
brew install sdl2 cmake
git clone --depth 1 --branch v8.3.11 --single-branch https://github.com/lvgl/lvgl.git third_party/lvgl
./scripts/build-sim.sh
SDL_VIDEODRIVER=dummy ./build-sim/calendar_sim --smoke-test
```

Run the interactive simulator:

```bash
./scripts/run-sim.sh
```

Export the canonical 400x300 render:

```bash
./scripts/render-check.sh build-sim/calendar-render.png
```

The simulator builds against the same `calendar_home/src/calendar_ui.c`,
`calendar_model.c`, theme, and generated font assets used by firmware.

## Verification

Default local workflow:

```bash
./scripts/dev-verify.sh
```

With firmware build:

```bash
./scripts/dev-verify.sh --esp32
```

The workflow runs Python tests, C tests, simulator render export, and
`scripts/check-render-png.py`. Per `AGENTS.md`, after any code/build
verification you must also inspect `build-sim/calendar-render.png` visually
with the RLCD render checklist before reporting the work complete.

Font coverage:

```bash
python3 scripts/generate-zh-font.py
python3 scripts/check-font-coverage.py
```

## Board Notes

Waveshare ESP32-S3-RLCD-4.2 includes:

- ESP32-S3-WROOM-1-N16R8
- 16MB flash
- 8MB PSRAM
- 4.2-inch 400x300 RLCD
- ES8311 audio codec
- ES7210 ADC / dual mic path
- PCF85063 RTC
- SHTC3 temperature/humidity sensor
