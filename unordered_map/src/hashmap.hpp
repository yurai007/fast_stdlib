#ifndef HASHMAP_HPP
#define HASHMAP_HPP

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
#include <emmintrin.h>
#include <smmintrin.h>

/*
 * iteration 0.

 * Dummy Hashmap.
   - size is not any special number
   - linear hashing/probing by dummy 'modulo':
     (h1(k, m) + j)%m where h1(k, m) = k % m
   - operations performed on long long if uniwersum is not limited due to overflows possibility

   - Results:

     benchmark

     operations_number = 3800000, uniwersum_size = 1000000000
     Preprocess data
     Hashmap start watch
     Hashmap stop watch: Time = 829 ms.
     STL Map start watch
     STL Map stop watch: Time = 3404 ms.
     STL Unordered Map start watch
     STL Unordered Map stop watch: Time = 934 ms.
     Summary
     inserts = 1899892, members = 1900108, hits = 1860, stl hits = 1860, hashmap.size = 1898067, stl map size = 1898067
     hashmap.collisions = 51957874, colisions per insert = 27
     OK :)

   - Even dummy implementation (829 ms) is much better then std::map (3404 ms) and better then
     std::unordered_map (934 ms). Wow :)
   - colisions per insert = 27 when n/m = 1898067/2000000 ~ 95% on the end. Quite high.


 * iteration 1.

 * Still dummy hashmap with little change
   - in benchmark uniwersum is limited by 10^9 so long long arithmetic and conversions in h are useless

   - Results:

     benchmark

     operations_number = 3800000, uniwersum_size = 1000000000
     Preprocess data
     Hashmap start watch
     Hashmap stop watch: Time = 368 ms.
     STL Map start watch
     STL Map stop watch: Time = 3424 ms.
     STL Unordered Map start watch
     STL Unordered Map stop watch: Time = 862 ms.
     Summary
     inserts = 1897496, members = 1902504, hits = 1852, stl hits = 1852, hashmap.size = 1895666, stl map size = 1895666
     hashmap.collisions = 50350327, colisions per insert = 26
     OK :)

   - impressive. I hit hot spot. This little change give 2x speed up!

 * iteration 2.

 * Seems that hashmap size = prime number doesn't help at all.

 * iteration 3.
   - refactoring, Hashmap becomes tamplate with policy-based design.
     We have many different hash functions so it's necessary otherwise mess will happen.
     Limited_linear_hash is default now.
     After refactoring timings are the same.

 * iteration 4.
  - real_test_case_only_hashmap shows content of hashmap and gather statistics about islands
  - for limited_linear_hash_prime policy:

    hashmap capacity = 100000, operations_number = 190000, uniwersum_size = 1000000000
    Summary
    inserts = 94762, members = 95238, hits = 4, hashmap.size = 94756
    hashmap.collisions = 2640501, colisions per insert = 27
    hashmap.islands = 3198, hashmap.sum = 94755, avg length = 29, max len = 2250

  - for limited_quadratic_hash policy:

    hashmap capacity = 100000, operations_number = 190000, uniwersum_size = 1000000000
    Summary
    inserts = 94851, members = 95149, hits = 3, hashmap.size = 94848
    hashmap.collisions = 731920, colisions per insert = 7
    hashmap.islands = 4855, hashmap.sum = 94847, avg length = 19, max len = 352

    So distribution of elements for quadratic probing is much better then for linear -
    more much shorter islands, dumps confirms this.

  - Limited_linear_hash_prime vs Limited_quadratic_hash in benchmark.

    Thanks to much lower 'colisions per insert' for limited_quadratic_hash we have fastest hashmap so far:

    benchmark

    operations_number = 3800000, uniwersum_size = 1000000000
    Preprocess data
    Hashmap start watch
    Hashmap stop watch: Time = 296 ms.
    STL Map start watch
    STL Map stop watch: Time = 3029 ms.
    STL Unordered Map start watch
    STL Unordered Map stop watch: Time = 859 ms.
    Summary
    inserts = 1899862, members = 1900138, hits = 1831, stl hits = 1831, hashmap.size = 1897985, stl map size = 1897985
    hashmap.collisions = 14806907, colisions per insert = 7
    OK :)

    Now my hashmpa is ~10x faster then std::map and ~3x faster then std::unordered_map. WOW :)

  * iteration 5.
  - add extra Double_hash.

    hashmap capacity = 100003, operations_number = 190000, uniwersum_size = 1000000000
    Summary
    inserts = 94814, members = 95186, hits = 3, hashmap.size = 94810
    hashmap.collisions = 759955, colisions per insert = 8
    hashmap.islands = 4925, hashmap.sum = 94810, avg length = 19, max len = 202

    In terms of islands: number of islands and maximal island length are slightly better then for quadratic hash.

  - real_test_case_theory_vs_practice.
    Scenario - freeze hashmap with arbitrary set alfa and perform many search operations.
            With record_hits = false we test case when key is NOT in dictionary (with very high probability).
            With record_hits = true we test case when key IS in dictionary (wth probability = 1).
    During tests we collect comparisions number and compare with theoretical expected comparisions number.

    Because comparisions = collisions+1 I had to do extra collisions++; on the end of process_search.

    KEY IS IN HASHMAP

    real_test_case_theory_vs_practice
    Linear alpha = 0.849985, quadratic alpha = 0.849985, double alpha = 0.849985
    Measured: Linear comparisions per search = 3.756040, quadratic comparisions per search = 2.469440,double comparisions per search = 2.493920
    Theory: Linear comparisions per search = 3.833334, quadratic comparisions per search = 0.000000,double comparisions per search = 2.231906

    real_test_case_theory_vs_practice
    Linear alpha = 0.949971, quadratic alpha = 0.949971, double alpha = 0.949971
    Measured: Linear comparisions per search = 10.854580, quadratic comparisions per search = 3.601860,double comparisions per search = 3.637720
    Theory: Linear comparisions per search = 10.499998, quadratic comparisions per search = 0.000000,double comparisions per search = 3.153402

    KEY IS NOT IN HASHMAP

    real_test_case_theory_vs_practice
    Linear alpha = 0.849975, quadratic alpha = 0.849975, double alpha = 0.849975
    Measured: Linear comparisions per search = 21.083759, quadratic comparisions per search = 7.843280,double comparisions per search = 8.182100
    Theory: Linear comparisions per search = 22.722229, quadratic comparisions per search = 0.000000,double comparisions per search = 6.666668

    real_test_case_theory_vs_practice
    Linear alpha = 0.949971, quadratic alpha = 0.949971, double alpha = 0.949971
    Measured: Linear comparisions per search = 149.973877, quadratic comparisions per search = 24.930901,double comparisions per search = 24.391979
    Theory: Linear comparisions per search = 200.499908, quadratic comparisions per search = 0.000000,double comparisions per search = 19.999996

    Results:
    1. Theoretical avaerage number of comparisions is very accurate in most cases.
    2. In such scenarios I don't see clear advantage Double_hash over Limited_quadratic_hash. From the other side Limited_quadratic_hash is much faster
       (no overflows = no long longs).

   iter 6

   1. __attribute__((packed)) for lower cache misses number

From

 Performance counter stats for './main':

     1,187,497,849      cycles:u
       894,045,354      instructions:u            #    0.75  insn per cycle
        14,996,050      cache-misses:u

       0.478860794 seconds time elapsed

To

 Performance counter stats for './main':

     1,279,643,779      cycles:u
       919,536,392      instructions:u            #    0.72  insn per cycle
        11,800,318      cache-misses:u

       0.446597909 seconds time elapsed

  2. process_search__true + process_search__false

  stalled-cycles-frontend descreased.

 Performance counter stats for './main':

        447.914273      task-clock:u (msec)       #    0.998 CPUs utilized
                 0      context-switches:u        #    0.000 K/sec
                 0      cpu-migrations:u          #    0.000 K/sec
             3,666      page-faults:u             #    0.008 M/sec
     1,283,468,377      cycles:u                  #    2.865 GHz
       792,870,799      stalled-cycles-frontend:u #   61.78% frontend cycles idle
       916,141,808      instructions:u            #    0.71  insn per cycle
                                                  #    0.87  stalled cycles per insn
       198,162,698      branches:u                #  442.412 M/sec
         5,154,102      branch-misses:u           #    2.60% of all branches

       0.448609694 seconds time elapsed



  To

 Performance counter stats for './main':

        453.729333      task-clock:u (msec)       #    0.999 CPUs utilized
                 0      context-switches:u        #    0.000 K/sec
                 0      cpu-migrations:u          #    0.000 K/sec
             3,362      page-faults:u             #    0.007 M/sec
     1,273,262,977      cycles:u                  #    2.806 GHz
       779,097,236      stalled-cycles-frontend:u #   61.19% frontend cycles idle
       922,930,620      instructions:u            #    0.72  insn per cycle
                                                  #    0.84  stalled cycles per insn
       199,718,960      branches:u                #  440.172 M/sec
         5,160,064      branch-misses:u           #    2.58% of all branches

       0.454340974 seconds time elapsed


    basic_test_case

    operations_number = 100
    inserts = 47, members = 53, hits = 8, stl hits = 8

    real_test_case_theory_vs_practice

    Linear alpha = 0.649961, quadratic alpha = 0.649961, double alpha = 0.649961
    Measured:
    Linear comparisions per search = 3.582680, quadratic comparisions per search = 2.233240,double comparisions per search = 2.298640
    24
    Theory:
    Linear comparisions per search = 4.581632, quadratic comparisions per search = 0.000000,double comparisions per search = 2.857143
    :)
    real_test_case_theory_vs_practice

    Linear alpha = 0.749928, quadratic alpha = 0.749928, double alpha = 0.749928
    Measured:
    Linear comparisions per search = 7.517800, quadratic comparisions per search = 3.686060,double comparisions per search = 3.787540
    18
    Theory:
    Linear comparisions per search = 8.500000, quadratic comparisions per search = 0.000000,double comparisions per search = 4.000000
    :)
    real_test_case_theory_vs_practice

    Linear alpha = 0.849914, quadratic alpha = 0.849914, double alpha = 0.849914
    Measured:
    Linear comparisions per search = 21.389420, quadratic comparisions per search = 7.129440,double comparisions per search = 7.311160
    18
    Theory:
    Linear comparisions per search = 22.722229, quadratic comparisions per search = 0.000000,double comparisions per search = 6.666668
    :)
    real_test_case_theory_vs_practice

    Linear alpha = 0.949902, quadratic alpha = 0.949902, double alpha = 0.949902
    Measured:
    Linear comparisions per search = 205.778305, quadratic comparisions per search = 24.956120,double comparisions per search = 23.382900
    21
    Theory:
    Linear comparisions per search = 200.499908, quadratic comparisions per search = 0.000000,double comparisions per search = 19.999996
    :)Press <RETURN> to close this window...

   * iteration 6:
     - fast_member is experimental


 */

