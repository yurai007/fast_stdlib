﻿#include <string.h>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <utility>
#include <cassert>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cmath>

#include "hashmap.hpp"
#include "../../sstring/src/sstring.hpp"

namespace specialization_proof_of_concept
{

template<bool T>
struct another { constexpr static bool value = T; };

template<const int MaxSize>
class sstring final
{
public:
    sstring(const char (&)[MaxSize])
    {
        constexpr bool internal = (MaxSize <= 7);
        init2<internal>();
    }

    template<bool T>
    void init2()
    {
        init2(another<T>());
    }

private:
    void init2(another<true>)
    {
        printf("Internal\n");
    }

    void init2(another<false>)
    {
        printf("External\n");
    }
};

static void test_case()
{
    char buf[] {"foobar"};
    sstring<sizeof buf> str1(buf);

    char buf2[] {"foo894hfnsdjknfsbar"};
    sstring<sizeof buf2> str2(buf2);
}

}

namespace hashing_benchmark
{

constexpr int INF {-1};

struct sstring_holder final
{
    sstrings::sstring<7> content;
    bool mark;

    void init_as_empty()
    {
        content[0] = INF;
    }

    bool is_empty()
    {
        return content[0] == INF;
    }

	bool operator==(sstring_holder& holder)
	{
		for (int i = 0; i < 7; i++)
		if (content[i] != holder.content[i])
			return false;
		return true;
	}
//	bool operator==(const sstring_holder& cp) const
//	{
//		return content == cp.content;
//	}
    static int hash(sstring_holder& holder, int m)
    {
        int result = 0;
        int mul = 1;
        for (int i = 0; i < 7; i++)
        {
            result = result + (holder.content[i]*mul);
            mul *= 10;
        }
        return result % m;
    }
} __attribute__((packed));


template<unsigned Size>
using SStringHashmap = common::Hashmap<Size,
                                       sstring_holder,
                                       common::Limited_quadratic_hash>;

template<unsigned Size>
static sstrings::sstring<Size> rand_sstring()
{
    sstrings::sstring<Size> result;
    for (unsigned i = 0; i < Size; i++)
        result[i] = rand()%128;
    return result;
}

static std::string rand_string(unsigned max_size)
{
    std::string result(max_size, ' ');
    for (unsigned i = 0; i < result.size(); i++)
        result[i] = rand()%128;
    return result;
}

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

static void sstring_benchmark__only_stl_unordered_map()
{
    std::unordered_map<std::string, std::string> stl_unordered_map;
    constexpr unsigned operations_number {3800000};
    constexpr unsigned string_max_size {7};

    unsigned inserts_counter {0};
    unsigned members_counter {0};
    unsigned members_hits {0};
    unsigned stl_unordered_members_hits {0};
    stl_unordered_map.clear();

    std::string basic_config;

    printf("\n%s\n\n", __FUNCTION__);

    printf("Preprocess data\n");
    srand(time(nullptr));
    std::vector<std::pair<char, std::string>> ops;
    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = get_operation();
        ops.push_back({operation, rand_string(string_max_size)});
    }

    printf("STL Unordered Map start watch\n");
    uint64_t t0 = realtime_now();
    for (unsigned i = 0; i < operations_number; i++)
    {
        const char operation = ops[i].first;

        if (operation == 'I')
        {
            basic_config = ops[i].second;
            stl_unordered_map[basic_config] = basic_config;
            assert(stl_unordered_map.find(basic_config) != stl_unordered_map.end());
            assert(stl_unordered_map.size() > 0);
            inserts_counter++;
        }
        else
            if (operation == 'M')
            {
                basic_config = ops[i].second;
                auto stl_iter = stl_unordered_map.find(basic_config);
                if (stl_iter != stl_unordered_map.end())
                    stl_unordered_members_hits++;
                members_counter++;
            }
    }
    uint64_t t1 = realtime_now();
    uint64_t time_ms = (t1 - t0)/1000000;
    printf("STL Unordered Map stop watch: Time = %lu ms.\n", time_ms);

    printf("Summary\n");
    printf("inserts = %d, members = %d, hits = %d, hashmap.size = %zu\n",
           inserts_counter, members_counter, members_hits,
           stl_unordered_map.size());
    printf("OK :)\n");
}

