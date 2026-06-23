cmake_minimum_required(VERSION 3.16)

project(esp32_calendar_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(test_calendar_model
    test_calendar_model.c
    ../src/app/calendar_model.c
)

target_include_directories(test_calendar_model PRIVATE ../src/app)

enable_testing()
add_test(NAME calendar_model COMMAND test_calendar_model)
