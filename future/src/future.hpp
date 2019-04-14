#pragma once

#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <memory>
#include <stdexcept>

#define FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

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
private:
    bool ready() const {
        return _M_status == status::ready;
    }

    std::mutex               	_M_mutex;
    std::condition_variable  	_M_cond;
    enum class status : unsigned char
    {
        ready, not_ready
    } _M_status {status::not_ready};
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
        std::call_once(flag, [this]{ this->_M_fn(); });
    }
private:
    function _M_fn;
    std::once_flag flag;
};

enum class async_policy { deffered, async };

template<
        template<class T> class shared_ptr = std::shared_ptr>
class future_void {
    shared_ptr<state_base> _M_state;
public:
    future_void() = default;
    future_void(const future_void&) = delete;
    future_void& operator=(const future_void&) = delete;
    future_void(const shared_ptr<state_base> &state, bool) noexcept
        : _M_state(state) {}

    template<class Func>
    future_void(Func &&func, async_policy policy)
        : _M_state(alocate_state<Func>(FWD(func), policy))
    {}

    void wait() {
        _M_state->_M_complete_async();
        _M_state = nullptr;
    }
    bool valid() const noexcept {
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
        template<class T> class shared_ptr = std::shared_ptr>
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
        if (_M_future) {
            _M_future->_M_set_result();
        } else {
            throw std::runtime_error("");
        }
    }
};

future_void<> make_ready_future() {
    return future_void([]{}, async_policy::deffered);
}

template <typename T>
struct is_future : std::false_type {};

template <>
struct is_future<future_void<>> : std::true_type {};

#ifndef __clang__
template <typename T>
concept bool Future = is_future<T>::value;

template<class FuturesContainer>
requires requires (FuturesContainer c) {
    std::begin(c);
    std::end(c);
    is_future<decltype(*std::begin(c))>::value;
}
// [[nodiscard]]
inline auto when_all(FuturesContainer &&container) {
    if (std::all_of(container.begin(), container.end(),
                    [](auto &&f) {
                        f.wait();
                        return !f.valid(); })) {
        return make_ready_future();
    } else {
        throw std::runtime_error("");
    }
}
#endif

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
    explicit lightweight_stateless_function(func_ptr<Ret> func) : m_func(func) {}

    inline Ret operator()()
    {
        return m_func();
    }
    operator bool() const noexcept
    {
        return m_func != nullptr;
    }
    func_ptr<Ret> m_func {nullptr};
};

using lightweight_deffered_state = deffered_state<lightweight_stateless_function<void>>;

namespace experimental {

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
        return ::experimental::node{std::move(*this),
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


template<class T>
class future;

template <typename... T>
struct is_future : std::false_type {};

template <typename... T>
struct is_future<future<T...>> : std::true_type {};

#ifndef __clang__
template <typename T>
concept bool Future = is_future<T>::value;

template <typename Func, typename... T>
concept bool CanApply = requires (Func f, T... args) {
    f(std::forward<T>(args)...);
};
#endif


}
