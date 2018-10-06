#include <string.h>
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

/*
  Motivation:

  * I need string which can be effective hashable ans stored when
    size/capacity are small (8B)
  * from the other side I sill want to have possibility to have big strings on heap
  * ideas borrowed from std::string, seastar::sstring and
    '"short continuation" optimization' for seastar::task_holder

  * specialization_proof_of_concept
    - depending on bool constexpr in default constructor sstring should behaves differently
    - init_content internally or externally
    - it's not enaugh to prepare

      template<bool T>
      void init_content() {}

      and specialize this inside sstring class because  explicit specialization in non-namespace scope
      is not allowed :(

    - fortunately Johannes trick with private overloading works.
      ref: https://stackoverflow.com/questions/3052579/explicit-specialization-in-non-namespace-scope

  * possibility of storing small (internal) strings and big (external) strings in only 8B is tricky
    (I need extra bit for recognition which situation we have). So I embedded this one extra bit inside
    pointer (I can do this on 64-bit linux - ref:  https://www.kernel.org/doc/Documentation/vm/pagemap.txt)

  * -fpack-struct is cool (forces attribute packed) but breaks code which was compiled without this flags (like libstdc++).
    With -fpack-struct I get many strange compilation errors from std::iostream and std::vector internals:

/usr/include/c++/6.3.1/bits/ios_base.h: In member function ‘std::ios_base::fmtflags std::ios_base::setf(std::ios_base::fmtflags)’:
/usr/include/c++/6.3.1/bits/ios_base.h:649:7: error: cannot bind packed field ‘((std::ios_base*)this)->std::ios_base::_M_flags’ to ‘std::_Ios_Fmtflags&’
       _M_flags |= __fmtfl;

/usr/include/c++/6.3.1/bits/ios_base.h: In member function ‘void*& std::ios_base::pword(int)’:
/usr/include/c++/6.3.1/bits/ios_base.h:834:21: error: cannot bind packed field ‘__word.std::ios_base::_Words::_M_pword’ to ‘void*&’
       return __word._M_pword;

    So in order to using it I removed iostreams and changed a bit hashmap (no vector::assign).

  * for only-moveable objects I can't do std::fill

  * iteration 6:

    sstring_benchmark__only_hashmap__iter2 vs sstring_benchmark__only_stl_unordered_map__iter2
    shows that in scenario with very huge number of searches when string is NOT in hashmap my hashmap + sstring
    is 5x faster then std::unordered_map + std::string.

  * iteration 7:
    - static vars in template function is NOT good idea -> every instantiation has own static.
      I had to move out hashmaps outside template (bad_alloc with ulimit = 4G).
    - original sstring binary in bin/release had 40KB without any template magic. Now it has ~365KB and still grows :)
      BUT. If I remove -g then automaticly 365KB -> 27KB ! If I add -s then 19KB!
    - why log_debug slow downs things?

       Time:
            inserts = 25000000, members = 0, hits = 0, size/capacity = 0.500000
            Time = 1503 ms.
            inserts = 25000000, members = 0, hits = 100000000, size/capacity = 0.500000
            Time = 1676 ms.
            VS
            inserts = 25000000, members = 0, hits = 0, size/capacity = 0.685430
            Time = 7621 ms.
            inserts = 25000000, members = 0, hits = 100000000, size/capacity = 0.685430
            Time = 6549 ms.

            I'm 4-5x faster.

       Memory:
            430M (-fpack-struct) VS 2500M -> I use 6x less memory.

  * iteration 8:
    - I have faster hash in sstring_holder - for small strings I don't need so many modulo.
      Faster hash is really faster :)

      Time:
            inserts = 25000000, members = 0, hits = 0, size/capacity = 0.500000
            hashmap.collisions = 141115548, colisions per insert = 1, avg find time = 12ns
            Time = 1251 ms.

            inserts = 25000000, members = 0, hits = 100000000, size/capacity = 0.500000
            hashmap.collisions = 11621561, colisions per insert = 0, avg find time = 13ns
            Time = 1360 ms.
      Memory:
            Still 430M (-fpack-struct).

      Speedup ~17%-19%.

  - TO DO: After all run on arm!

  TO DO1: should I use malloc instead new? (Like in sstring.hh?) Any overhead?
  TO DO2: test for 4GB sstring ?
  TO DO3: some variadic helpers?
  TO DO4: hashing?
*/
namespace sstrings
{

template<const unsigned MaxSize>
class sstring final
{
public:
    using value_type = char;
    using reference = char&;
    using size_type = unsigned;

