#pragma once

#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>

namespace sstrings {

template<const unsigned MaxSize>
class sstring {
public:
    static_assert(MaxSize < std::numeric_limits<uint32_t>::max(), "Only 32bit size is supported");
    static_assert(MaxSize > 0u, "ISO C++ forbids zero-size array");
    using value_type = char;
    using reference = char&;
    using size_type = unsigned;

    sstring(const sstring &) = delete;
    sstring& operator=(const sstring &) = delete;
    sstring& operator=(const sstring &&) = delete;

    sstring() {
        init_content();
    }

    sstring(const char (&input_cstring)[MaxSize]) {
        init_content(input_cstring);
    }

    sstring& operator=(sstring &&another) noexcept {
        static_assert(_is_internal());
        content = another.content;
        another.init_content();
        return *this;
    }

    sstring(sstring&& another) noexcept {
        static_assert(_is_internal());
        content = another.content;
        another.init_content();
    }

    char& operator[](unsigned pos) noexcept {
        static_assert(_is_internal());
        return content.internal.buffer[pos];
    }

    bool is_internal() const noexcept {
        return content.internal.size & 0x10;
    }

    bool operator==(const sstring& another) const noexcept {
        static_assert(_is_internal());
        return content.internal_for_cmp.value ==
                another.content.internal_for_cmp.value;
    }

    bool operator!=(const sstring& another) const noexcept {
        return !(*this == another);
    }

    ~sstring() {
        if (!is_internal())
            delete[] content.external.buffer;
    }

private:
    // I use type punning through union here
    // which is allowed on gcc and clang: https://stackoverflow.com/questions/54762186/unions-aliasing-and-type-punning-in-practice-what-works-and-what-does-not
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

    constexpr static bool _is_internal() {
        return MaxSize <= 7u;
    }

    void init_content(const char (&input_cstring)[MaxSize]) {
        if constexpr(_is_internal()) {
            content.internal_for_cmp.value = 0u;
            std::memcpy(content.internal.buffer, input_cstring, MaxSize);
            content.internal.size = (MaxSize & 0xf) | 0x10;
        } else {
            constexpr auto extra_space = sizeof(unsigned);
            content.external.buffer = new char[MaxSize + extra_space];
            std::memcpy(content.external.buffer + extra_space, input_cstring, MaxSize);

            auto size = MaxSize;
            content.external.buffer[0] = *reinterpret_cast<char*>(&size);
        }
    }

    void init_content() {
        if constexpr(_is_internal()) {
            content.internal_for_cmp.value = 0u;
            content.internal.size = 0x10;
        } else {
            constexpr auto extra_space = sizeof(unsigned);
            auto size = MaxSize;
            content.external.buffer = new char[MaxSize + extra_space];
            content.external.buffer[0] = *reinterpret_cast<char*>(&size);
        }
    }
};

}
