#pragma once

#include <iostream>
#include <cassert>

template <template <class> class queue_sut>
class sequential_producer_consumer_test
{
public:
    void run_test()
    {
        producer();
        consumer();
        std::cout << "Verdict: OK\n";
    }

private:
    void producer()
    {
        for (int i = 0; i < iterations; i++)
        {
            queue.push(i);
        }
    }

    void consumer()
    {
        while (!queue.empty())
        {
            int value = 0;
            queue.try_pop(value);
            sum += value;
        }
        assert(sum == iterations*(iterations-1)/2);
    }

    queue_sut<int> queue;
    constexpr static auto iterations = 2000000L;
    long sum  = 0L;
};
