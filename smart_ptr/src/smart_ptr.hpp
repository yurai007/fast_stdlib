#pragma once

#include <utility>
#include <cstddef>
#include <cassert>
#include <type_traits>

namespace smart
{

using nullptr_t = decltype(nullptr);

template<class T>
class default_storage_policy
{
public:
    using pointer_type = T*;
    using counter_type = size_t*;
    using countee_type = size_t;
    static constexpr bool has_support_for_constructors = true;

protected:
    void set_ptr(pointer_type pointee)
    {
        ptr = pointee;
    }

    void reset_storage()
    {
        reset_counter();
        reset_ptr();
    }

    void reset_counter(pointer_type pointee)
    {
        counter = (pointee)? new size_t(1) : nullptr;
    }

    pointer_type get_ptr() const
    {
        return ptr;
    }

    void set_counter(counter_type count)
    {
        counter = count;
    }

    void dec_counter()
    {
        (*counter)--;
    }

    void inc_counter()
    {
        (*counter)++;
    }

    bool check_counter() const
    {
        return counter != nullptr;
    }

    countee_type get_counter() const
    {
        return *counter;
    }

    counter_type get_counter_ptr() const
    {
        return counter;
    }

    void delete_storage()
    {
        delete counter;
        delete ptr;
    }

    void set_storage(const default_storage_policy<T> &other)
    {
        set_ptr(other.get_ptr());
        set_counter(other.get_counter_ptr());
    }

private:

    void reset_counter()
    {
        counter = nullptr;
    }

    void reset_ptr()
    {
        ptr = nullptr;
    }

    pointer_type ptr;
    counter_type counter;
};



template<class T>
class fit_storage_policy
{
public:
    using pointer_type = T*;
    using counter_type = size_t*;
    using countee_type = size_t;
    static constexpr bool has_support_for_constructors = false;

    pointer_type allocate_storage()
    {
        common_ptr = new char[(sizeof(countee_type) + sizeof(T))];
        reset_counter();
        return (pointer_type)(static_cast<size_t*>(common_ptr) + 1);
    }

    counter_type get_counter_ptr() const
    {
        return static_cast<size_t*>(common_ptr);
    }

protected:

    void reset_storage()
    {
        common_ptr = nullptr;
    }

    inline pointer_type get_ptr() const
    {
        return (pointer_type)(static_cast<size_t*>(common_ptr) + 1*(common_ptr != nullptr));
    }

    void dec_counter()
    {
        (*get_counter_ptr())--;
    }

    void inc_counter()
    {
        (*get_counter_ptr())++;
    }

    bool check_counter() const
    {
        return common_ptr != nullptr;
    }

    countee_type get_counter() const
    {
        return *(get_counter_ptr());
    }

    void set_storage(const fit_storage_policy<T> &other)
    {
        common_ptr = other.get_counter_ptr();
    }

    template<class T2>
    void set_storage(const fit_storage_policy<T2> &other)
    {
        common_ptr = other.get_counter_ptr();
    }

    void delete_storage()
    {
        ((pointer_type)(static_cast<size_t*>(common_ptr) + 1))->~T();
        delete[] static_cast<char*>(common_ptr);
    }

private:

    void reset_counter()
    {
        *(static_cast<size_t*>(common_ptr)) = 1;
    }

    void *common_ptr;
};


template <class T,
          template <class U> class storage = default_storage_policy
          >
class smart_ptr : public storage<T>
{
public:

    smart_ptr()
    {
        this->reset_storage();
    }

    explicit smart_ptr(nullptr_t)
    {
        this->reset_storage();
    }

    explicit smart_ptr(T *ptr_)
    {
        static_assert(storage<T>::has_support_for_constructors,
                      "storage has no support for constructors. Please use make_shared");
        this->set_ptr(ptr_);
        this->reset_counter(ptr_);
    }

    smart_ptr(const smart_ptr &other)
    {
        this->set_storage(other);

        if (this->check_counter())
            this->inc_counter();
    }

    template<class T2,
             template <class TOut> class storage2 = default_storage_policy>
    smart_ptr(const smart_ptr<T2, storage2> &other)
    {
        this->set_storage(other);

        if (this->check_counter())
            this->inc_counter();
    }

    smart_ptr(smart_ptr &&other)
    {
        this->set_storage(other);
        other.reset_storage();
    }

    smart_ptr &operator=(nullptr_t)
    {
        update_and_check();
        this->reset_storage();
        return *this;
    }

    smart_ptr &operator=(const smart_ptr &other)
    {
        if (&other != this)
        {
            update_and_check();
            this->set_storage(other);
            if (this->check_counter())
                this->inc_counter();
        }
        return *this;
    }

    smart_ptr &operator=(smart_ptr &&other)
    {
        update_and_check();
        this->set_storage(other);
        other.reset_storage();
        return *this;
    }

    T& operator*() const
    {
        return *this->get_ptr();
    }

    inline T* operator->() const
    {
        return this->get_ptr();
    }

    T* get() const
    {
        return this->get_ptr();
    }

    ~smart_ptr()
    {
        update_and_check();
    }

    size_t use_count() const
    {
        return (this->check_counter())? this->get_counter() :0;
    }

    friend bool operator== (const smart_ptr &left, const smart_ptr &right)
    {
        return left.get() == right.get();
    }

    friend bool operator== (nullptr_t, const smart_ptr &right)
    {
        return right.get() == nullptr;
    }

    friend bool operator== (const smart_ptr &left, nullptr_t)
    {
        return left.get() == nullptr;
    }

    friend bool operator!= (const smart_ptr &left, const smart_ptr &right)
    {
        return left.get() != right.get();
    }

    friend bool operator!= (nullptr_t, const smart_ptr &right)
    {
        return right.get() != nullptr;
    }

    friend bool operator!= (const smart_ptr &left, nullptr_t)
    {
        return left.get() != nullptr;
    }

private:

    void update_and_check()
    {
        if (!this->check_counter())
            return;
        this->dec_counter();
        if (this->get_counter() == 0)
            this->delete_storage();
    }
};


template< class T, class... Args >
inline smart_ptr<T, fit_storage_policy> smart_make_shared(Args && ...args)
{
    auto result_ptr = smart_ptr<T, fit_storage_policy>();
    void *ptr = result_ptr.allocate_storage();
    new ( ptr ) T(std::forward<Args>(args)...);
    return result_ptr;
}

template<class T>
using fit_smart_ptr = smart_ptr<T, fit_storage_policy>;


}