    sstring(const sstring &) = delete;
    sstring& operator=(const sstring &) = delete;
    sstring& operator=(const sstring &&) = delete;

    sstring()
    {
        constexpr auto internal {(MaxSize <= 7)};
        init_content<internal>();
    }

    sstring(const char (&input_cstring)[MaxSize])
    {
        constexpr auto internal {(MaxSize <= 7)};
        init_content<internal>(input_cstring);
    }

    // now it works only for internal strings
    sstring& operator=(sstring &&another) noexcept
    {
        content = another.content;
        another.init_content<true>();
        return *this;
    }

    // now it works only for internal strings
    sstring(sstring&& another) noexcept
    {
        content = another.content;
        another.init_content<true>();
    }

    char& operator[](unsigned pos) {
        return content.internal.buffer[pos];
    }

    bool is_internal() const
    {
        return (content.internal.size & 0x10);
    }

    // that's flat comparision and works only for internal (small) strings
    bool operator==(const sstring& another) const
    {
        return content.internal_for_cmp.value ==
                another.content.internal_for_cmp.value;
    }

    ~sstring()
    {
        if (!is_internal())
            delete[] content.external.buffer;
    }

private:
    union contents
    {
        struct internal_type
        {
            char buffer[7]; //0-55
            char size; //56-63, but size is 0..7 (4bits) so bits 56,57,58,59 are used and bit 60 is unused
            // from the other side from https://www.kernel.org/doc/Documentation/vm/pagemap.txt
            // in linux 64bit virtual address bits 57-60 zero are ALWAYS zero.
            // So finally I may use bit 60! It means I may use bit(4) in size ()
            // Bit(60) == 0 => external
            // Bit(60) == 1 => internal!
        } internal;
        struct internal_type_for_cmp
        {
            uint64_t value;
        } internal_for_cmp;
        struct external_type
        {
            char *buffer;
        } external;
        static_assert(sizeof(internal_type) == 8 && sizeof(external_type) == 8, "storage too big");
    } content;
    static_assert(sizeof(content) == 8, "storage is fucked up");

    template<bool T>
    struct is_internal_helper { constexpr static bool value = T; };

    template<bool T>
    void init_content(const char (&input_cstring)[MaxSize])
    {
        init_content(input_cstring, is_internal_helper<T>());
    }

    template<bool T>
    void init_content()
    {
        init_content(is_internal_helper<T>());
    }

    void init_content(is_internal_helper<true>)
    {
        static_assert(MaxSize <= 7, "input string is too big");
        content.internal.size = 0x10;
    }

    void init_content(is_internal_helper<false>)
    {
        static_assert(MaxSize > 7, "input string is too small");

        unsigned size {MaxSize};
        content.external.buffer = new char[MaxSize + 4];
        content.external.buffer[0] = *reinterpret_cast<char*>(&size);
    }

    void init_content(const char (&input_cstring)[MaxSize], is_internal_helper<true>)
    {
        static_assert(MaxSize <= 7, "input string is too big");
        std::memcpy(content.internal.buffer, input_cstring, MaxSize);
        content.internal.size = (MaxSize & 0xf) | 0x10;
    }

