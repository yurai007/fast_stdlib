#include "queues.hpp"
#include "../../thread_safe_queue/src/test_utils.hpp"
#include <iostream>

static void test_memcheck()
{
    std::cout << "Running memcheck tests...\n";
    producer_consumer_test<thread_safe_queue>{}.producer();
    std::cout << "Verdict: OK\n";
}

static void test_case()
{
    std::cout << "Running perf tests...\n";
    producer_consumer_test<thread_safe_queue>{}.pararell_test(1, 4);
}

int main()
{
    test_case();
    test_memcheck();
    return 0;
}

