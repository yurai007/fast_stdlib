#pragma once

#include "future.hpp"
#include <future>

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

class static_thread_pool;

// TO DO: it's only skeleton
class static_thread_pool_executor final {
public:
    static_thread_pool_executor(static_thread_pool &pool)
        : pool_(pool) {}

    template <class Function>
    [[nodiscard]] inline auto twoway_execute(Function &&f) const noexcept {
        return std::future<double>{};
    }
private:
    static_thread_pool& pool_;
};

class static_thread_pool {
public:
    static_thread_pool(unsigned threads) {}
    using executor_type = static_thread_pool_executor;

    executor_type executor() noexcept {
        return static_thread_pool_executor{*this};
    }
};

}
