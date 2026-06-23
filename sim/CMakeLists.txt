cmake_minimum_required(VERSION 3.16)

project(esp32_calendar_sim C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

if(NOT DEFINED LVGL_ROOT)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../third_party/lvgl/CMakeLists.txt")
        set(LVGL_ROOT "${CMAKE_CURRENT_LIST_DIR}/../third_party/lvgl")
    else()
        message(FATAL_ERROR "LVGL_ROOT is required. Run with -DLVGL_ROOT=/absolute/path/to/lvgl or place LVGL at third_party/lvgl.")
    endif()
endif()

set(LV_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/lv_conf.h" CACHE STRING "LVGL config path")
set(LV_BUILD_EXAMPLES OFF CACHE BOOL "Disable LVGL examples for simulator builds" FORCE)
set(LV_BUILD_DEMOS OFF CACHE BOOL "Disable LVGL demos for simulator builds" FORCE)
add_compile_definitions(LV_CONF_INCLUDE_SIMPLE=1)

add_subdirectory("${LVGL_ROOT}" "${CMAKE_BINARY_DIR}/lvgl" EXCLUDE_FROM_ALL)

add_executable(calendar_sim
    main_sdl.c
    ../src/app/calendar_model.c
    ../src/app/calendar_theme.c
    ../src/app/calendar_ui.c
)

target_include_directories(calendar_sim PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/../src/app"
)

target_link_libraries(calendar_sim PRIVATE lvgl PkgConfig::SDL2)
