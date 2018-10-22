#pragma once

#include <memory>
#include <cassert>
#include <iostream>
#include "../../thread_safe_queue/src/queues.hpp"
#include "../../thread_safe_queue/src/test_utils.hpp"
#include "smart_ptr.hpp"

namespace std_shared_ptr_tests
{

static void test_case_ok_nullptr()
{
    std::make_shared<char>();

    std::shared_ptr<char> ptr1(nullptr);
    assert(ptr1 == nullptr);

    std::shared_ptr<char> ptr2(nullptr);
    assert(ptr2 == nullptr);

    std::shared_ptr<char> ptr3;
    assert(ptr3 == nullptr);

}

struct raw_pi
{
    void *mutex_stuff;
    int use_count, weak_count;
    // void *vptr;
};

struct raw_shared_ptr
{
    void *ptr;
    raw_pi *pi;
};

static void test_case_ok()
{
    std::cout << "test_case_ok\n";
    std::shared_ptr<char> ptr_to_char1(new char);
    *ptr_to_char1 = 123;

    auto ptr_to_char2 = ptr_to_char1;

    std::cout << "Sizeof:	" << sizeof(ptr_to_char1) << "\n";

    std::cout << "Pointers:		" << static_cast<void*>(ptr_to_char1.get()) << " " <<
                 static_cast<void*>(ptr_to_char2.get()) << "\n";

    std::cout << "Pointees:		" << static_cast<int>(*ptr_to_char1) << " " <<
                 static_cast<int>(*ptr_to_char2) << "\n";

    std::cout << "Use counts:		" << ptr_to_char1.use_count() << " " << ptr_to_char2.use_count() << "\n\n";

    assert(sizeof(ptr_to_char1) == 2*sizeof(void*));
    raw_shared_ptr *raw_ptr_to_char1 = reinterpret_cast<raw_shared_ptr*>(&ptr_to_char1);
    raw_shared_ptr *raw_ptr_to_char2 = reinterpret_cast<raw_shared_ptr*>(&ptr_to_char2);
    std::cout << "Unsafe pointer:	 " << raw_ptr_to_char1->ptr << "\n";
    std::cout << "Unsafe pi for ptr1:	 " << raw_ptr_to_char1->pi << "\n";
    std::cout << "Unsafe pi for ptr2:	 " << raw_ptr_to_char2->pi << "\n";

    std::cout << "Unsafe use_count:	 " << raw_ptr_to_char1->pi->use_count << "\n";
    std::cout << "Unsafe weak_count:	 " << raw_ptr_to_char1->pi->weak_count << "\n";

    assert(ptr_to_char1.use_count() == 2 &&
           ptr_to_char2.use_count() == 2);
}

static void test_case_ub_nok()
{
    std::cout << "test_case_ub_nok\n";
    std::shared_ptr<char> ptr_to_char1(new char);
    *ptr_to_char1 = 123;
    auto ptr_to_char2 = ptr_to_char1;
    std::shared_ptr<char> ptr_to_char3(ptr_to_char2.get());//NOT OK, we bypass reference counting mechanism

    std::cout << "Pointers:		" << static_cast<void*>(ptr_to_char1.get()) << " " <<
                 static_cast<void*>(ptr_to_char2.get()) << " " <<
                 static_cast<void*>(ptr_to_char3.get()) << "\n";

    std::cout << "Pointees:		" << static_cast<int>(*ptr_to_char1) << " " <<
                 static_cast<int>(*ptr_to_char2) << " "
              << static_cast<int>(*ptr_to_char3) << "\n";

    std::cout << "Use counts:		" << ptr_to_char1.use_count() << " " << ptr_to_char2.use_count() <<
                 " " << ptr_to_char3.use_count() << "\n";

    assert(ptr_to_char1.use_count() == 2 &&
           ptr_to_char2.use_count() == 2 &&
           ptr_to_char3.use_count() == 1);
}

static void test_case_weak_ptr()
{
    std::cout << "test_case_weak_ptr\n";
    std::shared_ptr<char> ptr_to_char1(new char);
    *ptr_to_char1 = 123;

    raw_shared_ptr *raw_ptr_to_char1 = reinterpret_cast<raw_shared_ptr*>(&ptr_to_char1);
    std::cout << "Unsafe use_count:	 " << raw_ptr_to_char1->pi->use_count << "\n";
    std::cout << "Unsafe weak_count:	 " << raw_ptr_to_char1->pi->weak_count << "\n";

    std::weak_ptr<char>	weak_ptr_to_char1(ptr_to_char1);

    //assert(*weak_ptr_to_char1 == 321);
    std::cout << "After weak_ptr:\n";
    std::cout << "Unsafe use_count:	 " << raw_ptr_to_char1->pi->use_count << "\n";
    std::cout << "Unsafe weak_count:	 " << raw_ptr_to_char1->pi->weak_count << "\n";
}

// shared_ptr has another layout with std::make_shared
static void test_case_make_shared()
{
    std::cout << "test_case_weak_ptr\n";
    auto ptr_to_char1 = std::make_shared<char>();
    *ptr_to_char1 = 123;
    std::cout << "Use counts:		" << ptr_to_char1.use_count() << "\n";
}

static void test_move_semantics()
{
    auto ptr = std::make_shared<int>(-123);
    assert(*ptr == -123);
    assert(ptr.use_count() == 1);
    auto ptr2 = std::move(ptr);
    assert(*ptr2 == -123);
    assert(ptr2.use_count() == 1);
    assert(ptr == nullptr);
}


static void test_case()
{
    test_case_ok_nullptr();
    test_case_ok();
    test_case_ub_nok();
    test_case_weak_ptr();
    test_case_make_shared();
    test_move_semantics();
}

}

