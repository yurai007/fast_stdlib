#include "sstring.hpp"
#include <algorithm>
#include <vector>
#include <array>
#include <type_traits>
#include <cassert>
#include <cstdio>

namespace sstrings {

static void preliminaries() {
    static_assert(sizeof(sstring<1>) == 8);

    sstring<1> s;
    static_assert(std::is_constructible<decltype(s)>::value == true);
    static_assert(std::is_copy_constructible<decltype(s)>::value == false);
    static_assert(std::is_copy_assignable<decltype(s)>::value == false);
    static_assert(std::is_nothrow_copy_constructible<decltype(s)>::value == false);
    static_assert(std::is_assignable<decltype(s), decltype(s)>::value == true);
    static_assert(std::is_move_assignable<decltype(s)>::value == true);

    // some extra noexcept properties
    static_assert(std::is_nothrow_default_constructible<decltype(s)>::value == false);
    static_assert(std::is_nothrow_default_constructible<decltype(s)>::value == false);
    static_assert(noexcept(s[0]) == true);
}

// Extra 'bool' needed only for gcc 8.2
template <class T>
concept bool EqualityComparable = requires (T& t) {
     operator==(t, t);
     operator!=(t, t);
};

template <class T>
concept bool Container = requires (T& t) {
    t();
    std::is_copy_constructible_v<T>;
    std::is_move_constructible_v<T>;
    std::is_copy_assignable_v<T>;
    std::is_move_assignable_v<T>;
    std::is_destructible_v<T>;
    t.begin();
    t.end();
    t.cbegin();
    t.cend();
    EqualityComparable<T>;
    t.swap(t);
    std::swap(t, t);
    t.size();
    t.max_size();
    t.empty();
};

template <class T>
concept bool boolean = requires {
    std::is_same_v<T, bool>;
};

template <class T>
concept bool totally_ordered = requires(const std::remove_reference_t<T>& a,
    const std::remove_reference_t<T>& b) {
    { a <  b } -> boolean;
    { a >  b } -> boolean;
    { a <= b } -> boolean;
    { a >= b } -> boolean;
};

template< class T, class U > concept bool SameAs = std::is_same_v<T, U>;

template<class I>
concept bool LegacyRandomAccessIterator =
  totally_ordered<I> &&
  requires(I i, size_t n) {
    { i += n } -> SameAs<I&>;
    { i -= n } -> SameAs<I&>;
    { i +  n } -> SameAs<I>;
    { n +  i } -> SameAs<I>;
    { i -  n } -> SameAs<I>;
    { i -  i } -> SameAs<decltype(n)>;
    {  i[n]  } -> SameAs<I&>;
  };

template<class T>
concept bool ContinousContainer = Container<T> && LegacyRandomAccessIterator<typename T::iterator>
    && LegacyRandomAccessIterator<typename T::const_iterator>;

static void preliminaries_concepts() {
    // static_assert(ContinousContainer<std::vector<char>>);
    static_assert(EqualityComparable<std::vector<char>>);
}

static void test_case() {
    {
        char foo[] = "foob";
        sstring<5> str(foo);
        assert(str.is_internal());
        assert(*str.begin() == 'f' && *(str.end()-1) == 'b');
    }
    {
        char foo[] {"baaz"};
        sstring str(foo);
        assert(str.is_internal());
    }
    {
        char buf[] = "foobar";
        sstring<sizeof buf> str(buf);
        assert(str.is_internal());
    }
    {
        sstring str("foobar");
        assert(str.is_internal());
    }
    {
        assert(sstring<5>().is_internal());
        assert(sstring<7>().is_internal());
        assert(!sstring<8>().is_internal());
    }
#if 0
    {
        char buf[] = "foobar";
        const char *buf_ptr = buf;
        sstring<sizeof buf> str(buf_ptr);
    }
#endif
    {
        char buf[] = "foobarr";
        sstring str(buf);
        assert(!str.is_internal());
        assert(*str.begin() == 'f' && *(str.end()-1) == 'r');
    }
    {
        char buf[] {"foobar"};
        sstring str {buf};
        str[0] = 'F';
        str[1] = 'O';
        assert(str[0] == 'F' && str[1] == 'O' && str[2] == 'o');
    }
    {
        char buf[] {"foobar"};
        sstring str {buf};
        assert(str.is_internal());

        char buf2[] {"foo894hfnsdjknfsbar"};
        sstring str2 {buf2};
        assert(!str2.is_internal());
    }
    {
        sstring str1("foo"), str2("bar");
        char buf[] {"foo"};
        sstring str3(buf);
        assert(str1 == str3);
        assert(str1 != str2);
    }
    {
        sstring<4> s;
        s = sstring("foo");
        assert(s == sstring("foo"));
    }
    {
         sstring<3> s1;
         sstring<3> s2 = "ba";
         s1 = std::move(s2);
         assert(s1 == sstring("ba"));
    }
    {
        sstring<3> s2 = "ba";
        sstring<3> s1 = std::move(s2);
        assert(s1 == sstring("ba"));
    }
#if 0
    {
        sstring<3> s1 = "fo";
        assert(!(s1 == ""));
    {
         sstring<4> s1;
         sstring<3> s2 = "ba";
         s1 = std::move(s2);
    }
#endif
    {
        std::array a = { sstring("one"), sstring("two"), sstring("111"), sstring("222") };
#if 0
        std::vector b = { sstring("one"), sstring("two"), sstring("111"), sstring("222") };
#endif
    }
    {
        sstring s("foo894hfnsdjknfsbar");
        assert(s[0] == 'f');
        assert(s[10] == 'd');
    }
    {
        sstring s1("foo894hfnsdjknfsbar"), s2("foo894hfnsdjknfsbar");
        assert(s1 == s2);
        sstring s3("foo894hfnsdjkn");
        sstring s4("bla");
        assert(s1 != s3);
    }

    {
         char buf[] = "foo894hfnsdjknfsbar";
         constexpr auto size = sizeof buf;
         sstring<size> s1;
         sstring<size> s2 = buf;
         s1 = std::move(s2);
         assert(s1 == sstring("foo894hfnsdjknfsbar"));
         // s2 is not in valid state
    }
    {
        char buf[] = "foo894hfnsdjknfsbar";
        constexpr auto size = sizeof buf;
        sstring<size> s2 = buf;
        sstring<size> s1 = std::move(s2);
        assert(s1 == sstring("foo894hfnsdjknfsbar"));
        // s2 is not in valid state
    }

#if 0
    {
        std::array sstrings_src = {sstring("one"), sstring("two"), sstring("three"), sstring("external1"),
                                  sstring("external2"), sstring("external3")};
    }
#endif
    {
        std::array sstrings_src = {sstring("one"), sstring("two"), sstring("111"), sstring("333"),
                                  sstring("abc"), sstring("cba")};
        sstring c = "abc";
        assert(*std::find(sstrings_src.begin(), sstrings_src.end(), c) == c);
        assert(std::find(sstrings_src.begin(), sstrings_src.end(), sstring("234htre8rng")) == sstrings_src.end());
    }

    printf("%s ok\n", __FUNCTION__);
}

}

int main() {
    sstrings::preliminaries_concepts();
    sstrings::preliminaries();
    sstrings::test_case();
    return 0;
}
