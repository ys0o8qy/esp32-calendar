# SDL2 LVGL Simulator

This target runs the shared calendar LVGL UI in a desktop SDL2 window at the same logical size as the RLCD layout: `400x300`.

## Prerequisites

```bash
brew install sdl2 cmake
```

Provide LVGL v8 source in one of two ways:

```bash
git clone --depth 1 --branch v8.3.11 https://github.com/lvgl/lvgl.git third_party/lvgl
```

or pass an existing checkout:

```bash
cmake -S sim -B build-sim -DLVGL_ROOT=/absolute/path/to/lvgl
cmake --build build-sim
./build-sim/calendar_sim
```

The simulator uses mock data from `calendar_model_sample()`. ESP32 firmware should fill the same `calendar_model_t` from Wi-Fi, NTP, weather HTTP, RTC, and local sensors.
