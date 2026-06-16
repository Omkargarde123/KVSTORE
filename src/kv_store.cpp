#include "server.h"
#include <sstream>

KVStore::KVStore(size_t max_capacity) : capacity(max_capacity) {}

bool KVStore::is_expired(const std::string& key) {
    // TTL check is done during purge; here we just ensure key exists
    return false;
}

void KVStore::evict_lru() {
    if (lru_list.empty()) return;
    auto last = lru_list.back();
    cache_map.erase(last.key);
    lru_list.pop_back();
}

std::string KVStore::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(store_mutex);
    auto it = cache_map.find(key);
    if (it == cache_map.end()) return "-1";          // Key not found
    // Move to front (most recently used)
    lru_list.splice(lru_list.begin(), lru_list, it->second);
    return it->second->value;
}

bool KVStore::set(const std::string& key, const std::string& value, int ttl_seconds) {
    std::lock_guard<std::mutex> lock(store_mutex);
    auto it = cache_map.find(key);
    if (it != cache_map.end()) {
        it->second->value = value;
        lru_list.splice(lru_list.begin(), lru_list, it->second);
    } else {
        if (cache_map.size() >= capacity) evict_lru();
        lru_list.emplace_front(LRUNode{key, value});
        cache_map[key] = lru_list.begin();
    }
    if (ttl_seconds > 0) {
        TTLEntry entry;
        entry.key    = key;
        entry.expiry = std::chrono::steady_clock::now()
                     + std::chrono::seconds(ttl_seconds);
        ttl_queue.push(entry);
    }
    return true;
}

bool KVStore::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(store_mutex);
    auto it = cache_map.find(key);
    if (it == cache_map.end()) return false;
    lru_list.erase(it->second);
    cache_map.erase(it);
    return true;
}

std::string KVStore::keys() {
    std::lock_guard<std::mutex> lock(store_mutex);
    std::ostringstream oss;
    for (auto& node : lru_list) oss << node.key << "\n";
    std::string result = oss.str();
    return result.empty() ? "(empty)" : result;
}

size_t KVStore::size() {
    std::lock_guard<std::mutex> lock(store_mutex);
    return cache_map.size();
}

void KVStore::purge_expired() {
    std::lock_guard<std::mutex> lock(store_mutex);
    auto now = std::chrono::steady_clock::now();
    while (!ttl_queue.empty() && ttl_queue.top().expiry <= now) {
        const std::string& key = ttl_queue.top().key;
        auto it = cache_map.find(key);
        if (it != cache_map.end()) {
            lru_list.erase(it->second);
            cache_map.erase(it);
        }
        ttl_queue.pop();
    }
}
