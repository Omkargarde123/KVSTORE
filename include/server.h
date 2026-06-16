#pragma once
#include <string>
#include <unordered_map>
#include <list>
#include <queue>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <functional>
#include <chrono>
#include <condition_variable>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using socklen_t = int;
  using socket_t = SOCKET;
#else
  #include <netinet/in.h>
  using socket_t = int;
#endif

// ─────────────────────────────────────────────
//  TTL Entry: key + expiry time
// ─────────────────────────────────────────────
struct TTLEntry {
    std::string key;
    std::chrono::steady_clock::time_point expiry;
    bool operator>(const TTLEntry& o) const { return expiry > o.expiry; }
};

// ─────────────────────────────────────────────
//  LRU Cache Node
// ─────────────────────────────────────────────
struct LRUNode {
    std::string key;
    std::string value;
};

// ─────────────────────────────────────────────
//  Thread Pool
// ─────────────────────────────────────────────
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();
    void enqueue(std::function<void()> task);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop{false};
};

// ─────────────────────────────────────────────
//  KV Store (LRU + TTL)
// ─────────────────────────────────────────────
class KVStore {
public:
    explicit KVStore(size_t max_capacity = 1000);

    std::string get(const std::string& key);
    bool set(const std::string& key, const std::string& value, int ttl_seconds = -1);
    bool del(const std::string& key);
    std::string keys();
    size_t size();
    void purge_expired();

private:
    size_t capacity;
    std::list<LRUNode> lru_list;
    std::unordered_map<std::string, std::list<LRUNode>::iterator> cache_map;
    std::priority_queue<TTLEntry, std::vector<TTLEntry>, std::greater<TTLEntry>> ttl_queue;
    std::mutex store_mutex;

    void evict_lru();
    bool is_expired(const std::string& key);
};

// ─────────────────────────────────────────────
//  TCP Server
// ─────────────────────────────────────────────
class Server {
public:
    Server(int port, size_t thread_count = 8, size_t lru_capacity = 1000);
    void start();
    void stop();

private:
    int port;
    socket_t server_fd;
    size_t thread_count;
    size_t lru_capacity;
    std::atomic<bool> running{false};
    KVStore store;
    ThreadPool pool;

    void accept_loop();
    void handle_client(socket_t client_fd);
    std::string process_command(const std::string& cmd);
    std::string parse_and_execute(const std::vector<std::string>& tokens);
};