    void init_content(const char (&input_cstring)[MaxSize], is_internal_helper<false>)
    {
        static_assert(MaxSize > 7, "input string is too small");

        content.external.buffer = new char[MaxSize + 4];
        std::memcpy(content.external.buffer + 4, input_cstring, MaxSize);

        unsigned size {MaxSize};
        content.external.buffer[0] = *reinterpret_cast<char*>(&size);
    }
};

static void test_case()
{
    {
        char foo[5] {"foob"};
        sstring<5> str1(foo);
        assert(str1.is_internal());
    }
    {
        char foo[] {"baaz"};
        sstring<5> str1(foo);
        assert(str1.is_internal());
    }
    {
        char buf[] {"foobar"};
        sstring<sizeof buf> str1(buf);
        assert(str1.is_internal());
    }
    {
        constexpr size_t size {sizeof("foobar")};
        sstring<size> str1("foobar");
        assert(str1.is_internal());
    }
    {
        sstring<5> str1;
        assert(str1.is_internal());
    }
    //    {
    //        char buf[] = "foobar";
    //        char *buf_ptr = buf;
    //        sstring<sizeof buf> str1(buf_ptr);
    //    }
    {
        char buf[] = "foobarr";
        sstring<sizeof buf> str1(buf);
        assert(!str1.is_internal());
    }
    {
        char buf[] {"foobar"};
        sstring<sizeof buf> str {buf};
        str[0] = 'F';
        str[1] = 'O';
        assert(str[0] == 'F' && str[1] == 'O' && str[2] == 'o');
    }
    {
        char buf[] {"foobar"};
        sstring<sizeof buf> str1 {buf};
        assert(str1.is_internal());

        char buf2[] {"foo894hfnsdjknfsbar"};
        sstring<sizeof buf2> str2 {buf2};
        assert(!str2.is_internal());
    }

    {
        std::array<sstring<7>, 123> strings;
        sstring<7> c;
        //std::fill(strings.begin(), strings.end(), c);
    }

    printf("%s ok\n", __FUNCTION__);
}

}

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

/*
 * std::unordered_map<std::string, std::string>:
 * I get 1380 ms

 * SStringHashmap:
 * With sizeof(config) = 16 + std::vector I get ~640ms

 * SStringHashmap:
 * With sizeof(config) = 9 + static + std::array I get ~390ms
   That's probably because of changing vector -> static std::array but I need to verify

 * SStringHashmap:
   + improved operator== I get sometimes 380 ms (610M -> 570M branches)

   So now my SStringHashmap is ~4x faster then unordered_map with string

 */
//static void sstring_benchmark__only_hashmap()
//{
//    static SStringHashmap<> hash_map(2000003);
//    constexpr unsigned operations_number {3800000};
//    constexpr unsigned string_max_size {7};

//    unsigned inserts_counter {0};
//    unsigned members_counter {0};
//    unsigned members_hits {0};
//    hash_map.reset();

//    config basic_config;
//    basic_config.mark = false;

//    printf("\n%s\n\n", __FUNCTION__);
//    printf("sizeof(config) = %zu, hashmap.capacity() = %u, operations_number = %u, string_max_size = %u\n",
//           sizeof(basic_config), hash_map.capacity(), operations_number, string_max_size);

//    printf("Preprocess data\n");
//    srand(time(nullptr));
//    std::vector<std::pair<char, sstrings::sstring<7>>> ops;
//    for (unsigned i = 0; i < operations_number; i++)
//    {
//        const char operation = get_operation();
//        ops.push_back({operation, rand_sstring(string_max_size)});
//    }

//    printf("Hashmap start watch\n");
//    uint64_t t0 = realtime_now();
//    for (unsigned i = 0; i < operations_number; i++)
//    {
//        const char operation = ops[i].first;

//        if (operation == 'I')
//        {
//            basic_config.content = ops[i].second;
//            hash_map.insert(basic_config);
//            assert(hash_map.member(basic_config));
//            assert(hash_map.size() > 0);
//            inserts_counter++;
//        }
//        else
//            if (operation == 'M')
//            {
//                basic_config.content = ops[i].second;
//                bool hit = hash_map.member(basic_config);
//                if (hit)
//                    members_hits++;
//                members_counter++;
//            }
//    }
//    uint64_t t1 = realtime_now();
//    uint64_t time_ms = (t1 - t0)/1000000;
//    printf("Hashmap stop watch: Time = %lu ms.\n", time_ms);

//    printf("Summary\n");
//    printf("inserts = %d, members = %d, hits = %d, hashmap.size = %d\n",
//           inserts_counter, members_counter, members_hits,
//           hash_map.size());
//    printf("hashmap.collisions = %d, colisions per insert = %d\n", hash_map.collisions,
//           (hash_map.collisions/(inserts_counter)));

//    printf("OK :)\n");
//}

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
//    specialization_proof_of_concept::test_case();
    sstrings::test_case();

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