namespace common
{

constexpr int INF {-1};

struct int_holder final
{
    int content;
    bool mark;

    void init_as_empty()
    {
        content = INF;
    }

    bool is_empty() const
    {
        return content == INF;
    }

    bool operator==(const int_holder& holder)
    {
        return (content == holder.content);
    }

    static int hash(const int_holder& holder, int m)
    {
        return holder.content % m;
    }
} __attribute__((packed));

class Linear_hash;
class Limited_quadratic_hash;
class Limited_linear_hash;
class Limited_linear_hash_prime;
class Double_hash;

struct Iter0;
struct Iter1;
struct Iter4_Broken_But_Fast;

template<unsigned>
struct Iter3;

template<unsigned Size,
         class Holder = int_holder,
         class Hash = Limited_quadratic_hash>
class Hashmap
{
public:
    using key_type = Holder;

    Hashmap()
    {
        for (auto &e : table)
        {
            e.mark = false;
            e.init_as_empty();
        }
    }

    void insert(Holder &c)
    {
        const int i = process_search__false(c);
        if (!(table[i] == c))
        {
            table[i] = std::move(c);
            n++;
        }
    }

    void erase(Holder &c)
    {
        const int i = process_search__true(c);
        if (table[i] == c)
        {
            c.mark = true;
            n--;
        }
    }

