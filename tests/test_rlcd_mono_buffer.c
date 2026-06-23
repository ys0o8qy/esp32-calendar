#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "rlcd_mono_buffer.h"

static void test_landscape_pixel_packing_matches_st7305_buffer_layout(void)
{
    uint8_t buffer[4];
    memset(buffer, 0, sizeof(buffer));

    rlcd_mono_buffer_set_landscape_pixel(buffer, 4, 8, 0, 7, true);
    assert(buffer[0] == 0x80);

    rlcd_mono_buffer_set_landscape_pixel(buffer, 4, 8, 1, 7, true);
    assert(buffer[0] == 0xc0);

    rlcd_mono_buffer_set_landscape_pixel(buffer, 4, 8, 0, 6, true);
    assert(buffer[0] == 0xe0);

    rlcd_mono_buffer_set_landscape_pixel(buffer, 4, 8, 2, 7, true);
    assert(buffer[2] == 0x80);

    rlcd_mono_buffer_set_landscape_pixel(buffer, 4, 8, 1, 7, false);
    assert(buffer[0] == 0xa0);
}

static void test_out_of_bounds_pixels_are_ignored(void)
{
    uint8_t buffer[4];
    memset(buffer, 0x55, sizeof(buffer));

    rlcd_mono_buffer_set_landscape_pixel(buffer, 4, 8, 4, 0, true);
    rlcd_mono_buffer_set_landscape_pixel(buffer, 4, 8, 0, 8, true);

    for (size_t i = 0; i < sizeof(buffer); i++) {
        assert(buffer[i] == 0x55);
    }
}

static void test_near_white_stays_white_but_antialias_gray_becomes_black(void)
{
    assert(rlcd_mono_pixel_is_white(247) == RLCD_MONO_WHITE);
    assert(rlcd_mono_pixel_is_white(216) == RLCD_MONO_BLACK);
}

void test_rlcd_mono_buffer(void)
{
    test_landscape_pixel_packing_matches_st7305_buffer_layout();
    test_out_of_bounds_pixels_are_ignored();
    test_near_white_stays_white_but_antialias_gray_becomes_black();
}
