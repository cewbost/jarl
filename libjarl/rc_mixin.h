#ifndef RC_MIXIN_H_INCLUDED
#define RC_MIXIN_H_INCLUDED

#include <cstddef>
#include <new>
#include <type_traits>

#ifndef NDEBUG
#include <cstdio>
//#define PRINT_REFCOUNTS
#endif

template<class T>
class rc_ptr;
template<class T>
class rc_weak_ptr;

template<class T>
class RcDirectMixin;

/**
  @brief Inherit from this class to make a pointer reference countable.
*/
template<class T>
class RcMixin{
private:

  mutable unsigned short refcount_;
  mutable unsigned short weakrefcount_;
  
protected:

  void incRefCount()const{
    ++refcount_;
    #ifdef PRINT_REFCOUNTS
    fprintf(stderr, "incRefCount: %p -> %d\n", this, refcount_);
    #endif
  }
  void decRefCount()const{
    --refcount_;
    #ifdef PRINT_REFCOUNTS
    fprintf(stderr, "decRefCount: %p -> %d\n", this, refcount_);
    #endif
    if(refcount_ == 0){
      static_cast<const T*>(this)->~T();
      if(weakrefcount_ == 0){
        T::operator delete(
          const_cast<typename std::remove_const<T>::type*>(
            static_cast<const T*>(this)
          )
        );
      }
    }
  }
  
  void incWeakRefCount()const{
    ++weakrefcount_;
    #ifdef PRINT_REFCOUNTS
    fprintf(stderr, "incWeakRefCount: %p -> %d\n", this, weakrefcount_);
    #endif
  }
  void decWeakRefCount()const{
    --weakrefcount_;
    #ifdef PRINT_REFCOUNTS
    fprintf(stderr, "decWeakRefCount: %p -> %d\n", this, weakrefcount_);
    #endif
    if(refcount_ + weakrefcount_ == 0){
      T::operator delete(
        const_cast<typename std::remove_const<T>::type*>(
          static_cast<const T*>(this)
        )
      );
    }
  }

public:
  
  void operator=(const RcMixin&){
    refcount_ = 0;
    weakrefcount_ = 0;
  }

  unsigned short getRefCount()const{return refcount_;}
  unsigned short getWeakRefCount()const{return weakrefcount_;}
  
  template<class Y>
  friend class rc_ptr;
  template<class Y>
  friend class rc_weak_ptr;
  
private:
  
  RcMixin(): refcount_(0), weakrefcount_(0){}
  RcMixin(const RcMixin&): refcount_(0), weakrefcount_(0){}
  
  friend T;
  friend RcDirectMixin<T>;
};

template<class T>
class RcDirectMixin: public RcMixin<T>{
public:
  using RcMixin<T>::incRefCount;
  using RcMixin<T>::decRefCount;
  using RcMixin<T>::incWeakRefCount;
  using RcMixin<T>::decWeakRefCount;
  
private:
  
  RcDirectMixin(){}
  friend T;
};

/**
  @brief Smart pointer that does reference counting.
  @param T value type, must inherit from RcMixin
  @note Do not assign objects to this pointer unless they are allocated with new
*/
template<class T>
class rc_ptr{
private:
  T* ptr_;
  
public:

  //counstructors
  rc_ptr(){
    ptr_ = nullptr;
  }
  rc_ptr(const rc_ptr& other)noexcept{
    ptr_ = other.get();
    if(ptr_) ptr_->incRefCount();
  }
  rc_ptr(rc_ptr&& other)noexcept{
    ptr_ = other.ptr_;
    other.ptr_ = nullptr;
  }
  rc_ptr(T* other){
    ptr_ = other;
    if(ptr_) ptr_->incRefCount();
  }
  rc_ptr(std::nullptr_t null){
    ptr_ = nullptr;
  }
  template<class Y>
  rc_ptr(Y* other){
    ptr_ = static_cast<T*>(other);
    if(ptr_) ptr_->incRefCount();
  }
  template<class Y>
  rc_ptr(const rc_ptr<Y>& other){
    ptr_ = static_cast<T*>(other.get());
    if(ptr_) ptr_->incRefCount();
  }

