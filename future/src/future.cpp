#include "future.hpp"
#include <iostream>
#include <cassert>
#include <array>

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

int main()
{
    test_basics();
    test_async();
    test_space_cost();
    test_inline_then_concept();
    return 0;
}
