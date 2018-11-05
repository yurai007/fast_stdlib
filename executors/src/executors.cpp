#include "../../future/src/future.hpp"
#include "executors.hpp"
#include <boost/range/irange.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/numeric.hpp>
#include <iomanip>
#include <iostream>
#include <cassert>
#include <array>
#include <vector>
#include <atomic>
#include <ctime>

static inline uint64_t realtime_now()
{
    #define TIMESPEC_NSEC(ts) ((ts)->tv_sec * 1000000000ULL + (ts)->tv_nsec)
    struct timespec now_ts;
    clock_gettime(CLOCK_REALTIME, &now_ts);
    return TIMESPEC_NSEC(&now_ts);
}

namespace executors {

static void basic_test() {
    execution::inline_executor executor;
    auto agent = execution::require(executor, execution::single, execution::oneway);
    auto done = false;
    agent.execute([&done](){
        done = true;
    });
    assert(done);
}

static void oneway_test() {
    std::array<std::atomic<unsigned>, 4> sums = {0u, 0u, 0u, 0u};
    constexpr auto tasks = 10000u; //10000000u;
    auto t0 = realtime_now();
    {
        execution::static_thread_pool pool {4};
        for (auto i : boost::irange(0u, tasks))
        {
            pool.oneway_execute([i, &sums](){
                sums[i%4] += i;
            });
        }
        pool.start();
    }
    auto t1 = realtime_now();
    std::cout << "sum: " << sums[0] + sums[1] + sums[2] + sums[3] << std::endl;
    std::cout << "time: " << (t1 - t0)/10000000u << "ms" << std::endl;
}

namespace twoway_test {

template <class Executor, class Function>
auto async(Executor ex, Function f)
{
  return execution::require(ex, execution::twoway).twoway_execute(std::move(f));
}

const auto chunks = std::thread::hardware_concurrency();

static double sumLeibnitzBetween(unsigned start, unsigned end)
{
    return boost::accumulate( boost::irange(start, end), 0.0, [](auto res, auto i){
        return res + ((i%2 == 0)? 1.0/(1+2*i) : -1.0/(1+2*i));
    });
}

static double sumLeibnitzSerial(unsigned n)
{
    const auto step = n/chunks;
    return boost::accumulate( boost::irange(0u, n-1, step), 0.0, [step](auto res, auto i){
        return res + sumLeibnitzBetween(i, i + step - 1);
    });
}

static double sumLeibnitzPararell(unsigned n)
{
    const auto step = n/chunks;
    auto result = 0.0;
    std::vector<std::future<double>> partials;
    execution::static_thread_pool pool{chunks};

    for (auto i = 0u; i < n; i += step)
    {
        partials.emplace_back(async(pool.executor(), [i, step]{
            return sumLeibnitzBetween(i, i + step - 1);
        }));
    }
    for (auto &&p : partials)
    {
        result += p.get();
    }
    return result;
}

static void tests()
{
    std::cout << "chunks: " << chunks << "\n";
    {
        auto t0 = realtime_now();
        std::cout << "serial:   " << std::setprecision(17) << 4.0*sumLeibnitzSerial(2000u) << "\n"; //2000000000u
        auto t1 = realtime_now();
        std::cout << "time: " << (t1 - t0)/10000000u << "ms" << std::endl;
    }
    {
        auto t0 = realtime_now();
        std::cout << "pararell: " << std::setprecision(17) << 4.0*sumLeibnitzPararell(2000u) << "\n";
        auto t1 = realtime_now();
        std::cout << "time: " << (t1 - t0)/10000000u << "ms" << std::endl;
    }
}

}
}

int main()
{
    executors::basic_test();
    executors::oneway_test();
    executors::twoway_test::tests();
    return 0;
}
