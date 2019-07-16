﻿#include <experimental/thread_pool>
#include <type_traits>
#include <cassert>
#include <stdexcept>
#include <unistd.h>
#include <sys/syscall.h>
#include <memory>

namespace stdx = std::experimental;
namespace execution = stdx::execution;
using stdx::static_thread_pool;

namespace properties {

static constexpr auto constexpr_preliminaries() {
    static_assert(std::is_constructible<static_thread_pool>::value == false);
    static_assert(std::is_copy_constructible<static_thread_pool>::value == false);
    static_assert(std::is_assignable<static_thread_pool, static_thread_pool>::value == false);
    static_assert(std::is_move_assignable<static_thread_pool>::value == false);
    static_assert(std::is_move_constructible<static_thread_pool>::value == false);
}

static auto preliminaries_without_concepts() {
    static_thread_pool pool{1};
    static_assert(std::is_constructible<decltype(pool)>::value == false);
    static_assert(std::is_copy_constructible<decltype(pool)>::value == false);
    static_assert(std::is_assignable<decltype(pool), decltype(pool)>::value == false);
    static_assert(std::is_move_assignable<decltype(pool)>::value == false);
    static_assert(std::is_move_constructible<decltype(pool)>::value == false);

    auto executor = pool.executor();
    static_assert(std::is_copy_constructible<decltype(executor)>::value == true);
    static_assert(std::is_destructible<decltype(executor)>::value == true);
    // missing - static_assert(std::is_equality_comparable<decltype(executor)>::value == true);
    // actual type is :executor_impl<execution::possibly_blocking_t, execution::not_continuation_t, execution::not_outstanding_work_t,
    //      std::allocator<void> >
    static_assert(std::is_same<decltype(executor), static_thread_pool::executor_type>::value == true);
    // Ok, not mutally exclusive.
    static_assert(execution::is_oneway_executor<decltype(executor)>::value == true);
    static_assert(execution::is_twoway_executor<decltype(executor)>::value == true);
    // (allocator, ptr to pool)
    static_assert(sizeof executor == 2*sizeof(void*));
}

#ifndef __clang__
/*
    Executor type shall satisfy the requirements of CopyConstructible (C++Std [copyconstructible]), Destructible (C++Std [destructible]),
    and EqualityComparable (C++Std [equalitycomparable]).
*/
// stolen from <concepts> (header since C++20).
template <class T>
concept bool Destructible = std::is_nothrow_destructible_v<T>;

template <class T, class... Args>
concept bool Constructible = Destructible<T> && std::is_constructible_v<T, Args...>;

template <class From, class To>
concept bool ConvertibleTo = std::is_convertible_v<From, To> && requires (From (&f)()) {
    static_cast<To>(f());
};

template <class T>
concept bool MoveConstructible = Constructible<T, T> && ConvertibleTo<T, T>;

template <class T>
concept bool CopyConstructible =
    MoveConstructible<T> &&
    Constructible<T, T&> && ConvertibleTo<T&, T> &&
    Constructible<T, const T&> && ConvertibleTo<const T&, T> &&
    Constructible<T, const T> && ConvertibleTo<const T, T>;

// simplified comparing with https://en.cppreference.com/w/cpp/concepts/EqualityComparable
// hmm, constexpr concept is not prohibited
template <class T>
constexpr concept bool EqualityComparable = requires (T& t) {
     operator==(t, t);
     operator!=(t, t);
};

template <class E>
concept bool OneWayExecutor = CopyConstructible<E> && Destructible<E> && EqualityComparable<E>;

static constexpr auto preliminaries_executors_concepts() {
    using some_polymorphic_executor = execution::executor<execution::oneway_t, execution::single_t,
        execution::always_blocking_t, execution::possibly_blocking_t>;
    static_assert(OneWayExecutor<some_polymorphic_executor> == true);
}
#endif

static auto test() {
    static_thread_pool pool{1};
    auto executor = pool.executor();
    // generic execute() - static_thread_pool::execute(Blocking, Continuation, const ProtoAllocator& alloc, Function f)
    // only notify worker thread in pool by cv.notify()
    executor.execute([]() noexcept {
        std::cout << "done1\n";
    });
    // require_fn::operator() forwards execution::always_blocking to:
    //  executor_impl<execution::always_blocking_t, Continuation, Work, ProtoAllocator>
    //          static_thread_pool::executor_impl::require(execution::always_blocking_t)
    // which creates executor_impl<always_blocking_t, ... >

    auto executor2 = execution::require(executor, execution::always_blocking);
    // with always_blocking another execution path
    // specialized execute() - execute(execution::always_blocking_t, Continuation, const ProtoAllocator& alloc, Function f)
    // extra promise + future wrapping
    executor2.execute([]() noexcept {
        std::cout << "done2\n";
    });
    pool.wait();
}

// internals ?
static auto test_continuation_property() {
    static_thread_pool pool{32};
    auto executor = execution::require(pool.executor(), execution::continuation);
    std::cout << "init on " << syscall(SYS_gettid) << "\n";
    executor.execute([]() noexcept {
        std::cout << "step 1 on " << syscall(SYS_gettid) << "\n";
    });
    executor.execute([&executor, internal = execution::require(executor, execution::continuation)] {
        std::cout << "step 2 on " << syscall(SYS_gettid) << "\n";
        executor.execute([]() noexcept {
            std::cout << "step 3 on " << syscall(SYS_gettid) << "\n";
        });
    });
    executor.execute([]() noexcept {
        std::cout << "step 4 on " << syscall(SYS_gettid) << "\n";
    });
    pool.wait();
}

static auto test_outstanding_work() {
    static_thread_pool pool{2};
    auto work_ex = execution::prefer(pool.executor(), execution::outstanding_work);
    auto possibly_blocking_ex = execution::prefer(work_ex, execution::possibly_blocking);
    work_ex.execute([] {
        std::cout << "async work on " << syscall(SYS_gettid) << "\n";
    });
    possibly_blocking_ex.execute(
        [] {
          std::cout << "possibly blocking on " << syscall(SYS_gettid) << "\n";
        }
      );
    // blocks
    pool.wait();
}

}

