#pragma once

#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <cerrno>
#include <mutex>
#include <queue>
#include <condition_variable>

template<class T>
class raw_queue
{
public:
    raw_queue() = default;
    raw_queue(const raw_queue&) = delete;
    raw_queue& operator=(const raw_queue&) = delete;

    void push(T new_value)
    {
        node *new_tail = new node;
        new_tail->value = new_value;
        new_tail->next = nullptr;
        if (tail != nullptr)
            tail->next = new_tail;
        else
            head = new_tail;
        tail = new_tail;
    }

    bool try_pop(T &value)
    {
        if (empty())
            return false;
        value = head->value;
        node *old_head = head;
        head = head->next;
        if (tail == old_head)
            tail = nullptr;
        delete old_head;
        return true;
    }

    bool empty() const
    {
        return head == nullptr;
    }

private:
    struct node
    {
        node *next;
        T value;
    };
    node *head {nullptr}, *tail {nullptr};
};


template<class T>
class std_shared_ptr_queue
{
public:
    std_shared_ptr_queue() = default;
    std_shared_ptr_queue(const std_shared_ptr_queue&) = delete;
    std_shared_ptr_queue& operator=(const std_shared_ptr_queue&) = delete;

    void push(T new_value)
    {
        auto new_tail = std::make_shared<node>();
        new_tail->value = new_value;
        new_tail->next = nullptr;
        if (tail != nullptr)
            tail->next = new_tail;
        else
            head = new_tail;
        tail = new_tail;
    }

    bool try_pop(T &value)
    {
        if (empty())
            return false;
        value = std::move(head->value);
        auto old_head = head;
        head = std::move(head->next);
        if (tail == old_head)
            tail = nullptr;
        return true;
    }

    bool empty() const
    {
        return head == nullptr;
    }

private:
    struct node
    {
        std::shared_ptr<node> next;
        T value;
    };
    std::shared_ptr<node> head, tail;
};

template<class T>
class std_thread_safe_queue
{
public:
    void push(T new_value)
    {
        std::lock_guard lock(mutex_);
        queue_.push(std::move(new_value));
        cv_.notify_one();
    }

    std::shared_ptr<T> front_and_pop()
    {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this]{ return !queue_.empty(); });
        auto result = std::make_shared<T>(std::move(queue_.front()));
        queue_.pop();
        return result;
    }
private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable cv_;
};

template<class T>
class thread_safe_queue
{
public:
    thread_safe_queue() = default;
    thread_safe_queue(const thread_safe_queue&) = delete;
    thread_safe_queue& operator=(const thread_safe_queue&) = delete;

    void push(T new_value)
    {
        auto new_tail = new node;

        pthread_mutex_lock(&mutex);
        new_tail->next = nullptr;
        new_tail->value = new_value;

        if (tail != nullptr)
            tail->next = new_tail;
        else
            head = new_tail;
        tail = new_tail;
        pthread_mutex_unlock(&mutex);
    }

    bool try_pop(T *value)
    {
        pthread_mutex_lock(&mutex);
        if (internal_empty())
        {
            pthread_mutex_unlock(&mutex);
            return false;
        }
        *value = head->value;
        auto old_head = head;
        head = head->next;
        if (tail == old_head)
            tail = nullptr;

        delete old_head;
        pthread_mutex_unlock(&mutex);
        return true;
    }

    bool empty() const
    {
        pthread_mutex_lock(&mutex);
        auto empty = head == nullptr;
        pthread_mutex_unlock(&mutex);
        return empty;
    }

    ~thread_safe_queue()
    {
        auto next = head;
        while (next != nullptr)
        {
            auto nn = next->next;
            delete next;
            next = nn;
        }
    }

private:
    bool internal_empty() const
    {
        return head == nullptr;
    }

    struct node
    {
        node *next;
        T value;
    };

    node *head {nullptr}, *tail {nullptr};
    mutable pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
};

template<class T>
class fast_queue
{
    struct node
    {
        node *next;
        T value;
    };

public:
    fast_queue() :
        head(new node),
        tail(head)
    {
        tail->next = nullptr;
    }

    node *get_tail()
    {
        std::lock_guard tail_lock(tail_mutex);
        return tail;
    }

    // now we store new_value to current tail and create new empty dummy tail.
    // Notice we don't touch head but we may touch value pointing by head (if head == tail)
    void push(T new_value)
    {
        node *new_dummy_tail = new node;
        new_dummy_tail->next = nullptr;

        std::lock_guard tail_lock(tail_mutex);
        tail->value = new_value;
        tail->next = new_dummy_tail;
        tail = new_dummy_tail;
    }

    node *pop_head()
    {
        std::lock_guard head_lock(head_mutex);
        if (head == get_tail())
            return nullptr;
        node *old_head = head;
        head = head->next;
        return old_head;
    }

    bool try_pop(T &value)
    {
        node *old_head = pop_head();
        if (old_head != nullptr)
        {
            value = old_head->value;
            delete old_head;
            return true;
        }
        else
            return false;
    }

    bool empty() const
    {
        return head == tail;
    }

private:
    node *head, *tail;
    std::mutex head_mutex;
    std::mutex tail_mutex;
};

