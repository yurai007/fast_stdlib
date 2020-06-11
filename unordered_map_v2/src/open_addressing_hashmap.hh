#pragma once

#include <utility>
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <type_traits>

namespace open_addressing {

template<class T>
struct holder {
    T content;
    bool mark;
    using type = int;

    void init_as_empty() {
        content = infinity;
    }

    bool is_empty() const {
        return content == infinity;
    }

    bool operator==(const holder& h) {
        return content == h.content;
    }

    static int hash(const holder& h, int m) {
        return h.content % m;
    }
    constexpr static auto infinity = -1;
} __attribute__((packed));

static_assert(std::is_fundamental_v<holder<int>::type>);
static_assert(std::is_trivially_copyable_v<holder<int>> && std::is_standard_layout_v<holder<int>>);
static_assert(std::has_unique_object_representations_v<holder<int>>);

template<class Holder = holder<int>>
class set {
public:
    set(const set&) = delete;
    set& operator=(const set&) = delete;

    using key_type = typename Holder::type;
    set(unsigned size)
        : _capacity(size) {
        // best speed when capacity is prime
        table = new Holder[capacity()];
        for (auto i = 0u; i < capacity(); i++) {
            auto &e = table[i];
            e.mark = false;
            e.init_as_empty();
        }
    }

    ~set() {
        delete[] table;
    }

    void insert(key_type item) {
        Holder c = {item, false};
        auto i = process_search__false(c);
        if (!(table[i] == c)) {
            table[i] = std::move(c);
            n++;
        }
    }

    void erase(key_type item) {
        Holder c = {item, false};
        auto i = process_search__true(c);
        if (table[i] == c) {
            c.mark = true;
            n--;
        }
    }

    bool search(key_type item) const {
        Holder c = {item, false};
        auto i = process_search__true(c);
        return table[i] == c;
    }

    unsigned size() const {
        return n;
    }

    unsigned capacity() const {
        return _capacity;
    }

    mutable unsigned collisions = 0;
private:
    static int h(int k, int j, int m) {
        auto tmp = k + j + j*j;
        return (tmp >= m)? (tmp%m) : tmp;
    }

    int process_search__true(Holder &c) const {
        const int m = capacity();
        const int hash_holder = c.content % m;
        auto j = 0;
        auto i = hash_holder;

        while ( !(table[i] == c) && (!table[i].is_empty())) {
            j++;
            i = h(hash_holder, j, m);
            collisions++;
        }
        return i;
    }

    int process_search__false(Holder &c) const {
        const int m = capacity();
        const int hash_holder = Holder::hash(c, m);
        auto j = 0;
        auto i = h(hash_holder, j, m);

        while ( !(table[i] == c) && (!table[i].is_empty()) && !table[i].mark) {
            j++;
            i = h(hash_holder, j, m);
        }
        return i;
    }

    unsigned n = 0;
    unsigned _capacity = 0;
    Holder *table = nullptr;
};

}


