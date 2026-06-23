cmake_minimum_required(VERSION 3.16)

set(LVGL_COMPONENT_DIR "${CMAKE_CURRENT_LIST_DIR}/third_party/lvgl")
if(NOT EXISTS "${LVGL_COMPONENT_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "LVGL is required for the ESP32 build. Run: git clone --depth 1 --branch v8.3.11 --single-branch https://github.com/lvgl/lvgl.git third_party/lvgl")
endif()
set(EXTRA_COMPONENT_DIRS "${LVGL_COMPONENT_DIR}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_calendar)
