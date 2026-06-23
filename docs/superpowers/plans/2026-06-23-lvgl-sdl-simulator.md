# LVGL SDL Simulator Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a shared LVGL calendar UI architecture with an SDL2 desktop simulator path and ESP32 platform boundary.

**Architecture:** Shared UI code lives under `src/app` and consumes a plain `calendar_model_t`. ESP32 and SDL simulator code live in separate platform folders so display, Wi-Fi, time, weather, and sensor access do not leak into LVGL screen construction.

**Tech Stack:** C11, LVGL v8-compatible API, SDL2 for simulator display, ESP-IDF for firmware, CMake/CTest for simulator-side tests.

---

### Task 1: Shared Calendar Model

**Files:**
- Create: `src/app/calendar_model.h`
- Create: `src/app/calendar_model.c`
- Create: `tests/test_calendar_model.c`
- Create: `tests/CMakeLists.txt`

- [x] Write tests for status text, event markers, and fixed month grid.
- [x] Run `cmake -S tests -B build-tests && cmake --build build-tests && ctest --test-dir build-tests --output-on-failure` and verify tests fail before implementation.
- [x] Implement `calendar_model_sample`, `calendar_model_status_text`, and `calendar_model_month_grid`.
- [x] Re-run the test command and verify it passes.

### Task 2: LVGL UI Boundary

**Files:**
- Create: `src/app/calendar_ui.h`
- Create: `src/app/calendar_ui.c`
- Create: `src/app/calendar_theme.h`
- Create: `src/app/calendar_theme.c`
- Modify: `main/CMakeLists.txt`

- [x] Implement LVGL screen construction around `calendar_model_t`.
- [x] Keep all external data access out of UI code.
- [x] Add app model/platform sources to ESP-IDF component registration.

### Task 3: SDL2 Simulator Target

**Files:**
- Create: `sim/CMakeLists.txt`
- Create: `sim/main_sdl.c`
- Create: `sim/lv_conf.h`
- Create: `sim/README.md`
- Modify: `README.md`

- [x] Add an SDL2 executable target that accepts LVGL source through `LVGL_ROOT` or `third_party/lvgl`.
- [x] Implement a 400x300 SDL window and LVGL flush callback.
- [x] Document `cmake -S sim -B build-sim -DLVGL_ROOT=/path/to/lvgl && cmake --build build-sim`.

### Task 4: ESP32 Platform Boundary

**Files:**
- Create: `src/platform/esp32/calendar_platform.h`
- Create: `src/platform/esp32/calendar_platform.c`
- Modify: `main/main.c`

- [x] Replace scaffold logging loop with platform model update calls.
- [x] Keep hardware-specific implementation as stubs until display driver and network credentials are added.

### Verification

- [x] Run simulator unit tests with CTest.
- [x] Run `cmake -S sim -B build-sim && cmake --build build-sim` with a real LVGL checkout.
- [x] Run ESP-IDF build through the project-local ESP-IDF toolchain using `idf.py -B build-idf build`.
