#include "perf_smart_ptr_tests.hpp"
#include "ut_smart_ptr.hpp"
#include "ut_fit_smart_ptr.hpp"

int main()
{
    smart::ut_fit_smart_ptr::run_all();
    smart::ut_smart_ptr::run_all();
    smart_perf::test_case();
    return 0;
}
