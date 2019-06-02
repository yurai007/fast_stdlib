#include <experimental/coroutine>
#include <numeric>
#include <future>
#include <iostream>

/*
ref1: http://cpp.mimuw.edu.pl/files/await-yield-c++-coroutines.pdf
advanced TODO: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1341r0.pdf

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

/*
 * Awaiter type implements the 3 methods that are called as part of a co_await expression: await_ready, await_suspend and await_resume
 */
namespace co_await_basics {

template <typename Socket, typename BufferSeq, typename Error>
auto async_read_some(Socket& socket, const BufferSeq &buffer) {
    struct [[nodiscard]] Awaitable {
        Socket& s;
        const BufferSeq & b;
        Error ec;
        size_t n;

        bool await_ready() { return false; }
        void await_suspend(std::experimental::coroutine_handle<> h) {
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

static auto test() {

}

}


int main() {

    co_return_basics::test();
    co_yield_basics::test();
    assert(co_yield_basics::test2() == 1190);
    co_await_basics::test();
    return 0;
}
