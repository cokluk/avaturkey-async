#ifndef AVATURKEY_CONST_H
#define AVATURKEY_CONST_H

#include <stddef.h>

#define GAME_TCP_PORT    4333
#define REDIS_HOST       "127.0.0.1"
#define REDIS_PORT       6379
#define MAX_SLOTS        10000
#define MSG_LIMIT        5000
#define AFK_TIMEOUT_SEC  420
#define BG_INTERVAL_SEC  600

/* Legacy alias */
#define SERVER_PORT GAME_TCP_PORT

extern const char CROSS_DOMAIN_XML[];
extern const size_t CROSS_DOMAIN_XML_LEN;

#endif
