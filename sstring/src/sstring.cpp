#include "sstring.hpp"
#include <algorithm>
#include <vector>
#include <array>
#include <type_traits>
#include <cassert>
#include <cstdio>

namespace sstrings {

#ifndef __clang__
// simplified comparing with https://en.cppreference.com/w/cpp/concepts/EqualityComparable
// hmm, constexpr concept is not prohibited
template <class T>
constexpr concept bool EqualityComparable = requires (T& t) {
     operator==(t, t);
     operator!=(t, t);
};

// TO DO: https://en.cppreference.com/w/cpp/named_req/SequenceContainer
#endif

static void preliimnaries() {
    static_assert(sizeof(sstring<1>) == 8);

    sstring<1> s;
    // similar properties as std::unique_ptr
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

static void test_case() {
    {
        char foo[] = "foob";
        sstring<5> str(foo);
        assert(str.is_internal());
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
        assert(s == "foo");
    }
    {
         sstring<3> s1;
         sstring<3> s2 = "ba";
         s1 = std::move(s2);
         assert(s1 == "ba");
    }
    {
        sstring<3> s2 = "ba";
        sstring<3> s1 = std::move(s2);
        assert(s1 == "ba");
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

#if 0
    {
        sstring s("foo894hfnsdjknfsbar");
        assert(s[0] == 'f');
        assert(s[10] == 'd');
    }

    {
        sstring s1("foo894hfnsdjknfsbar"), s2("foo894hfnsdjknfsbar");
        assert(s1 == s2);
    }
    {
         char buf[] = "foo894hfnsdjknfsbar";
         constexpr auto size = sizeof buf;
         sstring<size> s1;
         sstring<size> s2 = buf;
         s1 = std::move(s2);
         assert(s1 == "foo894hfnsdjknfsbar");
    }
    {
        char buf[] = "foo894hfnsdjknfsbar";
        constexpr auto size = sizeof buf;
        sstring<size> s2 = buf;
        sstring<size> s1 = std::move(s2);
        assert(s1 == "foo894hfnsdjknfsbar");
    }
#endif

#if 0
    {
        std::array sstrings_src = {sstring("one"), sstring("two"), sstring("three"), sstring("external1"),
                                  sstring("external2"), sstring("external3")};
    }

    {
        std::array sstrings_src = {sstring("one"), sstring("two"), sstring("111"), sstring("333"),
                                  sstring("abc"), sstring("cba")};
        sstring c = "abc";
        assert(*std::find_if(sstrings_src.begin(), sstrings_src.end(), c) == c);
    }
#endif
    printf("%s ok\n", __FUNCTION__);
}

}

int main() {
    sstrings::preliimnaries();
    sstrings::test_case();
    return 0;
}
