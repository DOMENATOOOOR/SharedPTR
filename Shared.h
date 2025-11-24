#pragma once
#include <tuple>
#include <iostream>

template<class Type, class TDeleter = std::default_delete<Type>>
class WeakPtr;


template<class Type, class TDeleter = std::default_delete<Type>>
class SharedPTR {
    using t_SharedPTR = SharedPTR<Type, TDeleter>;
    template<class U, class D> friend class WeakPtr;

    using ControlBlockTuple = std::tuple<size_t, size_t, Type*, TDeleter>;
    ControlBlockTuple* control_block;

    size_t& shared_count() const{
        return std::get<0>(*control_block);
    }
    size_t& weak_count() const{
        return std::get<1>(*control_block);
    }
    Type*& ptr() const{
        return std::get<2>(*control_block);
    }
    TDeleter& del() const{
        return std::get<3>(*control_block);
    }

    void release(){
        if (!control_block) return;
        if (--shared_count() == 0) {
            del()(ptr());
            ptr() = nullptr;
            if (weak_count() == 0) {
                delete control_block;
            }
        }
        control_block = nullptr;
    }; // Release ownership of any stored pointer.

public: // Constructors and destructor.
    SharedPTR(): control_block(nullptr){};

    SharedPTR(Type *pObj) {
        if (pObj) {
            control_block = new ControlBlockTuple(1, 0, pObj, TDeleter{});
        }
        else {
            control_block = nullptr;
        }

    };

    SharedPTR(Type *pObj, TDeleter deleter){
        if (pObj){
            control_block = new ControlBlockTuple(1, 0, pObj, deleter);
        }
        else{
            control_block = nullptr;
        }
    }
    SharedPTR(t_SharedPTR &&uniquePTR): control_block(uniquePTR.control_block){
        uniquePTR.control_block = nullptr;
    }; // Move constructor.

    SharedPTR(const t_SharedPTR& other) : control_block(other.control_block) {
        if (other.control_block) {
            ++shared_count();
        }
    }

    SharedPTR(const WeakPtr<Type, TDeleter>& weak_ptr): control_block(weak_ptr.control_block) {
        if (control_block && shared_count() > 0) {
            ++shared_count();
        }
        else control_block = nullptr;
    }

    ~SharedPTR(){
        release();
        std::cout<<"Вызвался деструктор!"<<std::endl;
    };

public: // Assignment.
    t_SharedPTR& operator=(t_SharedPTR &&sharedPTR){
        if (this != &sharedPTR){
            release();
            control_block = sharedPTR.control_block;
            sharedPTR.control_block = nullptr;
        }
        return *this;
    };

    t_SharedPTR& operator=(const t_SharedPTR& other){
        if (this != &other){
            release();
            control_block = other.control_block;
            if (control_block) ++shared_count();
        }
        return *this;
    };

    void reset(Type *pObject = nullptr){
        release();
        if (pObject) control_block = new ControlBlockTuple(1, 0, pObject, TDeleter{});
    }; // Replace the stored pointer.

    t_SharedPTR& operator=(Type *other){
        reset(other);
        return *this;
    };

public: // Observers.
    Type* get() const{
        return control_block ? ptr() : nullptr;
    }; // Return the stored pointer.
    Type& operator*() const{
        return *get();
    }; // Dereference the stored pointer.
    Type* operator->() const{
        return get();
    }; // Return the stored pointer.
    TDeleter& get_deleter(){
        return control_block ? del() : TDeleter{};
    }; // Return a reference to the stored deleter.
    operator bool() const{
        return get() != nullptr;
    }; // Return false if the stored pointer is null.
    size_t use_count() const{
        return control_block ? shared_count() : 0;
    };
public: // Modifiers.
    void swap(t_SharedPTR &sharedPTR) noexcept {
        std::swap(control_block, sharedPTR);
    }; // Exchange the pointer with another object.
};


// WeakPtr
template<class Type, class TDeleter>
class WeakPtr {
    using t_WeakPtr = WeakPtr<Type, TDeleter>;
    friend class SharedPTR<Type, TDeleter>;

private:
    using ControlBlockTuple = std::tuple<size_t, size_t, Type*, TDeleter>;
    ControlBlockTuple* control_block;

    size_t& shared_count() const { return std::get<0>(*control_block); }
    size_t& weak_count() const { return std::get<1>(*control_block); }

    void increment_weak_count() {
        if (control_block != nullptr) {
            ++weak_count();
        }
    }

    void decrement_weak_count() {
        if (control_block) {
            if (--weak_count() == 0 && shared_count() == 0) {
                delete control_block;
                control_block = nullptr;
            }
        }
    }

public:
    WeakPtr() : control_block(nullptr) {}

    WeakPtr(const SharedPTR<Type, TDeleter>& shared_ptr)
        : control_block(shared_ptr.control_block) {
        increment_weak_count();
    }

    WeakPtr(const t_WeakPtr& other) : control_block(other.control_block) {
        increment_weak_count();
    }

    WeakPtr(t_WeakPtr&& other) : control_block(other.control_block) {
        other.control_block = nullptr;
    }

    ~WeakPtr() {
        decrement_weak_count();
    }

    t_WeakPtr& operator=(const SharedPTR<Type, TDeleter>& shared_ptr) {
        decrement_weak_count();
        control_block = shared_ptr.control_block;
        increment_weak_count();
        return *this;
    }

    t_WeakPtr& operator=(const t_WeakPtr& other) {
        if (this != &other) {
            decrement_weak_count();
            control_block = other.control_block;
            increment_weak_count();
        }
        return *this;
    }

    SharedPTR<Type, TDeleter> lock() const {
        return SharedPTR<Type, TDeleter>(*this);
    }

    bool expired() const {
        return !control_block || shared_count() == 0;
    }

    size_t use_count() const {
        return control_block ? shared_count() : 0;
    }
};

//make_shared
template <typename Type, typename ...Args>
SharedPTR<Type> make_shared(Args&& ... args) {
    return SharedPTR<Type>(new Type(std::forward<Args>(args)...));
}
