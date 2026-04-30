// main.cpp
#include "kv_cache.hpp"
#include "flusher.hpp"
#include "consumer.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

namespace {

constexpr std::size_t kPayloadBytes = 256;

struct alignas(pw::kCacheLine) Quote {
    std::uint64_t ts_ns;
    double bid;
    double ask;
    std::uint32_t bid_sz;
    std::uint32_t ask_sz;
    char _pad[pw::kCacheLine - 32];
};
static_assert(sizeof(Quote) == pw::kCacheLine);

using CacheT = kv::Cache<kPayloadBytes>;
CacheT g_cache{};
std::atomic<bool> g_stop{false};

void writer_thread() {
    std::uint64_t i = 0;
    while (!g_stop.load(std::memory_order_relaxed)) {
        kv::flush(g_cache, [&](auto& payload) {
            auto* q = reinterpret_cast<Quote*>(&payload.bytes[0]);
            for (int k = 0; k < 4; ++k) {
                q[k].ts_ns = i + k;
                q[k].bid   = 100.0 + (i & 0xff) * 0.01;
                q[k].ask   = q[k].bid + 0.01;
                q[k].bid_sz = 100;
                q[k].ask_sz = 100;
            }
        });
        ++i;
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

void consumer_thread() {
    alignas(pw::kCacheLine) std::byte buf[kPayloadBytes];
    std::uint64_t reads = 0, fails = 0;
    while (!g_stop.load(std::memory_order_relaxed)) {
        auto r = kv::snapshot<kPayloadBytes, 1>(g_cache, buf);
        if (r.ok) ++reads; else ++fails;
    }
    std::printf("consumer: %llu reads, %llu fails\n",
                (unsigned long long)reads, (unsigned long long)fails);
}

} // namespace

int main() {
    std::thread w(writer_thread);
    std::thread c(consumer_thread);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    g_stop.store(true, std::memory_order_relaxed);
    w.join();
    c.join();
    return 0;
}