    bool member(Holder &c)
    {
        int i = process_search__true(c);
        return (table[i] == c);
    }

    bool find(Holder &c) { return member(c); }

    unsigned size() const
    {
        return n;
    }

    unsigned capacity() const
    {
        return table.size();
    }

    unsigned bucket_count() const
    {
        return capacity();
    }

    void reset()
    {
        n = 0;
        collisions = 0;
        for (auto &e : table)
        {
            e.mark = false;
            e.init_as_empty();
        }
    }

    void clear() { reset(); }

    unsigned collisions {0};

protected:

    int process_search__true(Holder &c)
    {
        const int m = table.size();
        const int hash_holder = Holder::hash(c, m);
        int j = 0;
        int i = Hash::h(hash_holder, j, m);

        while ( !(table[i] == c) && (!table[i].is_empty()))
        {
            j++;
            i = Hash::h(hash_holder, j, m);
            collisions++;
        }
        return i;
    }

    int process_search__false(Holder &c)
    {
        const int m = table.size();
        const int hash_holder = Holder::hash(c, m);
        int j = 0;
        int i = Hash::h(hash_holder, j, m);

        while ( !(table[i] == c) && (!table[i].is_empty()) && !table[i].mark)
        {
            j++;
            i = Hash::h(hash_holder, j, m);
            collisions++;
        }
        return i;
    }

