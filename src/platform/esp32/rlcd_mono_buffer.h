#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define RLCD_MONO_BLACK false
#define RLCD_MONO_WHITE true
#define RLCD_MONO_WHITE_BRIGHTNESS_THRESHOLD 224

size_t rlcd_mono_buffer_size(uint16_t width, uint16_t height);
bool rlcd_mono_pixel_is_white(uint8_t brightness);
void rlcd_mono_buffer_fill(uint8_t *buffer, size_t length, bool white);
void rlcd_mono_buffer_set_landscape_pixel(
    uint8_t *buffer,
    uint16_t width,
    uint16_t height,
    uint16_t x,
    uint16_t y,
    bool white);
