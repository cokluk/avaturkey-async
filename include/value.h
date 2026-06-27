#ifndef AVATURKEY_VALUE_H
#define AVATURKEY_VALUE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "buf.h"

typedef enum {
    VAL_NULL = 0,
    VAL_BOOL,
    VAL_INT,
    VAL_LONG,
    VAL_DOUBLE,
    VAL_STRING,
    VAL_DICT,
    VAL_ARRAY,
    VAL_DATE
} val_type_t;

typedef struct value value_t;

struct value {
    val_type_t type;
    union {
        bool b;
        int32_t i32;
        int64_t i64;
        double f64;
        char *str;
        int64_t date_ms;
    } u;
    char **dict_keys;
    value_t **dict_vals;
    size_t dict_len;
    value_t **arr_items;
    size_t arr_len;
};

value_t *val_null(void);
value_t *val_bool(bool v);
value_t *val_int(int64_t v);
value_t *val_double(double v);
value_t *val_string(const char *s);
value_t *val_string_n(const char *s, size_t n);
value_t *val_dict(size_t n);
value_t *val_array(size_t n);
void     val_dict_set(value_t *d, const char *k, value_t *v);
void     val_array_set(value_t *a, size_t i, value_t *v);
void     val_free(value_t *v);

value_t *val_decode(buf_t *b);
bool     val_encode(const value_t *v, dynbuf_t *out, bool dict_key);
bool     val_encode_array(const value_t **items, size_t count, dynbuf_t *out);

const char *val_as_string(const value_t *v);
int64_t     val_as_int(const value_t *v, int64_t def);
value_t    *val_dict_get(const value_t *d, const char *key);

#endif
