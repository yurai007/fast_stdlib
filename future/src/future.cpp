#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <memory>
#include <iostream>
#include <cassert>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

class state_base {
public:
    virtual void _M_complete_async() {
        std::unique_lock __lock(_M_mutex);
        _M_cond.wait(__lock, [&] { return ready(); });
    }
    virtual ~state_base() = default;
    void _M_set_result() {
        _M_status = status::ready;
        _M_cond.notify_all();
    }
    bool ready() const {
        return _M_status == status::ready;
    }
private:
    std::mutex               	_M_mutex;
    std::condition_variable  	_M_cond;
    enum class status : unsigned char {ready, not_ready};
    status _M_status {status::not_ready};
};

template<class function = std::function<void(void)>>
class deffered_state final : public state_base {
public:
    deffered_state() = default;
    deffered_state(function && func) : _M_fn(func) {}
    void _M_complete_async() override {
        _M_fn();
    }
private:
    function _M_fn;
};

template<class function = std::function<void(void)>>
class async_state final : public state_base {
public:
    async_state() = default;
    async_state(function && func) : _M_fn(func) {}
    void _M_complete_async() override {
        std::call_once(flag, [this](){ this->_M_fn(); });
    }
private:
    function _M_fn;
    std::once_flag flag;
};

enum class async_policy { deffered, async };

template<
        template<class T> class shared_ptr = std::shared_ptr
        >
class future_void {
    shared_ptr<state_base> _M_state;
public:
    future_void() = default;
    future_void(const future_void&) = delete;
    future_void& operator=(const future_void&) = delete;
    future_void(const shared_ptr<state_base> &state, bool) : _M_state(state) {}

    template<class Func>
    future_void(Func &&func, async_policy policy)
        : _M_state(alocate_state<Func>(FWD(func), policy))
    {}

    void wait() {
        _M_state->_M_complete_async();
        _M_state = nullptr;
    }
    bool valid() const {
        return static_cast<bool>(_M_state);
    }
private:
    template<class Func>
    static inline shared_ptr<state_base> alocate_state(Func &&func, async_policy policy) {
        if (policy == async_policy::deffered)
            return std::make_shared<deffered_state<Func>>(FWD(func));
        else
            return std::make_shared<async_state<Func>>(FWD(func));
    }
};

template<
        template<class T> class shared_ptr = std::shared_ptr
        >
class promise_void {
    shared_ptr<state_base> _M_future;
public:
    promise_void() = default;
    promise_void(const promise_void&) = delete;
    promise_void& operator=(const promise_void&) = delete;
    future_void<> get_future() {
        _M_future = std::make_shared<state_base>();
        return future_void<>(_M_future, true);
    }

    void set_value() {
        _M_future->_M_set_result();
    }
};

template<typename Func>
[[nodiscard]] inline auto async_void(Func&& f, async_policy policy = async_policy::deffered)
{
    return future_void(FWD(f), policy);
}

template<typename Ret>
using func_ptr = Ret (*)();

template<typename Ret>
struct lightweight_stateless_function
{
    lightweight_stateless_function(func_ptr<Ret> func) : m_func(func) {}

    inline Ret operator()()
    {
        return m_func();
    }
    operator bool() const
    {
        return m_func != nullptr;
    }
    func_ptr<Ret> m_func {nullptr};
};

using lightweight_deffered_state = deffered_state<lightweight_stateless_function<void>>;

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

// ref: https://vittorioromeo.info/index/blog/zeroalloc_continuations_p0.html

struct root {};

template <typename Parent, typename F>
struct node;

template <typename F>
auto initiate(F&& f)
{
    return node{root{}, FWD(f)};
}

template <typename Parent, typename F>
struct node : Parent, F
{
    template <typename ParentFwd, typename FFwd>
    node(ParentFwd&& p, FFwd&& f) : Parent{FWD(p)}, F{FWD(f)}
    {}

    auto& as_parent() noexcept
    {
        return static_cast<Parent&>(*this);
    }

    auto& as_f() noexcept
    {
        return static_cast<F&>(*this);
    }

    void call_with_parent()
    {
        if constexpr(std::is_same_v<Parent, root>)
        {
            // ok, we are in root call lambda<>::operator()
            as_f()();
        }
        else
        {
            // we are not in root, we have parent (base) class so delegate to it
            as_f()(as_parent());
        }
    }

    template <typename FThen>
    auto then(FThen&& f_then)
    {
        return ::node{std::move(*this),
            [f_then = FWD(f_then)](auto& parent) mutable
            {
                parent.call_with_parent();
                return f_then();
            }};
    }

    template <typename Scheduler>
    void execute(Scheduler&& s)
    {
        s([this]() -> decltype(auto)
        {
            call_with_parent();
        });
    }
};

template <typename ParentFwd, typename FFwd>
node(ParentFwd&&, FFwd&&)
    -> node<std::decay_t<ParentFwd>, std::decay_t<FFwd>>;

struct inline_scheduler
{
    template <typename F>
    void operator()(F&& f) const
    {
        f();
    }
};

static void test_inline_then_concept()
{
    {
        // creates node<root, lambda<>> which is derived from lambda,
        // so callable is stored inside lambda
        auto f = initiate([]{
            std::cout << "init\n";
        })
        .then([](){
             std::cout << "and then...\n";
        })
        .then([](){
             std::cout << "and finally we are here.\n";
        });

        // execute() -> synchronous_scheduler::operator() ->
        //           -> call_with_parent() -> as_f()(as_parent()) ->
        // lambda()::operator()
       f.execute(inline_scheduler{});
    }
}

int main()
{
    test_basics();
    test_async();
    test_inline_then_concept();
    return 0;
}
