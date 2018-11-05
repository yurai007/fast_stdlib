#include "future.hpp"
#include <iostream>
#include <cassert>
#include <array>
#include <vector>
#include <syscall.h>
#include <unistd.h>
#include <climits>
#include <atomic>

static inline uint64_t realtime_now()
{
    #define TIMESPEC_NSEC(ts) ((ts)->tv_sec * 1000000000ULL + (ts)->tv_nsec)
    struct timespec now_ts;
    clock_gettime(CLOCK_REALTIME, &now_ts);
    return TIMESPEC_NSEC(&now_ts);
}

static void test_basics()
{
    {
        // shared state outside class
        static_assert(sizeof(future_void<>) == 16);

        future_void<> f;
        // similar properties as std::unique_ptr
        static_assert(std::is_constructible<decltype(f)>::value == true);
        static_assert(std::is_copy_constructible<decltype(f)>::value == false);
        static_assert(std::is_assignable<decltype(f), decltype(f)>::value == false);
        static_assert(std::is_move_assignable<decltype(f)>::value == false);
    }
    // ok, ill-formed
    #if 0
    {
        promise_void p1;
        // can't copy construct
        promise_void p2 = p1;
        // can't assign
        p2 = p1;

        future_void f1;
        // can't copy construct
        future_void f2 = f1;
        // can't assign
        f2 = f1;
    }
    #endif

    // basic scenario - could be benchmarked and compared with libstdc++ and seastar?
    {
        promise_void<> p;
        // create shared state
        future_void f = p.get_future();
        assert(f.valid());
        // push to state
        p.set_value();
        // here it's ok, we should return immediately
        f.wait();
        assert(!f.valid());
    }
    {
        promise_void<> p;
        // create shared state
        future_void f = p.get_future();
        assert(f.valid());
        // DEADLOCK here!
        //f.wait();
        assert(f.valid());
    }
    {
        auto done = false;
        auto f = async_void([&done](){
            std::cout << "Hello Async World!\n";
            done = true;
        });
        assert(f.valid());
        assert(!done);
        f.wait();
        assert(!f.valid());
        assert(done);
    }
    auto done = false;
    {
        auto f = async_void([&done](){
            std::cout << "Hello Async World!\n";
            done = true;
        });
        assert(f.valid());
        assert(!done);
    }
    assert(!done);
    {
        auto f = [](){ std::cout << "Hello! "; };
        auto g = [](){ std::cout << "World\n"; };
        auto f1 = async_void([f]{ f(); });
        auto f2 = async_void([g]{ g(); });
        f1.wait();
        f2.wait();
    }
    #if 0 // TO DO: broken test
    {
        promise_void<> p;
        auto done = false;
        std::thread new_work_thread([&]() {
            p.get_future().wait();
            done = true;
        });
        assert(!done);
        p.set_value();
        new_work_thread.join();
        assert(done);
    }
    #endif
}

static void test_async()
{
    {
        auto f = [](){ std::cout << "f! "; };
        auto g = [](){ std::cout << "g\n"; };
        auto f1 = async_void([f]{ f(); });
        auto f2 = async_void([g]{ g(); });
        f1.wait();
        f2.wait();
    }
    {
        auto done = false;
        auto f = [&](){
            std::cout << "async policy!\n";
            done = true;
        };
        auto f1 = async_void([f]{ f(); }, async_policy::async);
        assert(f1.valid());
        assert(!done);
        f1.wait();
        assert(!f1.valid());
        assert(done);
    }
    {
        // TO DO: now there is implicit sync point as we lazily run thread in wait()
        auto done1 = false, done2 = false;
        auto f = [&done1](){
            std::cout << "f\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            done1 = true;
        };
        auto g = [&done2](){
            std::cout << "g\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            done2 = true;
        };
        auto f1 = async_void([f]{ f(); }, async_policy::async);
        auto f2 = async_void([g]{ g(); }, async_policy::async);
        f1.wait();
        f2.wait();
        assert(done1 && done2);
    }
    {
        auto func = static_cast<lightweight_stateless_function<void>>([](){
            std::cout << "test_customized_async\n";
        });
        auto f = async_void(func);
        assert(f.valid());
        f.wait();
        assert(!f.valid());
    }
}

