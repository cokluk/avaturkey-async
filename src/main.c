#include "server.h"
#include "const.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#ifdef _WIN32
#include <winsock2.h>
#endif

static server_t g_server;
static volatile int g_running = 1;

static void on_signal(int sig) {
    (void)sig;
    g_running = 0;
    g_server.running = false;
}

int main(void) {
    signal(SIGINT, on_signal);
#ifndef _WIN32
    signal(SIGTERM, on_signal);
#endif

    if (!server_init(&g_server)) {
        fprintf(stderr,
                "Failed to start game server: could not connect to Redis at %s:%d\n"
                "Redis is required as the persistence backend for player data.\n",
                REDIS_HOST, REDIS_PORT);
        return 1;
    }
    if (!server_listen(&g_server, "0.0.0.0", GAME_TCP_PORT)) {
        fprintf(stderr, "Failed to bind TCP game port %d\n", GAME_TCP_PORT);
        server_shutdown(&g_server);
        return 1;
    }
    printf("Avaturkey game server ready\n");
    printf("  TCP clients: 0.0.0.0:%d\n", GAME_TCP_PORT);
    printf("  Redis store: %s:%d\n", REDIS_HOST, REDIS_PORT);
    while (g_running)
        server_run(&g_server);
    server_shutdown(&g_server);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
