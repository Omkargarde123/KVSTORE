#include "server.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
#endif

// ─── helpers ────────────────────────────────
static std::vector<std::string> split(const std::string& s) {
    std::istringstream iss(s);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

static std::string to_upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

// ─── Server ctor ────────────────────────────
Server::Server(int port, size_t thread_count, size_t lru_capacity)
    : port(port)
    , server_fd(-1)
    , thread_count(thread_count)
    , lru_capacity(lru_capacity)
    , store(lru_capacity)
    , pool(thread_count)
{}

#ifdef _WIN32
static bool init_winsock() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

static void cleanup_winsock() {
    WSACleanup();
}

static void close_socket(socket_t sock) {
    closesocket(sock);
}

static bool socket_invalid(socket_t sock) {
    return sock == INVALID_SOCKET;
}
#else
static bool init_winsock() {
    return true;
}

static void cleanup_winsock() {}

static void close_socket(socket_t sock) {
    close(sock);
}

static bool socket_invalid(socket_t sock) {
    return sock < 0;
}
#endif

void Server::start() {
    if (!init_winsock()) {
        std::cerr << "Failed to initialize Winsock\n";
        return;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_invalid(server_fd)) { perror("socket"); cleanup_winsock(); return; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); close_socket(server_fd); cleanup_winsock(); return;
    }
    if (listen(server_fd, 128) < 0) {
        perror("listen"); close_socket(server_fd); cleanup_winsock(); return;
    }

    running.store(true);
    std::cout << "[KVStore] Server listening on port " << port << "\n";
    std::cout << "[KVStore] Thread pool size: " << thread_count << " | LRU capacity: " << lru_capacity << "\n";
    std::cout << "[KVStore] Commands: SET <k> <v> [EX <sec>] | GET <k> | DEL <k> | KEYS | SIZE | PING\n\n";

    // Background TTL purge thread
    std::thread([this] {
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            store.purge_expired();
        }
    }).detach();

    accept_loop();
}

void Server::stop() {
    running.store(false);
    close_socket(server_fd);
    cleanup_winsock();
}

void Server::accept_loop() {
    while (running.load()) {
        sockaddr_in client_addr{};
        socklen_t   client_len = sizeof(client_addr);
        socket_t client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (socket_invalid(client_fd)) {
            if (running.load()) perror("accept");
            continue;
        }
        char ip[INET_ADDRSTRLEN];
#ifdef _WIN32
        const char* addr_str = inet_ntoa(client_addr.sin_addr);
        strncpy(ip, addr_str ? addr_str : "0.0.0.0", sizeof(ip));
        ip[sizeof(ip) - 1] = '\0';
#else
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
#endif
        std::cout << "[+] Client connected: " << ip << "\n";

        pool.enqueue([this, client_fd] { handle_client(client_fd); });
    }
}

void Server::handle_client(socket_t client_fd) {
    char buf[4096];
    // Send welcome banner
    std::string banner = "KVStore v1.0  |  Commands: SET GET DEL KEYS SIZE PING\r\n> ";
    send(client_fd, banner.c_str(), banner.size(), 0);

    while (true) {
        memset(buf, 0, sizeof(buf));
        int n = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) break;

        std::string cmd(buf, n);
        // Strip \r\n
        cmd.erase(std::remove(cmd.begin(), cmd.end(), '\r'), cmd.end());
        cmd.erase(std::remove(cmd.begin(), cmd.end(), '\n'), cmd.end());
        if (cmd.empty()) { send(client_fd, "> ", 2, 0); continue; }

        std::string response = process_command(cmd) + "\r\n> ";
        send(client_fd, response.c_str(), response.size(), 0);
    }
    std::cout << "[-] Client disconnected\n";
    close_socket(client_fd);
}

std::string Server::process_command(const std::string& cmd) {
    auto tokens = split(cmd);
    if (tokens.empty()) return "ERR empty command";
    return parse_and_execute(tokens);
}

std::string Server::parse_and_execute(const std::vector<std::string>& t) {
    std::string cmd = to_upper(t[0]);

    // PING
    if (cmd == "PING") return "PONG";

    // GET <key>
    if (cmd == "GET") {
        if (t.size() < 2) return "ERR usage: GET <key>";
        std::string val = store.get(t[1]);
        return val == "-1" ? "(nil)" : val;
    }

    // SET <key> <value> [EX <seconds>]
    if (cmd == "SET") {
        if (t.size() < 3) return "ERR usage: SET <key> <value> [EX <seconds>]";
        int ttl = -1;
        if (t.size() >= 5 && to_upper(t[3]) == "EX") {
            try { ttl = std::stoi(t[4]); } catch (...) { return "ERR invalid TTL"; }
        }
        store.set(t[1], t[2], ttl);
        return "OK";
    }

    // DEL <key>
    if (cmd == "DEL") {
        if (t.size() < 2) return "ERR usage: DEL <key>";
        return store.del(t[1]) ? "(integer) 1" : "(integer) 0";
    }

    // KEYS
    if (cmd == "KEYS") return store.keys();

    // SIZE
    if (cmd == "SIZE") return "(integer) " + std::to_string(store.size());

    return "ERR unknown command '" + t[0] + "'";
}