  //destructor
  ~rc_ptr(){
    if(ptr_) ptr_->decRefCount();
  }

  //assignment
  template<class Y>
  rc_ptr<T>& operator=(Y* other){
    if(ptr_) ptr_->decRefCount();
    ptr_ = (T*)other;
    if(ptr_) ptr_->incRefCount();
    return *this;
  }
  template<class Y>
  rc_ptr<T>& operator=(const rc_ptr<Y>& other){
    if(ptr_) ptr_->decRefCount();
    ptr_ = (T*)other.get();
    if(ptr_) ptr_->incRefCount();
    return *this;
  }
  rc_ptr<T>& operator=(rc_ptr<T>&& other){
    if(ptr_) ptr_->decRefCount();
    ptr_ = other.ptr_;
    other.ptr_ = nullptr;
    return *this;
  }
  rc_ptr<T>& operator=(std::nullptr_t){
    if(ptr_) ptr_->decRefCount();
    ptr_ = nullptr;
    return *this;
  }

  //dereference
  T& operator*()const{
    return *ptr_;
  }
  T* operator->()const{
    return ptr_;
  }

  //comparison
  bool operator==(const rc_ptr<T>& other) const{
    return ptr_ == other.ptr_;
  }
  bool operator!=(const rc_ptr<T>& other) const{
    return ptr_ != other.ptr_;
  }
  bool operator==(T* other) const{
    return ptr_ == other;
  }
  bool operator!=(T* other) const{
    return ptr_ != other;
  }
  bool operator==(std::nullptr_t) const{
    return ptr_ == nullptr;
  }
  bool operator!=(std::nullptr_t) const{
    return ptr_ != nullptr;
  }

  //typecasting
  template<class Y>
  operator Y() const{
    return (Y)ptr_;
  }
  operator bool() const{
    return ptr_ != nullptr;
  }

  //other functions
  /**
    @brief Returns the underlying pointer
  */
  T* get() const{
    return ptr_;
  }
  void swap(rc_ptr<T>& other){
    T* temp = other.ptr_;
    other.ptr_ = ptr_;
    ptr_ = temp;
  }
  /**
    @brief Returns a weak pointer to the pointed-to object
  */
  rc_weak_ptr<T> weak(){
    return ptr_;
  }
  
  /**
    @brief Gets the reference count.
    @return The reference count of the pointed-to object or 0 if the pointer is null.
  */
  unsigned short getRefCount()
  {if(ptr_) return ptr_->getRefCount(); else return 0;}
  /**
    @brief Gets the weak reference count.
    @return The weak reference count of the pointed-to object or 0 if the pointer is
    null.
  */
  unsigned short getWeakRefCount()
  {if(ptr_) return ptr_->getWeakRefCount(); else return 0;}
};

/**
  @brief Smart weak reference pointer that does reference counting.
  
  This pointer does not keep the object alive. It can point to already destructed
  objects.
  
  @param T value type, must inherit from RcMixin
  @note Do not assign objects to this pointer unless they are allocated with new
*/
template<class T>
class rc_weak_ptr{
private:
  T* ptr_;
  
public:

  //counstructors
  rc_weak_ptr(){
    ptr_ = nullptr;
  }
  rc_weak_ptr(const rc_ptr<T>& other){
    ptr_ = other.get();
    if(ptr_) ptr_->incWeakRefCount();
  }
  rc_weak_ptr(const rc_weak_ptr& other)noexcept{
    ptr_ = other.get();
    if(ptr_) ptr_->incWeakRefCount();
  }
  rc_weak_ptr(rc_weak_ptr&& other)noexcept{
    ptr_ = other.ptr_;
    other.ptr_ = nullptr;
  }
  rc_weak_ptr(T* other){
    ptr_ = other;
    if(ptr_) ptr_->incWeakRefCount();
  }
  rc_weak_ptr(std::nullptr_t null){
    ptr_ = nullptr;
  }
  template<class Y>
  rc_weak_ptr(Y* other){
    ptr_ = static_cast<T*>(other);
    if(ptr_) ptr_->incWeakRefCount();
  }
  template<class Y>
  rc_weak_ptr(const rc_weak_ptr<Y>& other){
    ptr_ = static_cast<T*>(other.get());
    if(ptr_) ptr_->incWeakRefCount();
  }
  template<class Y>
  rc_weak_ptr(const rc_ptr<Y>& other){
    ptr_ = static_cast<T*>(other.get());
    if(ptr_) ptr_->incWeakRefCount();
  }

