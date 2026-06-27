#include "server.h"
#include "client.h"
#include "protocol.h"
#include "const.h"
#include "module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

static int slot_has(server_t *s, const char *uid) {
    for (size_t i = 0; i < s->slot_count; i++)
        if (strcmp(s->slots[i], uid) == 0) return 1;
    return 0;
}

static void slot_add(server_t *s, const char *uid) {
    if (slot_has(s, uid)) return;
    if (s->slot_count >= SERVER_MAX_SLOTS) return;
    strncpy(s->slots[s->slot_count++], uid, 31);
}

client_t *server_find_online(server_t *s, const char *uid) {
    for (size_t i = 0; i < s->client_count; i++)
        if (s->clients[i]->uid && strcmp(s->clients[i]->uid, uid) == 0)
            return s->clients[i];
    return NULL;
}

room_t *server_find_room(server_t *s, const char *name) {
    for (size_t i = 0; i < s->room_count; i++)
        if (strcmp(s->rooms[i].name, name) == 0)
            return &s->rooms[i];
    return NULL;
}

room_t *server_get_or_create_room(server_t *s, const char *name) {
    room_t *r = server_find_room(s, name);
    if (r) return r;
    if (s->room_count >= SERVER_MAX_ROOMS) return NULL;
    r = &s->rooms[s->room_count++];
    memset(r, 0, sizeof(*r));
    strncpy(r->name, name, sizeof(r->name) - 1);
    return r;
}

value_t *server_get_appearance(server_t *s, const char *uid) {
    char key[128];
    snprintf(key, sizeof(key), "uid:%s:appearance", uid);
    size_t n = 0;
    char **items = redis_lrange(&s->redis, key, 0, -1, &n);
    if (!items || n < 25) {
        redis_free_array(items, n);
        return NULL;
    }
    const char *fields[] = {
        "n","nct","g","sc","ht","hc","brt","brc","et","ec","fft","fat","fac",
        "ss","ssc","mt","mc","sh","shc","rg","rc","pt","pc","bt","bc"
    };
    value_t *d = val_dict(25);
    for (size_t i = 0; i < 25 && i < n; i++) {
        if (i == 0)
            val_dict_set(d, fields[i], val_string(items[i] ? items[i] : ""));
        else
            val_dict_set(d, fields[i], val_int(items[i] ? atoll(items[i]) : 0));
    }
    redis_free_array(items, n);
    return d;
}

bool server_get_user_data(server_t *s, const char *uid, value_t **out) {
    char key[128];
    const char *suffix[] = {"slvr","enrg","gld","exp","emd","lvt","trid","crt","hrt","act","role","premium"};
    value_t *d = val_dict(12);
    bool ok = false;
    for (size_t i = 0; i < 12; i++) {
        snprintf(key, sizeof(key), "uid:%s:%s", uid, suffix[i]);
        char *v = redis_get(&s->redis, key);
        if (i == 0 && !v) {
            val_free(d);
            redis_free_str(v);
            return false;
        }
        val_dict_set(d, suffix[i], val_int(v ? atoll(v) : 0));
        redis_free_str(v);
        ok = true;
    }
    *out = d;
    return ok;
}

