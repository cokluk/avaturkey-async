#include "protocol.h"
#include <stdlib.h>
#include <string.h>

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len) {
    crc = ~crc;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xEDB88320u & (~(crc & 1u) + 1u));
    }
    return ~crc;
}

frame_t *protocol_process_frame(const uint8_t *data, size_t len, bool client_mode) {
    buf_t b = { data, len, 0 };
    uint8_t mask = buf_read_u8(&b);
    bool checksummed = (mask & (1 << 3)) != 0;
    if (checksummed) {
        uint32_t checksum = buf_read_u32(&b);
        size_t msg_len = len - b.pos;
        uint32_t real = crc32_update(0, data + b.pos, msg_len);
        if (checksum != real)
            return NULL;
    }
    if (client_mode)
        b.pos += 4;
    int8_t type = buf_read_i8(&b);
    value_t *arr = val_decode(&b);
    if (!arr || arr->type != VAL_ARRAY) {
        val_free(arr);
        return NULL;
    }
    frame_t *f = malloc(sizeof(frame_t));
    if (!f) { val_free(arr); return NULL; }
    f->type = type;
    f->msg = arr;
    return f;
}

void protocol_free_frame(frame_t *f) {
    if (!f) return;
    val_free(f->msg);
    free(f);
}

bool protocol_build_message(int8_t type, const value_t **items, size_t count,
                            dynbuf_t *out, bool checksummed) {
    dynbuf_t body;
    dynbuf_init(&body);
    dynbuf_append_u8(&body, (uint8_t)type);
    if (!val_encode_array(items, count, &body)) {
        dynbuf_free(&body);
        return false;
    }

    uint8_t mask = 0;
    size_t header_len = 1;
    if (checksummed) {
        mask |= (1 << 3);
        header_len += 4;
    }

    dynbuf_t frame;
    dynbuf_init(&frame);
    dynbuf_append_u8(&frame, mask);
    if (checksummed) {
        uint32_t crc = crc32_update(0, body.data, body.len);
        uint8_t raw[4] = {
            (uint8_t)(crc >> 24), (uint8_t)(crc >> 16),
            (uint8_t)(crc >> 8), (uint8_t)crc
        };
        dynbuf_append(&frame, raw, 4);
    }
    dynbuf_append(&frame, body.data, body.len);
    dynbuf_free(&body);

    uint32_t total = (uint32_t)(frame.len + header_len);
    uint8_t len_raw[4] = {
        (uint8_t)(total >> 24), (uint8_t)(total >> 16),
        (uint8_t)(total >> 8), (uint8_t)total
    };
    dynbuf_append(out, len_raw, 4);
    dynbuf_append(out, frame.data, frame.len);
    dynbuf_free(&frame);
    return true;
}
