#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <errno.h>

namespace framework
{

template<class T>
class thread_safe_queue
{
public:

    thread_safe_queue()
        : mutex(PTHREAD_MUTEX_INITIALIZER)
    {
        head = NULL;
        tail = NULL;
    }

    ~thread_safe_queue()
    {
        // TO DO:
    }

    bool is_empty()
    {
        pthread_mutex_lock(&mutex);
        bool result = head == NULL;
        pthread_mutex_unlock(&mutex);
        return result;
    }

    void push(T new_value)
    {
        node *new_tail = new node;

        pthread_mutex_lock(&mutex);
        new_tail->next = NULL;
        new_tail->value = new_value;

        if (tail != NULL)
            tail->next = new_tail;
        else
            head = new_tail;
        tail = new_tail;
        pthread_mutex_unlock(&mutex);
    }

    bool try_pop(T *value)
    {
        pthread_mutex_lock(&mutex);
        if (internal_is_empty())
        {
            pthread_mutex_unlock(&mutex);
            return false;
        }
        *value = head->value;
        node *old_head = head;
        head = head->next;
        if (tail == old_head)
            tail = NULL;

        delete old_head;
        pthread_mutex_unlock(&mutex);
        return true;
    }

private:
    void check_errors(int result, const char *message)
    {
        if (result < 0)
        {
            perror(message);
            exit(errno);
        }
    }

    bool internal_is_empty()
    {
        return head == NULL;
    }

    struct node
    {
        node *next;
        T value;
    };

    node *head, *tail;
    pthread_mutex_t mutex;
};

}

#endif // THREAD_SAFE_QUEUE_HPP
