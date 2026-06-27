#include "server.h"
#include "client.h"
#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/types.h>
#define MKDIR(p) mkdir(p, 0755)
#endif

int mod_client_error_on_message(server_t *s, client_t *c, value_t **msg, size_t msg_len) {
    (void)s;
    if (msg_len < 3 || !msg[2] || msg[2]->type != VAL_DICT) return 0;
    value_t *logv = val_dict_get(msg[2], "log");
    value_t *msgv = val_dict_get(msg[2], "message");
    if (!c->uid) return 0;

    char dir[128];
    snprintf(dir, sizeof(dir), "errors/%s", c->uid);
    MKDIR("errors");
    MKDIR(dir);

    char filename[64];
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(filename, sizeof(filename), "%d.%m.%Y_%H:%M:%S.log", tm);

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", dir, filename);
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%s\r\n", val_as_string(msgv));
        fprintf(f, "%s", val_as_string(logv));
        fclose(f);
    }
    fprintf(stderr, "Client %s error logged\n", c->uid);
    return 0;
}
