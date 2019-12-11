#include "channel.hh"
#include "cooperative_scheduler.hh"
#include <experimental/coroutine>
#include <experimental/thread_pool>
#include <numeric>
#include <future>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <thread>
#include <mutex>
#include <cassert>

#include <unistd.h>
#include <vector>
#include <setjmp.h>
#include <ucontext.h>
#include <dlfcn.h>
#include <setjmp.h>
#include <cstdlib>
#include <cstdio>
#include <string.h>
#include <string>
#include <cassert>
#include <signal.h>
#include <time.h>
#include <syscall.h>
#include <functional>

/*
ref1: http://cpp.mimuw.edu.pl/files/await-yield-c++-coroutines.pdf

* those naked coroutines can't be use out-of-the-box, some extra helpers are needed,
  another problem - co_await doesn't work with std::future, can work with boost::future but 'then' is needed :(

* co_return - finalization point
* co_yield - lazy generation, suspends coroutine and resume when caller needs extra value
* co_await - suspension point, suspends coroutine and resume when result is available, needs await_ready

future<int> can be both awaited on and returned from a coroutine.
generator<int> can be returned from a coroutine, but cannot be awaited on!

* Boost.Coroutines - stackful (fibers), C#/C++20 - stackless (only locals + return address),
    - so top-level one stack frame is preserved. Each "stack frame" is dynamicly allocated.

ref2: https://en.cppreference.com/w/cpp/language/coroutines
ref3: https://lewissbaker.github.io/2018/09/05/understanding-the-promise-type

* using one of co_return, co_yield or/and co_await triggers  compiler to generate state-machine
  that allows it to suspend execution at particular points within the function and then later resume execution

* Promise concept:
  - coroutine’s promise object =  “coroutine state controller” != std::promise
  - manipulated from inside the coroutine

  coroutine function with <body-statements> generated to:

    {
      // initial part
      co_await promise.initial_suspend();
      // resumable part
      try
      {
        <body-statements>
      }
      catch (...)
      {
        promise.unhandled_exception();
      }
    // cleanup
    FinalSuspend:
      co_await promise.final_suspend();
    }

* coroutine_handle - provides a way of controlling the execution and lifetime of the coroutine.
  stdx::suspend_always - operation is lazily started
  stdx::suspend_never - operation eagerly started
  It's manipulated from outside the coroutine. This is a non-owning handle used to resume execution of the coroutine or to destroy the coroutine frame.

* coroutine state - internal, heap-allocated (unless the allocation is optimized out), object that contains
    - the promise object
    - the parameters (all copied by value)
    - some representation of the current suspension point, so that resume knows where to continue and destroy knows what local variables were in scope
    - local variables and temporaries whose lifetime spans the current suspension point

* heap allocation maybe optimized out (e.g if the size of coroutine frame is known at the call site) and then coroutine state is embedded in the caller's stack frame

* test2 - demonstration of efficiency, everything computed during compilation,
    generated:
        mov     eax, 1190
        ret
* await_transform - for cancellation

* ref2: https://llvm.org/devmtg/2016-11/Slides/Nishanov-LLVMCoroutines.pdf
*/

namespace stdx = std::experimental;
namespace execution = stdx::execution;
using stdx::static_thread_pool;
template<class T>
using future = execution::executor_future_t<static_thread_pool::executor_type, T>;
template<class T>
using promise = stdx::executors_v1::promise<T>;

namespace co_return_basics {

/* f - coroutine/suspending function

  Flow in f():
  1. create stdx::coroutine_traits<std::future<R>>::promise_type
  2. get_return_object()
  3. initial_suspend()
  4. suspend_never::await_ready()
  5. suspend_never::await_resume()
  6. "Entered" !
  7. co_return
  8. return_value()
  9. final_suspend()
  10. suspend_never::await_ready()
  11. suspend_never::await_resume()
  12. result.get()
*/
static std::future<int> f() {
    std::cout << "Entered" << std::endl;
    co_return 42;
}

static void g() {
    auto result = f();
    assert(result.get() == 42);
}

static auto test() {
    f();
    g();
}
}