namespace polymorphic_executors {

using pexecutor = execution::executor<execution::oneway_t, execution::single_t,
    execution::always_blocking_t, execution::possibly_blocking_t>;

class inline_executor final {
public:
    constexpr friend bool operator==(const inline_executor&, const inline_executor&) noexcept {
        return true;
    }

    constexpr friend bool operator!=(const inline_executor&, const inline_executor&) noexcept {
        return false;
    }

    template <class Function>
    inline void execute(Function &&f) const noexcept {
        f();
    }

    inline_executor require(execution::always_blocking_t) const noexcept {
        return *this;
    }
};

static auto test() {
    // will be shared (as ptr) among executors
    static_thread_pool pool{1};
    {
        auto executor = pool.executor();
        assert(&execution::query(executor, execution::context) == &pool);
    }
    {
        // normal executor - just (allocator, ptr to pool)
        auto executor = pool.executor();
        // polymorphic one - more complicated - has atomic refcount
        pexecutor executor2 = execution::require(executor, execution::possibly_blocking);
        assert(executor == executor2);
    }
    {
        inline_executor iexecutor;
        // inline_executor::require is needed
        pexecutor executor3 = execution::require(iexecutor, execution::always_blocking);
        assert(iexecutor == executor3);
    }
    {
        auto executor = pool.executor();
        pexecutor executor4 = execution::require(executor, execution::possibly_blocking);
        executor4.execute([executor5 = execution::require(executor4, execution::always_blocking)]{
            std::cout << "1\n";
            executor5.execute([]{ std::cout << "2\n"; });
            std::cout << "3\n";
          });
    }
    pool.wait();
}
}
/*
 1. static_thread_pool bug reproducer
    yurai@archlinux debug]$ ./std-executors
    /home/yurai/programs/executors-impl/include/experimental/bits/static_thread_pool.h:239:7:
        runtime error: member access within address 0x603000000040 which does not point to an object of type
            'std::experimental::executors_v1::static_thread_pool::func<(lambda at ../../src/std_executors.cpp:135:22), std::__1::allocator<void> >'
 */
namespace experimental_bugs {

template<class T>
struct allocator {
    template<class Arg>
    T *allocate(Arg &arg) {
        return new T(arg);
    }

    void deallocate(T *ptr) {
        delete ptr;
    }
};

struct func_base {
    virtual void destroy() = 0;
    virtual ~func_base() = default;
    std::unique_ptr<int> next_;
};

struct func : func_base {
    explicit func(allocator<func> &a) : allocator_(a) {}

    static func_base *create(allocator<func> &a) {
        auto *ptr = a.allocate(a);
        return ptr;
    }
    virtual void destroy()
    {
      func* p = this;
      p->~func();
      // it's unlegal/UB
      allocator_.deallocate(p);
    }
    std::function<void(void)> function_;
    allocator<func> allocator_;
};

static auto Ubsan_minimal_reproducer1() {
    allocator<func> allocator;
    auto *f = func::create(allocator);
    f->destroy();
}
}

int main() {
    properties::preliminaries_without_concepts();
#ifndef __clang__
    properties::preliminaries_executors_concepts();
#endif
    polymorphic_executors::test();
    properties::test_continuation_property();
    experimental_bugs::Ubsan_minimal_reproducer1();
    properties::test_outstanding_work();
    return 0;
}