static void server_auth(server_t *s, client_t *c, value_t *msg) {
    if (!msg || msg->type != VAL_ARRAY || msg->arr_len < 3) {
        client_close(c);
        return;
    }
    const char *key = val_as_string(msg->arr_items[2]);
    char rkey[256];
    snprintf(rkey, sizeof(rkey), "auth:%s", key);
    char *uid = redis_get(&s->redis, rkey);
    if (!uid) {
        value_t *a0 = val_int(5);
        value_t *a1 = val_string("Key is invalid");
        value_t *a2 = val_dict(0);
        const value_t *items[] = { a0, a1, a2 };
        client_send(c, 2, items, 3);
        val_free(a0); val_free(a1); val_free(a2);
        client_close(c);
        return;
    }

    snprintf(rkey, sizeof(rkey), "uid:%s:banned", uid);
    char *banned = redis_get(&s->redis, rkey);
    if (banned) {
        value_t *a0 = val_int(10);
        value_t *a1 = val_string("User is banned");
        value_t *a2 = val_dict(0);
        const value_t *items[] = { a0, a1, a2 };
        client_send(c, 2, items, 3);
        val_free(a0); val_free(a1); val_free(a2);
        redis_free_str(banned);
        redis_free_str(uid);
        client_close(c);
        return;
    }
    redis_free_str(banned);

    client_t *existing = server_find_online(s, uid);
    if (existing) {
        value_t *a0 = val_int(6);
        value_t *a1 = val_dict(0);
        const value_t *items[] = { a0, a1 };
        client_send(existing, 3, items, 2);
        val_free(a0); val_free(a1);
        client_close(existing);
    }

    value_t *user_data = NULL;
    if (!server_get_user_data(s, uid, &user_data)) {
        char buf[32];
        const char *init_keys[] = {"slvr","gld","enrg","exp","emd","lvt"};
        const char *init_vals[] = {"1000","6","100","493500","0","0"};
        for (size_t i = 0; i < 6; i++) {
            snprintf(rkey, sizeof(rkey), "uid:%s:%s", uid, init_keys[i]);
            redis_set(&s->redis, rkey, init_vals[i]);
        }
        server_get_user_data(s, uid, &user_data);
    }

    if (s->slot_count >= MAX_SLOTS && !slot_has(s, uid)) {
        value_t *a0 = val_int(10);
        value_t *a1 = val_string("User is banned");
        value_t *a2 = val_dict(0);
        const value_t *items[] = { a0, a1, a2 };
        client_send(c, 2, items, 3);
        val_free(a0); val_free(a1); val_free(a2);
        val_free(user_data);
        redis_free_str(uid);
        client_close(c);
        return;
    }

    slot_add(s, uid);
    c->uid = uid;

    snprintf(rkey, sizeof(rkey), "uid:%s:lvt", uid);
    char ts[32];
    snprintf(ts, sizeof(ts), "%lld", (long long)time(NULL));
    redis_set(&s->redis, rkey, ts);

    snprintf(rkey, sizeof(rkey), "uid:%s:ip", uid);
    redis_set(&s->redis, rkey, c->addr);

    value_t *a0 = val_string(uid);
    value_t *a1 = val_string("");
    value_t *a2 = val_bool(true);
    value_t *a3 = val_bool(false);
    value_t *a4 = val_bool(false);
    const value_t *items[] = { a0, a1, a2, a3, a4 };
    client_send(c, 1, items, 5);
    val_free(a0); val_free(a1); val_free(a2); val_free(a3); val_free(a4);
    val_free(user_data);

    c->checksummed = true;
    redis_sadd(&s->redis, "online_tr", uid);
}

void server_process_data(server_t *s, client_t *c, int8_t type, value_t *msg) {
    if (!c->uid) {
        if (type != 1) {
            client_close(c);
            return;
        }
        server_auth(s, c, msg);
        return;
    }
    if (type == 2) {
        client_close(c);
        return;
    }
    if (type == 17 && msg && msg->type == VAL_ARRAY && msg->arr_len > 0) {
        value_t *a0 = msg->arr_items[0];
        value_t *a1 = val_string(c->uid);
        const value_t *items[] = { a0, a1 };
        client_send(c, 17, items, 2);
        val_free(a1);
        return;
    }
    if (type != 34 || !msg || msg->type != VAL_ARRAY || msg->arr_len < 2)
        return;

    if (c->uid) {
        for (size_t i = 0; i < s->client_count; i++) {
            if (s->clients[i] == c) {
                s->msgmeter[i]++;
                if (s->msgmeter[i] > MSG_LIMIT) {
                    value_t *m0 = val_string("cp.ms.rsm");
                    value_t *m1 = val_dict(0);
                    val_dict_set(m1, "txt", val_string("You were kicked for exceeding message limits."));
                    const value_t *warn[] = { m0, m1 };
                    client_send(c, 34, warn, 2);
                    val_free(m0); val_free(m1);
                    client_close(c);
                    return;
                }
                break;
            }
        }
    }
    c->last_msg = time(NULL);

    const char *cmd = val_as_string(msg->arr_items[1]);
    if (strcmp(cmd, "clerr") == 0)
        return;

    char prefix[32] = "";
    const char *dot = strchr(cmd, '.');
    if (dot) {
        size_t len = (size_t)(dot - cmd);
        if (len >= sizeof(prefix)) len = sizeof(prefix) - 1;
        memcpy(prefix, cmd, len);
        prefix[len] = '\0';
    }

    module_t *mod = modules_find(s, prefix);
    if (!mod) {
        fprintf(stderr, "Command %s not found\n", cmd);
        return;
    }
    mod->on_message(s, c, msg->arr_items, msg->arr_len);
}

