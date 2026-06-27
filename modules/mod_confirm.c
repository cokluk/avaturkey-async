#include "server.h"
#include "client.h"
#include "value.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    char uid[32];
    char at[64];
    char target[32];
    int completed;
} confirm_t;

static confirm_t confirms[256];
static size_t confirm_count;

static confirm_t *find_confirm(const char *uid) {
    for (size_t i = 0; i < confirm_count; i++)
        if (strcmp(confirms[i].uid, uid) == 0)
            return &confirms[i];
    return NULL;
}

int mod_confirm_on_message(server_t *s, client_t *c, value_t **msg, size_t msg_len) {
    if (msg_len < 3) return 0;
    const char *cmd = val_as_string(msg[1]);
    const char *sub = strchr(cmd, '.');
    if (!sub) return 0;
    sub++;

    if (strcmp(sub, "uc") == 0 && msg[2]->type == VAL_DICT) {
        const char *target = val_as_string(val_dict_get(msg[2], "uid"));
        const char *at = val_as_string(val_dict_get(msg[2], "at"));
        if (!c->uid || strcmp(target, c->uid) == 0) return 0;
        client_t *other = server_find_online(s, target);
        if (!other) return 0;
        if (confirm_count < 256) {
            confirm_t *cf = &confirms[confirm_count++];
            strncpy(cf->uid, c->uid, 31);
            strncpy(cf->target, target, 31);
            strncpy(cf->at, at, 63);
            cf->completed = 0;
        }
        value_t *a0 = val_string("cf.uc");
        value_t *a1 = val_dict(0);
        val_dict_set(a1, "uid", val_string(c->uid));
        val_dict_set(a1, "at", val_string(at));
        const value_t *items[] = { a0, a1 };
        client_send(other, 34, items, 2);
        val_free(a0); val_free(a1);
    } else if (strcmp(sub, "uca") == 0 && msg[2]->type == VAL_DICT) {
        const char *from = val_as_string(val_dict_get(msg[2], "uid"));
        const char *at = val_as_string(val_dict_get(msg[2], "at"));
        client_t *other = server_find_online(s, from);
        confirm_t *cf = find_confirm(from);
        if (!other || !cf || strcmp(cf->at, at) != 0 || strcmp(cf->target, c->uid) != 0)
            return 0;
        cf->completed = 1;
        value_t *a0 = val_string("cf.uca");
        value_t *a1 = val_dict(0);
        val_dict_set(a1, "uid", val_string(c->uid));
        val_dict_set(a1, "at", val_string(at));
        const value_t *items[] = { a0, a1 };
        client_send(other, 34, items, 2);
        val_free(a0); val_free(a1);
    } else if (strcmp(sub, "ucd") == 0 && msg[2]->type == VAL_DICT) {
        const char *from = val_as_string(val_dict_get(msg[2], "uid"));
        const char *at = val_as_string(val_dict_get(msg[2], "at"));
        client_t *other = server_find_online(s, from);
        confirm_t *cf = find_confirm(from);
        if (!other || !cf || strcmp(cf->at, at) != 0 || strcmp(cf->target, c->uid) != 0)
            return 0;
        *cf = confirms[--confirm_count];
        value_t *a0 = val_string("cf.ucd");
        value_t *a1 = val_dict(0);
        val_dict_set(a1, "uid", val_string(c->uid));
        val_dict_set(a1, "at", val_string(at));
        const value_t *items[] = { a0, a1 };
        client_send(other, 34, items, 2);
        val_free(a0); val_free(a1);
    }
    return 0;
}
