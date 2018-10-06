#include <memory>
#include <cassert>
#include <iostream>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/for_each.hpp>

#include "ut_fit_smart_ptr.hpp"
#include "../common/smart_ptr.hpp"

namespace smart
{

template<class T>
using fit_smart_ptr = smart_ptr<T, fit_storage_policy>;

void ut_fit_smart_ptr::test_case_constructor_destructor()
{
    static_assert(sizeof(fit_smart_ptr<int>) == sizeof(void*), "should be equal");
    static_assert(sizeof(fit_smart_ptr<fit_smart_ptr<int>>) == sizeof(void*), "should be equal");

    static_assert(std::is_same< fit_smart_ptr<int>::pointer_type, int* >::value, "should be true");
    static_assert(std::is_same< fit_smart_ptr<const char>::pointer_type, const char* >::value, "should be true");

    static_assert(std::is_same< fit_smart_ptr<int>::counter_type, size_t* >::value, "should be true");
    static_assert(std::is_same< fit_smart_ptr<const char>::counter_type, size_t* >::value, "should be true");

    static_assert(std::is_same< fit_smart_ptr<int>::countee_type, size_t >::value, "should be true");
    static_assert(std::is_same< fit_smart_ptr<const char>::countee_type, size_t >::value, "should be true");

    //const fit_smart_ptr<const char> ptr1(new char); // elegant compilation error from assert
    //const fit_smart_ptr<const char> ptr2(nullptr); // elegant compilation error from assert
    const fit_smart_ptr<const char> ptr3(); // that's function declaration!
    //const fit_smart_ptr<const char> ptr4(NULL); // elegant compilation error from assert
    //const fit_smart_ptr<const char> ptr5; // elegant compilation error from assert
}

class dummy
{
public:
    dummy() = default;
    // trick just against -Wunused-parameter
    dummy(int x) { (void)x; }
    dummy(int x, double y) { (void)x, (void)y; }
    dummy(int x, double y, const dummy &z) { (void)x, (void)y, (void)z; }
};

void ut_fit_smart_ptr::test_case_make_shared()
{
    const fit_smart_ptr<int> ptr1 = smart_make_shared<int>(45);
    assert(ptr1.use_count() == 1);

    char foo = 45;
    const fit_smart_ptr<char> ptr2 = smart_make_shared<char>(foo);
    assert(ptr2.use_count() == 1);

    const char foo2 = 45;
    const auto ptr3 = smart_make_shared<char>(foo2);
    assert(ptr3.use_count() == 1);

    const auto dummy_ptr1 = smart_make_shared<dummy>();
    assert(dummy_ptr1.use_count() == 1);

    const auto dummy_ptr2 = smart_make_shared<dummy>(1);
    assert(dummy_ptr2.use_count() == 1);

    const auto dummy_ptr3 = smart_make_shared<dummy>(1, 3.14);
    assert(dummy_ptr3.use_count() == 1);

    const auto dummy_ptr4 = smart_make_shared<dummy>(1, 3.14, *dummy_ptr3);
    assert(dummy_ptr4.use_count() == 1);
}

void ut_fit_smart_ptr::test_case_dereference()
{
    const fit_smart_ptr<char> ptr1;
    assert(ptr1 == nullptr);
    assert(ptr1.use_count() == 0);

    const fit_smart_ptr<char> ptr2 = smart_make_shared<char>(321);
    *ptr2 = 123;
    assert(*ptr2 == 123);
}

void ut_fit_smart_ptr::test_case_copy_and_assignment()
{
    auto ptr1 = smart_make_shared<char>(45);
    assert(*ptr1 == 45);
    assert(ptr1.use_count() == 1);

    fit_smart_ptr<char> ptr2(nullptr);
    fit_smart_ptr<char> ptr3(nullptr);
    auto ptr4(ptr1);
    assert(*ptr4 == 45);
    assert(ptr4.use_count() == 2);
    assert(ptr1.use_count() == 2);

    ptr2 = ptr4;
    assert(*ptr2 == 45);
    ptr3 = ptr1;
    assert(*ptr3 == 45);
    *ptr1 = 123;
    assert(*ptr2 == 123);
    ptr3 = ptr1;
    assert(*ptr3 == 123);

    // nullptr is r-value so move operator=
    ptr4 = fit_smart_ptr<char>(nullptr);
    ptr4 = nullptr;
    ptr3 = fit_smart_ptr<char>(nullptr);
    ptr1 = ptr2 = ptr3;

    fit_smart_ptr<char> ptr5 = smart_make_shared<char>(12);
    ptr5 = ptr5 = ptr5;
    assert(*ptr5 == 12);
}

void ut_fit_smart_ptr::test_case_get()
{
    auto ptr1 = smart_make_shared<char>(45);
    assert(ptr1.get() != nullptr);

    fit_smart_ptr<char> ptr2(nullptr);
    assert(ptr2.get() == nullptr);

    fit_smart_ptr<char> ptr3(nullptr);
    assert(ptr3.get() == nullptr);

    auto ptr4(ptr1);
    assert(ptr4.get() == ptr1.get());

    ptr2 = ptr4;
    assert(ptr2.get() == ptr4.get() && ptr4.get() == ptr1.get());
    ptr3 = ptr1;
    assert(ptr3.get() == ptr1.get());
    *ptr1 = 123;
    assert(ptr3.get() == ptr1.get());

    ptr4 = nullptr;
    assert(ptr4.get() == nullptr);
    ptr3 = fit_smart_ptr<char>(nullptr);
    assert(ptr3.get() == nullptr);

    ptr1 = ptr2 = ptr3;
    assert(ptr1.get() == nullptr);
    assert(ptr2.get() == nullptr);
}

void ut_fit_smart_ptr::test_case_use_count()
{
    auto ptr1 = smart_make_shared<char>(45);
    assert(ptr1.use_count() == 1);

    fit_smart_ptr<char> ptr2(nullptr);
    fit_smart_ptr<char> ptr3(nullptr);
    auto ptr4(ptr1);
    assert(ptr4.use_count() == 2);
    assert(ptr3.use_count() == 0);
    assert(ptr2.use_count() == 0);
    assert(ptr1.use_count() == 2);

    ptr2 = ptr4;
    assert(ptr4.use_count() == 3);
    assert(ptr3.use_count() == 0);
    assert(ptr2.use_count() == 3);
    assert(ptr1.use_count() == 3);
    ptr3 = ptr1;
    assert(ptr3.use_count() == 4);
    assert(ptr4.use_count() == 4);
    assert(ptr2.use_count() == 4);
    assert(ptr1.use_count() == 4);
    *ptr1 = 123;
    assert(ptr3.use_count() == 4);
    assert(ptr4.use_count() == 4);
    assert(ptr2.use_count() == 4);
    assert(ptr1.use_count() == 4);


    // testing overloading for nullptr
    ptr4 = nullptr;
    assert(ptr4.use_count() == 0);
    assert(ptr3.use_count() == 3);
    assert(ptr2.use_count() == 3);
    assert(ptr1.use_count() == 3);
    // NULL is r-value so move operator=
    ptr3 = fit_smart_ptr<char>(nullptr);
    assert(ptr4.use_count() == 0);
    assert(ptr3.use_count() == 0);
    assert(ptr2.use_count() == 2);
    assert(ptr1.use_count() == 2);
    ptr1 = ptr2 = ptr3;
    assert(ptr4.use_count() == 0);
    assert(ptr3.use_count() == 0);
    assert(ptr2.use_count() == 0);
    assert(ptr1.use_count() == 0);
}

void ut_fit_smart_ptr::test_case_comparisions()
{
    auto ptr1 = smart_make_shared<char>(45);
    assert(ptr1 != nullptr);

    fit_smart_ptr<char> ptr2(nullptr);
    assert(ptr2 == nullptr);

    fit_smart_ptr<char> ptr3(nullptr);
    assert(ptr3 == nullptr);

    auto ptr4 = ptr1;
    assert(ptr4 == ptr1);

    ptr2 = ptr4;
    assert(ptr2 == ptr4 && ptr4 == ptr1);
    ptr3 = ptr1;
    assert(ptr3 == ptr1);
    *ptr1 = 123;
    assert(ptr3 == ptr1);

    ptr4 = nullptr;
    assert(ptr4 == nullptr);
    ptr3 = fit_smart_ptr<char>(nullptr);
    assert(ptr3 == nullptr);

    assert(nullptr != ptr1);
    ptr1 = ptr2 = ptr3;
    assert(ptr1 == nullptr);
    assert(ptr2 == nullptr);

    assert(nullptr == ptr1);
}

//void ut_fit_smart_ptr::rvalue_references_reminder()
//{
//    // here we have normal, old l-value references known as references
//    int foo = 1234;
//    int &bar = foo;
//    assert(bar == 1234);
//    // unfortunately in pre-C++11 it was impossible to take reference to
//    // r-value (except const&)
//    //int &baz = 54321; -- compilation error
//    // now it's possible
//    int &&baz = 54321;
//    // but in this case compilation ofc fails (only r-values):
//    // int &&quk = foo;

//    // So r-values are typically temporaries
//}

void ut_fit_smart_ptr::test_case_move_semantics()
{
    auto ptr = smart_make_shared<char>(-123);
    assert(*ptr == -123);
    assert(ptr.use_count() == 1);
    auto ptr2 = std::move(ptr);
    assert(*ptr2 == -123);
    assert(ptr == nullptr);
    assert(ptr.use_count() == 0);
    assert(ptr2.use_count() == 1);
    auto ptr3 = ptr2;
    assert(*ptr3 == -123);
    assert(ptr3.use_count() == 2);
    assert(ptr2.use_count() == 2);
    ptr = ptr3;
    assert(ptr3.use_count() == 3);
    assert(ptr2.use_count() == 3);
    assert(ptr.use_count() == 3);
    ptr = std::move(ptr3);
    assert(ptr3.use_count() == 0);
    assert(ptr3 == nullptr);
    assert(ptr2.use_count() == 2);
    assert(ptr.use_count() == 2);
    auto ptr4 = std::move(ptr2);
    assert(ptr4.use_count() == 2);
    assert(ptr3 == nullptr);
    assert(ptr3.use_count() == 0);
    assert(ptr2 == nullptr);
    assert(ptr2.use_count() == 0);
    assert(ptr.use_count() == 2);
}

struct node
{
    fit_smart_ptr<node> dummy;
};

void test_case_constructor_no_make_shared()
{
    // no constructor for pointee (node). So ptr->dummy is not constructed
    // here
    const fit_smart_ptr<node> ptr;
    // But ptr destructor -> ptr->dummy destructor

}

/*
   Automatic enumeration for mesages is quite good but has one drawback.
   Brace initialization doesn't work for derived classes (even POD-s) so I must
   add explicit constructor for every mesage.
 */
namespace message_enumerator_proof_of_concept
{
    struct message1;
    struct message2;
    struct message3;
    struct message4;

