# tickprime

Staged cacheline pre-warming on the consumer side and cooperative line demotion (CLDEMOTE) on the writer side, for C++20 publish/consume paths where `std::atomic::wait` is too heavy and a plain seqlock leaves the reader paying cross-core snoop latency on every payload read.

> **Status: early sketch, does not build on its own.** This repository contains only `main.cpp`, a writer/consumer demo. The headers it includes (`kv_cache.hpp`, `flusher.hpp`, `consumer.hpp`) are not committed here.
>
> The complete, tested library lives in **[fin-prewarm](https://github.com/Mattbusel/fin-prewarm)**: the same `kv::Cache` / `kv::flush` / `kv::snapshot` design, with headers, CMake package, ThreadSanitizer test, `objdump` checks for `cldemote`/`prefetcht0`, and a latency benchmark. Use that one.

## What `main.cpp` shows

- A 256-byte `kv::Cache` holding four cacheline-sized `Quote` structs (timestamp, bid, ask, sizes).
- A writer thread that publishes a new generation every microsecond through `kv::flush(cache, mutate)`.
- A consumer thread that spins on `kv::snapshot<256, 1>(cache, buf)` (payload size, prefetch lead) and counts successful and torn reads.
- The program runs for 50 ms and prints `consumer: N reads, M fails`.

## Building

With the headers from fin-prewarm on the include path:

```sh
g++ -std=c++20 -O3 -march=native -pthread -I<fin-prewarm>/include/fin_prewarm main.cpp -o tickprime
./tickprime
```

Note that `main.cpp` predates fin-prewarm's final API (it writes through `payload.bytes`, while fin-prewarm passes a raw `void*` buffer to the mutate callback), so small edits are needed. x86-64 only.
