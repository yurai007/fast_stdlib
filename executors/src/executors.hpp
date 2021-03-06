﻿#pragma once

#include "../../thread_safe_queue/src/queues.hpp"
#include "../../future/src/future.hpp"
#include <boost/range/irange.hpp>
#include <future>
#include <functional>
#include <thread>
#include <vector>
#include <atomic>
#include <variant>

namespace execution {

struct require_fn
{
    template<class Executor, class Property1, class Property2>
    constexpr auto operator()(Executor&& ex, Property1&&, Property2&&) const {
        return FWD(ex);
    }

    template<class Executor, class Property1>
    constexpr auto operator()(Executor&& ex, Property1&&) const {
        return FWD(ex);
    }
};

template<class T = require_fn>
constexpr T customization_point{};

constexpr const auto& require = customization_point<>;

struct oneway_t {};
struct twoway_t {};
struct single_t {};

constexpr oneway_t oneway;
constexpr twoway_t twoway;
constexpr single_t single;

class inline_executor final {
public:
    friend bool operator==(const inline_executor&, const inline_executor&) noexcept {
        return true;
    }

    friend bool operator!=(const inline_executor&, const inline_executor&) noexcept {
        return false;
    }

    template <class Function>
    inline void execute(Function &&f) const noexcept {
        f();
    }
};

class static_thread_pool {
public:
    static_thread_pool(const static_thread_pool&) = delete;
    static_thread_pool& operator=(const static_thread_pool&) = delete;
    static_thread_pool(unsigned threads) {
    }

    class static_thread_pool_executor final {
    public:
        static_thread_pool_executor(static_thread_pool &pool)
            : pool_(pool) {}

        template <class Function>
        [[nodiscard]] inline auto twoway_execute(Function &&f) const noexcept {
            return pool_.twoway_execute(FWD(f));
        }

        template <class Function>
        inline void oneway_execute(Function &&f) const noexcept {
            pool_.oneway_execute(FWD(f));
        }

        friend bool operator==(const static_thread_pool_executor&a, const static_thread_pool_executor&b) noexcept {
            return &a.pool_ == &b.pool_;
        }

        friend bool operator!=(const static_thread_pool_executor&a, const static_thread_pool_executor&b) noexcept {
            return &a.pool_ != &b.pool_;
        }

    private:
        static_thread_pool& pool_;
    };

    using executor_type = static_thread_pool_executor;
    using task = std::variant<std::packaged_task<double()>, std::function<void()>>;

    executor_type executor() noexcept {
        return static_thread_pool_executor{*this};
    }

    void start() {
        for (auto i : boost::irange(0u, cores))
            threads_.emplace_back([i, this](){
                while (!_queues[i].empty())
                {
                    task task_;
                    try {
                        task_ = std::move(*_queues[i].front_and_pop());
                        std::get<0>(task_)();
                    }
                    catch (std::bad_variant_access&) {
                        std::get<1>(task_)();
                    }
                }
            });
    }

    template <class Function>
    inline void oneway_execute(Function &&f) noexcept {
        const auto i = (counter++) % cores;
        _queues[i].push(FWD(f));
    }

    template <class Function>
    [[nodiscard]] inline auto twoway_execute(Function &&f) noexcept {
        std::packaged_task<decltype(f())()> task(std::move(f));
        std::future<decltype(f())> future = task.get_future();
        const auto i = (counter++) % cores;
        _queues[i].push(std::move(task));
        return future;
    }

    ~static_thread_pool() {
        for (auto &&thread : threads_)
            thread.join();
     }

private:
    const unsigned cores = std::thread::hardware_concurrency();
    std::vector<std::thread> threads_;
    std::vector<std_thread_safe_queue<task>> _queues {cores};
    std::atomic<unsigned> counter {0u};
};

}
