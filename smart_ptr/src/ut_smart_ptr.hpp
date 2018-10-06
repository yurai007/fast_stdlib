#ifndef UT_SMART_PTR_H
#define UT_SMART_PTR_H

#include <string>
#include "../common/smart_ptr.hpp"

namespace smart
{

struct drawable
{
    virtual void load_image() = 0;
    virtual void draw(int active_player_x, int active_player_y) = 0;
    virtual ~drawable() {}

    std::string drawable_buffer;
};

class client_player;

class ut_smart_ptr
{
public:
    ut_smart_ptr() = default;
    void run_all();

private:
    void test_case_constructor_destructor();
    void test_case_dereference();
    void test_case_copy_and_assignment();
    void test_case_get();
    void test_case_use_count();
    void test_case_comparisions();
    void rvalue_references_reminder();
    void test_case_move_semantics();

    void dump_vtable(size_t **vptr, unsigned n);

    void test_case_simple_polymorphism();
    void some_dummy_helper__plain(drawable* object);
    void some_dummy_helper__smart_ptr(smart::smart_ptr<client_player> object);
    void some_dummy_helper__plain_vptr_magic(drawable* object);

    void test_case__little_more_advanced_polymorphism__plain();
    void test_case__little_more_advanced_polymorphism__smart_ptr();

    void some_dummy_helper__fit_magic_and_workaround(smart::fit_smart_ptr<drawable> object);
    void test_case__little_more_advanced_polymorphism__fit();
    void dummy_test_case_polymorphism();
};

}

#endif // UT_SMART_PTR_H
