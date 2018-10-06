#include "hashmap.hpp"

namespace benchmarks
{

common::Hashmap<2000003> hashmap;
std::map<int, common::int_holder> stl_map;
std::unordered_map<int, common::int_holder> stl_unordered_map;

#define TIMESPEC_NSEC(ts) ((ts)->tv_sec * 1000000000ULL + (ts)->tv_nsec)

static inline uint64_t realtime_now()
{
	struct timespec now_ts;
	clock_gettime(CLOCK_REALTIME, &now_ts);
	return TIMESPEC_NSEC(&now_ts);
}

static inline char get_operation()
{
    return (rand()%2 == 1)? 'I' : 'M';
}

/* This benchmark test only I+M.
 *
 * TO DO:
    4. Choose the best one and use as reference to cuckoo hashing :)
    5. Delete?
 */
static void benchmark()
{
    constexpr unsigned operations_number {3800000};
    constexpr unsigned uniwersum_size {1000000000};

    unsigned inserts_counter {0};
    unsigned members_counter {0};

    unsigned members_hits {0};
    unsigned stl_members_hits {0};
    unsigned stl_unordered_members_hits {0};

    hashmap.reset();
    stl_map.clear();
    stl_unordered_map.clear();

    assert(hashmap.size() == 0);
    assert(stl_map.size() == 0);
    assert(stl_unordered_map.size() == 0);

    common::int_holder basic_config;
    basic_config.mark = false;

    printf("\n%s\n\n", __FUNCTION__);
    printf("operations_number = %u, uniwersum_size = %u\n", operations_number,
           uniwersum_size);

    printf("Preprocess data\n");
    srand(time(nullptr));
    std::vector<std::pair<char, int>> ops;
    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = get_operation();
        ops.push_back({operation, rand()%uniwersum_size});
    }

    printf("Hashmap start watch\n");
    uint64_t t0 = realtime_now();
    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = ops[i].first;

        if (operation == 'I')
        {
            basic_config.content = ops[i].second;
            hashmap.insert(basic_config);
            assert(hashmap.member(basic_config));
            assert(hashmap.size() > 0);
            inserts_counter++;
        }
        else
            if (operation == 'M')
            {
                basic_config.content = ops[i].second;
                bool hit = hashmap.member(basic_config);
                if (hit)
                    members_hits++;
                members_counter++;
            }
    }
    uint64_t t1 = realtime_now();
    uint64_t time_ms = (t1 - t0)/1000000;
    printf("Hashmap stop watch: Time = %lu ms.\n", time_ms);


    printf("STL Map start watch\n");
    t0 = realtime_now();
    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = ops[i].first;

        if (operation == 'I')
        {
            basic_config.content = ops[i].second;
            stl_map[basic_config.content] = basic_config;
            assert(stl_map.find(basic_config.content) != stl_map.end());
            assert(stl_map.size() > 0);
            inserts_counter++;
        }
        else
            if (operation == 'M')
            {
                basic_config.content = ops[i].second;
                auto stl_iter = stl_map.find(basic_config.content);
                if (stl_iter != stl_map.end())
                    stl_members_hits++;
                members_counter++;
            }
    }
    t1 = realtime_now();
    time_ms = (t1 - t0)/1000000;
    printf("STL Map stop watch: Time = %lu ms.\n", time_ms);


    printf("STL Unordered Map start watch\n");
    t0 = realtime_now();
    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = ops[i].first;

        if (operation == 'I')
        {
            basic_config.content = ops[i].second;
            stl_unordered_map[basic_config.content] = basic_config;
            assert(stl_unordered_map.find(basic_config.content) != stl_unordered_map.end());
            assert(stl_unordered_map.size() > 0);
            inserts_counter++;
        }
        else
            if (operation == 'M')
            {
                basic_config.content = ops[i].second;
                auto stl_iter = stl_unordered_map.find(basic_config.content);
                if (stl_iter != stl_unordered_map.end())
                    stl_unordered_members_hits++;
                members_counter++;
            }
    }
    t1 = realtime_now();
    time_ms = (t1 - t0)/1000000;
    printf("STL Unordered Map stop watch: Time = %lu ms.\n", time_ms);


    printf("Summary\n");
    printf("inserts = %d, members = %d, hits = %d, stl hits = %d, hashmap.size = %d, stl map size = %ld\n",
           inserts_counter/3, members_counter/3, members_hits, stl_members_hits,
           hashmap.size(), stl_map.size());
    printf("hashmap.collisions = %d, colisions per insert = %d\n", hashmap.collisions,
           (hashmap.collisions/(inserts_counter/3)));

    assert(members_hits == stl_members_hits);
    assert(members_hits == stl_unordered_members_hits);
    assert(hashmap.size() == stl_map.size());
    assert(hashmap.size() == stl_unordered_map.size());

    printf("OK :)\n");
}

