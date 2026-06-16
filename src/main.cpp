#include "server.h"
#include <iostream>
#include <csignal>
#include <cstdlib>

static Server* g_server = nullptr;

void signal_handler(int sig) {
    std::cout << "\n[KVStore] Shutting down (signal " << sig << ")...\n";
    if (g_server) g_server->stop();
    exit(0);
}

int main(int argc, char* argv[]) {
    int    port         = 6379;      // Same default as Redis
    size_t threads      = 8;
    size_t lru_capacity = 1000;

    if (argc >= 2) port         = std::atoi(argv[1]);
    if (argc >= 3) threads      = std::atoi(argv[2]);
    if (argc >= 4) lru_capacity = std::atoi(argv[3]);

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    Server server(port, threads, lru_capacity);
    g_server = &server;
    server.start();

    return 0;
}