    unsigned n {0};
public:
    static_assert((Size == 50000021) || (Size == 10000019) || (Size == 4000037) || (Size == 2000003) || (Size == 200003)
                  || (Size == 100003) || (Size == 500), "Size not supported");
    std::array<Holder, Size> table;
};

template<unsigned Size, class Hash = Limited_quadratic_hash>
class ExperimentalHashmap final : public Hashmap<Size, Hash>
{
public:
    using Hashmap<Size, Hash>::n;
    using Hashmap<Size, Hash>::table;
    using Hashmap<Size, Hash>::process_search__true;

    template<
            template<unsigned> class Func = Iter3
            >
    bool fast_member(int_holder &c)
    {
        int i = 0;
        if (n > (4*table.size()/5))
        {
            i = Func<Size>::process_search__true__optimized(table, c);
        }
        else
        {
            i = process_search__true(c);
        }
        return (table[i] == c);
    }
};

class Linear_hash final
{
public:
    static int h1(int x, int m)
    {
        return x % m;
    }

    static int h(int k, int j, int m)
    {
        return int( ( (long long)(h1(k, m)) + (long long)(j)  )%m );
    }
};

class Limited_quadratic_hash final
{
public:

	static int h(int k, int j, int m)
	{
		return (k + j + j*j)%m;
	}
};

/*
 * in our benchmark uniwersum is limited by 10^9 so long long arithmetic and conversions in h are useless
 */
class Limited_linear_hash final
{
public:
    static int h1(int x, int m)
    {
        return x % m;
    }

	static int h(int k, int j, int m)
	{
		return (h1(k, m) + j)%m;
	}
};

constexpr int p {100003};
constexpr int a {5};
constexpr int b {7};

class Limited_linear_hash_prime final
{
public:
    static int h1(int x, int m)
    {
        return ((a*x + b) % p) % m;
    }

	static int h(int k, int j, int m)
	{
		return (h1(k, m) + j)%m;
	}
};

/*
 * Here it's required that m is prime number.
 */
class Double_hash final
{
public:
    static int h1(int x, int m)
    {
        return x % m;
    }

    static int h2(int x, int m)
    {
        return 1 + x%(m-1);
    }