    using registered_types = boost::mpl::vector<message1, message2, message3, message4>;

    template <class T>
    struct message_numerator
    {
        static char message_id()
        {
            using iter = typename boost::mpl::find<registered_types, T>::type;
            static_assert(iter::pos::value < boost::mpl::size<registered_types>::value,
                          "type is NOT registered");
            return (char)iter::pos::value;
        }
    };

    struct message1 : public message_numerator<message1>
    {
    };

    struct message2 : public message_numerator<message2>
    {
    };

    struct message3 : public message_numerator<message3>
    {
    };

    struct message4 : public message_numerator<message4>
    {
        int foo;
        std::string bar;
    };

    struct verify
    {
        template<typename Msg> void operator()(Msg)
        {
            assert((unsigned)Msg::message_id() == counter);
            counter++;
        }

        static unsigned counter;
    };

    unsigned verify::counter = 0;


    struct dummy_message_numerator
    {
        dummy_message_numerator() = default;
    };

    struct message : dummy_message_numerator
    {
        int foo;
        std::string bar;
    };


    void test_case()
    {
        // Brace initialization doesn't work for message because it has base class so it's not
        // aggregated. Since it's not an aggregate, it can't be initialized with aggregate initialization.
        // message msg = {123, "dupa"};

        std::cout << __PRETTY_FUNCTION__ << "\n";
        // not registered
        message4::message_id();

//        message4 msg = {123, "dupa"};
        // not unique
        boost::mpl::for_each<registered_types>(verify());
    }
}


void ut_fit_smart_ptr::run_all()
{
    message_enumerator_proof_of_concept::test_case();

    std::cout << "Running ut_fit_smart_ptr tests...\n";
    test_case_constructor_destructor();
    test_case_make_shared();
    test_case_dereference();
    test_case_copy_and_assignment();
    test_case_get();
    test_case_use_count();
    test_case_comparisions();
    test_case_move_semantics();
    test_case_constructor_no_make_shared();
    std::cout << "All ut_fit_smart_ptr tests passed\n";
}

}
