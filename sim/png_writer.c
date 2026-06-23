#include "png_writer.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PNG_MAX_STORED_BLOCK 65535u

static uint32_t g_crc_table[256];
static bool g_crc_table_ready;

static void set_error(char *error, size_t error_len, const char *message)
{
    if (error != NULL && error_len > 0) {
        snprintf(error, error_len, "%s", message);
    }
}

static void set_errno_error(char *error, size_t error_len, const char *prefix)
{
    if (error != NULL && error_len > 0) {
        snprintf(error, error_len, "%s: %s", prefix, strerror(errno));
    }
}

static void crc_table_init(void)
{
    if (g_crc_table_ready) {
        return;
    }

    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) {
            c = (c & 1u) ? (0xedb88320u ^ (c >> 1)) : (c >> 1);
        }
        g_crc_table[n] = c;
    }
    g_crc_table_ready = true;
}

static uint32_t crc32_update(uint32_t crc, const unsigned char *data, size_t len)
{
    crc_table_init();
    uint32_t c = crc ^ 0xffffffffu;
    for (size_t i = 0; i < len; i++) {
        c = g_crc_table[(c ^ data[i]) & 0xffu] ^ (c >> 8);
    }
    return c ^ 0xffffffffu;
}

static uint32_t adler32(const unsigned char *data, size_t len)
{
    uint32_t a = 1;
    uint32_t b = 0;
    for (size_t i = 0; i < len; i++) {
        a = (a + data[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

static bool write_all(FILE *fp, const void *data, size_t len)
{
    return fwrite(data, 1, len, fp) == len;
}

static bool write_be32(FILE *fp, uint32_t value)
{
    unsigned char bytes[] = {
        (unsigned char)(value >> 24),
        (unsigned char)(value >> 16),
        (unsigned char)(value >> 8),
        (unsigned char)value,
    };
    return write_all(fp, bytes, sizeof(bytes));
}

static bool write_chunk(FILE *fp, const char type[4], const unsigned char *data, uint32_t len)
{
    uint32_t crc = crc32_update(0, (const unsigned char *)type, 4);
    if (data != NULL && len > 0) {
        crc = crc32_update(crc, data, len);
    }

    return write_be32(fp, len) &&
           write_all(fp, type, 4) &&
           (len == 0 || write_all(fp, data, len)) &&
           write_be32(fp, crc);
}

static bool make_zlib_stored_stream(
    const unsigned char *raw,
    size_t raw_len,
    unsigned char **out,
    size_t *out_len)
{
    size_t blocks = (raw_len + PNG_MAX_STORED_BLOCK - 1u) / PNG_MAX_STORED_BLOCK;
    if (blocks == 0) {
        blocks = 1;
    }
    size_t len = 2u + raw_len + blocks * 5u + 4u;
    unsigned char *stream = malloc(len);
    if (stream == NULL) {
        return false;
    }

    size_t pos = 0;
    stream[pos++] = 0x78;
    stream[pos++] = 0x01;

    size_t raw_pos = 0;
    for (size_t block = 0; block < blocks; block++) {
        size_t remaining = raw_len - raw_pos;
        uint16_t block_len = (uint16_t)(remaining > PNG_MAX_STORED_BLOCK ? PNG_MAX_STORED_BLOCK : remaining);
        bool final_block = block == blocks - 1u;
        stream[pos++] = final_block ? 0x01 : 0x00;
        stream[pos++] = (unsigned char)(block_len & 0xffu);
        stream[pos++] = (unsigned char)(block_len >> 8);
        uint16_t nlen = (uint16_t)~block_len;
        stream[pos++] = (unsigned char)(nlen & 0xffu);
        stream[pos++] = (unsigned char)(nlen >> 8);
        if (block_len > 0) {
            memcpy(stream + pos, raw + raw_pos, block_len);
        }
        pos += block_len;
        raw_pos += block_len;
    }

    uint32_t adler = adler32(raw, raw_len);
    stream[pos++] = (unsigned char)(adler >> 24);
    stream[pos++] = (unsigned char)(adler >> 16);
    stream[pos++] = (unsigned char)(adler >> 8);
    stream[pos++] = (unsigned char)adler;

    *out = stream;
    *out_len = pos;
    return true;
}

bool png_write_argb8888(
    const char *path,
    const uint32_t *pixels,
    uint32_t width,
    uint32_t height,
    char *error,
    size_t error_len)
{
    if (path == NULL || path[0] == '\0') {
        set_error(error, error_len, "path is required");
        return false;
    }
    if (pixels == NULL) {
        set_error(error, error_len, "pixels are required");
        return false;
    }
    if (width == 0 || height == 0) {
        set_error(error, error_len, "width and height must be non-zero");
        return false;
    }

    size_t stride = (size_t)width * 4u + 1u;
    size_t raw_len = stride * (size_t)height;
    unsigned char *raw = malloc(raw_len);
    if (raw == NULL) {
        set_error(error, error_len, "raw PNG buffer allocation failed");
        return false;
    }

    for (uint32_t y = 0; y < height; y++) {
        unsigned char *row = raw + (size_t)y * stride;
        row[0] = 0;
        for (uint32_t x = 0; x < width; x++) {
            uint32_t argb = pixels[(size_t)y * width + x];
            unsigned char *dst = row + 1u + (size_t)x * 4u;
            dst[0] = (unsigned char)(argb >> 16);
            dst[1] = (unsigned char)(argb >> 8);
            dst[2] = (unsigned char)argb;
            dst[3] = (unsigned char)(argb >> 24);
        }
    }

    unsigned char *zlib_stream = NULL;
    size_t zlib_len = 0;
    if (!make_zlib_stored_stream(raw, raw_len, &zlib_stream, &zlib_len)) {
        free(raw);
        set_error(error, error_len, "zlib stream allocation failed");
        return false;
    }

    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        free(zlib_stream);
        free(raw);
        set_errno_error(error, error_len, "failed to open PNG output");
        return false;
    }

    static const unsigned char signature[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    unsigned char ihdr[13] = {
        (unsigned char)(width >> 24), (unsigned char)(width >> 16), (unsigned char)(width >> 8), (unsigned char)width,
        (unsigned char)(height >> 24), (unsigned char)(height >> 16), (unsigned char)(height >> 8), (unsigned char)height,
        8, 6, 0, 0, 0,
    };

    bool ok = write_all(fp, signature, sizeof(signature)) &&
              write_chunk(fp, "IHDR", ihdr, sizeof(ihdr)) &&
              write_chunk(fp, "IDAT", zlib_stream, (uint32_t)zlib_len) &&
              write_chunk(fp, "IEND", NULL, 0);

    if (fclose(fp) != 0) {
        ok = false;
    }

    free(zlib_stream);
    free(raw);

    if (!ok) {
        set_error(error, error_len, "failed to write PNG file");
        return false;
    }
    set_error(error, error_len, "");
    return true;
}
