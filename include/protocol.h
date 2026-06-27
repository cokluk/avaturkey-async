#ifndef AVATURKEY_PROTOCOL_H
#define AVATURKEY_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "buf.h"
#include "value.h"

typedef struct {
    int8_t type;
    value_t *msg;
} frame_t;

frame_t *protocol_process_frame(const uint8_t *data, size_t len, bool client_mode);
void     protocol_free_frame(frame_t *f);
bool     protocol_build_message(int8_t type, const value_t **items, size_t count,
                              dynbuf_t *out, bool checksummed);

#endif
