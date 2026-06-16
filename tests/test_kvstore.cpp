/*
 * Basic test suite — no external deps, pure C++
 * Run: ./kvstore-test
 */
#include "../include/server.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

int passed = 0, failed = 0;

#define TEST(name, expr) \
    do { \
        if (expr) { std::cout << "[PASS] " << name << "\n"; ++passed; } \
        else      { std::cout << "[FAIL] " << name << "\n"; ++failed; } \
    } while(0)

void test_set_get() {
    KVStore store(100);
    store.set("name", "google");
    TEST("SET then GET returns value",  store.get("name") == "google");
    TEST("GET missing key returns -1",  store.get("xyz")  == "-1");
}

void test_overwrite() {
    KVStore store(100);
    store.set("k", "v1");
    store.set("k", "v2");
    TEST("Overwrite key updates value", store.get("k") == "v2");
}

void test_delete() {
    KVStore store(100);
    store.set("x", "1");
    TEST("DEL existing returns true",  store.del("x") == true);
    TEST("GET after DEL returns -1",   store.get("x") == "-1");
    TEST("DEL missing returns false",  store.del("x") == false);
}

void test_lru_eviction() {
    KVStore store(3);   // capacity = 3
    store.set("a", "1");
    store.set("b", "2");
    store.set("c", "3");
    store.get("a");     // access 'a' -> 'b' becomes LRU
    store.set("d", "4"); // should evict 'b'
    TEST("LRU eviction: evicted key gone", store.get("b") == "-1");
    TEST("LRU eviction: recent key intact", store.get("a") != "-1");
}

void test_ttl_expiry() {
    KVStore store(100);
    store.set("temp", "data", 1);   // 1 second TTL
    TEST("TTL key exists before expiry", store.get("temp") == "data");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    store.purge_expired();
    TEST("TTL key gone after expiry", store.get("temp") == "-1");
}

void test_size() {
    KVStore store(100);
    store.set("a", "1");
    store.set("b", "2");
    TEST("SIZE returns correct count", store.size() == 2);
    store.del("a");
    TEST("SIZE decrements after DEL", store.size() == 1);
}

void test_concurrent_access() {
    KVStore store(1000);
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&store, i] {
            for (int j = 0; j < 100; ++j) {
                store.set("key" + std::to_string(i), std::to_string(j));
                store.get("key" + std::to_string(i));
            }
        });
    }
    for (auto& t : threads) t.join();
    TEST("Concurrent SET/GET no crash", true);
    TEST("Concurrent: store has 10 keys", store.size() == 10);
}

int main() {
    std::cout << "=== KVStore Test Suite ===\n\n";
    test_set_get();
    test_overwrite();
    test_delete();
    test_lru_eviction();
    test_ttl_expiry();
    test_size();
    test_concurrent_access();

    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n";
    return failed == 0 ? 0 : 1;
}
