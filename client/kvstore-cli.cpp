/*
 * kvstore-cli  — interactive client for KVStore server
 * Usage: ./kvstore-cli [host] [port]
 */
#include <iostream>
#include <string>
#include <cstring>
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using socket_t = SOCKET;
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
#else
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
  using socket_t = int;
  static bool init_winsock() { return true; }
  static void cleanup_winsock() {}
  static void close_socket(socket_t sock) { close(sock); }
#endif

int main(int argc, char* argv[]) {
    const char* host = argc >= 2 ? argv[1] : "127.0.0.1";
    int         port = argc >= 3 ? std::atoi(argv[2]) : 6379;

    if (!init_winsock()) {
        std::cerr << "Failed to initialize network stack\n";
        return 1;
    }

    socket_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == INVALID_SOCKET) { perror("socket"); cleanup_winsock(); return 1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
#ifdef _WIN32
    addr.sin_addr.s_addr = inet_addr(host);
#else
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        std::cerr << "Invalid address: " << host << "\n";
        close_socket(fd);
        cleanup_winsock();
        return 1;
    }
#endif

    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close_socket(fd);
        cleanup_winsock();
        return 1;
    }

    std::cout << "Connected to KVStore at " << host << ":" << port << "\n";

    char buf[4096];
    // Read banner from server
    memset(buf, 0, sizeof(buf));
    recv(fd, buf, sizeof(buf) - 1, 0);
    std::cout << buf;

    while (true) {
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line == "exit" || line == "quit") break;

        line += "\r\n";
        send(fd, line.c_str(), line.size(), 0);

        memset(buf, 0, sizeof(buf));
        recv(fd, buf, sizeof(buf) - 1, 0);
        std::cout << buf;
    }

    close_socket(fd);
    cleanup_winsock();
    return 0;
}
