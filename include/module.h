#ifndef AVATURKEY_MODULE_H
#define AVATURKEY_MODULE_H

#include <stddef.h>
#include "value.h"

struct client;
struct server;

typedef int (*module_handler_t)(struct server *s, struct client *c,
                                value_t **msg, size_t msg_len);

typedef struct module {
    const char *prefix;
    module_handler_t on_message;
} module_t;

void modules_register_all(struct server *s);
module_t *modules_find(struct server *s, const char *prefix);

#endif
