#include "buf.h"
#include <stdlib.h>
#include <string.h>

void dynbuf_init(dynbuf_t *b) {
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

void dynbuf_free(dynbuf_t *b) {
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

void dynbuf_reserve(dynbuf_t *b, size_t need) {
    if (b->len + need <= b->cap)
        return;
    size_t ncap = b->cap ? b->cap : 64;
    while (ncap < b->len + need)
        ncap *= 2;
    uint8_t *p = realloc(b->data, ncap);
    if (!p)
        return;
    b->data = p;
    b->cap = ncap;
}

void dynbuf_append(dynbuf_t *b, const void *src, size_t n) {
    dynbuf_reserve(b, n);
    memcpy(b->data + b->len, src, n);
    b->len += n;
}

void dynbuf_append_u8(dynbuf_t *b, uint8_t v) {
    dynbuf_append(b, &v, 1);
}

bool buf_read(buf_t *b, void *dst, size_t n) {
    if (b->pos + n > b->len)
        return false;
    memcpy(dst, b->data + b->pos, n);
    b->pos += n;
    return true;
}

int8_t buf_read_i8(buf_t *b) {
    int8_t v = 0;
    buf_read(b, &v, 1);
    return v;
}

uint8_t buf_read_u8(buf_t *b) {
    uint8_t v = 0;
    buf_read(b, &v, 1);
    return v;
}

int32_t buf_read_i32(buf_t *b) {
    uint8_t raw[4];
    if (!buf_read(b, raw, 4))
        return 0;
    return (int32_t)((raw[0] << 24) | (raw[1] << 16) | (raw[2] << 8) | raw[3]);
}

uint32_t buf_read_u32(buf_t *b) {
    uint8_t raw[4];
    if (!buf_read(b, raw, 4))
        return 0;
    return ((uint32_t)raw[0] << 24) | ((uint32_t)raw[1] << 16) |
           ((uint32_t)raw[2] << 8) | (uint32_t)raw[3];
}

int64_t buf_read_i64(buf_t *b) {
    uint8_t raw[8];
    if (!buf_read(b, raw, 8))
        return 0;
    int64_t v = 0;
    for (int i = 0; i < 8; i++)
        v = (v << 8) | raw[i];
    return v;
}

double buf_read_f64(buf_t *b) {
    uint8_t raw[8];
    if (!buf_read(b, raw, 8))
        return 0.0;
    uint64_t bits = 0;
    for (int i = 0; i < 8; i++)
        bits = (bits << 8) | raw[i];
    double d;
    memcpy(&d, &bits, sizeof(d));
    return d;
}

bool buf_read_string(buf_t *b, char **out) {
    int shift = 0;
    uint32_t length = 0;
    uint8_t byte;
    do {
        if (!buf_read(b, &byte, 1))
            return false;
        length |= (uint32_t)(byte & 0x7F) << shift;
        shift += 7;
        if (shift > 35)
            return false;
    } while (byte & 0x80);

    if (b->pos + length > b->len)
        return false;
    char *s = malloc(length + 1);
    if (!s)
        return false;
    memcpy(s, b->data + b->pos, length);
    s[length] = '\0';
    b->pos += length;
    *out = s;
    return true;
}
