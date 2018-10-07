#include <cstring>
#include <cassert>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <array>
#include <cstdint>

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

int main()
{
    sstrings::test_case();
    return 0;
}
