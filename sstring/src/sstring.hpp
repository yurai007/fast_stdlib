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
    using iterator = char*;
    using const_iterator = const char*;

    sstring(const sstring &) = delete;
    sstring& operator=(const sstring &) = delete;

    sstring() {
        init_content();
    }

    sstring(const char (&input_cstring)[MaxSize]) {
        init_content(input_cstring);
    }

    sstring& operator=(sstring &&another) noexcept {
        if constexpr(_is_internal()) {
            content = another.content;
            another.init_content();
        } else {
            delete[] content.external.buffer;
            content.external.buffer = another.content.external.buffer;
            another.content.external.buffer = nullptr;
        }
        return *this;
    }

    sstring(sstring&& another) noexcept {
        if constexpr(_is_internal()) {
            content = another.content;
            another.init_content();
        } else {
            content.external.buffer = another.content.external.buffer;
            another.content.external.buffer = nullptr;
        }
    }

    char& operator[](unsigned pos) noexcept {
        return data()[pos];
    }

    const char& operator[](unsigned pos) const noexcept {
        return data()[pos];
    }

    bool is_internal() const noexcept {
        return content.internal.size & 0x10;
    }

    template<const unsigned MaxSizeAnother>
    bool operator==(const sstring<MaxSizeAnother>& another) const noexcept {
        if constexpr(_is_internal()) {
            return content.internal_for_cmp.value ==
                    another.content.internal_for_cmp.value;
        } else {
            return (_size() == another._size()) && (std::memcmp(content.external.buffer + extra_space,
                                                                another.content.external.buffer + extra_space, _size()) == 0);
        }
    }

    template<const unsigned MaxSizeAnother>
    bool operator!=(const sstring<MaxSizeAnother>& another) const noexcept {
        return !(*this == another);
    }

    ~sstring() {
        if (!is_internal())
            delete[] content.external.buffer;
    }

    iterator begin() noexcept {
        return data();
    }
    iterator end() noexcept {
        return data() +  MaxSize - 1u;
    }
    const_iterator cbegin() const noexcept {
        return data();
    }
    const_iterator cend() const noexcept {
        return data() +  MaxSize - 1u;
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

    template<const unsigned>
    friend class sstring;

    constexpr static auto extra_space = sizeof(unsigned);

    constexpr static bool _is_internal() {
        return MaxSize <= 7u;
    }

    constexpr static unsigned _size() {
        return MaxSize;
    }

    char* data() noexcept {
        if constexpr(_is_internal()) {
            return content.internal.buffer;
        } else {
            return &content.external.buffer[extra_space];
        }
    }

    const char* data() const noexcept {
        if constexpr(_is_internal()) {
            return content.internal.buffer;
        } else {
            return &content.external.buffer[extra_space];
        }
    }

    void init_content(const char (&input_cstring)[MaxSize]) {
        if constexpr(_is_internal()) {
            content.internal_for_cmp.value = 0u;
            std::memcpy(content.internal.buffer, input_cstring, MaxSize);
            content.internal.size = (MaxSize & 0xf) | 0x10;
        } else {
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
            auto size = MaxSize;
            content.external.buffer = new char[MaxSize + extra_space];
            content.external.buffer[0] = *reinterpret_cast<char*>(&size);
        }
    }
};

}