/* This benchmark test only I+M.
 *
 * TO DO:
    4. Choose the best one and use as reference to cuckoo hashing :)
    5. Delete?
 */
static void benchmark__only_hashmap()
{
    constexpr unsigned operations_number {3800000};
    constexpr unsigned uniwersum_size {1000000000};

    unsigned inserts_counter {0};
    unsigned members_counter {0};
    unsigned members_hits {0};
    hashmap.reset();

    common::int_holder basic_config;
    basic_config.mark = false;

    printf("\n%s\n\n", __FUNCTION__);
    printf("sizeof(config) = %zu, hashmap.capacity() = %u, operations_number = %u, uniwersum_size = %u\n",
           sizeof(basic_config), hashmap.capacity(), operations_number, uniwersum_size);

    printf("Preprocess data\n");
    srand(time(nullptr));
    std::vector<std::pair<char, int>> ops;
    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = get_operation();
        ops.push_back({operation, rand()%uniwersum_size});
    }

    printf("Hashmap start watch\n");
    uint64_t t0 = realtime_now();
    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = ops[i].first;

        if (operation == 'I')
        {
            basic_config.content = ops[i].second;
            hashmap.insert(basic_config);
            assert(hashmap.member(basic_config));
            assert(hashmap.size() > 0);
            inserts_counter++;
        }
        else
            if (operation == 'M')
            {
                basic_config.content = ops[i].second;
                bool hit = hashmap.member(basic_config);
                if (hit)
                    members_hits++;
                members_counter++;
            }
    }
    uint64_t t1 = realtime_now();
    uint64_t time_ms = (t1 - t0)/1000000;
    printf("Hashmap stop watch: Time = %lu ms.\n", time_ms);

    printf("Summary\n");
    printf("inserts = %d, members = %d, hits = %d, hashmap.size = %d\n",
           inserts_counter, members_counter, members_hits,
           hashmap.size());
    printf("hashmap.collisions = %d, colisions per insert = %d\n", hashmap.collisions,
           (hashmap.collisions/(inserts_counter)));

    printf("OK :)\n");
}

/*
     * _mm_sra_epi32 - is bad because treats input vector as ints so
      if input = 0xffffffff after shifting by 31 it's still 0xffffffff !
      That's why _mm_srli_epi32 is needed.
*/
static void test_intrinsics1()
{
    common::hash_uvec msb = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
    __m128i *MSB = (__m128i *)&msb;
    common::hash_vec tmp2 = {0, 0, 0, 0};
    common::hash_vec zer = {0, 0, 0, 0};
    __m128i *TMP2 = (__m128i *)&tmp2;
    __m128i *ZER = (__m128i *)&zer;
    __m128i V1 = _mm_set_epi32(0, 0, 0, 0);

    *TMP2 = _mm_and_si128(V1, *MSB);
    assert(0 == tmp2.i0);
    assert(0 == tmp2.i1);
    assert(0 == tmp2.i2);
    assert(0 == tmp2.i3);
    // if TMP2 = ZER =>  0xffffffff = -1
    *TMP2 = _mm_cmpeq_epi32(*TMP2, *ZER);
    assert(-1 == tmp2.i0);
    assert(-1 == tmp2.i1);
    assert(-1 == tmp2.i2);
    assert(-1 == tmp2.i3);

    //__m128i SHIFT_31 = _mm_set_epi32(31, 31, 31, 31);
    *TMP2 = _mm_srli_epi32(*TMP2, 31); //SHIFT_31

    assert(1 == tmp2.i0);
    assert(1 == tmp2.i1);
    assert(1 == tmp2.i2);
    assert(1 == tmp2.i3);
    printf("%s OK\n", __FUNCTION__);
}

