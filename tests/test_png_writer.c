#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "png_writer.h"

static uint32_t read_be32(const unsigned char *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

void test_png_writer(void)
{
    const char *path = "/tmp/esp32-calendar-test-writer.png";
    uint32_t pixels[] = {
        0xff000000u, 0xffffffffu,
        0xffff0000u, 0xff00ff00u,
    };
    char error[128] = {0};

    assert(png_write_argb8888(path, pixels, 2, 2, error, sizeof(error)));

    FILE *fp = fopen(path, "rb");
    assert(fp != NULL);
    unsigned char data[64];
    size_t n = fread(data, 1, sizeof(data), fp);
    fclose(fp);

    static const unsigned char signature[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    assert(n >= 33);
    assert(memcmp(data, signature, sizeof(signature)) == 0);
    assert(memcmp(data + 12, "IHDR", 4) == 0);
    assert(read_be32(data + 16) == 2);
    assert(read_be32(data + 20) == 2);
    assert(data[24] == 8);
    assert(data[25] == 6);

    assert(!png_write_argb8888(path, NULL, 2, 2, error, sizeof(error)));
    assert(strstr(error, "pixels") != NULL);
}