// needed for co_yield
template <typename T> struct generator {
    struct promise_type {
        T current_value;
        // extension point for co_yield
        stdx::suspend_always yield_value(T value) {
            this->current_value = value;
            return {};
        }
        stdx::suspend_always initial_suspend() { return {}; }
        stdx::suspend_always final_suspend() { return {}; }
        generator get_return_object() { return generator{this}; }
        void unhandled_exception() { std::terminate(); }
        // extension point for co_return
        void return_void() {}
    };

    struct iterator {
        stdx::coroutine_handle<promise_type> _Coro;
        bool _Done;

        iterator(stdx::coroutine_handle<promise_type> Coro, bool Done)
            : _Coro(Coro), _Done(Done) {}

        iterator &operator++() {
            _Coro.resume();
            _Done = _Coro.done();
            return *this;
        }

        bool operator==(iterator const &_Right) const {
            return _Done == _Right._Done;
        }

        bool operator!=(iterator const &_Right) const { return !(*this == _Right); }
        T const &operator*() const { return _Coro.promise().current_value; }
        T const *operator->() const { return &(operator*()); }
    };

    iterator begin() {
        p.resume();
        return {p, p.done()};
    }
    iterator end() { return {p, true}; }
    // not Copyable but Movable
    generator(generator const&) = delete;
    generator(generator &&rhs) : p(rhs.p) { rhs.p = nullptr; }

    ~generator() {
        if (p)
            p.destroy();
    }

private:
    explicit generator(promise_type *p)
        : p(stdx::coroutine_handle<promise_type>::from_promise(*p)) {}

    stdx::coroutine_handle<promise_type> p;
};


namespace co_yield_basics {

static void test() {
    // coroutine in lambda
    auto coro = []() -> generator<int> {
        std::cout << __FUNCTION__ << " started\n";
        for(int i = 0; i < 10; ++i){
            std::cout<<"Next: "<<i;
            co_yield i;
        }
        std::cout << __FUNCTION__ << " ends\n";
    };

    for(auto i : coro()) {
        std::cout << " Got " << i << "\n";
    }
}

template <typename T>
generator<T> seq() {
    for (T i = {};; ++i)
        co_yield i;
}

template <typename T>
generator<T> take_until(generator<T>& g, T limit) {
    for (auto&& v: g)
        if (v < limit) co_yield v;
        else break;
}

template <typename T>
generator<T> multiply(generator<T>& g, T factor) {
    for (auto&& v: g)
        co_yield v * factor;
}

template <typename T>
generator<T> add(generator<T>& g, T adder) {
    for (auto&& v: g)
        co_yield v + adder;
}

static auto test2() {
    auto s = seq<int>();
    auto t = take_until(s, 10);
    auto m = multiply(t, 2);
    auto a = add(m, 110);
    return std::accumulate(a.begin(), a.end(), 0);
}
}

#include <chrono>

/*
 * Awaiter type implements the 3 methods that are called as part of a co_await expression: await_ready, await_suspend and await_resume
 */
namespace co_await_basics {

static_thread_pool _pool{1};

// co_await specialization for std::chrono::duration
// Stateless Awaitable
template <typename Rep, typename Period>
auto operator co_await(std::chrono::duration<Rep, Period> d) {
    struct [[nodiscard]] Awaitable {
        std::chrono::system_clock::duration _duration;

        Awaitable(std::chrono::system_clock::duration d) : _duration(d){}
        bool await_ready() const { return _duration.count() <= 0; }
        void await_resume() {}
        void await_suspend(stdx::coroutine_handle<> h){
            // here we should attach async sleep and then h.resume(); in countinuation
            // await_suspend should return immediately
            // ofc linux timer would be better
           execution::require(_pool.executor(), execution::single, execution::oneway).execute([this, h]() mutable {
                std::this_thread::sleep_for(_duration);
                h.resume();
            });
        }
    };
    return Awaitable{d};
}

// async_read_some which can work with co_await
// Stateful Awaitable
template <typename Socket, typename BufferSeq, typename Error>
auto async_read_some(Socket& socket, const BufferSeq &buffer) {
    struct [[nodiscard]] Awaitable {
        Socket& s;
        const BufferSeq & b;
        Error ec;
        size_t n;

        bool await_ready() { return false; }
        void await_suspend(stdx::coroutine_handle<> h) {
            s.async_read_some(b, [this, h](auto ec, auto n) mutable {
                this->ec = ec;
                this->n = n;
                h.resume();
            });
        }
        auto await_resume() {
            if (ec)
                throw std::system_error(ec);
            return n;
        }
    };
    return Awaitable{socket, buffer};
}

using namespace std::chrono;

static std::future<int> test() {
    std::cout << "just about go to sleep...\n";
    co_await 1s; //explicit then ??
    std::cout << "resumed\n";
    co_await 2s;
    std::cout << "resumed\n";
    _pool.wait();
    co_return 0;
}
}