static void test_space_cost()
{
    using namespace experimental;
    {
        auto f = initiate([]{
            std::cout << "1";
        });
        auto g = []{ std::cout << "1\n"; };
        static_assert(sizeof(f) == sizeof(g) && sizeof(f) == 1);
    }
    {
        auto f = initiate([]{
            std::cout << "0";
        })
        .then([]{
            std::cout << "1";
        })
        .then([]{
            std::cout << "2";
        })
        .then([]{
            std::cout << "3";
        })
        .then([]{
            std::cout << "4\n";
        })
        .then([]{
            std::cout << "5\n";
        });

        static_assert(sizeof(f) == 5);
    }
}

static void test_inline_then_concept()
{
    using namespace experimental;
    {
        // creates node<root, lambda<>> which is derived from lambda,
        // so callable is stored inside lambda
        std::array done = {false, false, false};
        auto f = initiate([&]{
            assert(done[0] || done[1] || done[2] == false);
            done[0] = true;
        })
        .then([&](){
             done[1] = true;
             assert(done[0] && done[1] && !done[2] == true);
        })
        .then([&](){
             done[2] = true;
             assert(done[0] && done[1] && done[2] == true);
        });

        // execute() -> synchronous_scheduler::operator() ->
        //           -> call_with_parent() -> as_f()(as_parent()) ->
        // lambda()::operator()
       assert(done[0] || done[1] || done[2] == false);
       f.execute(inline_scheduler{});
       assert(done[0] && done[1] && done[2] == true);
    }
}

namespace before {

enum status {ready, not_ready};

class state_base {
public:
    virtual void _M_complete_async() {
        std::unique_lock __lock(_M_mutex);
        _M_cond.wait(__lock, [&] { return ready(); });
    }
    virtual ~state_base() = default;
    void _M_set_result() {
        std::lock_guard __lock(_M_mutex);
        _M_status = status::ready;
        _M_cond.notify_all();
    }
    bool ready() const {
        return _M_status == status::ready;
    }
private:
    std::mutex               	_M_mutex;
    std::condition_variable  	_M_cond;
    status _M_status {status::not_ready};
};

}

namespace after {

#define _GLIBCXX_FUTEX_WAIT 0
#define _GLIBCXX_FUTEX_WAKE 1

enum status {ready, not_ready};

struct atomic_futex_unsigned_base
{
    static bool futex_wait_for(unsigned *addr, unsigned val)
    {
        syscall (SYS_futex, addr, _GLIBCXX_FUTEX_WAIT, val);
        return true;
    }

    static void futex_notify_all(unsigned* addr)
    {
        syscall (SYS_futex, addr, _GLIBCXX_FUTEX_WAKE, INT_MAX);
    }
};

template <unsigned _Waiter_bit = 0x80000000>
struct atomic_futex_unsigned : public atomic_futex_unsigned_base
{
    std::atomic<unsigned> _M_data;

    atomic_futex_unsigned(unsigned data) : _M_data(data) {}

    [[always_inline]] unsigned load(std::memory_order mo) const
    {
        return _M_data.load(mo) & ~_Waiter_bit;
    }

    // Returns the operand's value if equal is true or a different
    // value if equal is false.
    // The assumed value is the caller's assumption about the current value
    // when making the call.
    unsigned load_and_test_for_slow(unsigned assumed, unsigned operand, bool equal, std::memory_order mo)
    {
        for (;;)
        {
            // Don't bother checking the value again because we expect the caller to
            // have done it recently.
            // memory_order_relaxed is sufficient because we can rely on just the
            // modification order (store_notify uses an atomic RMW operation too),
            // and the futex syscalls synchronize between themselves.
            _M_data.fetch_or(_Waiter_bit, std::memory_order_relaxed);
            bool ret = futex_wait_for((unsigned*)(void*)&_M_data, assumed | _Waiter_bit);
            // Fetch the current value after waiting (clears _Waiter_bit).
            assumed = load(mo);
            if (!ret || ((operand == assumed) == equal))
                return assumed;
        }
    }

