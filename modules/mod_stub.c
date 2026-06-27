#include "server.h"
#include "client.h"
#include "common.h"
#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static value_t *build_clths(server_t *s, const char *uid) {
    (void)s;
    value_t *clths = val_dict(0);
    val_dict_set(clths, "cc", val_string("casual"));
    value_t *ccltns = val_dict(0);
    value_t *casual = val_dict(0);
    val_dict_set(casual, "cct", val_array(0));
    val_dict_set(casual, "cn", val_string(""));
    val_dict_set(casual, "ctp", val_string("casual"));
    val_dict_set(ccltns, "casual", casual);
    val_dict_set(clths, "ccltns", ccltns);
    (void)uid;
    return clths;
}

static value_t *build_plr(server_t *s, client_t *c) {
    value_t *plr = val_dict(0);
    value_t *apprnc = server_get_appearance(s, c->uid);
    if (!apprnc) return NULL;

    val_dict_set(plr, "uid", val_string(c->uid));
    val_dict_set(plr, "apprnc", apprnc);
    val_dict_set(plr, "clths", build_clths(s, c->uid));

    value_t *mbm = val_dict(0);
    val_dict_set(mbm, "ac", val_null());
    val_dict_set(mbm, "sk", val_string("blackMobileSkin"));
    val_dict_set(mbm, "rt", val_null());
    val_dict_set(plr, "mbm", mbm);

    value_t *usrinf = val_dict(0);
    val_dict_set(usrinf, "rl", val_int(0));
    val_dict_set(usrinf, "sid", val_string(c->uid));
    val_dict_set(plr, "usrinf", usrinf);

    value_t *locinfo = val_dict(0);
    val_dict_set(locinfo, "st", val_int(c->state));
    val_dict_set(locinfo, "s", val_string("127.0.0.1"));
    val_dict_set(locinfo, "at", val_string(c->action_tag));
    val_dict_set(locinfo, "d", val_int(c->dimension));
    val_dict_set(locinfo, "x", val_double(c->pos_x));
    val_dict_set(locinfo, "y", val_double(c->pos_y));
    val_dict_set(locinfo, "shlc", val_bool(true));
    val_dict_set(locinfo, "pl", val_string(""));
    val_dict_set(locinfo, "l", val_string(c->room ? c->room : ""));
    val_dict_set(plr, "locinfo", locinfo);

    value_t *user_data = NULL;
    server_get_user_data(s, c->uid, &user_data);
    value_t *ci = val_dict(0);
    if (user_data) {
        val_dict_set(ci, "exp", val_int(val_as_int(val_dict_get(user_data, "exp"), 0)));
        val_dict_set(ci, "crt", val_int(val_as_int(val_dict_get(user_data, "crt"), 0)));
        val_dict_set(ci, "hrt", val_int(val_as_int(val_dict_get(user_data, "hrt"), 0)));
        val_dict_set(ci, "actrt", val_int(val_as_int(val_dict_get(user_data, "act"), 0)));
        val_dict_set(ci, "lvt", val_int(val_as_int(val_dict_get(user_data, "lvt"), 0)));
        val_free(user_data);
    }
    val_dict_set(ci, "vip", val_bool(true));
    val_dict_set(plr, "ci", ci);
    val_dict_set(plr, "clif", val_null());
    return plr;
}

static void join_room(server_t *s, client_t *c, const char *room, const char *prefix) {
    if (c->room) {
        room_t *old = server_find_room(s, c->room);
        if (old) {
            for (size_t i = 0; i < old->uid_count; ) {
                if (strcmp(old->uids[i], c->uid) == 0) {
                    old->uid_count--;
                    if (i < old->uid_count)
                        memcpy(old->uids[i], old->uids[old->uid_count], 32);
                } else {
                    i++;
                }
            }
        }
        free(c->room);
        c->room = NULL;
    }
    room_t *r = server_get_or_create_room(s, room);
    if (!r) return;
    if (r->uid_count < SERVER_MAX_ONLINE)
        strncpy(r->uids[r->uid_count++], c->uid, 31);
    c->room = strdup(room);
    c->pos_x = -1.0;
    c->pos_y = -1.0;
    c->action_tag[0] = '\0';
    c->state = 0;
    c->dimension = 4;

    value_t *plr = build_plr(s, c);
    if (!plr) return;
    for (size_t i = 0; i < r->uid_count; i++) {
        client_t *other = server_find_online(s, r->uids[i]);
        if (!other) continue;
        value_t *a0 = val_string(NULL);
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "%s.r.jn", prefix);
        val_free(a0);
        a0 = val_string(cmd);
        value_t *a1 = val_dict(0);
        val_dict_set(a1, "plr", plr);
        val_dict_set(a1, "cc", val_null());
        const value_t *items[] = { a0, a1 };
        client_send(other, 34, items, 2);
        val_free(a0); val_free(a1);
        if (other != c) {
            value_t *opl = build_plr(s, other);
            if (opl) {
                value_t *b0 = val_string(cmd);
                value_t *b1 = val_dict(0);
                val_dict_set(b1, "plr", opl);
                val_dict_set(b1, "cc", val_null());
                const value_t *items2[] = { b0, b1 };
                client_send(c, 34, items2, 2);
                val_free(b0); val_free(b1);
            }
        }
        value_t *t0 = val_string(room);
        value_t *t1 = val_string(c->uid);
        const value_t *titems[] = { t0, t1 };
        client_send(other, 16, titems, 2);
        val_free(t0); val_free(t1);
    }
    val_free(plr);
}

