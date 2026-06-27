#ifndef AVATURKEY_SERVER_H
#define AVATURKEY_SERVER_H

/*
 * Core TCP game server: accepts Flash/ActionScript clients on GAME_TCP_PORT,
 * decodes the binary protocol, routes commands to game modules, and reads/writes
 * persistent state through Redis.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "redis_ctx.h"
#include "module.h"
#include "value.h"

typedef struct client client_t;

#define SERVER_MAX_ONLINE 4096
#define SERVER_MAX_SLOTS  10000
#define SERVER_MAX_ROOMS  512

typedef struct {
    char name[128];
    char uids[SERVER_MAX_ONLINE][32];
    size_t uid_count;
} room_t;

typedef struct server {
    redis_ctx_t redis;
    client_t *clients[SERVER_MAX_ONLINE];
    size_t client_count;
    char slots[SERVER_MAX_SLOTS][32];
    size_t slot_count;
    room_t rooms[SERVER_MAX_ROOMS];
    size_t room_count;
    module_t modules[64];
    size_t module_count;
    int msgmeter[SERVER_MAX_ONLINE];
    char kicked[256][32];
    size_t kicked_count;
    int listen_fd;
    bool running;
} server_t;

bool server_init(server_t *s);
void server_shutdown(server_t *s);
bool server_listen(server_t *s, const char *host, int port);
void server_run(server_t *s);
void server_process_data(server_t *s, client_t *c, int8_t type, value_t *msg);
client_t *server_find_online(server_t *s, const char *uid);
bool server_get_user_data(server_t *s, const char *uid, value_t **out);
value_t *server_get_appearance(server_t *s, const char *uid);
room_t *server_find_room(server_t *s, const char *name);
room_t *server_get_or_create_room(server_t *s, const char *name);

#endif
