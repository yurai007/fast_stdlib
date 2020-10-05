#pragma once

#include <type_traits>
#include <limits>
#include <cmath>
#include <cassert>
#include <tuple>
#include <vector>
#include <ranges>
#include <iostream>

namespace cuckoo {

template<class T = int>
class set {
    static_assert(std::is_fundamental_v<T>);
public:
    set(unsigned left, unsigned right)
        : n(0), left_capacity(left), right_capacity(right),
          table_left(new T[space(left_capacity)]{infinity}),
          table_right(new T[space(right_capacity)]{infinity}),
          loop_limit(log2(right_capacity)),
          rehash_counter(0)
    {
        // best speed when capacities are primes
        assert(right_capacity > left_capacity && right_capacity - left_capacity < 50);
    }

    ~set() {
        delete[] table_right;
        delete[] table_left;
    }

    set(const set&) = delete;
    set& operator=(const set&) = delete;

    void insert(T item) {
       if (search(item)) {
           return;
       }

       for (auto i = 0u; i < loop_limit; i++) {
           std::swap(item, table_left[h_left(item, left_capacity)]);
           if (item == infinity) {
               return;
           }
           std::swap(item, table_right[h_right(item, right_capacity)]);
           if (item == infinity) {
               return;
           }
       }
       rehash(item);
    }

    bool search(T item) const noexcept {
       return table_left[h_left(item, left_capacity)] == item || table_right[h_right(item, right_capacity)] == item;
    }

    void erase(T item) noexcept {
        auto left = h_left(item, left_capacity);
        if (table_left[left] == item) {
            table_left[left] = infinity;
        } else {
            auto right = h_right(item, right_capacity);
            if (table_right[right] == item) {
                table_right[right] = infinity;
            }
        }
    }

    unsigned size() const noexcept {
        return n;
    }

    std::tuple<unsigned, unsigned> capacities() const noexcept {
        return {left_capacity, right_capacity};
    }

private:

    static T h_left(T x, unsigned m) noexcept {
        return x % m;
    }

    static T h_right(T x, unsigned m) noexcept {
        return x % m;
    }

    void rehash(T x) {
        rehash_counter++;
        std::vector temporary_storage = {x};
        // use ranges
        for (auto i = 0u; i < left_capacity; i++) {
            auto &item = table_left[i];
            if (item != infinity)
                temporary_storage.push_back(item);
        }
        for (auto i = 0u; i < right_capacity; i++) {
            auto &item = table_right[i];
            if (item != infinity)
                temporary_storage.push_back(item);
        }
        left_capacity = prime(2*left_capacity);
        right_capacity = prime(left_capacity);
        loop_limit++;
        // assuming we are in space
        std::fill(table_left, table_left + left_capacity, infinity);
        std::fill(table_right, table_right + right_capacity, infinity);
        for (auto &item : temporary_storage) {
            insert(item);
        }
    }

    static unsigned space(unsigned capacity) noexcept {
        return capacity + static_cast<unsigned>(capacity * static_cast<float>(extra_rehash_space_percent)/100.0);
    }

    unsigned n;
    unsigned left_capacity, right_capacity;
    T *table_left, *table_right;
    constexpr static auto infinity = std::numeric_limits<int>::min();
    constexpr static auto extra_rehash_space_percent = 1610u;
    unsigned loop_limit;
public:
    static unsigned prime(unsigned from) noexcept {
        for (;;) {
            from++;
            auto last = unsigned(sqrt(from)) + 1u;
            auto i = 2u;
            for (; i <= last; i++)
                if (from % i == 0) {
                    break;
                }
            if (i == last + 1) {
                break;
            }
        }
        return from;
    }

    unsigned rehash_counter;
};

}
