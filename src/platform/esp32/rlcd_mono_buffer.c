#include "rlcd_mono_buffer.h"

#include <string.h>

size_t rlcd_mono_buffer_size(uint16_t width, uint16_t height)
{
    return ((size_t)width * (size_t)height) / 8u;
}

bool rlcd_mono_pixel_is_white(uint8_t brightness)
{
    return brightness >= RLCD_MONO_WHITE_BRIGHTNESS_THRESHOLD;
}

void rlcd_mono_buffer_fill(uint8_t *buffer, size_t length, bool white)
{
    memset(buffer, white ? 0xff : 0x00, length);
}

void rlcd_mono_buffer_set_landscape_pixel(
    uint8_t *buffer,
    uint16_t width,
    uint16_t height,
    uint16_t x,
    uint16_t y,
    bool white)
{
    if (x >= width || y >= height || buffer == NULL) {
        return;
    }

    uint16_t inv_y = (uint16_t)(height - 1u - y);
    uint16_t byte_x = (uint16_t)(x >> 1);
    uint16_t block_y = (uint16_t)(inv_y >> 2);
    uint16_t blocks_per_column = (uint16_t)(height >> 2);
    size_t index = (size_t)byte_x * blocks_per_column + block_y;
    uint8_t local_x = (uint8_t)(x & 0x01u);
    uint8_t local_y = (uint8_t)(inv_y & 0x03u);
    uint8_t bit = (uint8_t)(7u - ((local_y << 1) | local_x));
    uint8_t mask = (uint8_t)(1u << bit);

    if (white) {
        buffer[index] |= mask;
    } else {
        buffer[index] &= (uint8_t)~mask;
    }
}