static void test_intrinsics2()
{
    common::hash_uvec msb = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
    __m128i *MSB = (__m128i *)&msb;
    common::hash_uvec tmp2 = {0, 0, 0, 0};
    common::hash_vec zer = {0, 0, 0, 0};
    __m128i *TMP2 = (__m128i *)&tmp2;
    __m128i *ZER = (__m128i *)&zer;
    __m128i V1 = _mm_set_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);

    *TMP2 = _mm_and_si128(V1, *MSB);
    assert(0x80000000 == tmp2.i0);
    assert(0x80000000 == tmp2.i1);
    assert(0x80000000 == tmp2.i2);
    assert(0x80000000 == tmp2.i3);
    // if TMP2 = ZER =>  0xffffffff = -1
    *TMP2 = _mm_cmpeq_epi32(*TMP2, *ZER);
    assert(0 == tmp2.i0);
    assert(0 == tmp2.i1);
    assert(0 == tmp2.i2);
    assert(0 == tmp2.i3);

    //__m128i SHIFT_31 = _mm_set_epi32(31, 31, 31, 31);
    *TMP2 = _mm_srli_epi32(*TMP2, 31); //SHIFT_31

    assert(0 == tmp2.i0);
    assert(0 == tmp2.i1);
    assert(0 == tmp2.i2);
    assert(0 == tmp2.i3);
    printf("%s OK\n", __FUNCTION__);
}

static void test_intrinsics3()
{
    common::hash_uvec msb = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
    // MAX_INT+1 == MIN_INT-1 ??
    __m128i *MSB = (__m128i *)&msb;
    common::hash_vec tmp2 = {0, 0, 0, 0};
    common::hash_vec zer = {0, 0, 0, 0};
    __m128i *TMP2 = (__m128i *)&tmp2;
    __m128i *ZER = (__m128i *)&zer;
    __m128i V1 = _mm_set_epi32(0x80000000, 0, 0x80000000, 0);

    *TMP2 = _mm_and_si128(V1, *MSB);
    assert(0 == tmp2.i0);
    assert(0x80000000 == unsigned(tmp2.i1));
    assert(0 == tmp2.i2);
    assert(0x80000000 == unsigned(tmp2.i3));

    *TMP2 = _mm_cmpeq_epi32(*TMP2, *ZER);
    assert(-1 == tmp2.i0);
    assert(0 == tmp2.i1);
    assert(-1 == tmp2.i2);
    assert(0 == tmp2.i3);

    unsigned i0 = tmp2.i0;
    unsigned r4 = (i0>>31u);

    assert(1 != (tmp2.i0>>30));
    assert(1 != (tmp2.i0>>31));
    assert(1 == r4);
    assert(0 == (tmp2.i1>>31));
    //assert(-1 == tmp2.i2);
    //assert(0 == tmp2.i3);
    printf("%s OK\n", __FUNCTION__);
}


