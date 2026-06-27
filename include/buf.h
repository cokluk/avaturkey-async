#ifndef AVATURKEY_BUF_H
#define AVATURKEY_BUF_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    const uint8_t *data;
    size_t len;
    size_t pos;
} buf_t;

typedef struct {
    uint8_t *data;
    size_t len;
    size_t cap;
} dynbuf_t;

void     dynbuf_init(dynbuf_t *b);
void     dynbuf_free(dynbuf_t *b);
void     dynbuf_reserve(dynbuf_t *b, size_t need);
void     dynbuf_append(dynbuf_t *b, const void *src, size_t n);
void     dynbuf_append_u8(dynbuf_t *b, uint8_t v);

bool     buf_read(buf_t *b, void *dst, size_t n);
int8_t   buf_read_i8(buf_t *b);
uint8_t  buf_read_u8(buf_t *b);
int32_t  buf_read_i32(buf_t *b);
uint32_t buf_read_u32(buf_t *b);
int64_t  buf_read_i64(buf_t *b);
double   buf_read_f64(buf_t *b);
bool     buf_read_string(buf_t *b, char **out);

#endif
