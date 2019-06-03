#include <experimental/coroutine>
#include <numeric>
#include <future>
#include <iostream>

/*
ref1: http://cpp.mimuw.edu.pl/files/await-yield-c++-coroutines.pdf

* those naked coroutines can't be use out-of-the-box, some extra helpers are needed,
  another problem - co_await doesn't work with std::future, can work with boost::future but 'then' is needed :(

* co_return -
* co_yield - lazy generation, suspends coroutine and resume when caller needs extra value
* co_await - suspends coroutine and resume when result is available, needs await_ready

future<int> can be both awaited on and returned from a coroutine.
generator<int> can be returned from a coroutine, but cannot be awaited on!

* Boost.Coroutines - stackful (fibers), C#/C++20 - stackless (only locals + return address),
    - so top-level one stack frame is preserved. Each "stack frame" is dynamicly allocated.

ref2: https://en.cppreference.com/w/cpp/language/coroutines
ref3: https://lewissbaker.github.io/2018/09/05/understanding-the-promise-type

* using one of co_return, co_yield or/and co_await triggers  compiler to generate state-machine
  that allows it to suspend execution at particular points within the function and then later resume execution

* Promise concept:
  coroutine’s promise object =  “coroutine state controller” != std::promise

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

* test2 - demonstration of efficiency, everything computed during compilation,
    generated:
        mov     eax, 1190
        ret
*/

namespace stdx = std::experimental;

// needed for co_return, must be in global namespace
template <typename R, typename... Args>
struct stdx::coroutine_traits<std::future<R>, Args...> {
    //
    struct promise_type {
        std::promise<R> p;
        suspend_never initial_suspend() { return {}; }
        suspend_never final_suspend() { return {}; }
        void return_value(int v) {
            p.set_value(v);
        }
        // cannot have simultanuelsy return_void with return_value
        //void return_void() {}
        std::future<R> get_return_object() { return p.get_future(); }
        void unhandled_exception() { p.set_exception(std::current_exception()); }
    };
};

namespace co_return_basics {

// f - coroutine
static std::future<int> f() {
    //1. create promise_type<int> = p
    //2. call p.return_value() and then p.final_suspend()
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

// co_await specialization for std::chrono::duration
// Stateless Awaitable
template <typename Rep, typename Period>
auto operator co_await(std::chrono::duration<Rep, Period> d) {
    struct [[nodiscard]] Awaitable {
        std::chrono::system_clock::duration duration;

        Awaitable(std::chrono::system_clock::duration d) : duration(d){}
        bool await_ready() const { return duration.count() <= 0; }
        void await_resume() {}
        void await_suspend(stdx::coroutine_handle<> h){
            // here we should attach async sleep and then h.resume(); in countinuation
            // await_suspend should return immediately
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

static std::future<void> test() {
    std::cout << "just about go to sleep...\n";
    co_await 1s; //explicit then ??
    std::cout << "resumed\n";
    co_await 2s;
    std::cout << "resumed\n";
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

int main() {
    co_return_basics::test();
    co_yield_basics::test();
    assert(co_yield_basics::test2() == 1190);
    //compromise_executors_proposal::test();
    co_await_basics::test().get();
    return 0;
}