template<unsigned Size>
static sstring_holder rand_sstring_in_holder()
{
    sstring_holder basic_sstring_holder;
    basic_sstring_holder.mark = false;
    basic_sstring_holder.content = rand_sstring<Size>();
    return basic_sstring_holder;
}

template<class Map, class Key>
static inline bool adapted_find(Map &hashmap, Key &key);

static inline bool adapted_find(auto &hashmap, auto &key)
{
    return hashmap.find(key);
}

static inline bool adapted_find(std::unordered_map<std::string, std::string> &hashmap,
                                auto &key)
{
    return hashmap.find(key) != hashmap.end();
}

template<class Map, class Key>
static inline void adapted_insert(Map &hashmap, Key &key);

static inline void adapted_insert(auto &hashmap, auto &key)
{
    hashmap.insert(key);
}

static inline void adapted_insert(std::unordered_map<std::string, std::string> &hashmap,
                                  auto &key)
{
    hashmap.insert({key, key});
}

/* So now my SStringHashmap in iter2 benchmark is ~5x faster then unordered_map with string.
 */
template<class Hashmap, class KeyGenerator, class Stats>
static void SStringHashmap_perf(Hashmap &hash_map,
                                              KeyGenerator &&generator,
                                              Stats &&stats,
                                              unsigned inserts,
                                              bool present = false)
{
    using key_type = typename Hashmap::key_type;

    constexpr bool debug_logs {false};
    constexpr unsigned string_max_size {7};

    constexpr unsigned fixed_members = 1024;
    constexpr unsigned queries = 100000000;

    unsigned inserts_counter {0};
    unsigned members_counter {0};
    unsigned members_hits {0};

    hash_map.clear();

    if (debug_logs)
        printf("\n%s\n\n", __PRETTY_FUNCTION__);

    if (debug_logs)
        printf("hashmap.capacity = %u, inserts = %u, string_max_size = %u, queries = %u, "
               "sizeof(Hashmap::key_type) = %lu\n", static_cast<unsigned>(hash_map.bucket_count()),
               inserts, string_max_size, queries, sizeof(key_type));

    if (debug_logs)
        printf("Inserting strings to hashmap and queries preprocessing\n");
    srand(time(nullptr));

    std::vector<key_type> members;
    for (unsigned i = 0; i < inserts; i++)
    {
        auto holder = generator();
        adapted_insert(hash_map, holder);
        inserts_counter++;
        if (present && (i < fixed_members))
            members.emplace_back(std::move(holder));
    }

    if (!present)
    {
        for (unsigned i = 0; i < fixed_members; i++)
        {
            auto holder = generator();
            members.emplace_back(std::move(holder));
        }
    }

    if (debug_logs)
        printf("Start queries\n");
    uint64_t t0 = realtime_now();
    for (unsigned i = 0; i < queries; i++)
    {
        auto &&string = members[i%fixed_members];
        members_hits += adapted_find(hash_map, string);
    }

    uint64_t t1 = realtime_now();
    uint64_t time_ms = (t1 - t0)/1000000;
     if (debug_logs)
        printf("Stop queries\n");

    printf("inserts = %d, members = %d, hits = %d, size/capacity = %f\n",
           inserts_counter, members_counter, members_hits, hash_map.size()*1.0f/hash_map.bucket_count());
    stats(hash_map, inserts_counter, queries, time_ms);
    printf("Time = %lu ms.\n", time_ms);
}


}

