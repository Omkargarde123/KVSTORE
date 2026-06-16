CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude
LDFLAGS  := -lws2_32
SRC      := src/main.cpp src/server.cpp src/kv_store.cpp src/thread_pool.cpp
TEST_SRC := tests/test_kvstore.cpp src/kv_store.cpp src/server.cpp src/thread_pool.cpp
CLI_SRC  := client/kvstore-cli.cpp

# For Linux/Unix (uncomment on Linux):
# CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -pthread -Iinclude
# LDFLAGS  := -pthread

.PHONY: all clean test run

all: kvstore kvstore-cli

kvstore: $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ Built: kvstore"

kvstore-cli: $(CLI_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ Built: kvstore-cli"

test: kvstore-test
	./kvstore-test

kvstore-test: $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ Built: kvstore-test"

run: kvstore
	./kvstore 6379 8 1000

clean:
	rm -f kvstore kvstore-cli kvstore-test
	@echo "🧹 Cleaned"
