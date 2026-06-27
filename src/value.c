#include "value.h"
#include <stdlib.h>
#include <string.h>

value_t *val_null(void) {
    value_t *v = calloc(1, sizeof(value_t));
    if (v) v->type = VAL_NULL;
    return v;
}

value_t *val_bool(bool b) {
    value_t *v = calloc(1, sizeof(value_t));
    if (v) { v->type = VAL_BOOL; v->u.b = b; }
    return v;
}

value_t *val_int(int64_t n) {
    value_t *v = calloc(1, sizeof(value_t));
    if (!v) return NULL;
    if (n > 2147483647LL || n < -2147483648LL) {
        v->type = VAL_LONG;
        v->u.i64 = n;
    } else {
        v->type = VAL_INT;
        v->u.i32 = (int32_t)n;
    }
    return v;
}

value_t *val_double(double d) {
    value_t *v = calloc(1, sizeof(value_t));
    if (v) { v->type = VAL_DOUBLE; v->u.f64 = d; }
    return v;
}

value_t *val_string(const char *s) {
    return val_string_n(s, s ? strlen(s) : 0);
}

value_t *val_string_n(const char *s, size_t n) {
    value_t *v = calloc(1, sizeof(value_t));
    if (!v) return NULL;
    v->type = VAL_STRING;
    v->u.str = malloc(n + 1);
    if (!v->u.str) { free(v); return NULL; }
    memcpy(v->u.str, s, n);
    v->u.str[n] = '\0';
    return v;
}

value_t *val_dict(size_t n) {
    value_t *v = calloc(1, sizeof(value_t));
    if (!v) return NULL;
    v->type = VAL_DICT;
    v->dict_len = n;
    v->dict_keys = calloc(n, sizeof(char *));
    v->dict_vals = calloc(n, sizeof(value_t *));
    return v;
}

value_t *val_array(size_t n) {
    value_t *v = calloc(1, sizeof(value_t));
    if (!v) return NULL;
    v->type = VAL_ARRAY;
    v->arr_len = n;
    v->arr_items = calloc(n, sizeof(value_t *));
    return v;
}

static void val_dict_grow(value_t *d) {
    size_t n = d->dict_len + 1;
    d->dict_keys = realloc(d->dict_keys, n * sizeof(char *));
    d->dict_vals = realloc(d->dict_vals, n * sizeof(value_t *));
    d->dict_len = n;
}

void val_dict_set(value_t *d, const char *k, value_t *v) {
    if (!d || d->type != VAL_DICT) return;
    for (size_t i = 0; i < d->dict_len; i++) {
        if (d->dict_keys[i] && strcmp(d->dict_keys[i], k) == 0) {
            val_free(d->dict_vals[i]);
            d->dict_vals[i] = v;
            return;
        }
    }
    val_dict_grow(d);
    size_t i = d->dict_len - 1;
    d->dict_keys[i] = strdup(k);
    d->dict_vals[i] = v;
}

void val_array_set(value_t *a, size_t i, value_t *v) {
    if (!a || a->type != VAL_ARRAY || i >= a->arr_len) return;
    a->arr_items[i] = v;
}

void val_free(value_t *v) {
    if (!v) return;
    switch (v->type) {
    case VAL_STRING:
        free(v->u.str);
        break;
    case VAL_DICT:
        for (size_t i = 0; i < v->dict_len; i++) {
            free(v->dict_keys[i]);
            val_free(v->dict_vals[i]);
        }
        free(v->dict_keys);
        free(v->dict_vals);
        break;
    case VAL_ARRAY:
        for (size_t i = 0; i < v->arr_len; i++)
            val_free(v->arr_items[i]);
        free(v->arr_items);
        break;
    default:
        break;
    }
    free(v);
}

static void append_string_bytes(dynbuf_t *out, const char *s) {
    size_t length = strlen(s);
    while ((length & 0xFFFFFF80) != 0) {
        uint8_t b = (uint8_t)((length & 0x7F) | 0x80);
        dynbuf_append_u8(out, b);
        length >>= 7;
    }
    dynbuf_append_u8(out, (uint8_t)(length & 0x7F));
    dynbuf_append(out, s, strlen(s));
}