/*
 * The Compromise Executors Proposal: A lazy simplification of P0443
   ref: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1194r0.html
 * scheduled for C++23

Old way: fut = executor.bulk_twoway_execute(f, n, sf, rf);

New way:
   2 steps: task creation + task submission
   - enable lazy futures (separated lazy submission, before that - creation of future = submission to executor)
     because the code returning the future can rely on the fact that submit will be called later by the caller.
   - set_done ~ cancelletion channel
   - set_error - error channel
   - set_value - value channel
 */
namespace compromise_executors_proposal {

// property
struct receiver_t {
    static inline constexpr bool is_requirable = false;
    static inline constexpr bool is_preferable = false;
};

#ifndef __clang__ // Xclang -fconcepts-ts doesn't work yet
// ok, query(), set_done(), Receiver ~ std::promise ?
template <class To>
concept bool Receiver = requires (To& to) {
        query(to, receiver_t{});
        set_done(to);
};

template <class Error = std::exception_ptr, class... Args>
struct sender_desc {
    using error_type = Error;

    template <template <class...> class List>
    using value_types = List<Args...>;
};

// property
struct sender_t {
    static inline constexpr bool is_requirable = false;
    static inline constexpr bool is_preferable = false;
};

// Exposition only:
template <class From>
concept bool _Sender = requires (From& from) {
        query(from, sender_t{});
};

// ok, query() and get_executor()
template <class From>
concept bool Sender = _Sender<From> && requires (From& from) {
        get_executor(from) -> _Sender;
};

// ok, get_executor(), query(), set_done(), submit(), but why it's Receiver as well?
template <class From, class To>
concept bool SenderTo =
        Sender<From> &&
        Receiver<To> &&
        requires (From& from, To&& to) {
        submit(from, (To&&) to);
};

struct executor {
    template<class F>
    void execute(F&& func);

    template<class F, class __Sender>
    __Sender make_value_task(sender_t &&, F&& func) {
        static_assert(SenderTo<__Sender, __Sender>);
        return __Sender{};
    }

    template<class __Sender, class F, class RF, class PF>
    __Sender make_bulk_value_task(sender_t &&, F&& func, size_t n, RF &&rf, PF &&pf) {
        static_assert(SenderTo<__Sender, __Sender>);
        return __Sender{};
    }
};

struct my_sender {
    template <class To>
    void query(To, receiver_t);

    template <class To>
    void set_done(To to);

    template <class from>
    my_sender get_executor(from);

    template <class From, class To>
    void submit(From from, To&& to);

    template <class To>
    void submit(To&& to);
};

static auto test() {
    auto sf = 321, rf = 42;
    using futPromise = int;
    executor{}.make_bulk_value_task<my_sender>(sender_t{}, [](){}, 123, sf, rf).submit(futPromise{});
}
#endif

}

auto bye = -123;

std::future<int> producer(channel<int>& ch) {
    auto msg = 1;
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
    co_await ch.write(msg);
    co_return 0;
}

std::future<int> ok_consumer(channel<int>& ch) {
    auto [msg, ok] = co_await ch.read();
    std::cout << msg << std::endl;
    co_return 0;
}

std::future<int> producer2(channel<int>& ch) {
    auto msg = 1;
    co_await ch.write(msg);
    co_await ch.write(bye);
    co_return 0;
}

std::future<int> ok_consumer2(channel<int>& ch) {
    for (auto [msg, ok] = co_await ch.read(); ok && msg != bye;
         std::tie(msg, ok) = co_await ch.read()) {
            std::cout << msg << std::endl;
    }
    co_return 0;
}

channel<int, std::mutex> _channel;

static void test_channel_with_suspending_reader() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::thread p([]{
        producer(_channel).get();
    });

    std::thread c([]{
        ok_consumer(_channel).get();
    });
    p.join();
    c.join();
}

static void test_channel_with_suspending_writer() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::thread p([]{
        producer2(_channel).get();
    });

    std::thread c([]{
        ok_consumer2(_channel).get();
    });
    p.join();
    c.join();
}

