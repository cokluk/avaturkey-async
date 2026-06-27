#include "redis_ctx.h"
#include <hiredis.h>
#include <stdlib.h>
#include <string.h>

static char *xstrndup(const char *s, size_t n) {
    char *p = malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

bool redis_ctx_connect(redis_ctx_t *r, const char *host, int port) {
    r->ctx = redisConnect(host, port);
    if (!r->ctx || r->ctx->err) {
        if (r->ctx) redisFree(r->ctx);
        r->ctx = NULL;
        return false;
    }
    return true;
}

void redis_ctx_disconnect(redis_ctx_t *r) {
    if (r->ctx) {
        redisFree(r->ctx);
        r->ctx = NULL;
    }
}

static char *copy_reply_str(redisReply *reply) {
    if (!reply || reply->type != REDIS_REPLY_STRING || !reply->str)
        return NULL;
    return xstrndup(reply->str, reply->len);
}

char *redis_get(redis_ctx_t *r, const char *key) {
    redisReply *reply = redisCommand(r->ctx, "GET %s", key);
    char *s = copy_reply_str(reply);
    freeReplyObject(reply);
    return s;
}

bool redis_set(redis_ctx_t *r, const char *key, const char *val) {
    redisReply *reply = redisCommand(r->ctx, "SET %s %s", key, val);
    bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    return ok;
}

bool redis_del(redis_ctx_t *r, const char *key) {
    redisReply *reply = redisCommand(r->ctx, "DEL %s", key);
    bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    return ok;
}

bool redis_sadd(redis_ctx_t *r, const char *key, const char *member) {
    redisReply *reply = redisCommand(r->ctx, "SADD %s %s", key, member);
    bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    return ok;
}

bool redis_srem(redis_ctx_t *r, const char *key, const char *member) {
    redisReply *reply = redisCommand(r->ctx, "SREM %s %s", key, member);
    bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    return ok;
}

bool redis_sismember(redis_ctx_t *r, const char *key, const char *member) {
    redisReply *reply = redisCommand(r->ctx, "SISMEMBER %s %s", key, member);
    bool ok = reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 1;
    freeReplyObject(reply);
    return ok;
}

char **redis_smembers(redis_ctx_t *r, const char *key, size_t *out_count) {
    *out_count = 0;
    redisReply *reply = redisCommand(r->ctx, "SMEMBERS %s", key);
    if (!reply || reply->type != REDIS_REPLY_ARRAY) {
        freeReplyObject(reply);
        return NULL;
    }
    char **arr = calloc(reply->elements, sizeof(char *));
    if (!arr) { freeReplyObject(reply); return NULL; }
    for (size_t i = 0; i < reply->elements; i++) {
        if (reply->element[i]->type == REDIS_REPLY_STRING)
            arr[i] = xstrndup(reply->element[i]->str, reply->element[i]->len);
    }
    *out_count = reply->elements;
    freeReplyObject(reply);
    return arr;
}

char **redis_lrange(redis_ctx_t *r, const char *key, long start, long stop, size_t *out_count) {
    *out_count = 0;
    redisReply *reply = redisCommand(r->ctx, "LRANGE %s %ld %ld", key, start, stop);
    if (!reply || reply->type != REDIS_REPLY_ARRAY) {
        freeReplyObject(reply);
        return NULL;
    }
    char **arr = calloc(reply->elements, sizeof(char *));
    if (!arr) { freeReplyObject(reply); return NULL; }
    for (size_t i = 0; i < reply->elements; i++) {
        if (reply->element[i]->type == REDIS_REPLY_STRING)
            arr[i] = xstrndup(reply->element[i]->str, reply->element[i]->len);
    }
    *out_count = reply->elements;
    freeReplyObject(reply);
    return arr;
}

bool redis_incrby(redis_ctx_t *r, const char *key, int64_t delta) {
    redisReply *reply = redisCommand(r->ctx, "INCRBY %s %lld", key, (long long)delta);
    bool ok = reply && reply->type != REDIS_REPLY_ERROR;
    freeReplyObject(reply);
    return ok;
}

void redis_free_str(char *s) { free(s); }

void redis_free_array(char **arr, size_t count) {
    if (!arr) return;
    for (size_t i = 0; i < count; i++)
        free(arr[i]);
    free(arr);
}
