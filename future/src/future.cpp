#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <memory>
#include <iostream>
#include <cassert>

class state_base {
public:
    virtual void _M_complete_async() {
        std::unique_lock<std::mutex> __lock(_M_mutex);
        _M_cond.wait(__lock, [&] { return ready(); });
    }
    virtual ~state_base() = default;
    void _M_set_result() {
        _M_status = status::ready;
        _M_cond.notify_all();
    }
    bool ready() const {
        return (_M_status == status::ready);
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
    deffered_state(function && func) : _M_fn(func) {}
    void _M_complete_async() override {
        _M_fn();
    }
private:
    function _M_fn;
};

template<
        template<class T> class shared_ptr = std::shared_ptr
        >
class future_void {
    shared_ptr<state_base> _M_state;
public:
    future_void() = default;
    future_void(const future_void&) = delete;
    future_void& operator=(const future_void&) = delete;
    future_void(const shared_ptr<state_base> &state) : _M_state(state) {}
    template<class Func>
    future_void(Func &&func)
        : _M_state(std::make_shared<deffered_state<>>(func)){
    }
    void wait() {
        _M_state->_M_complete_async();
        _M_state = nullptr;
    }
    bool valid() const {
        return static_cast<bool>(_M_state);
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
        return future_void(_M_future);
    }

    void set_value() {
        _M_future->_M_set_result();
    }
};

template<class function = std::function<void(void)>>
class async_state final : public state_base {
public:
    void _M_complete_async() override {}
private:
    std::thread _M_thread;
    function _M_fn;
};

template<typename Func>
[[nodiscard]] inline auto async_void_deferred(Func&& f)
{
    return future_void(std::forward<Func>(f));
}

static void test_basics()
{
    /*
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
    */
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
        // f.wait();
        assert(f.valid());
    }
    // TO DO: cont this
    {
        auto done = false;
        auto f = async_void_deferred([&done](){
            std::cout << "Hello Async World!\n";
            done = true;
        });
        assert(f.valid());
        assert(!done);
        f.wait();
        assert(!f.valid());
        assert(done);
    }
}

int main()
{
    test_basics();
    return 0;
}