static void handle_h(server_t *s, client_t *c, const char *sub, value_t **msg, size_t msg_len) {
    if (strcmp(sub, "minfo") == 0) {
        if (msg_len >= 3 && msg[2]->type == VAL_DICT) {
            value_t *onl = val_dict_get(msg[2], "onl");
            if (onl && val_as_int(onl, 0))
                return;
        }
        value_t *apprnc = server_get_appearance(s, c->uid);
        if (!apprnc) {
            value_t *a0 = val_string("h.minfo");
            value_t *a1 = val_dict(0);
            val_dict_set(a1, "has.avtr", val_bool(false));
            const value_t *items[] = { a0, a1 };
            client_send(c, 34, items, 2);
            val_free(a0); val_free(a1);
            val_free(apprnc);
            return;
        }
        val_free(apprnc);
        value_t *plr = build_plr(s, c);
        value_t *a0 = val_string("h.minfo");
        value_t *a1 = val_dict(0);
        val_dict_set(a1, "plr", plr);
        val_dict_set(a1, "tm", val_int(1));
        val_dict_set(a1, "onl", val_bool(true));
        const value_t *items[] = { a0, a1 };
        client_send(c, 34, items, 2);
        val_free(a0); val_free(a1);
    } else if (strcmp(sub, "gr") == 0 && msg_len >= 3) {
        value_t *d = msg[2];
        const char *lid = val_as_string(val_dict_get(d, "lid"));
        const char *gid = val_as_string(val_dict_get(d, "gid"));
        const char *rid = val_as_string(val_dict_get(d, "rid"));
        char room[128];
        snprintf(room, sizeof(room), "%s_%s_%s", lid, gid, rid);
        join_room(s, c, room, "h");
        value_t *a0 = val_string("h.gr");
        value_t *a1 = val_dict(0);
        val_dict_set(a1, "rid", val_string(c->room ? c->room : room));
        const value_t *items[] = { a0, a1 };
        client_send(c, 34, items, 2);
        val_free(a0); val_free(a1);
    }
}

static void handle_o(server_t *s, client_t *c, const char *sub, value_t **msg, size_t msg_len) {
    if (strcmp(sub, "gr") == 0 && msg_len >= 3) {
        value_t *d = msg[2];
        const char *lid = val_as_string(val_dict_get(d, "lid"));
        const char *gid = val_as_string(val_dict_get(d, "gid"));
        char room[128];
        snprintf(room, sizeof(room), "%s_%s", lid, gid);
        join_room(s, c, room, "o");
        value_t *a0 = val_string("o.gr");
        value_t *a1 = val_dict(0);
        val_dict_set(a1, "rid", val_string(c->room ? c->room : room));
        const value_t *items[] = { a0, a1 };
        client_send(c, 34, items, 2);
        val_free(a0); val_free(a1);
    } else if (strcmp(sub, "info") == 0) {
        value_t *a0 = val_string("o.r.info");
        value_t *a1 = val_dict(0);
        const value_t *items[] = { a0, a1 };
        client_send(c, 34, items, 2);
        val_free(a0); val_free(a1);
    }
}

static void handle_dscr(server_t *s, client_t *c, const char *sub) {
    (void)s;
    if (strcmp(sub, "ldd") != 0) return;
    value_t *a0 = val_string("dscr.ldd");
    value_t *a1 = val_dict(0);
    val_dict_set(a1, "ol", val_array(0));
    const value_t *items[] = { a0, a1 };
    client_send(c, 34, items, 2);
    val_free(a0); val_free(a1);
}

int mod_stub_on_message(server_t *s, client_t *c, value_t **msg, size_t msg_len) {
    if (msg_len < 2) return 0;
    const char *cmd = val_as_string(msg[1]);
    const char *dot1 = strchr(cmd, '.');
    if (!dot1) return 0;
    const char *dot2 = strchr(dot1 + 1, '.');
    char sub[32] = "";
    if (dot2) {
        size_t len = (size_t)(dot2 - dot1 - 1);
        if (len >= sizeof(sub)) len = sizeof(sub) - 1;
        memcpy(sub, dot1 + 1, len);
        sub[len] = '\0';
    }

    char prefix[16] = "";
    size_t plen = (size_t)(dot1 - cmd);
    if (plen >= sizeof(prefix)) plen = sizeof(prefix) - 1;
    memcpy(prefix, cmd, plen);
    prefix[plen] = '\0';

    if (strcmp(prefix, "h") == 0) handle_h(s, c, sub, msg, msg_len);
    else if (strcmp(prefix, "o") == 0) handle_o(s, c, sub, msg, msg_len);
    else if (strcmp(prefix, "dscr") == 0) handle_dscr(s, c, sub);
    else
        fprintf(stderr, "Unhandled game command: %s\n", cmd);
    return 0;
}