namespace smart_perf
{

/*
<RAW_QUEUE>
       104.656779      task-clock (msec)         #    0.994 CPUs utilized
                6      context-switches          #    0.057 K/sec
                0      cpu-migrations            #    0.000 K/sec
           15,724      page-faults               #    0.150 M/sec
      311,082,785      cycles                    #    2.972 GHz
       77,231,405      stalled-cycles-frontend   #   24.83% frontend cycles idle
  <not supported>      stalled-cycles-backend
      700,521,466      instructions              #    2.25  insns per cycle
                                                 #    0.11  stalled cycles per insn
      153,840,604      branches                  # 1469.954 M/sec
           35,639      branch-misses             #    0.02% of all branches

      0.105337273 seconds time elapsed

      total heap usage: 2,000,001 allocs, 2,000,000 frees, 32,072,704 bytes allocated
      ERROR SUMMARY: 0 errors from 0 contexts
</RAW_QUEUE>


<STD_SHARED_PTR_QUEUE> (by make_shared)
        140.935152      task-clock (msec)         #    0.995 CPUs utilized
                 1      context-switches          #    0.007 K/sec
                 0      cpu-migrations            #    0.000 K/sec
            23,541      page-faults               #    0.167 M/sec
       419,930,600      cycles                    #    2.980 GHz
       116,944,650      stalled-cycles-frontend   #   27.85% frontend cycles idle
   <not supported>      stalled-cycles-backend
       906,590,614      instructions              #    2.16  insns per cycle
                                                  #    0.13  stalled cycles per insn
       205,344,330      branches                  # 1457.013 M/sec
            43,528      branch-misses             #    0.02% of all branches

       0.141641799 seconds time elapsed

      total heap usage: 2,000,001 allocs, 2,000,000 frees, 80,072,704 bytes allocated
      ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
</STD_SHARED_PTR_QUEUE>


<SMART_PTR_QUEUE>
        226.909140      task-clock (msec)         #    0.992 CPUs utilized
                16      context-switches          #    0.071 K/sec
                 0      cpu-migrations            #    0.000 K/sec
            31,351      page-faults               #    0.138 M/sec
       646,045,457      cycles                    #    2.847 GHz
       163,002,872      stalled-cycles-frontend   #   25.23% frontend cycles idle
   <not supported>      stalled-cycles-backend
     1,446,081,457      instructions              #    2.24  insns per cycle
                                                  #    0.11  stalled cycles per insn
       321,228,953      branches                  # 1415.672 M/sec
            58,602      branch-misses             #    0.02% of all branches

       0.228624065 seconds time elapsed

    total heap usage: 4,000,001 allocs, 4,000,000 frees, 64,072,704 bytes allocated
    ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
</SMART_PTR_QUEUE>

<FIT_SMART_PTR_QUEUE>

        121.372280      task-clock (msec)         #    0.997 CPUs utilized
                 4      context-switches          #    0.033 K/sec
                 0      cpu-migrations            #    0.000 K/sec
            15,728      page-faults               #    0.130 M/sec
       353,856,202      cycles                    #    2.915 GHz
        78,592,438      stalled-cycles-frontend   #   22.21% frontend cycles idle
   <not supported>      stalled-cycles-backend
       808,254,997      instructions              #    2.28  insns per cycle
                                                  #    0.10  stalled cycles per insn
       191,768,871      branches                  # 1580.006 M/sec
            34,249      branch-misses             #    0.02% of all branches

       0.121782171 seconds time elapsed

    total heap usage: 2,000,001 allocs, 2,000,000 frees, 48,072,704 bytes allocated
    ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
</FIT_SMART_PTR_QUEUE>

So. My fit_smart_ptr is about 20% faster than std::shared_ptr + make_shared and ~20% slower
than raw pointer.

My fit_smart_ptr reduce memory usage by 2x comparing to std::shared_ptr + make_shared and
use about 50% more memory than raw pointer.

Hot spot here is memory allocation/deallocation and allocations number has big impact
on benchmark times. Memory allocation is heavy because of many branches/locks/work inside.

*/
using smart::smart_ptr;

template<class T>
class smart_ptr_queue
{
    struct node
    {
        smart_ptr<node> next;
        T value;
    };

public:
    smart_ptr_queue() :
        head(nullptr),
        tail(nullptr)
    {}

