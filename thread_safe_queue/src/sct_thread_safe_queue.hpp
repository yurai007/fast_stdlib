#include <iostream>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <cassert>
#include <type_traits>
#include "pthread.h"

#include "../common/messages.hpp"
#include "../common/byte_buffer.hpp"
#include "../common/thread_safe_queue.hpp"

namespace thread_safe_queue_sct
{

#define MAX_CONSUMERS_SIZE 128
const int iterations = 1000000;
#define CONSUMERS_NUMBER 4

typedef long long int lli;

std::atomic<bool> finished {false};

pthread_mutex_t stdio_mutex = PTHREAD_MUTEX_INITIALIZER;
lli sums[MAX_CONSUMERS_SIZE];

framework::thread_safe_queue<int> queue;

void check_errors(int result, const char *message)
{
    if (result < 0)
    {
        perror(message);
        exit(errno);
    }
}

void* producer(void *)
{
    lli all_pushed = iterations*(iterations-1L)/2L;
    printf("Sum of all pushed items = %lld\n", all_pushed);
    for (int i = 0; i < iterations; i++)
    {
        queue.push(i);
    }
    finished = true;
    pthread_exit(NULL);
}

void* consumer(void *id)
{
    lli sum = 0;
    int thread_id = *((int*)id);

    while (finished == false || !queue.is_empty())
    {
        int value = 0;
        queue.try_pop(&value);
        sum += value;
    }
    pthread_mutex_lock(&stdio_mutex);
    printf("Sum of popped items by consumer %d = %lld\n", thread_id, sum);
    pthread_mutex_unlock(&stdio_mutex);
    sums[thread_id] = sum;
    pthread_exit(NULL);
}

void test_case()
{
    std::cout << "Running sct_thread_safe_queue tests...\n";
    pthread_t producer_struct;
    int producer_ids = CONSUMERS_NUMBER;
    int rc = pthread_create(&producer_struct, NULL, producer, &producer_ids);
    check_errors(rc, "Error creating the consumer thread..");

    pthread_t consumers_struct[CONSUMERS_NUMBER];
    int consumers_ids[CONSUMERS_NUMBER];
    for (int i = 0; i < CONSUMERS_NUMBER; i++)
    {
        consumers_ids[i] = i;
        sums[i] = 0;
        rc = pthread_create(&consumers_struct[i], NULL, consumer, (void *)&consumers_ids[i]);
        check_errors(rc, "Error creating the consumer thread..");
    }

    lli sum = 0;
    for (int i = 0; i < CONSUMERS_NUMBER; i++)
    {
        pthread_join(consumers_struct[i], NULL);
        sum += sums[i];
    }
    pthread_join(producer_struct, NULL);

    printf("sums = %lld\n", sum);
    assert(sum == 499999500000);
    std::cout << "Verdict: OK\n";
    std::cout << "All sct_thread_safe_queue tests passed\n";
}

}