int main()
{
    using hashing_benchmark::SStringHashmap_perf;

    using Hashmap2M = hashing_benchmark::SStringHashmap<2000003>;
    using Hashmap4M = hashing_benchmark::SStringHashmap<4000037>;
    using Hashmap10M = hashing_benchmark::SStringHashmap<10000019>;
    using Hashmap = hashing_benchmark::SStringHashmap<50000021>;

    using StlHashMap = std::unordered_map<std::string, std::string>;

    auto stl_generator = [](){
        return hashing_benchmark::rand_string(7);
    };
    auto my_generator = [](){
        return hashing_benchmark::rand_sstring_in_holder<7>();
    };

    auto my_stats = [](auto &hashmap, auto all_inserts, auto all_queries, auto time){
            printf("hashmap.collisions = %d, colisions per insert = %d, avg find time = %dns\n",
                   hashmap.collisions,
                   (hashmap.collisions/(all_inserts + all_queries)),
                   static_cast<int>((1000000LL*time)/all_queries));
    };

    auto stl_stats = [](auto &hashmap, auto all_inserts, auto all_queries, auto time){
        (void)hashmap; (void)all_inserts;
        printf("avg find time = %dns\n", static_cast<int>((1000000LL*time)/all_queries));
    };

    static Hashmap my_hash_map;
    static StlHashMap stl_hash_map;
    static Hashmap2M hash_map2m;
    static Hashmap4M hash_map4m;
    static Hashmap10M hash_map10m;

    SStringHashmap_perf<Hashmap2M>(hash_map2m, my_generator, my_stats, 190000);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 190000);
    printf("\n");

    SStringHashmap_perf<Hashmap2M>(hash_map2m, my_generator, my_stats, 400000);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 400000);
    printf("\n");

    SStringHashmap_perf<Hashmap2M>(hash_map2m, my_generator, my_stats, 800000);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 800000);
    printf("\n");

    SStringHashmap_perf<Hashmap2M>(hash_map2m, my_generator, my_stats, 1000000);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 1000000);
    printf("\n");

    SStringHashmap_perf<Hashmap2M>(hash_map2m, my_generator, my_stats, 1300000);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 1300000);
    printf("\n");

    SStringHashmap_perf<Hashmap2M>(hash_map2m, my_generator, my_stats, 1600000);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 1600000);
    printf("\n");

    SStringHashmap_perf<Hashmap4M>(hash_map4m, my_generator, my_stats, 2000000);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 2000000);
    printf("\n");

    SStringHashmap_perf<Hashmap10M>(hash_map10m, my_generator, my_stats, 4000000);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 4000000);
    printf("\n");

    SStringHashmap_perf<Hashmap10M>(hash_map10m, my_generator, my_stats, 6000000);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 6000000);
    printf("\n");

    SStringHashmap_perf<Hashmap10M>(hash_map10m, my_generator, my_stats, 7000000);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 7000000);
    printf("\n");



    SStringHashmap_perf<Hashmap2M>(hash_map2m, my_generator, my_stats, 190000, true);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 190000, true);
    printf("\n");

    SStringHashmap_perf<Hashmap2M>(hash_map2m, my_generator, my_stats, 400000, true);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 400000, true);
    printf("\n");

    SStringHashmap_perf<Hashmap2M>(hash_map2m, my_generator, my_stats, 800000, true);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 800000, true);
    printf("\n");

    SStringHashmap_perf<Hashmap2M>(hash_map2m, my_generator, my_stats, 1000000, true);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 1000000, true);
    printf("\n");

    SStringHashmap_perf<Hashmap2M>(hash_map2m, my_generator, my_stats, 1300000, true);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 1300000, true);
    printf("\n");

    SStringHashmap_perf<Hashmap2M>(hash_map2m, my_generator, my_stats, 1600000, true);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 1600000, true);
    printf("\n");

    SStringHashmap_perf<Hashmap2M>(hash_map2m, my_generator, my_stats, 1900000, true);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 1900000, true);
    printf("\n");

    SStringHashmap_perf<Hashmap4M>(hash_map4m, my_generator, my_stats, 2000000, true);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 2000000, true);
    printf("\n");

    SStringHashmap_perf<Hashmap10M>(hash_map10m, my_generator, my_stats, 4000000, true);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 4000000, true);
    printf("\n");

    SStringHashmap_perf<Hashmap10M>(hash_map10m, my_generator, my_stats, 6000000, true);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 6000000, true);
    printf("\n");

    SStringHashmap_perf<Hashmap10M>(hash_map10m, my_generator, my_stats, 7000000, true);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 7000000, true);
    printf("\n");



    SStringHashmap_perf<Hashmap>(my_hash_map, my_generator, my_stats, 25000000);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 25000000);
    printf("\n");
    SStringHashmap_perf<Hashmap>(my_hash_map, my_generator, my_stats, 25000000, true);
    SStringHashmap_perf<StlHashMap>(stl_hash_map, stl_generator, stl_stats, 25000000, true);
    printf("\n");
    return 0;
}