    void push(T new_value)
    {
        smart_ptr<node> new_tail(new node());
        new_tail->value = new_value;
        new_tail->next = nullptr;
        if (tail != nullptr)
            tail->next = new_tail;
        else
            head = new_tail;
        tail = new_tail;
    }

    bool try_pop(T &value)
    {
        if (empty())
            return false;
        value = std::move(head->value);
        auto old_head = head;
        head = std::move(head->next);
        if (tail == old_head)
            tail = nullptr;
        return true;
    }

    bool empty()
    {
        return head == nullptr;
    }

private:
    smart_ptr<node> head, tail;
};

template<class T>
using fit_smart_ptr = smart_ptr<T, smart::fit_storage_policy>;

template<class T>
class fit_smart_ptr_queue
{
    struct node
    {
        fit_smart_ptr<node> next;
        T value;
    };

public:
    fit_smart_ptr_queue() :
        head(nullptr),
        tail(nullptr)
    {}

    void push(T new_value)
    {
        auto new_tail = smart::smart_make_shared<node>();
        new_tail->value = new_value;
        new_tail->next = nullptr;
        if (tail != nullptr)
            tail->next = new_tail;
        else
            head = new_tail;
        tail = new_tail;
    }

    bool try_pop(T &value)
    {
        if (empty())
            return false;
        value = std::move(head->value);
        auto old_head = head;
        head = std::move(head->next);
        if (tail == old_head)
            tail = nullptr;
        return true;
    }

    bool empty()
    {
        return head == nullptr;
    }

private:
    fit_smart_ptr<node> head, tail;
};


static void test_case()
{
  //  std_shared_ptr_tests::test_case();
    std::cout << "Running perf tests...\n";
    producer_consumer_test<raw_queue>().sequential_test();
    producer_consumer_test<std_shared_ptr_queue>().sequential_test();

    producer_consumer_test<smart_ptr_queue>().sequential_test();
    producer_consumer_test<fit_smart_ptr_queue>().sequential_test();
    std::cout << "All perf tests passed\n";
}

}


