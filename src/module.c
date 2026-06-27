#include "module.h"
#include "server.h"
#include <string.h>

extern int mod_client_error_on_message(server_t *s, client_t *c, value_t **msg, size_t msg_len);
extern int mod_confirm_on_message(server_t *s, client_t *c, value_t **msg, size_t msg_len);
extern int mod_stub_on_message(server_t *s, client_t *c, value_t **msg, size_t msg_len);

static const char *ALL_PREFIXES[] = {
    "a", "al", "b", "c", "ca", "cf", "chtdc", "clerr", "clmb", "cln", "cm",
    "cn", "cp", "cptch", "crq", "crt", "ctmr", "dscr", "ev", "fa20", "frn",
    "h", "hs", "lgr", "lg", "mail", "mb", "o", "pl", "prf", "psp", "q", "r",
    "rl", "sh", "spt", "stat", "tr", "ur", "vp", "w"
};

static module_handler_t handler_for(const char *prefix) {
    if (strcmp(prefix, "clerr") == 0) return mod_client_error_on_message;
    if (strcmp(prefix, "cf") == 0) return mod_confirm_on_message;
    return mod_stub_on_message;
}

void modules_register_all(server_t *s) {
    size_t n = sizeof(ALL_PREFIXES) / sizeof(ALL_PREFIXES[0]);
    for (size_t i = 0; i < n && s->module_count < 64; i++) {
        module_t *m = &s->modules[s->module_count++];
        m->prefix = ALL_PREFIXES[i];
        m->on_message = handler_for(ALL_PREFIXES[i]);
    }
}

module_t *modules_find(server_t *s, const char *prefix) {
    for (size_t i = 0; i < s->module_count; i++)
        if (strcmp(s->modules[i].prefix, prefix) == 0)
            return &s->modules[i];
    return NULL;
}