/*

 * For process_search__true

 Hashmap stop watch: Time = 6253 ms.

 Performance counter stats for './main':

       6282.140518      task-clock:u (msec)       #    1.000 CPUs utilized
                 0      context-switches:u        #    0.000 K/sec
                 0      cpu-migrations:u          #    0.000 K/sec
             1,497      page-faults:u             #    0.238 K/sec
    20,951,621,704      cycles:u                  #    3.335 GHz
     8,441,156,915      stalled-cycles-frontend:u #   40.29% frontend cycles idle
    22,928,965,059      instructions:u            #    1.09  insn per cycle
                                                  #    0.37  stalled cycles per insn
     3,132,750,570      branches:u                #  498.676 M/sec
        51,362,603      branch-misses:u           #    1.64% of all branches

       6.283405714 seconds time elapsed

  * For process_search__true__optimized__iter4_bad (no loop, no if-s, no division)

  Hashmap stop watch: Time = 811 ms

 Performance counter stats for './main':

        837.306991      task-clock:u (msec)       #    0.999 CPUs utilized
                 0      context-switches:u        #    0.000 K/sec
                 0      cpu-migrations:u          #    0.000 K/sec
               985      page-faults:u             #    0.001 M/sec
     2,590,100,889      cycles:u                  #    3.093 GHz
     1,649,576,288      stalled-cycles-frontend:u #   63.69% frontend cycles idle
     3,220,164,818      instructions:u            #    1.24  insn per cycle
                                                  #    0.51  stalled cycles per insn
       188,044,836      branches:u                #  224.583 M/sec
           157,132      branch-misses:u           #    0.08% of all branches

       0.838234360 seconds time elapsed

    I managed to reach max ~1.37 insn per cycle (only one iteration + no if-s + no modulo)

 */
static void benchmark__only_hashmap_basic_for_member()
{
    static common::ExperimentalHashmap<200003, common::int_holder> hash_map;

    constexpr unsigned uniwersum_size {1000000000};

    constexpr unsigned inserts = 190000;
    constexpr unsigned fixed_members = 1024;
    constexpr unsigned queries = 60000000;

    unsigned inserts_counter {0};
    unsigned members_counter {0};
    unsigned members_hits {0};
    hash_map.reset();

    common::int_holder basic_config;
    basic_config.mark = false;

    printf("\n%s\n\n", __FUNCTION__);
    printf("sizeof(config) = %zu, hashmap.capacity() = %u, inserts = %u, uniwersum_size = %u, queries = %u\n",
           sizeof(basic_config), hash_map.capacity(), inserts, uniwersum_size, queries);

    printf("Preprocess data\n");
    srand(time(nullptr));
    for (unsigned i = 0; i < inserts; i++)
    {
        basic_config.content = rand()%uniwersum_size;
        hash_map.insert(basic_config);
        inserts_counter++;
    }

    std::vector<int> members;
    for (unsigned i = 0; i < fixed_members; i++)
    {
        members.push_back(rand()%uniwersum_size);
    }

    printf("Hashmap start watch\n");
    uint64_t t0 = realtime_now();
    for (unsigned i = 0; i < queries; i++)
    {
        basic_config.content = members[i%fixed_members];
        members_hits += hash_map.fast_member(basic_config);
    }

    uint64_t t1 = realtime_now();
    uint64_t time_ms = (t1 - t0)/1000000;
    printf("Hashmap stop watch: Time = %lu ms.\n", time_ms);

    printf("Summary\n");
    printf("inserts = %d, members = %d, hits = %d, hashmap.size = %d\n",
           inserts_counter, members_counter, members_hits,
           hash_map.size());
    printf("hashmap.collisions = %d, colisions per insert = %d\n", hash_map.collisions,
           (hash_map.collisions/(inserts_counter)));

    printf("OK :)\n");
}

}

int main()
{
    benchmarks::benchmark();

    benchmarks::test_intrinsics1();
    benchmarks::test_intrinsics2();
    benchmarks::test_intrinsics3();

    benchmarks::benchmark__only_hashmap_basic_for_member();
    return 0;
}
