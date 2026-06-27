#ifndef AVATURKEY_CLIENT_H
#define AVATURKEY_CLIENT_H

/*
 * Connected game client over TCP. Handles frame reassembly, cross-domain policy,
 * and outbound binary messages to the Flash client.
 */

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "value.h"

struct server;

typedef struct client {
    struct server *server;
    int fd;
    char addr[64];
    char *uid;
    bool drop;
    bool checksummed;
    char *room;
    double pos_x, pos_y;
    int dimension;
    int state;
    char action_tag[128];
    time_t last_msg;
    uint8_t *read_buf;
    size_t read_len;
    size_t read_cap;
} client_t;

client_t *client_create(struct server *s, int fd, const char *addr);
void      client_destroy(client_t *c);
void      client_on_data(client_t *c, const uint8_t *data, size_t len);
bool      client_send(client_t *c, int8_t type, const value_t **items, size_t count);
bool      client_send_simple(client_t *c, int8_t type, value_t *arr);
void      client_close(client_t *c);

#endif