  //destructor
  ~rc_weak_ptr(){
    if(ptr_) ptr_->decWeakRefCount();
  }

  //assignment
  rc_weak_ptr<T>& operator=(rc_ptr<T>& other){
    if(ptr_) ptr_->decWeakRefCount();
    ptr_ = other.get();
    if(ptr_) ptr_->incWeakRefCount();
    return *this;
  }
  rc_weak_ptr<T>& operator=(rc_weak_ptr<T>& other){
    if(ptr_) ptr_->decWeakRefCount();
    ptr_ = other;
    if(ptr_) ptr_->incWeakRefCount();
    return *this;
  }
  rc_weak_ptr<T>& operator=(T* other){
    if(ptr_) ptr_->decWeakRefCount();
    ptr_ = other;
    if(ptr_) ptr_->incWeakRefCount();
    return *this;
  }
  rc_weak_ptr<T>& operator=(std::nullptr_t){
    if(ptr_) ptr_->decWeakRefCount();
    ptr_ = nullptr;
    return *this;
  }

  //comparison
  bool operator==(const rc_weak_ptr<T>& other){
    return ptr_ == other.ptr_;
  }
  bool operator!=(const rc_weak_ptr<T>& other){
    return ptr_ != other.ptr_;
  }
  bool operator==(const rc_ptr<T>& other) const{
    return ptr_ == other.ptr_;
  }
  bool operator!=(const rc_ptr<T>& other) const{
    return ptr_ != other.ptr_;
  }
  bool operator==(T* other) const{
    return ptr_ == other;
  }
  bool operator!=(T* other) const{
    return ptr_ != other;
  }
  bool operator==(std::nullptr_t) const{
    return ptr_ == nullptr;
  }
  bool operator!=(std::nullptr_t) const{
    return ptr_ != nullptr;
  }

  //other functions
  void swap(rc_weak_ptr<T>& other){
    T* temp = other.ptr_;
    other.ptr_ = ptr_;
    ptr_ = temp;
  }
  /**
    @brief Gets the underlying pointer. This should be used to access objects through
    weak pointers as it checks if the object has expired.
    @return Pointer to the underlying object or nullptr if it has expired.
  */
  T* get(){
    if(!expired()) return ptr_;
    else return nullptr;
  }

  /**
    @brief Gets the reference count.
    @return The reference count of the pointed-to object or 0 if the pointer is null.
  */
  unsigned short getRefCount()
  {if(ptr_) return ptr_->getRefCount(); else return 0;}
  /**
    @brief Gets the weak reference count.
    @return The weak reference count of the pointed-to object or 0 if the pointer is
    null.
  */
  unsigned short getWeakRefCount()
  {if(ptr_) return ptr_->getWeakRefCount(); else return 0;}
  /**
    @brief Checks if the pointed to object has expired.
    @return true if the object has expired or the pointer is null. Else returns false.
  */
  bool expired()
  {if(ptr_) return ptr_->getRefCount() == 0; else return true;}

  operator bool() const{return expired();}
};

namespace std{
  template<class T>
  struct is_pointer<rc_ptr<T>>{
    static constexpr bool value = true;
    
    operator bool(){return value;}
    bool operator()(){return value;}
    
    typedef bool value_type;
    typedef std::integral_constant<bool, value> type;
  };
  template<class T>
  struct is_pointer<rc_weak_ptr<T>>{
    static constexpr bool value = true;
    
    operator bool(){return value;}
    bool operator()(){return value;}
    
    typedef bool value_type;
    typedef std::integral_constant<bool, value> type;
  };
}

#endif
