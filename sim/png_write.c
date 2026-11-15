// Minimal dependency-free PNG writer (stored/uncompressed deflate blocks).
// Used for simulator screenshots ('S' key) and the headless tests.
#include "png_write.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t crc_table[256];

static void crc_init(void)
{
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crc_table[n] = c;
    }
}

static uint32_t crc_update(uint32_t c, const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) c = crc_table[(c ^ buf[i]) & 0xFF] ^ (c >> 8);
    return c;
}

static void be32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static void chunk(FILE *f, const char *type, const uint8_t *data, uint32_t len)
{
    uint8_t hdr[8];
    be32(hdr, len);
    memcpy(hdr + 4, type, 4);
    fwrite(hdr, 1, 8, f);
    if (len) fwrite(data, 1, len, f);
    uint32_t c = crc_update(0xFFFFFFFFu, (const uint8_t *)type, 4);
    c = crc_update(c, data, len) ^ 0xFFFFFFFFu;
    uint8_t cb[4];
    be32(cb, c);
    fwrite(cb, 1, 4, f);
}

bool png_write_rgb565(const char *path, const uint16_t *px, int w, int h, int scale)
{
    crc_init();
    const int W = w * scale, H = h * scale;
    const size_t raw_len = (size_t)H * (size_t)(1 + W * 3);
    uint8_t *raw = malloc(raw_len);
    if (!raw) return false;
    uint8_t *p = raw;
    for (int y = 0; y < H; y++) {
        *p++ = 0; // filter: none
        for (int x = 0; x < W; x++) {
            uint16_t c = px[(y / scale) * w + x / scale];
            *p++ = (uint8_t)(((c >> 11) & 31) * 255 / 31);
            *p++ = (uint8_t)(((c >> 5) & 63) * 255 / 63);
            *p++ = (uint8_t)((c & 31) * 255 / 31);
        }
    }
    // zlib stream with stored blocks
    const size_t nblocks = (raw_len + 65534) / 65535;
    const size_t z_len = 2 + raw_len + nblocks * 5 + 4;
    uint8_t *z = malloc(z_len);
    if (!z) {
        free(raw);
        return false;
    }
    size_t o = 0;
    z[o++] = 0x78;
    z[o++] = 0x01;
    uint32_t a = 1, b = 0;
    for (size_t i = 0; i < raw_len; i += 65535) {
        size_t n = raw_len - i < 65535 ? raw_len - i : 65535;
        z[o++] = (uint8_t)(i + n >= raw_len ? 1 : 0);
        z[o++] = (uint8_t)(n & 0xFF);
        z[o++] = (uint8_t)(n >> 8);
        z[o++] = (uint8_t)(~n & 0xFF);
        z[o++] = (uint8_t)((~n >> 8) & 0xFF);
        memcpy(z + o, raw + i, n);
        for (size_t k = 0; k < n; k++) {
            a = (a + raw[i + k]) % 65521u;
            b = (b + a) % 65521u;
        }
        o += n;
    }
    be32(z + o, (b << 16) | a);
    o += 4;

    FILE *f = fopen(path, "wb");
    bool ok = f != NULL;
    if (ok) {
        static const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
        fwrite(sig, 1, 8, f);
        uint8_t ihdr[13];
        be32(ihdr, (uint32_t)W);
        be32(ihdr + 4, (uint32_t)H);
        ihdr[8] = 8;  // bit depth
        ihdr[9] = 2;  // RGB
        ihdr[10] = 0;
        ihdr[11] = 0;
        ihdr[12] = 0;
        chunk(f, "IHDR", ihdr, 13);
        chunk(f, "IDAT", z, (uint32_t)o);
        chunk(f, "IEND", NULL, 0);
        fclose(f);
    }
    free(z);
    free(raw);
    return ok;
}
