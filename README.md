# tickprime
Header-only C++20 primitives for staged cacheline pre-warming on the consumer side and cooperative line demotion on the writer side. Targets sub-µs publish/consume paths where std::atomic::wait is too heavy and a naive seqlock leaves the consumer paying cross-core snoop latency on every payload read.
