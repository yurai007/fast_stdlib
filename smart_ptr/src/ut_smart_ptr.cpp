#include <memory>
#include <cassert>
#include <iostream>
#include <vector>

#include "ut_smart_ptr.hpp"

// ref: http://web.archive.org/web/20131210001207/http://thomas-sanchez.net/computer-sciences
// /2011/08/15/what-every-c-programmer-should-know-the-hard-part/

namespace smart
{

void ut_smart_ptr::test_case_constructor_destructor()
{
    static_assert(sizeof(smart_ptr<int>) == sizeof(void*) + sizeof(void*), "should be equal");
    static_assert(sizeof(smart_ptr<smart_ptr<int>>) == sizeof(void*) + sizeof(void*),
                  "should be equal");

    static_assert(std::is_same< smart_ptr<int>::pointer_type, int* >::value, "should be true");
    static_assert(std::is_same< smart_ptr<const char>::pointer_type, const char* >::value,
                  "should be true");

    static_assert(std::is_same< smart_ptr<int>::counter_type, size_t* >::value, "should be true");
    static_assert(std::is_same< smart_ptr<const char>::counter_type, size_t* >::value,
                  "should be true");

    static_assert(std::is_same< smart_ptr<int>::countee_type, size_t >::value, "should be true");
    static_assert(std::is_same< smart_ptr<const char>::countee_type, size_t >::value,
                  "should be true");

    const smart_ptr<const char> ptr1(new char);
    const smart_ptr<const char> ptr2(nullptr);
    const smart_ptr<const char> ptr3(); // that's function declaration!
    const smart_ptr<const char> ptr4(nullptr);
    const smart_ptr<const char> ptr5;
}

void ut_smart_ptr::test_case_dereference()
{
    const smart_ptr<char> ptr1;
    assert(ptr1 == nullptr);
    assert(ptr1.use_count() == 0);

    const smart_ptr<char> ptr2(new char);
    *ptr2 = 123;
    assert(*ptr2 == 123);
}

void ut_smart_ptr::test_case_copy_and_assignment()
{
    smart_ptr<char> ptr1(new char(45));
    assert(*ptr1 == 45);

    smart_ptr<char> ptr2(nullptr);
    smart_ptr<char> ptr3(nullptr);
    auto ptr4(ptr1);
    assert(*ptr4 == 45);

    ptr2 = ptr4;
    assert(*ptr2 == 45);
    ptr3 = ptr1;
    assert(*ptr3 == 45);
    *ptr1 = 123;
    assert(*ptr2 == 123);
    ptr3 = ptr1;
    assert(*ptr3 == 123);

    // nullptr is r-value so move operator=
    ptr4 = smart_ptr<char>(nullptr);
    ptr4 = nullptr;
    ptr3 = smart_ptr<char>(nullptr);
    ptr1 = ptr2 = ptr3;

    smart_ptr<const char> ptr5(new char(12));
    ptr5 = ptr5 = ptr5;
    assert(*ptr5 == 12);
}

void ut_smart_ptr::test_case_get()
{
    smart_ptr<char> ptr1(new char(45));
    assert(ptr1.get() != nullptr);

    smart_ptr<char> ptr2(nullptr);
    assert(ptr2.get() == nullptr);

    smart_ptr<char> ptr3(nullptr);
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
    ptr3 = smart_ptr<char>(nullptr);
    assert(ptr3.get() == nullptr);

    ptr1 = ptr2 = ptr3;
    assert(ptr1.get() == nullptr);
    assert(ptr2.get() == nullptr);
}

void ut_smart_ptr::test_case_use_count()
{
    smart_ptr<char> ptr1(new char(45));
    assert(ptr1.use_count() == 1);

    smart_ptr<char> ptr2(nullptr);
    smart_ptr<char> ptr3(nullptr);
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
    ptr3 = smart_ptr<char>(nullptr);
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

void ut_smart_ptr::test_case_comparisions()
{
    smart_ptr<char> ptr1(new char(45));
    assert(ptr1 != nullptr);

    smart_ptr<char> ptr2(nullptr);
    assert(ptr2 == nullptr);

    smart_ptr<char> ptr3(nullptr);
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
    ptr3 = smart_ptr<char>(nullptr);
    assert(ptr3 == nullptr);

    assert(nullptr != ptr1);
    ptr1 = ptr2 = ptr3;
    assert(ptr1 == nullptr);
    assert(ptr2 == nullptr);

    assert(nullptr == ptr1);
}

void ut_smart_ptr::rvalue_references_reminder()
{
    // here we have normal, old l-value references known as references
    int foo = 1234;
    int &bar = foo;
    assert(bar == 1234);
    // unfortunately in pre-C++11 it was impossible to take reference to
    // r-value (except const&)
    //int &baz = 54321; -- compilation error
    // now it's possible
    int &&baz = 54321; (void)baz;
    // but in this case compilation ofc fails (only r-values):
    // int &&quk = foo;

    // So r-values are typically temporaries
}

void ut_smart_ptr::test_case_move_semantics()
{
    smart_ptr<int> ptr(new int(-123));
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



struct Base
{
    virtual std::string get_name() = 0;
    virtual ~Base() = default;
};

struct Derived1 : public Base
{
    std::string get_name() override { return name1; }
    std::string name1 {"Derived1"};
};

struct Derived2 : public Base
{
    std::string get_name() override { return name2; }
    std::string name2 {"Derived2"};
};

class IncompleteType;

struct base_vptr
{
    size_t **vptr;
};

void ut_smart_ptr::dump_vtable(size_t **vptr, unsigned n)
{
    std::cout << "v-ptr: " << vptr << "\n";
    size_t *vtable = (size_t*)((vptr));
    for (unsigned i = 0; i < n;i++)
        std::cout << "v-table[" << i << "]: " <<  std::hex << vtable[i] << "\n";
}

void ut_smart_ptr::test_case_simple_polymorphism()
{
    // just plain pointer
    Base *ptr1 = new Derived1();
    base_vptr *ptr1_vptr = (base_vptr*)(ptr1);
    std::cout <<"\n";
    dump_vtable(ptr1_vptr->vptr, 3);

    assert(ptr1->get_name() == "Derived1");

    Base *ptr2 = new Derived2();
    assert(ptr2->get_name() == "Derived2");

    delete ptr2;
    delete ptr1;

    // just shared_ptr
    std::shared_ptr<Base> p1 = std::make_shared<Derived1>();
    assert(p1->get_name() == "Derived1");

    std::shared_ptr<Base> p2 = std::make_shared<Derived2>();
    assert(p2->get_name() == "Derived2");

//    // "incomplete type" from constructor because of void
//        void *foo1;
//        std::shared_ptr<void> p3(foo1);

//    // "incomplete type" from constructor because of forward declaration
//        IncompleteType *foo2;
//        std::shared_ptr<void> p4(foo2);

    // smart_ptr; implicit conversion
    smart_ptr<Base> sp1(new Derived1);
    assert(sp1->get_name() == "Derived1");

    smart_ptr<Base> sp2(new Derived2);
    assert(sp2->get_name() == "Derived2");

    // fit_smart_ptr
    fit_smart_ptr<Base> spp1 = smart_make_shared<Derived1>();
    assert(spp1->get_name() == "Derived1");

    fit_smart_ptr<Base> spp2 = smart_make_shared<Derived2>();
    assert(spp2->get_name() == "Derived2");

    auto helper_lambda = [](const fit_smart_ptr<Base> base_ptr){
        (void) base_ptr;
    };

    //helper_lambda(smart_make_shared<Base>());
    helper_lambda(smart_make_shared<Derived1>());
    helper_lambda(smart_make_shared<Derived2>());
    //helper_lambda(smart_make_shared<void>());
}

struct game_object
{
    virtual void tick(unsigned short tick_counter) = 0;
    virtual ~game_object() = default;

    std::string game_object_buffer;
};

struct client_player : public game_object, public drawable
{
    void tick(unsigned short) override {game_object_buffer = "client_player.tick";}
    void draw(int , int ) override
        {drawable_buffer = "client_player.draw";}
    void load_image() override {drawable_buffer = "client_player.load_image";}
};

struct client_enemy : public game_object, public drawable
{
    void tick(unsigned short) override {game_object_buffer = "client_enemy.tick";}
    void load_image() override {drawable_buffer = "client_enemy.load_image";}
    void draw(int , int ) override
        {drawable_buffer = "client_enemy.draw";}
};

void ut_smart_ptr::some_dummy_helper__plain(drawable* object)
{
    assert(typeid(*object) == typeid(client_player));
    assert(object->drawable_buffer == "");

    object->load_image();

    auto buffer = object->drawable_buffer;
    assert(buffer == "client_player.load_image");
}

void ut_smart_ptr::some_dummy_helper__smart_ptr(smart::smart_ptr<client_player> object)
{
    assert(typeid(*object) == typeid(client_player));
    assert(object->drawable_buffer == "");

    object->load_image();

    auto buffer = object->drawable_buffer;
    assert(buffer == "client_player.load_image");
}

using vtable_drawable = void (*)(drawable*);

void ut_smart_ptr::some_dummy_helper__plain_vptr_magic(drawable* object)
{
    assert(typeid(*object) == typeid(client_player));
    assert(object->drawable_buffer == "");

    base_vptr *ptr1_vptr = (base_vptr*)(object);
    size_t *vtable = (size_t*)((ptr1_vptr->vptr));
    vtable_drawable ptr = (vtable_drawable)vtable[0];

    ptr(object);

    auto buffer = object->drawable_buffer;
    assert(buffer == "client_player.load_image");
}


void ut_smart_ptr::test_case__little_more_advanced_polymorphism__plain()
{
    std::cout << "\nStart " << __FUNCTION__ << "\n";
    std::vector<client_player*> players;

    players.push_back(new client_player());
    players.push_back(new client_player());
    players.push_back(new client_player());

    for (auto &player : players)
    {
        assert(typeid(*player) == typeid(client_player));
        some_dummy_helper__plain(player);
    }

    for (auto &player : players)
    {
        delete player;
    }

    std::cout << __FUNCTION__ << " verdict: OK\n";
}

void ut_smart_ptr::test_case__little_more_advanced_polymorphism__smart_ptr()
{
    std::cout << "\nStart " << __FUNCTION__ << "\n";
    std::vector<smart::smart_ptr<client_player>> players;

    players.push_back(smart::smart_ptr<client_player>(new client_player()));
    players.push_back(smart::smart_ptr<client_player>(new client_player()));
    players.push_back(smart::smart_ptr<client_player>(new client_player()));

    for (auto &player : players)
    {
        assert(typeid(*player.get()) == typeid(client_player));
        some_dummy_helper__smart_ptr(player);
    }

    std::cout << __FUNCTION__ << " verdict: OK\n";
}

void ut_smart_ptr::some_dummy_helper__fit_magic_and_workaround(smart::fit_smart_ptr<drawable> object)
{
    assert(typeid(*object.get()) == typeid(client_player));
    assert(object->drawable_buffer == "");

    // notice we need extra offset here to jump to vptr2
    base_vptr *ptr2_vptr = (base_vptr*)((char*)object.get() + sizeof(game_object));
    size_t *vtable = (size_t*)((ptr2_vptr->vptr));
    vtable_drawable ptr = (vtable_drawable)vtable[0];

    ptr(object.get());

    std::string buffer = object->drawable_buffer;
    assert(buffer == "client_player.load_image");
}

void ut_smart_ptr::test_case__little_more_advanced_polymorphism__fit()
{
    std::cout << "\nStart " << __FUNCTION__ << "\n";
    std::vector<fit_smart_ptr<client_player>> players;

    players.push_back(smart_make_shared<client_player>());
    players.push_back(smart_make_shared<client_player>());
    players.push_back(smart_make_shared<client_player>());

    // This is the tale of one assembler instruction - add rdx, 0x20 :)
    // I need vptr2 (drawable), not vptr1 (game_object) !
    // So I need some workaround - add offset myself
    // Hipothesis is that - in fit_smart_ptr we miss information about pointer type (T* -> void*)
    //        so compiler has no idea about dynamic binding and doesn't generate extra code
    //        So far it worked only because vptr location is on the beginning :)
    // Smart_ptr with default_storage_policy holds T* explicitly so offset is added by compiler
    // and calling load_image works as expected

    for (auto &player : players)
    {
        assert(typeid(*player.get()) == typeid(client_player));
        some_dummy_helper__fit_magic_and_workaround(player);
    }

    std::cout << __FUNCTION__ << " verdict: OK\n";
}

void ut_smart_ptr::run_all()
{
    std::cout << "Running ut_smart_ptr tests...\n";
    test_case_constructor_destructor();
    test_case_dereference();
    test_case_copy_and_assignment();
    test_case_get();
    test_case_use_count();
    test_case_comparisions();
    rvalue_references_reminder();
    test_case_move_semantics();

    test_case_simple_polymorphism();
    test_case__little_more_advanced_polymorphism__plain();
    test_case__little_more_advanced_polymorphism__smart_ptr();
    test_case__little_more_advanced_polymorphism__fit();
    std::cout << "All ut_smart_ptr tests passed\n";
}

}