bool server_init(server_t *s) {
    memset(s, 0, sizeof(*s));
    s->listen_fd = -1;
    s->running = true;
    if (!redis_ctx_connect(&s->redis, REDIS_HOST, REDIS_PORT))
        return false;
    modules_register_all(s);
    return true;
}

void server_shutdown(server_t *s) {
    s->running = false;
    for (size_t i = 0; i < s->client_count; i++) {
        client_t *c = s->clients[i];
        value_t *a0 = val_int(6);
        value_t *a1 = val_string("Restart");
        value_t *a2 = val_dict(0);
        const value_t *items[] = { a0, a1, a2 };
        client_send(c, 2, items, 3);
        val_free(a0); val_free(a1); val_free(a2);
    }
    if (s->listen_fd >= 0) {
#ifdef _WIN32
        closesocket(s->listen_fd);
#else
        close(s->listen_fd);
#endif
    }
    redis_ctx_disconnect(&s->redis);
}

bool server_listen(server_t *s, const char *host, int port) {
    (void)host;
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    s->listen_fd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (s->listen_fd < 0) return false;
    int opt = 1;
    setsockopt(s->listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);
    if (bind(s->listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        return false;
    if (listen(s->listen_fd, 128) < 0)
        return false;
    return true;
}

void server_run(server_t *s) {
    fd_set rfds;
    while (s->running) {
        FD_ZERO(&rfds);
        FD_SET((SOCKET)s->listen_fd, &rfds);
        int maxfd = s->listen_fd;
        for (size_t i = 0; i < s->client_count; i++) {
            int fd = s->clients[i]->fd;
            if (fd >= 0) {
                FD_SET((SOCKET)fd, &rfds);
                if (fd > maxfd) maxfd = fd;
            }
        }
        struct timeval tv = { 0, 200000 };
        int n = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (n < 0) break;
        if (FD_ISSET((SOCKET)s->listen_fd, &rfds)) {
            struct sockaddr_in cli;
            socklen_t cl = sizeof(cli);
            int fd = (int)accept(s->listen_fd, (struct sockaddr *)&cli, &cl);
            if (fd >= 0) {
                char addr[64];
                snprintf(addr, sizeof(addr), "%s", inet_ntoa(cli.sin_addr));
                client_t *c = client_create(s, fd, addr);
                if (c && s->client_count < SERVER_MAX_ONLINE)
                    s->clients[s->client_count++] = c;
            }
        }
        for (size_t i = 0; i < s->client_count; ) {
            client_t *c = s->clients[i];
            if (c->fd < 0 || c->drop) {
                client_destroy(c);
                s->clients[i] = s->clients[s->client_count - 1];
                s->client_count--;
                continue;
            }
            if (FD_ISSET((SOCKET)c->fd, &rfds)) {
                uint8_t buf[4096];
#ifdef _WIN32
                int r = recv(c->fd, (char *)buf, sizeof(buf), 0);
#else
                ssize_t r = recv(c->fd, buf, sizeof(buf), 0);
#endif
                if (r <= 0) {
                    client_close(c);
                } else {
                    client_on_data(c, buf, (size_t)r);
                }
            }
            i++;
        }
    }
}
