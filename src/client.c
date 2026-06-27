#include "client.h"
#include "server.h"
#include "protocol.h"
#include "const.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#define SEND(s, d, l) send(s, (const char *)(d), (int)(l), 0)
#define CLOSE(s) closesocket(s)
#else
#include <unistd.h>
#include <sys/socket.h>
#define SEND(s, d, l) send(s, d, l, 0)
#define CLOSE(s) close(s)
#endif

static const uint8_t POLICY_REQ[] = {
    0x3c, 0x70, 0x6f, 0x6c, 0x69, 0x63, 0x79, 0x2d,
    0x66, 0x69, 0x6c, 0x65, 0x2d, 0x72, 0x65, 0x71,
    0x75, 0x65, 0x73, 0x74, 0x2f, 0x3e, 0x00
};

client_t *client_create(server_t *s, int fd, const char *addr) {
    client_t *c = calloc(1, sizeof(client_t));
    if (!c) return NULL;
    c->server = s;
    c->fd = fd;
    c->checksummed = false;
    c->dimension = 4;
    c->last_msg = time(NULL);
    if (addr)
        strncpy(c->addr, addr, sizeof(c->addr) - 1);
    return c;
}

void client_destroy(client_t *c) {
    if (!c) return;
    free(c->uid);
    free(c->room);
    free(c->read_buf);
    CLOSE(c->fd);
    free(c);
}

static void client_remove_online(client_t *c) {
    server_t *s = c->server;
    if (!c->uid) return;
    if (c->uid)
        redis_srem(&s->redis, "online_tr", c->uid);
    char key[128];
    snprintf(key, sizeof(key), "uid:%s:lvt", c->uid);
    char ts[32];
    snprintf(ts, sizeof(ts), "%lld", (long long)time(NULL));
    redis_set(&s->redis, key, ts);
}

void client_close(client_t *c) {
    if (!c || c->drop) return;
    c->drop = true;
    client_remove_online(c);
    CLOSE(c->fd);
    c->fd = -1;
}

bool client_send(client_t *c, int8_t type, const value_t **items, size_t count) {
    if (!c || c->drop || c->fd < 0) return false;
    dynbuf_t out;
    dynbuf_init(&out);
    if (!protocol_build_message(type, items, count, &out, c->checksummed)) {
        dynbuf_free(&out);
        return false;
    }
    size_t sent = 0;
    while (sent < out.len) {
        int n = SEND(c->fd, out.data + sent, out.len - sent);
        if (n <= 0) {
            dynbuf_free(&out);
            client_close(c);
            return false;
        }
        sent += (size_t)n;
    }
    dynbuf_free(&out);
    return true;
}

bool client_send_simple(client_t *c, int8_t type, value_t *arr) {
    if (!arr || arr->type != VAL_ARRAY) return false;
    const value_t **items = (const value_t **)arr->arr_items;
    bool ok = client_send(c, type, items, arr->arr_len);
    return ok;
}

static void process_buffer(client_t *c) {
    buf_t b = { c->read_buf, c->read_len, 0 };

    if (c->read_len == sizeof(POLICY_REQ) &&
        memcmp(c->read_buf, POLICY_REQ, sizeof(POLICY_REQ)) == 0) {
        uint8_t resp[512];
        size_t n = CROSS_DOMAIN_XML_LEN;
        memcpy(resp, CROSS_DOMAIN_XML, n);
        resp[n++] = 0;
        SEND(c->fd, resp, n);
        c->read_len = 0;
        return;
    }

    while (c->read_len - b.pos > 4) {
        size_t save = b.pos;
        int32_t length = buf_read_i32(&b);
        if (length < 0 || (size_t)length > c->read_len - b.pos) {
            b.pos = save;
            break;
        }
        frame_t *frame = protocol_process_frame(b.data + b.pos, (size_t)length, true);
        b.pos += (size_t)length;
        if (!frame) {
            if (!c->uid) client_close(c);
            break;
        }
        server_process_data(c->server, c, frame->type, frame->msg);
        frame->msg = NULL;
        protocol_free_frame(frame);
        if (c->drop) break;
    }

    if (b.pos > 0) {
        size_t rem = c->read_len - b.pos;
        if (rem > 0)
            memmove(c->read_buf, c->read_buf + b.pos, rem);
        c->read_len = rem;
    }
}

void client_on_data(client_t *c, const uint8_t *data, size_t len) {
    if (c->drop) return;
    size_t need = c->read_len + len;
    if (need > c->read_cap) {
        size_t cap = c->read_cap ? c->read_cap : 4096;
        while (cap < need) cap *= 2;
        uint8_t *p = realloc(c->read_buf, cap);
        if (!p) return;
        c->read_buf = p;
        c->read_cap = cap;
    }
    memcpy(c->read_buf + c->read_len, data, len);
    c->read_len += len;
    process_buffer(c);
}