bool val_encode(const value_t *v, dynbuf_t *out, bool dict_key) {
    if (!v) return false;
    switch (v->type) {
    case VAL_NULL:
        dynbuf_append_u8(out, 0);
        break;
    case VAL_BOOL:
        dynbuf_append_u8(out, 1);
        dynbuf_append_u8(out, v->u.b ? 1 : 0);
        break;
    case VAL_INT: {
        dynbuf_append_u8(out, 2);
        uint8_t raw[4] = {
            (uint8_t)(v->u.i32 >> 24), (uint8_t)(v->u.i32 >> 16),
            (uint8_t)(v->u.i32 >> 8), (uint8_t)v->u.i32
        };
        dynbuf_append(out, raw, 4);
        break;
    }
    case VAL_LONG: {
        dynbuf_append_u8(out, 3);
        uint8_t raw[8];
        int64_t n = v->u.i64;
        for (int i = 7; i >= 0; i--) { raw[i] = (uint8_t)(n & 0xFF); n >>= 8; }
        dynbuf_append(out, raw, 8);
        break;
    }
    case VAL_DOUBLE: {
        dynbuf_append_u8(out, 4);
        uint64_t bits;
        memcpy(&bits, &v->u.f64, sizeof(bits));
        uint8_t raw[8];
        for (int i = 7; i >= 0; i--) { raw[i] = (uint8_t)(bits & 0xFF); bits >>= 8; }
        dynbuf_append(out, raw, 8);
        break;
    }
    case VAL_STRING:
        if (!dict_key)
            dynbuf_append_u8(out, 5);
        append_string_bytes(out, v->u.str ? v->u.str : "");
        break;
    case VAL_DICT: {
        dynbuf_append_u8(out, 6);
        uint8_t cnt[4] = {
            (uint8_t)(v->dict_len >> 24), (uint8_t)(v->dict_len >> 16),
            (uint8_t)(v->dict_len >> 8), (uint8_t)v->dict_len
        };
        dynbuf_append(out, cnt, 4);
        for (size_t i = 0; i < v->dict_len; i++) {
            append_string_bytes(out, v->dict_keys[i] ? v->dict_keys[i] : "");
            val_encode(v->dict_vals[i], out, false);
        }
        break;
    }
    case VAL_ARRAY:
        dynbuf_append_u8(out, 7);
        return val_encode_array((const value_t **)v->arr_items, v->arr_len, out);
    case VAL_DATE: {
        dynbuf_append_u8(out, 8);
        uint8_t raw[8];
        int64_t n = v->u.date_ms;
        for (int i = 7; i >= 0; i--) { raw[i] = (uint8_t)(n & 0xFF); n >>= 8; }
        dynbuf_append(out, raw, 8);
        break;
    }
    default:
        return false;
    }
    return true;
}

bool val_encode_array(const value_t **items, size_t count, dynbuf_t *out) {
    uint8_t cnt[4] = {
        (uint8_t)(count >> 24), (uint8_t)(count >> 16),
        (uint8_t)(count >> 8), (uint8_t)count
    };
    dynbuf_append(out, cnt, 4);
    for (size_t i = 0; i < count; i++)
        if (!val_encode(items[i], out, false))
            return false;
    return true;
}

value_t *val_decode(buf_t *b) {
    int8_t tag = buf_read_i8(b);
    switch (tag) {
    case 0: return val_null();
    case 1: return val_bool(buf_read_i8(b) != 0);
    case 2: return val_int(buf_read_i32(b));
    case 3: {
        value_t *v = val_int(0);
        v->type = VAL_LONG;
        v->u.i64 = buf_read_i64(b);
        return v;
    }
    case 4: return val_double(buf_read_f64(b));
    case 5: {
        char *s = NULL;
        if (!buf_read_string(b, &s)) return NULL;
        value_t *v = val_string(s);
        free(s);
        return v;
    }
    case 6: {
        int32_t fields = buf_read_i32(b);
        value_t *d = val_dict(0);
        for (int32_t i = 0; i < fields; i++) {
            char *key = NULL;
            if (!buf_read_string(b, &key)) { val_free(d); return NULL; }
            value_t *val = val_decode(b);
            val_dict_set(d, key, val);
            free(key);
        }
        return d;
    }
    case 7: {
        int32_t len = buf_read_i32(b);
        value_t *a = val_array((size_t)len);
        for (int32_t i = 0; i < len; i++)
            val_array_set(a, (size_t)i, val_decode(b));
        return a;
    }
    case 8: {
        value_t *v = val_int(0);
        v->type = VAL_DATE;
        v->u.date_ms = buf_read_i64(b);
        return v;
    }
    default:
        return NULL;
    }
}

const char *val_as_string(const value_t *v) {
    if (v && v->type == VAL_STRING && v->u.str) return v->u.str;
    return "";
}

int64_t val_as_int(const value_t *v, int64_t def) {
    if (!v) return def;
    if (v->type == VAL_INT) return v->u.i32;
    if (v->type == VAL_LONG) return v->u.i64;
    if (v->type == VAL_BOOL) return v->u.b ? 1 : 0;
    return def;
}

value_t *val_dict_get(const value_t *d, const char *key) {
    if (!d || d->type != VAL_DICT) return NULL;
    for (size_t i = 0; i < d->dict_len; i++)
        if (d->dict_keys[i] && strcmp(d->dict_keys[i], key) == 0)
            return d->dict_vals[i];
    return NULL;
}
