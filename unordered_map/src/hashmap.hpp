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

}

#endif // HASHMAP_HPP