    [[always_inline]] void load_when_equal(unsigned val, std::memory_order mo)
    {
        // fast path: atomic load + check if expected
        unsigned i = load(mo);
        if ((i & ~_Waiter_bit) == val)
            return;
        // slow path: we need to check because not expected
        load_and_test_for_slow(i, val, true, mo);
    }

    [[always_inline]] void store_notify_all(unsigned val, std::memory_order mo)
    {
        unsigned* futex = (unsigned *)(void *)&_M_data;
        if (_M_data.exchange(val, mo) & _Waiter_bit)
            futex_notify_all(futex);
    }
};

class state_base {
public:
    virtual void _M_complete_async() {
       _M_status.load_when_equal(status::ready, std::memory_order_acquire);
    }
    virtual ~state_base() = default;
    void _M_set_result() {
        _M_status.store_notify_all(status::ready, std::memory_order_release);
    }
    bool ready() const {
        auto status = _M_status.load(std::memory_order_acquire);
        return status == status::ready;
    }
private:
    atomic_futex_unsigned<>	_M_status { status::not_ready};
};

}

static void test_space_savings()
{
    {
        before::state_base sb;
        static_assert(sizeof sb == 104);
        sb._M_set_result();
        sb._M_complete_async();
        assert(sb.ready());
    }
    // nothing from ltrace here
    {
        after::state_base sb;
        static_assert(sizeof sb == 16);
        sb._M_set_result();
        sb._M_complete_async();
        assert(sb.ready());
    }
}

// pthread_mutex_lock, pthread_mutex_unlock, pthread_cond_wait, pthread_cond_signal
// no futex calls
static void test_no_threads()
{
    for (auto _ = 0; _ < 10; _++) {
        before::state_base base;
        base._M_set_result();
        base._M_complete_async();
        assert(base.ready());
    }
}

// pthread_mutex_lock, pthread_mutex_unlock, pthread_cond_wait, pthread_cond_signal
// no futex calls
static void test_threads_dummy()
{
    before::state_base base;
    std::thread t{
        [&base](){
                base._M_complete_async();
                assert(base.ready());
        }
    };
    base._M_set_result();
    t.join();
}

// slow_path: FUTEX_WAIT/FUTEX_WAKE
static void test_threads_after_force_futex_and_slow_path()
{
    using namespace std::chrono_literals;
    std::vector<after::state_base> bases {10};
    std::thread t{
        [&bases](){
            for (auto &&base : bases) {
                base._M_complete_async();
                assert(base.ready());
            }
        }
    };

    for (auto &&base : bases) {
        std::this_thread::sleep_for(1ms);
        base._M_set_result();
    }
    t.join();
}

// pthread_mutex_lock, pthread_mutex_unlock, pthread_cond_wait, pthread_cond_signal
// no futex calls
static void perf_test_threads_before()
{
    std::vector<before::state_base> bases {100000};
    std::thread t{
        [&bases](){
            auto t0 = realtime_now();
            for (auto &&base : bases) {
                base._M_complete_async();
                assert(base.ready());
            }
            auto t1 = realtime_now();
            std::cout << "time before: " << (t1 - t0)/1000000u << "ms" << std::endl;
        }
    };

    for (auto &&base : bases) {
        base._M_set_result();
    }
    t.join();

}

// no futex calls
static void perf_test_threads_after()
{
    std::vector<after::state_base> bases {100000};
    std::thread t{
        [&bases](){
            auto t0 = realtime_now();
            for (auto &&base : bases) {
                base._M_complete_async();
                assert(base.ready());
            }
            auto t1 = realtime_now();
            std::cout << "time after: " << (t1 - t0)/1000000u << "ms" << std::endl;
        }
    };

    for (auto &&base : bases) {
        base._M_set_result();
    }
    t.join();
}

int main()
{
    test_basics();
    test_async();
    test_space_cost();
    test_inline_then_concept();

    test_space_savings();
    test_threads_after_force_futex_and_slow_path();
    // for 100000000
    perf_test_threads_before();
    perf_test_threads_after();
    return 0;
}
