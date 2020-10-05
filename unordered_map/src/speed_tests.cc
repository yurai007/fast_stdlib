#include "cuckoo_hashmap.hh"
#include "open_addressing_hashmap.hh"
#include <ctime>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <sys/types.h>

#define TIMESPEC_NSEC(ts) ((ts)->tv_sec * 1000000000ULL + (ts)->tv_nsec)

static inline uint64_t realtime_now() {
    struct timespec now_ts;
    clock_gettime(CLOCK_REALTIME, &now_ts);
    return TIMESPEC_NSEC(&now_ts);
}

static inline char get_operation() {
    return (rand()%2 == 1)? 'I' : 'M';
}

static long perf_event_open(perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

static int perf_open(perf_event_attr *pe) {
    int fd = perf_event_open(pe, 0, -1, -1, 0);
    if (fd == -1) {
       fprintf(stderr, "Error opening leader %llx\n", pe->config);
       exit(EXIT_FAILURE);
    }
    return fd;
}

static void perf_enable(int fd) {
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
}

static void perf_disable(int fd) {
    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
}

int fd1, fd2, fd3;

static void perf_init() {
    perf_event_attr pe;

    std::memset(&pe, 0, sizeof(perf_event_attr));
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(perf_event_attr);
    pe.config = PERF_COUNT_HW_INSTRUCTIONS;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    fd1 = perf_open(&pe);

    pe.config = PERF_COUNT_HW_CACHE_REFERENCES;
    fd2 = perf_open(&pe);

    pe.config = PERF_COUNT_HW_CACHE_MISSES;
    fd3 = perf_open(&pe);
}

static void perf_close() {
    close(fd3);
    close(fd2);
    close(fd1);
}

constexpr auto stats = true;

namespace raw_array_access {

   static void benchmark(auto capacity, auto operations_number) {
   if constexpr (stats) {
       perf_init();
   }
   const auto uniwersum_size = capacity;
   std::vector<int> raw_set(capacity, -1);
   srand(time(nullptr));
   std::vector<int> lookups_set;
   for (auto i = 0u; i < operations_number; i++) {
       auto operation = get_operation();
       auto item = rand()%uniwersum_size;

       if (operation == 'I') {
           raw_set[item] = item;
       } else {
           lookups_set.push_back(item);
       }
   }
   auto t0 = realtime_now();
   if constexpr (stats) {
       perf_enable(fd1);
       perf_enable(fd2);
       perf_enable(fd3);
   }
   auto found = 0u;
   for (auto n : lookups_set) {
       found += static_cast<unsigned>(raw_set[n] != -1);
   }
   if constexpr (stats) {
       perf_disable(fd1);
       perf_disable(fd2);
       perf_disable(fd3);
   }

   auto t1 = realtime_now();
   auto time_ms = (t1 - t0)/1000000;
   auto alpha = operations_number*1.0f/(2*capacity);
   auto latency = 1'000'000.0f*time_ms/float(operations_number);
   auto throughput = static_cast<unsigned>(1'000*4.0f/latency);
   std::cout << "Test only S:    searches = " << lookups_set.size()
          << "  alpha = " << alpha << "  time = " << time_ms << " ms     latency of search op = "
          << latency << " ns     throughput = " << throughput << " MB/s found = " << found << std::endl;
   if constexpr (stats) {
       long long count1, count2, count3;
       read(fd1, &count1, sizeof(count1));
       read(fd2, &count2, sizeof(count2));
       read(fd3, &count3, sizeof(count3));
       std::cout << "Used " << count1 << " instructions     " << count2 << " cache-references     " <<
                     count3 << " cache-misses" << std::endl;
       perf_close();
   }
}
}

namespace open_addressing_hashmap_benchmarks {

static void benchmark(auto capacity, auto operations_number) {
    constexpr auto uniwersum_size = 2'000'000'000u;
    open_addressing::set<> hashmap(capacity);
    srand(time(nullptr));
    std::vector<int> lookups_set;
    for (auto i = 0u; i < operations_number; i++) {
        auto operation = get_operation();
        auto item = rand()%uniwersum_size;

        if (operation == 'I') {
            hashmap.insert(item);
        } else {
            lookups_set.push_back(item);
        }
    }
    auto t0 = realtime_now();
    auto found = 0u;
    for (auto n : lookups_set) {
       found += static_cast<unsigned>(hashmap.search(n));
    }
    auto t1 = realtime_now();
    auto time_ms = (t1 - t0)/1000000;
    auto alpha = operations_number*1.0f/(2*hashmap.capacity());
    auto latency = 1'000'000.0f*time_ms/float(operations_number);
    auto throughput = static_cast<unsigned>(1'000*4.0f/latency);
    std::cout << "Test only S:    searches = " << lookups_set.size() << " alpha = " << alpha << " collisions = " <<
                 hashmap.collisions << " colisions/search = " <<
                 1.0f*hashmap.collisions/(lookups_set.size()) << " time = " << time_ms << " ms     latency of search op = "
              << latency << " ns" << "   throughput = " << throughput << " MB/s found = " << found << std::endl;

}
}
/* This benchmark test only I+M.

  - with alpha = ~95% it's 5 collisions/op and 62 ns/op.
  - with alpha ~76% it's only 2 collisions/op and 43ns/op.
  - with simple avoidance of %m seems a little bit faster 41ns/op.

    Test only NOK lookups with almost no hits
    Test only S:    searches = 400101 alpha = 0.159999 collisions = 81215, colisions/search = 0.202986      time = 5 ms     time/search op = 6.250000 ns
    Test only S:    searches = 900179 alpha = 0.359999 collisions = 570825, colisions/search = 0.634124      time = 19 ms     time/search op = 10.555555 ns
    Test only S:    searches = 1299544 alpha = 0.519998 collisions = 1646367, colisions/search = 1.266881      time = 43 ms     time/search op = 16.538462 ns
    Test only S:    searches = 1899717 alpha = 0.759997 collisions = 7298450, colisions/search = 3.841862      time = 164 ms     time/search op = 43.157894 ns


  - from GB POV hashmap::member looks quite ok.
    https://godbolt.org/z/yjMaET

  - the trick with hashing though smaller prime + no branching + unrolling first 3 doesn't help much:
    i = hash_holder + 1 + 1*1;
    i = hash_holder + 2 + 2*2;
    ...

  - now we have as reference:

    Test only NOK lookups with almost no hits. WS = 2.5M
    Test only S:    searches = 399830 alpha = 0.159999 collisions = 81062, colisions/search = 0.202741      time = 6 ms     time/search op = 7.500000 ns
    Test only S:    searches = 899558 alpha = 0.359999 collisions = 572405, colisions/search = 0.636318      time = 23 ms     time/search op = 12.777778 ns
    Test only S:    searches = 1300415 alpha = 0.519998 collisions = 1643135, colisions/search = 1.263547      time = 51 ms     time/search op = 19.615385 ns
    Test only S:    searches = 1900521 alpha = 0.759997 collisions = 7284988, colisions/search = 3.833153      time = 180 ms     time/search op = 47.368420 ns
    Test only NOK lookups with almost no hits. WS = 25M
    Test only S:    searches = 3999098 alpha = 0.159999 collisions = 800800, colisions/search = 0.200245      time = 97 ms     time/search op = 12.125000 ns
    Test only S:    searches = 9001370 alpha = 0.359998 collisions = 5609488, colisions/search = 0.623182      time = 292 ms     time/search op = 16.222221 ns
    Test only S:    searches = 12999116 alpha = 0.519998 collisions = 16093382, colisions/search = 1.238037      time = 635 ms     time/search op = 24.423077 ns
    Test only S:    searches = 19004609 alpha = 0.759997 collisions = 70527263, colisions/search = 3.711061      time = 2346 ms     time/search op = 61.736839 ns
 */
/*
 * preliminaries:
   - always disable freq scaling from 'powersave' to 'performance' for all cores. This one works!: -- one path so remove space
     for CPUFREQ in /sys/devices/system/cpu/cpu* /cpufreq/scaling_governor; do [ -f $CPUFREQ ] || continue; echo -n performance > $CPUFREQ; done

 * results from clang:

Test raw access to vector as reference. WS = 2MB
Test only S:    searches = 100496  alpha = 0.199996  time = 0 ms     latency of search op = 0 ns     throughput = 0 MB/s found = 18199
Used 85009 instructions     146708 cache-references     34420 cache-misses
Test only S:    searches = 200152  alpha = 0.399993  time = 0 ms     latency of search op = 0 ns     throughput = 0 MB/s found = 65785
Used 169151 instructions     294780 cache-references     68581 cache-misses
Test only S:    searches = 300421  alpha = 0.599989  time = 0 ms     latency of search op = 0 ns     throughput = 0 MB/s found = 135059
Used 253618 instructions     480321 cache-references     24589 cache-misses
Test only S:    searches = 450363  alpha = 0.899984  time = 0 ms     latency of search op = 0 ns     throughput = 0 MB/s found = 266328
Used 380288 instructions     726076 cache-references     28254 cache-misses
Test raw access to vector as reference. WS = 10MB
Test only S:    searches = 400415  alpha = 0.159999  time = 2 ms     latency of search op = 2.5 ns     throughput = 1600 MB/s found = 58786
Used 338174 instructions     589026 cache-references     406410 cache-misses
Test only S:    searches = 899824  alpha = 0.359999  time = 5 ms     latency of search op = 2.77778 ns     throughput = 1440 MB/s found = 272058
Used 759444 instructions     1336906 cache-references     854685 cache-misses
Test only S:    searches = 1299885  alpha = 0.519998  time = 7 ms     latency of search op = 2.69231 ns     throughput = 1485 MB/s found = 527225
Used 1096975 instructions     1927983 cache-references     1222645 cache-misses
Test only S:    searches = 1900307  alpha = 0.759997  time = 10 ms     latency of search op = 2.63158 ns     throughput = 1520 MB/s found = 1011165
Used 1603624 instructions     2820567 cache-references     1784319 cache-misses
Test raw access to vector as reference. WS = 100MB
Test only S:    searches = 4000444  alpha = 0.159999  time = 36 ms     latency of search op = 4.5 ns     throughput = 888 MB/s found = 590450
Used 3375687 instructions     8928776 cache-references     5927819 cache-misses
Test only S:    searches = 9003039  alpha = 0.359998  time = 80 ms     latency of search op = 4.44444 ns     throughput = 899 MB/s found = 2721901
Used 7596662 instructions     20028312 cache-references     13334650 cache-misses
Test only S:    searches = 13002316  alpha = 0.519998  time = 115 ms     latency of search op = 4.42308 ns     throughput = 904 MB/s found = 5270570
Used 10970927 instructions     29333269 cache-references     19330059 cache-misses
Test only S:    searches = 19005237  alpha = 0.759997  time = 170 ms     latency of search op = 4.47368 ns     throughput = 894 MB/s found = 10111480
Used 16035973 instructions     42229221 cache-references     28121984 cache-misses
OA: test only NOK lookups with almost no hits. WS = 2MB
Test only S:    searches = 100069 alpha = 0.199996 collisions = 26944 colisions/search = 0.269254 time = 1 ms     latency of search op = 5 ns   throughput = 800 MB/s found = 7
Test only S:    searches = 200177 alpha = 0.399993 collisions = 152336 colisions/search = 0.761007 time = 5 ms     latency of search op = 12.5 ns   throughput = 320 MB/s found = 34
Test only S:    searches = 300509 alpha = 0.599989 collisions = 533909 colisions/search = 1.77668 time = 12 ms     latency of search op = 20 ns   throughput = 200 MB/s found = 64
Test only S:    searches = 450531 alpha = 0.899984 collisions = 4939040 colisions/search = 10.9627 time = 21 ms     latency of search op = 23.3333 ns   throughput = 171 MB/s found = 129
OA: test only NOK lookups with almost no hits. WS = 10MB
Test only S:    searches = 400948 alpha = 0.159999 collisions = 81581 colisions/search = 0.20347 time = 6 ms     latency of search op = 7.5 ns   throughput = 533 MB/s found = 88
Test only S:    searches = 900734 alpha = 0.359999 collisions = 571570 colisions/search = 0.63456 time = 23 ms     latency of search op = 12.7778 ns   throughput = 313 MB/s found = 438
Test only S:    searches = 1300552 alpha = 0.519998 collisions = 1644593 colisions/search = 1.26453 time = 49 ms     latency of search op = 18.8462 ns   throughput = 212 MB/s found = 918
Test only S:    searches = 1900721 alpha = 0.759997 collisions = 7273708 colisions/search = 3.82682 time = 134 ms     latency of search op = 35.2632 ns   throughput = 113 MB/s found = 1964
OA: test only NOK lookups with almost no hits. WS = 100MB
Test only S:    searches = 4002384 alpha = 0.159999 collisions = 798013 colisions/search = 0.199384 time = 73 ms     latency of search op = 9.125 ns   throughput = 438 MB/s found = 8593
Test only S:    searches = 8998149 alpha = 0.359998 collisions = 5606608 colisions/search = 0.623085 time = 257 ms     latency of search op = 14.2778 ns   throughput = 280 MB/s found = 42881
Test only S:    searches = 12997563 alpha = 0.519998 collisions = 16095905 colisions/search = 1.23838 time = 554 ms     latency of search op = 21.3077 ns   throughput = 187 MB/s found = 89333
Test only S:    searches = 18999894 alpha = 0.759997 collisions = 70630340 colisions/search = 3.71741 time = 1656 ms     latency of search op = 43.5789 ns   throughput = 91 MB/s found = 190135
Cuckoo: test only NOK lookups with almost no hits. WS = 2MB
Test only S:    rehashes = 1 searches = 99909   capacities = 500029,500041  alpha = 0.19999  time = 1 ms     latency of search op = 5 ns   throughput = 800 MB/s   found = 3
Test only S:    rehashes = 1 searches = 200135   capacities = 500029,500041  alpha = 0.399979  time = 1 ms     latency of search op = 2.5 ns   throughput = 1600 MB/s   found = 15
Test only S:    rehashes = 1 searches = 299996   capacities = 500029,500041  alpha = 0.599969  time = 2 ms     latency of search op = 3.33333 ns   throughput = 1200 MB/s   found = 45
Test only S:    rehashes = 2 searches = 449683   capacities = 1000081,1000099  alpha = 0.899953  time = 6 ms     latency of search op = 6.66667 ns   throughput = 600 MB/s   found = 96
Cuckoo: test only NOK lookups with almost no hits. WS = 10MB
Test only S:    rehashes = 1 searches = 399662   capacities = 2500027,2500049  alpha = 0.159999  time = 7 ms     latency of search op = 8.75 ns   throughput = 457 MB/s   found = 74
Test only S:    rehashes = 1 searches = 899506   capacities = 2500027,2500049  alpha = 0.359997  time = 15 ms     latency of search op = 8.33333 ns   throughput = 480 MB/s   found = 453
Test only S:    rehashes = 1 searches = 1299702   capacities = 2500027,2500049  alpha = 0.519996  time = 22 ms     latency of search op = 8.46154 ns   throughput = 472 MB/s   found = 930
Test only S:    rehashes = 2 searches = 1899609   capacities = 5000077,5000081  alpha = 0.759995  time = 35 ms     latency of search op = 9.21053 ns   throughput = 434 MB/s   found = 1984
Cuckoo: Test only NOK lookups with almost no hits. WS = 100MB
Test only S:    rehashes = 1 searches = 3998130   capacities = 25000363,25000373  alpha = 0.159998  time = 84 ms     latency of search op = 10.5 ns   throughput = 380 MB/s   found = 8334
Test only S:    rehashes = 2 searches = 8999724   capacities = 50000729,50000747  alpha = 0.359995  time = 203 ms     latency of search op = 11.2778 ns   throughput = 354 MB/s   found = 42462
Test only S:    rehashes = 2 searches = 13001127   capacities = 50000729,50000747  alpha = 0.519993  time = 292 ms     latency of search op = 11.2308 ns   throughput = 356 MB/s   found = 89155
Test only S:    rehashes = 4 searches = 19004742   capacities = 200002939,200002951  alpha = 0.759989  time = 789 ms     latency of search op = 20.7632 ns   throughput = 192 MB/s   found = 190907

 Performance counter stats for './speed_tests_cl':
       64315052037      cycles:u                  #    2.716 GHz

 * some summary:
   - direct random access to 25M WS is 4.5ns, then we have for OA (alpha = 0.15) 10 ns.
     For alpha >= 0.35 Cuckoo wins with stable latency = 11.75ns - 12ns even until alpha = 0.52.
     OA for alpha = 0.55 has 23 ns which is much slower then Cuckoo.

   - Number of LLC misses is high and every miss in theory is even 100 ns but here looks like it's ~4ns.
     The only reasonable way to hash is to keep WS <= L3 (2.5MB) then comparing OA with Cuckoo:

OA: test only NOK lookups with almost no hits. WS = 2MB
Test only S:    searches = 100069 alpha = 0.199996 collisions = 26944 colisions/search = 0.269254 time = 1 ms     latency of search op = 5 ns   throughput = 800 MB/s found = 7
Test only S:    searches = 200177 alpha = 0.399993 collisions = 152336 colisions/search = 0.761007 time = 5 ms     latency of search op = 12.5 ns   throughput = 320 MB/s found = 34
Test only S:    searches = 300509 alpha = 0.599989 collisions = 533909 colisions/search = 1.77668 time = 12 ms     latency of search op = 20 ns   throughput = 200 MB/s found = 64
Test only S:    searches = 450531 alpha = 0.899984 collisions = 4939040 colisions/search = 10.9627 time = 21 ms     latency of search op = 23.3333 ns   throughput = 171 MB/s found = 129

VS

Cuckoo: test only NOK lookups with almost no hits. WS = 2MB
Test only S:    rehashes = 1 searches = 99909   capacities = 500029,500041  alpha = 0.19999  time = 1 ms     latency of search op = 5 ns   throughput = 800 MB/s   found = 3
Test only S:    rehashes = 1 searches = 200135   capacities = 500029,500041  alpha = 0.399979  time = 1 ms     latency of search op = 2.5 ns   throughput = 1600 MB/s   found = 15
Test only S:    rehashes = 1 searches = 299996   capacities = 500029,500041  alpha = 0.599969  time = 2 ms     latency of search op = 3.33333 ns   throughput = 1200 MB/s   found = 45
Test only S:    rehashes = 2 searches = 449683   capacities = 1000081,1000099  alpha = 0.899953  time = 6 ms     latency of search op = 6.66667 ns   throughput = 600 MB/s   found = 96

Sometimes for alpha = 0.19 Cuckoo has ~10 ns (2x slower then OA) but except that in general Cuckoo has stable latency <= 5ns for alpha <= 0.6
with throughput arround 1GB/s.

 * interesting - there is no 'realloc' wrapper in  C++ (like new for malloc) and I can't realloc on
                pointer from new (UB), so I need to use malloc+realloc instead :(
 * TO DO1: rehash needs realocation - do it like in std::vector via _ZNSt6vectorIiSaIiEE11_S_relocateEPiS2_S2_RS0_
          which is std::vector<int, std::allocator<int> >::_S_relocate(int*, int*, int*, std::allocator<int>&)
          but no idea where is the source :( Gdb for rescue :)
  TO DO2:
       This one - http://www.cs.cmu.edu/~dongz/papers/cuckooswitch.pdf, according paper cache miss penalty is even ~100 ns.
 */
namespace cuckoo_hashmap_benchmarks {

static void preliminaries() {
    auto n = 1;
    for (auto i = 0; i < 50; i++) {
        n = cuckoo::set<>::prime(n);
        std::cout << n++ << " ";
    }
    std::cout << std::endl;
}

static void benchmark(auto capacity, auto operations_number) {
    constexpr auto uniwersum_size = 2'000'000'000u;
    auto left = static_cast<unsigned>(capacity), right = cuckoo::set<>::prime(left+1);
    cuckoo::set<> hashmap(left, right);
    srand(time(nullptr));
    std::vector<int> lookups_set;
    for (auto i = 0u; i < operations_number; i++) {
        auto operation = get_operation();
        auto item = rand()%uniwersum_size;

        if (operation == 'I') {
            hashmap.insert(item);
        } else {
            lookups_set.push_back(item);
        }
    }
    auto t0 = realtime_now();
    auto found = 0u;
    for (auto n : lookups_set) {
        found += static_cast<unsigned>(hashmap.search(n));
    }
    auto t1 = realtime_now();
    auto time_ms = (t1 - t0)/1000000;
    auto [nleft, nright] = hashmap.capacities();
    auto latency = 1'000'000.0f*time_ms/float(operations_number);
    auto throughput = static_cast<unsigned>(1'000*4.0f/latency);
    auto alpha = operations_number*1.0f/(2*(2*capacity));
    std::cout << "Test only S:    rehashes = " << hashmap.rehash_counter << " searches = " << lookups_set.size()
           << "   capacities = " << nleft << "," << nright << "  alpha = " << alpha << "  time = " << time_ms << " ms     latency of search op = "
           << latency << " ns   throughput = " << throughput << " MB/s   found = " << found << std::endl;
}
}

int main() {
    std::cout << "Test raw access to vector as reference. WS = 2MB\n";
    raw_array_access::benchmark(500'009, 200'000u);
    raw_array_access::benchmark(500'009, 400'000u);
    raw_array_access::benchmark(500'009, 600'000u);
    raw_array_access::benchmark(500'009, 900'000u);

    std::cout << "Test raw access to vector as reference. WS = 10MB\n";
    raw_array_access::benchmark(2'500'009, 800'000u);
    raw_array_access::benchmark(2'500'009, 1'800'000u);
    raw_array_access::benchmark(2'500'009, 2'600'000u);
    raw_array_access::benchmark(2'500'009, 3'800'000u);

    std::cout << "Test raw access to vector as reference. WS = 100MB\n";
    raw_array_access::benchmark(25'000'109, 8'000'000u);
    raw_array_access::benchmark(25'000'109, 18'000'000u);
    raw_array_access::benchmark(25'000'109, 26'000'000u);
    raw_array_access::benchmark(25'000'109, 38'000'000u);

    std::cout << "OA: test only NOK lookups with almost no hits. WS = 2MB\n";
    open_addressing_hashmap_benchmarks::benchmark(500'009, 200'000u);
    open_addressing_hashmap_benchmarks::benchmark(500'009, 400'000u);
    open_addressing_hashmap_benchmarks::benchmark(500'009, 600'000u);
    open_addressing_hashmap_benchmarks::benchmark(500'009, 900'000u);

    std::cout << "OA: test only NOK lookups with almost no hits. WS = 10MB\n";
    open_addressing_hashmap_benchmarks::benchmark(2'500'009, 800'000u);
    open_addressing_hashmap_benchmarks::benchmark(2'500'009, 1'800'000u);
    open_addressing_hashmap_benchmarks::benchmark(2'500'009, 2'600'000u);
    open_addressing_hashmap_benchmarks::benchmark(2'500'009, 3'800'000u);

    std::cout << "OA: test only NOK lookups with almost no hits. WS = 100MB\n";
    open_addressing_hashmap_benchmarks::benchmark(25'000'109, 8'000'000u);
    open_addressing_hashmap_benchmarks::benchmark(25'000'109, 18'000'000u);
    open_addressing_hashmap_benchmarks::benchmark(25'000'109, 26'000'000u);
    open_addressing_hashmap_benchmarks::benchmark(25'000'109, 38'000'000u);

    std::cout << "Cuckoo: test only NOK lookups with almost no hits. WS = 2MB\n";
    cuckoo_hashmap_benchmarks::benchmark(250'013, 200'000u);
    cuckoo_hashmap_benchmarks::benchmark(250'013, 400'000u);
    cuckoo_hashmap_benchmarks::benchmark(250'013, 600'000u);
    cuckoo_hashmap_benchmarks::benchmark(250'013, 900'000u);

    std::cout << "Cuckoo: test only NOK lookups with almost no hits. WS = 10MB\n";
    // cuckoo_hashmap_benchmarks::preliminaries();
    // left capacity + right capacity ~= 2.5M
    cuckoo_hashmap_benchmarks::benchmark(1'250'009, 800'000u);
    cuckoo_hashmap_benchmarks::benchmark(1'250'009, 1'800'000u);
    cuckoo_hashmap_benchmarks::benchmark(1'250'009, 2'600'000u);
    cuckoo_hashmap_benchmarks::benchmark(1'250'009, 3'800'000u);

    std::cout << "Cuckoo: Test only NOK lookups with almost no hits. WS = 100MB\n";
    cuckoo_hashmap_benchmarks::benchmark(12'500'177, 8'000'000u);
    cuckoo_hashmap_benchmarks::benchmark(12'500'177, 18'000'000u);
    cuckoo_hashmap_benchmarks::benchmark(12'500'177, 26'000'000u);
    cuckoo_hashmap_benchmarks::benchmark(12'500'177, 38'000'000u);
    return 0;
}
