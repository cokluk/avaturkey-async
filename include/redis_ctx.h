#ifndef AVATURKEY_REDIS_CTX_H
#define AVATURKEY_REDIS_CTX_H

/*
 * Thin Redis client wrapper used as the game server's persistence layer.
 * Player data, auth keys, rooms, and inventory are stored in Redis;
 * game clients connect over TCP, not to Redis directly.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct redisContext;

typedef struct {
    struct redisContext *ctx;
} redis_ctx_t;

bool redis_ctx_connect(redis_ctx_t *r, const char *host, int port);
void redis_ctx_disconnect(redis_ctx_t *r);

char  *redis_get(redis_ctx_t *r, const char *key);
bool   redis_set(redis_ctx_t *r, const char *key, const char *val);
bool   redis_del(redis_ctx_t *r, const char *key);
bool   redis_sadd(redis_ctx_t *r, const char *key, const char *member);
bool   redis_srem(redis_ctx_t *r, const char *key, const char *member);
bool   redis_sismember(redis_ctx_t *r, const char *key, const char *member);
char **redis_smembers(redis_ctx_t *r, const char *key, size_t *out_count);
char **redis_lrange(redis_ctx_t *r, const char *key, long start, long stop, size_t *out_count);
bool   redis_incrby(redis_ctx_t *r, const char *key, int64_t delta);
void   redis_free_str(char *s);
void   redis_free_array(char **arr, size_t count);

#endif
