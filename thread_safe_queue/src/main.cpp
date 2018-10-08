#include "thread_safe_queue.hpp"
#include <iostream>
#include <atomic>
#include <cassert>
#include <pthread.h>

constexpr auto max_consumers_number = 128u;
constexpr auto iterations = 1000000;
constexpr auto consumers_number = 4;
using lli = long long int;

static std::atomic<bool> finished {false};
static pthread_mutex_t stdio_mutex = PTHREAD_MUTEX_INITIALIZER;
static lli sums[max_consumers_number];
static thread_safe_queue<int> queue;

static void check_errors(int result, const char *message)
{
    if (result < 0)
    {
        perror(message);
        exit(errno);
    }
}

static void* producer(void *)
{
    lli all_pushed = iterations*(iterations-1L)/2L;
    printf("Sum of all pushed items = %lld\n", all_pushed);
    for (int i = 0; i < iterations; i++)
    {
        queue.push(i);
    }
    finished = true;
    pthread_exit(nullptr);
}

static void* consumer(void *id)
{
    auto thread_id = *static_cast<int*>(id);
    lli sum = 0;

    while (!finished || !queue.empty())
    {
        int value = 0;
        queue.try_pop(&value);
        sum += value;
    }
    pthread_mutex_lock(&stdio_mutex);
    printf("Sum of popped items by consumer %d = %lld\n", thread_id, sum);
    pthread_mutex_unlock(&stdio_mutex);
    sums[thread_id] = sum;
    pthread_exit(nullptr);
}

static void test_memcheck()
{
    std::cout << "Running sct_thread_safe_queue tests...\n";
    pthread_t producer_struct;
    auto producer_ids = consumers_number;
    auto rc = pthread_create(&producer_struct, nullptr, producer, &producer_ids);
    check_errors(rc, "Error creating the producer thread..");
    pthread_join(producer_struct, nullptr);
    std::cout << "Verdict: OK\n";
}

static void test_case()
{
    std::cout << "Running sct_thread_safe_queue tests...\n";
    pthread_t producer_struct;
    auto producer_ids = consumers_number;
    auto rc = pthread_create(&producer_struct, nullptr, producer, &producer_ids);
    check_errors(rc, "Error creating the producer thread..");

    pthread_t consumers_struct[consumers_number];
    int consumers_ids[consumers_number];
    for (int i = 0; i < consumers_number; i++)
    {
        consumers_ids[i] = i;
        sums[i] = 0;
        rc = pthread_create(&consumers_struct[i], nullptr, consumer, (void *)&consumers_ids[i]);
        check_errors(rc, "Error creating the consumer thread..");
    }

    lli sum = 0;
    for (int i = 0; i < consumers_number; i++)
    {
        pthread_join(consumers_struct[i], nullptr);
        sum += sums[i];
    }
    pthread_join(producer_struct, nullptr);

    printf("sums = %lld\n", sum);
    assert(sum == 499999500000);
    std::cout << "Verdict: OK\n";
    std::cout << "All sct_thread_safe_queue tests passed\n";
}

int main()
{
    test_case();
    test_memcheck();
    return 0;
}

