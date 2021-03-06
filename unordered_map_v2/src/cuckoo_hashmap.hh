#pragma once

#include <type_traits>
#include <limits>
#include <cmath>
#include <cassert>
#include <tuple>
#include <vector>
#include <iostream>

namespace cuckoo {

template<class T = int>
class set {
    static_assert(std::is_fundamental_v<T>);
public:
    set(unsigned left, unsigned right)
        : n(0), left_capacity(left), right_capacity(right),
          table_left(left_capacity, {infinity, infinity, infinity, infinity}),
          table_right(right_capacity, infinity),
          loop_limit(log2(right_capacity)),
          rehash_counter(0)
    {
        // best speed when capacities are primes
        assert(right_capacity > left_capacity && right_capacity - left_capacity < 50);
    }

    set(const set&) = delete;
    set& operator=(const set&) = delete;

    void insert(T item) {
       if (search(item)) {
           return;
       }

       for (auto i = 0u; i < loop_limit; i++) {
           auto left = h_left(item, left_capacity);
           if (table_left[left].slot[0] == infinity) {
               table_left[left].slot[0] = item;
               return;
           } else if (table_left[left].slot[1] == infinity) {
               table_left[left].slot[1] = item;
               return;
           } else if (table_left[left].slot[2] == infinity) {
               table_left[left].slot[2] = item;
               return;
           }  else if (table_left[left].slot[3] == infinity) {
               table_left[left].slot[3] = item;
               return;
           }
           std::swap(item, table_left[left].slot[i%4]);

           std::swap(item, table_right[h_right(item, right_capacity)]);
           if (item == infinity) {
               return;
           }
       }
       rehash(item);
    }

    bool search(T item) const noexcept {
       auto left = h_left(item, left_capacity);
       return table_left[left].slot[0] == item || table_left[left].slot[1] == item || table_left[left].slot[2] == item || table_left[left].slot[3] == item
               || table_right[h_right(item, right_capacity)] == item;
    }

    void erase(T item) noexcept {
        auto left = h_left(item, left_capacity);
        if (table_left[left].slot[0] == item) {
            table_left[left].slot[0] = infinity;
        } else if (table_left[left].slot[1] == item) {
            table_left[left].slot[1] = infinity;
        } else if (table_left[left].slot[2] == item) {
            table_left[left].slot[2] = infinity;
        } else if (table_left[left].slot[3] == item) {
            table_left[left].slot[3] = infinity;
        }
        else
        {
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
        for (auto &item : table_left) {
            if (item.slot[0] != infinity)
                temporary_storage.push_back(item.slot[0]);
            if (item.slot[1] != infinity)
                temporary_storage.push_back(item.slot[1]);
            if (item.slot[2] != infinity)
                temporary_storage.push_back(item.slot[2]);
            if (item.slot[3] != infinity)
                temporary_storage.push_back(item.slot[3]);
        }
        for (auto &item : table_right) {
            if (item != infinity)
                temporary_storage.push_back(item);
        }
        left_capacity = prime(2*left_capacity);
        right_capacity = prime(left_capacity);
        loop_limit++;
        if (left_capacity > table_left.size()) {
            table_left.resize(left_capacity);
        }
        if (right_capacity > table_right.size()) {
            table_right.resize(right_capacity);
        }
        std::fill(table_left.begin(), table_left.begin() + left_capacity, bucket{infinity, infinity, infinity, infinity});
        std::fill(table_right.begin(), table_right.begin() + right_capacity, infinity);
        for (auto &item : temporary_storage) {
            insert(item);
        }
    }

    unsigned n;
    unsigned left_capacity, right_capacity;
    struct bucket {
        T slot[4];
    } __attribute__((packed));

    std::vector<bucket> table_left;
    std::vector<T> table_right;
    constexpr static auto infinity = std::numeric_limits<int>::min();
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
