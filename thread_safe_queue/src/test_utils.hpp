#pragma once

#include <iostream>
#include <cassert>
#include <thread>
#include <atomic>
#include <vector>
#include <numeric>
#include <mutex>

template <template <class> class queue_sut>
class producer_consumer_test
{
public:
    void sequential_test() {
        producer();
        auto sum = consumer();
        assert(sum == iterations*(iterations-1)/2);
        std::cout << "Verdict: OK\n";
    }

    void pararell_test(unsigned num_producers, unsigned num_consumers) {
        constexpr auto pushed = iterations*(iterations-1)/2;
        constexpr auto max_consumers_number = 128u;
        std::cout << "Sum of all pushed items = " << pushed << "\n";
        std::vector<std::thread> consumers, producers;
        for (auto i = 0u; i < num_producers; i++) {
            producers.emplace_back([this](){
                producer();
            });
        }
        std::vector<long> sums(max_consumers_number, 0);
        for (auto i = 0u; i < num_consumers; i++) {
            consumers.emplace_back([this, i, &sum = sums[i]]() mutable {
                sum = pararell_consumer();
                std::lock_guard lock(mutex_);
                std::cout << "Sum of popped items by consumer " << i << " = " << sum  << "\n";
            });
        }
        for (auto &&consumer : consumers) {
            consumer.join();
        }
        for (auto &&producer : producers) {
            producer.join();
        }
        long all = std::accumulate(sums.begin(), sums.end(), 0L);
        std::cout << all << "\n";
        assert(all == 499999500000L);
        std::cout << "Verdict: OK\n";
    }

    void producer() {
        for (int i = 0; i < iterations; i++)
        {
            queue.push(i);
        }
        done = true;
    }
private:
    long consumer() {
        long sum = 0L;
        while (!queue.empty())
        {
            int value = 0;
            queue.try_pop(value);
            sum += value;
        }
        return sum;
    }

    long pararell_consumer() {
        long sum = 0L;
        while (!done || !queue.empty())
        {
            int value = 0;
            queue.try_pop(value);
            sum += value;
        }
        return sum;
    }

    queue_sut<int> queue;
    constexpr static auto iterations = 1000000L;
    std::atomic<bool> done {false};
    std::mutex mutex_;
};
