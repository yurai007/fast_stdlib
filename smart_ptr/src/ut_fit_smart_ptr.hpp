#ifndef UT_FIT_SMART_PTR_H
#define UT_FIT_SMART_PTR_H

namespace smart
{

class ut_fit_smart_ptr
{
public:
    ut_fit_smart_ptr() = default;
    void run_all();

private:
    void test_case_constructor_destructor();
    void test_case_make_shared();
    void test_case_dereference();
    void test_case_copy_and_assignment();
    void test_case_get();
    void test_case_use_count();
    void test_case_comparisions();
    void test_case_move_semantics();
};

}

#endif // UT_FIT_SMART_PTR_H