static std::future<int> no_threads() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    // fiber 1
    auto [rmsg, ok] = co_await _channel.read();
    std::cout << "Read done " << rmsg << std::endl;
    // fiber 2
    // I need cooperative multitasking between both
    auto msg = 1;
    co_await _channel.write(msg);
    std::cout << "Write done " << std::endl;

    co_return 0;
}

// #include <https://raw.githubusercontent.com/Quuxplusone/coro/master/include/coro/unique_generator.h>
namespace dangling_reference_issue {

/*
ref: https://quuxplusone.github.io/blog/2019/07/10/ways-to-get-dangling-references-with-coroutines/
It would be useful to see state machine generated from Clang AST:
    https://www.andreasfertig.blog/2019/09/coroutines-in-c-insights.html

    * suspend points by deafult may only communicate through shared state on one stack frame
    * test_nok1 - only one stack frame is preserved - ch, s survive but the referenced temporary string not -
    it lives on frame below, use-after-free/dangling reference

    * test_nok2 - still wrong even with local variable ! Still use-after-free/dangling reference.

    * test_nok3 - it's still UB.
    Interesting how it behaves - from link with -O1 it return 255 on godbolt.

    More on coroutines and issues:

    0. Basic - https://stackoverflow.com/questions/43503656/what-are-coroutines-in-c20/44244451#44244451
    1. Easy - http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0973r0.pdf
    2. Hard - http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1745r0.pdf
*/

#if 0
unique_generator<char> explode(const std::string& s) {
    for (char ch : s) {
        co_yield ch;
    }
}

static auto test_ok() {
    using namespace std::string_literals;
    auto s = "Hello World"s;
    for (char ch : explode(s)) {
        std::cout << ch << '\n';
    }
}

static auto test_nok1() {
    for (char ch : explode("Hello World")) {
        std::cout << ch << '\n';
    }
}

unique_generator<char> explode2(const std::string& rs) {
    std::string s = rs;
    for (char ch : s) {
        co_yield ch;
    }
}

static auto test_nok2() {
    for (char ch : explode2("Hello World")) {
        std::cout << ch << '\n';
    }
}

unique_generator<char> explode3(const std::string& s) {
    return [s]() -> unique_generator<char> {
        for (char ch : s) {
            co_yield ch;
        }
    }();
}

static auto test_nok3() {
    for (char ch : explode3("Hello World")) {
        std::cout << ch << '\n';
    }
}
#endif

}

std::promise<bool> done;
std::future<bool> donef = done.get_future();

void fiber1() {
    auto task = []() -> std::future<int> {
       std::cout << "thread1: start" << std::endl;
       auto [msg, ok] = co_await _channel.read();
       std::cout << msg << std::endl;
       std::cout << "thread1: end" << std::endl;
       co_return 0;
    };
    task().get();
    done.set_value(true);
}

void fiber2() {
   auto task = []() -> std::future<int> {
       std::cout << "thread2: start" << std::endl;
       auto msg = 1;
       co_await _channel.write(msg);
       std::cout << "thread2: end" << std::endl;
       co_return 0;
   };
   task().get();
   donef.get();
}


// ref: https://gist.github.com/DanGe42/7148946
namespace scheduler_with_coroutines_demo {

std::promise<bool> done;
std::future<bool> donef = done.get_future();

void fiber1() {
    auto task = []() -> std::future<int> {
       std::cout << "fiber1: start" << std::endl;
       auto [msg, ok] = co_await _channel.read();
       std::cout << msg << std::endl;
       std::cout << "fiber1: end" << std::endl;
       co_return 0;
    };
    task().get();
    done.set_value(true);
}

void fiber2() {
   auto task = []() -> std::future<int> {
       std::cout << "fiber2: start" << std::endl;
       auto msg = 1;
       co_await _channel.write(msg);
       std::cout << "fiber2: end" << std::endl;
       co_return 0;
   };
   task().get();
   donef.get();
}

static void test() {
    cooperative_scheduler{fiber1, fiber2};
}
}

int main() {
    //co_await_basics::test().get();
//    test_channel_with_suspending_reader();
//    test_channel_with_suspending_writer();
    //assert(co_yield_basics::test2() == 1190);
   // no_threads().get();
    scheduler_with_coroutines_demo::test();
    return 0;
}