	static int h(int k, long long int j, int m)
	{
		return int( ( (long long)(h1(k, m)) + j*(long long)(h2(k, m))  )%m );
	}
};

struct __attribute__ ((aligned (16))) hash_vec
{
    int i0, i1, i2, i3;
};

struct __attribute__ ((aligned (16))) hash_uvec
{
    unsigned i0, i1, i2, i3;
};

struct Iter0
{
    static int process_search__true__optimized(std::vector<int_holder> &table, int_holder &c)
    {
        const int m = table.size();
        const int hash_int_holder = c.content % m;
        int j = 0;
        int i = (hash_int_holder + j + j*j)%m;

        while ( (table[i].content != c.content) && (table[i].content >= 0))
        {
            j++;
            i = (hash_int_holder + j + j*j)%m;
        }
        return i;
    }
};

struct Iter1
{
    static int process_search__true__optimized(std::vector<int_holder> &table, int_holder &c)
    {
        const int m = table.size();
        const int hash_int_holder = c.content % m;
        int j = 0;
        int i = (hash_int_holder + j + j*j)%m;

        int out = ~(table[i].content - c.content == 0) & ((table[i].content & 0x80000000) == 0);

        while (out != 0)
        {
            j++;
            i = (hash_int_holder + j + j*j)%m;
            out = ~(table[i].content - c.content == 0) & ((table[i].content & 0x80000000) == 0);
        }
        return i;
    }
};

struct Iter4_Broken_But_Fast
{
    // optimized when  quadratic alpha > 0.85 =>  avg quadratic comparisions per search ~ 7
    // quadratic alpha > 0.75 => avg quadratic comparisions per search ~ 3.7
    static int process_search__true__optimized(std::vector<int_holder> &table, int_holder &c)
    {
        const int m = table.size();
        const int hc = c.content % m;
        hash_vec v;
        hash_vec jj = {0, 1, 2, 3};
        hash_vec zer = {0, 0, 0, 0};
        hash_uvec msb = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
        hash_vec cont;

        __m128i V2 = _mm_set_epi32(c.content, c.content, c.content, c.content);
        __m128i HC = _mm_set_epi32(hc, hc, hc, hc);
        __m128i ADD_4 = _mm_set_epi32(4, 4, 4, 4);
        __m128i *VJ = (__m128i *)&jj;
        __m128i VJ_2;
        __m128i *ZER = (__m128i *)&zer;
        __m128i *MSB = (__m128i *)&msb;
        __m128i *V = (__m128i *)&v;

        hash_vec tmp1, tmp2;
        __m128i *TMP1 = (__m128i *)&tmp1, *TMP2 = (__m128i *)&tmp2, *CONT = (__m128i *)&cont;

        for (int i = 0; i < 8; i++)
        {
            VJ_2 = _mm_mullo_epi32(*VJ, *VJ);
            *V = _mm_add_epi32(*VJ, VJ_2);
            *V = _mm_add_epi32(HC, *V);
            // Now V = hc + j + j^2
            //        v.i0 = v.i0 % m;
            //        v.i1 = v.i1 % m;
            //        v.i2 = v.i2 % m;
            //        v.i3 = v.i3 % m;

            __m128i V1 = _mm_set_epi32(table[v.i3].content, table[v.i2].content,
                    table[v.i1].content, table[v.i0].content);

            *TMP1 =_mm_sub_epi32(V1, V2);
            *TMP2 = _mm_and_si128(V1, *MSB);
            // produces 0xffffffff instead 0x1, I must shift
            *TMP1 = _mm_cmpeq_epi32(*TMP1, *ZER);
            *TMP1 = _mm_srli_epi32(*TMP1, 31);
            *TMP2 = _mm_cmpeq_epi32(*TMP2, *ZER);
            *TMP2 = _mm_srli_epi32(*TMP2, 31);
            *CONT = _mm_andnot_si128(*TMP1, *TMP2);

            //        if (cont.i0 == 0)
            //            return v.i0;

            //        if (cont.i1 == 0)
            //            return v.i1;

            //        if (cont.i2 == 0)
            //            return v.i2;

            //        if (cont.i3 == 0)
            //            return v.i3;

            *VJ = _mm_add_epi32(*VJ, ADD_4);
        }
       //  }  while (true);
       return cont.i0 + v.i3;
    }
};

template<unsigned Size>
struct Iter3 final
{
    // optimized when  quadratic alpha > 0.85 =>  avg quadratic comparisions per search ~ 7
    // quadratic alpha > 0.75 => avg quadratic comparisions per search ~ 3.7
    static int process_search__true__optimized(std::array<int_holder, Size> &table, int_holder &c)
    {
        const int m = table.size();
        const int hc = c.content % m;
        hash_vec v;
        hash_vec jj = {0, 1, 2, 3};
        hash_vec zer = {0, 0, 0, 0};
        hash_uvec msb = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
        hash_vec cont;

        __m128i V2 = _mm_set_epi32(c.content, c.content, c.content, c.content);
        __m128i HC = _mm_set_epi32(hc, hc, hc, hc);
        __m128i ADD_4 = _mm_set_epi32(4, 4, 4, 4);
        __m128i *VJ = (__m128i *)&jj;
        __m128i VJ_2;
        __m128i *ZER = (__m128i *)&zer;
        __m128i *MSB = (__m128i *)&msb;
        __m128i *V = (__m128i *)&v;

        hash_vec tmp1, tmp2;
        __m128i *TMP1 = (__m128i *)&tmp1, *TMP2 = (__m128i *)&tmp2, *CONT = (__m128i *)&cont;

        // step = 4
        do
        {
            VJ_2 = _mm_mullo_epi32(*VJ, *VJ);
            *V = _mm_add_epi32(*VJ, VJ_2);
            *V = _mm_add_epi32(HC, *V);
            // Now V = hc + j + j^2
            v.i0 = v.i0 % m;
            v.i1 = v.i1 % m;
            v.i2 = v.i2 % m;
            v.i3 = v.i3 % m;

            __m128i V1 = _mm_set_epi32(table[v.i3].content, table[v.i2].content,
                    table[v.i1].content, table[v.i0].content);

            *TMP1 =_mm_sub_epi32(V1, V2);
            *TMP2 = _mm_and_si128(V1, *MSB);
            // produces 0xffffffff instead 0x1, I must shift
            *TMP1 = _mm_cmpeq_epi32(*TMP1, *ZER);
            *TMP1 = _mm_srli_epi32(*TMP1, 31);
            *TMP2 = _mm_cmpeq_epi32(*TMP2, *ZER);
            *TMP2 = _mm_srli_epi32(*TMP2, 31);
            *CONT = _mm_andnot_si128(*TMP1, *TMP2);

            if (cont.i0 == 0)
                return v.i0;

            if (cont.i1 == 0)
                return v.i1;

            if (cont.i2 == 0)
                return v.i2;

            if (cont.i3 == 0)
                return v.i3;

            *VJ = _mm_add_epi32(*VJ, ADD_4);
            // return v.i3;
        }  while (true);
    }
};

//// optimized when  quadratic alpha > 0.85 =>  avg quadratic comparisions per search ~ 7
//// quadratic alpha > 0.75 => avg quadratic comparisions per search ~ 3.7
//static int process_search__true__optimized__iter2(std::vector<int_holder> &table, int_holder &c)
//{
//    const int m = table.size();
//    const int hc = Hash::hash(c, m);
//    int ii = Hash::h(hc, 0, m);
//    hash_vec v;
//    hash_vec jj = {1, 2, 3, 4};
//    hash_vec zer = {0, 0, 0, 0};
//    hash_uvec msb = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
//    hash_vec cont;

//    bool contt = (table[ii].content != c.content) && (table[ii].content >= 0);
//    if (!contt)
//        return ii;

//    __m128i V2 = _mm_set_epi32(c.content, c.content, c.content, c.content);
//    __m128i HC = _mm_set_epi32(hc, hc, hc, hc);
//    __m128i ADD_4 = _mm_set_epi32(4, 4, 4, 4);
//    __m128i *VJ = (__m128i *)&jj;
//    __m128i VJ_2;
//    __m128i *ZER = (__m128i *)&zer;
//    __m128i *MSB = (__m128i *)&msb;
//    __m128i *V = (__m128i *)&v;

//    hash_vec tmp1, tmp2;
//    __m128i *TMP1 = (__m128i *)&tmp1, *TMP2 = (__m128i *)&tmp2, *CONT = (__m128i *)&cont;

//    // step = 4
//    do
//    {
//        VJ_2 = _mm_mullo_epi32(*VJ, *VJ);
//        *V = _mm_add_epi32(*VJ, VJ_2);
//        *V = _mm_add_epi32(HC, *V);
//        // Now V = hc + j + j^2
//        v.i0 = v.i0 % m;
//        v.i1 = v.i1 % m;
//        v.i2 = v.i2 % m;
//        v.i3 = v.i3 % m;

//        __m128i V1 = _mm_set_epi32(table[v.i3].content, table[v.i2].content,
//                table[v.i1].content, table[v.i0].content);

//        *TMP1 =_mm_sub_epi32(V1, V2);
//        *TMP2 = _mm_and_si128(V1, *MSB);
//        // produces 0xffffffff instead 0x1, I must shift
//        *TMP1 = _mm_cmpeq_epi32(*TMP1, *ZER);
//        *TMP1 = _mm_srli_epi32(*TMP1, 31);
//        *TMP2 = _mm_cmpeq_epi32(*TMP2, *ZER);
//        *TMP2 = _mm_srli_epi32(*TMP2, 31);
//        *CONT = _mm_andnot_si128(*TMP1, *TMP2);

//        if (cont.i0 == 0)
//            return v.i0;

//        if (cont.i1 == 0)
//            return v.i1;

//        if (cont.i2 == 0)
//            return v.i2;

//        if (cont.i3 == 0)
//            return v.i3;

//        *VJ = _mm_add_epi32(*VJ, ADD_4);
//    } while (true);
//}


//static int process_search__true__optimized_iter1(std::vector<int_holder> &table, int_holder &c)
//{
//    const int m = table.size();
//    const int hc = Hash::hash(c, m);
//    int j = 0;
//    int ii = Hash::h(hc, j, m);
//    hash_vec v = {0, 0, 0, 0};
//    hash_vec zer = {0, 0, 0, 0};
//    hash_uvec msb = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
//    hash_vec cont;

//    bool contt = (table[ii].content != c.content) && (table[ii].content >= 0);
//    if (!contt)
//        return ii;

//    __m128i V2 = _mm_set_epi32(c.content, c.content, c.content, c.content);
//    __m128i *ZER = (__m128i *)&zer;
//    __m128i *MSB = (__m128i *)&msb;

//    hash_vec tmp1, tmp2;
//    __m128i *TMP1 = (__m128i *)&tmp1, *TMP2 = (__m128i *)&tmp2, *CONT = (__m128i *)&cont;

//    // step = 4
//    do
//    {
//        v.i0 = (hc + (j+1) + (j+1)*(j+1))%m;
//        v.i1 = (hc + (j+2) + (j+2)*(j+2))%m;
//        v.i2 = (hc + (j+3) + (j+3)*(j+3))%m;
//        v.i3 = (hc + (j+4) + (j+4)*(j+4))%m;

//        // V1 = table[v.i0].content;
//        // V2 = c.content
//        // ZER = [0,0,0,0]
//        // MSB = [0x80000000, 0x80000000, 0x80000000, 0x80000000]

//        // TMP1 = sub(V1, V2);
//        // TMP1 = cmp(TMP1, ZER)
//        // TMP2 = and(V1, MSB);
//        // TMP2 = cmp(TMP2, ZER);
//        // CONT = and(neg(TMP1), TMP2);

//        __m128i V1 = _mm_set_epi32(table[v.i3].content, table[v.i2].content,
//                table[v.i1].content, table[v.i0].content);

//        *TMP1 =_mm_sub_epi32(V1, V2);
//        *TMP2 = _mm_and_si128(V1, *MSB);
//        // produces 0xffffffff instead 0x1, I must shift
//        *TMP1 = _mm_cmpeq_epi32(*TMP1, *ZER);
//        *TMP1 = _mm_srli_epi32(*TMP1, 31);
//        *TMP2 = _mm_cmpeq_epi32(*TMP2, *ZER);
//        *TMP2 = _mm_srli_epi32(*TMP2, 31);
//        *CONT = _mm_andnot_si128(*TMP1, *TMP2);

//        if (cont.i0 == 0)
//            return v.i0;

//        if (cont.i1 == 0)
//            return v.i1;

//        if (cont.i2 == 0)
//            return v.i2;

//        if (cont.i3 == 0)
//            return v.i3;

//        j += 4;
//    } while (true);
//}



}

#endif // HASHMAP_HPP

