#include "sstring.hpp"

namespace sstrings
{

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

int main()
{
    sstrings::test_case();
    return 0;
}
