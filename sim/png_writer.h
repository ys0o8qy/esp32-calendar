#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool png_write_argb8888(
    const char *path,
    const uint32_t *pixels,
    uint32_t width,
    uint32_t height,
    char *error,
    size_t error_len);
