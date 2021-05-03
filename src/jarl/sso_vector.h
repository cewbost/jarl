#ifndef SSO_VECTOR_H_INCLUDED
#define SSO_VECTOR_H_INCLUDED

/** 
  @file
  @author Erik Boström <cewbostrom@gmail.com>
  @date 15.8.2015
  
  @section LICENSE
  
  MIT License (MIT)

  Copyright (c) 2015 Erik Boström

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
  
  @section DESCRIPTION
  
  A dynamic buffer with "small vector optimization". Vector sizes below a certain
  threshold are stored inside the object itself, avoiding dynamic allocations. This
  can be used for optimization purposes when you know that a vector will likely hold
  a small amount of elements, but you need it to be able to hold an unlimited amount.
  
  usage:
  
  pretty much the same as std::vector
*/

#include <new>
#include <cstddef>
#include <iterator>
#include <initializer_list>
#include <utility>

/**
  @param Type value type
  @param Size max size for in-object buffer
  @param TypeSize size in bytes of a single element, defaults to sizeof(Type).
    This is used for incomplete types.
*/
template<class Type, int Size, int TypeSize = sizeof(Type)>
class SSOVector{
public:

  typedef Type value_type;

private:
  
  size_t size_ = 0;
  union{
    char ss_buffer_[TypeSize * Size];
    struct{
      size_t capacity;
      char* buffer;
    }ls_buffer_;
  };
  
  Type* sBuff_(){return reinterpret_cast<Type*>(ss_buffer_);}
  const Type* sBuff_()const{return reinterpret_cast<const Type*>(ss_buffer_);}
  Type* lBuff_(){return reinterpret_cast<Type*>(ls_buffer_.buffer);}
  const Type* lBuff_()const{return reinterpret_cast<const Type*>(ls_buffer_.buffer);}
  
  static Type* osBuff_(SSOVector& other)
  {return reinterpret_cast<Type*>(other.ss_buffer_);}
  static const Type* osBuff_(const SSOVector& other)
  {return reinterpret_cast<const Type*>(other.ss_buffer_);}
  static Type* olBuff_(SSOVector& other)
  {return reinterpret_cast<Type*>(other.ls_buffer_.buffer);}
  static const Type* olBuff_(const SSOVector& other)
  {return reinterpret_cast<const Type*>(other.ls_buffer_.buffer);}
  
  void destroyContent_(){
    if(size_ > Size){
      for(size_t i = 0; i < size_; ++i)
        lBuff_()[i].~Type();
      delete[] ls_buffer_.buffer;
    }else{
      for(size_t i = 0; i < size_; ++i)
        sBuff_()[i].~Type();
    }
  }
  
  void makeRoomInitial_(){
    char* buffer = new char[TypeSize * Size * 2];
    
    for(size_t i = 0; i < size_; ++i){
      new(buffer + TypeSize * i) Type(std::move(sBuff_()[i]));
      sBuff_()[i].~Type();
    }
    
    ls_buffer_.buffer = buffer;
    ls_buffer_.capacity = Size * 2;
  }
  
  void makeRoomForMore_(){
    size_t new_capacity = (ls_buffer_.capacity * 3) >> 1;
    char* buffer = new char[TypeSize * new_capacity];
    
    for(size_t i = 0; i < size_; ++i){
      new(buffer + TypeSize * i) Type(std::move(lBuff_()[i]));
      lBuff_()[i].~Type();
    }
    
    delete[] ls_buffer_.buffer;
    ls_buffer_.buffer = buffer;
    ls_buffer_.capacity = new_capacity;
  }
  
  void moveBack_(){
    Type* buffer = lBuff_();
    for(size_t i = 0; i < size_; ++i){
      new(sBuff_() + i) Type(std::move(buffer[i]));
      buffer[i].~Type();
    }
    delete[] reinterpret_cast<char*>(buffer);
  }

  
  Type* activeBuffer_(){
    if(size_ > Size) return lBuff_();
    else return sBuff_();
  }
  const Type* activeBuffer_() const{
    if(size_ > Size) return lBuff_();
    else return sBuff_();
  }
  
  static void moveForward_(Type* from, Type* to, size_t steps){
    if(from == to) return;
    Type* current = to - 1;
    Type* target = current + steps;
    
    if((size_t)(to - from) <= steps){
      do{
        new(target) Type(std::move(*current));
        current->~Type();
        --current;
        --target;
      }while((uintptr_t)current >= (uintptr_t)from);
    }else{
      for(int i = steps - 1; i >= 0; --i){
        new(target) Type(std::move(*current));
        --current;
        --target;
      }
      do{
        *target = std::move(*current);
        --target;
        --current;
      }while(current >= from);
      for(size_t i = 0; i < steps; ++i){
        from[i].~Type();
      }
    }
  }
  
  static void moveForward_(Type* from, Type* to){
    if(from == to) return;
    
    new(to) Type(std::move(*(to - 1)));
    --to;
    if(to != from){
      *to = std::move(*(to - 1));
      --to;
    }
    from->~Type();
  }
  
  static void moveBackward_(Type* from, Type* to, size_t steps){
    Type* target = from - steps;
    if(steps >= (size_t)(to - from)){
      while(from != to){
        new(target) Type(std::move(*from));
        from->~Type();
        ++target;
        ++from;
      }
    }else{
      for(size_t i = 0; i < steps; ++i){
        new(target) Type(std::move(*from));
        ++target;
        ++from;
      }
      while(from != to){
        *target = std::move(*from);
        ++target;
        ++from;
      }
      from -= steps;
      for(size_t i = 0; i < steps; ++i){
        from->~Type();
        ++from;
      }
    }
  }
  
  static void moveBackward_(Type* from, Type* to){
    new(from - 1) Type(std::move(*from));
    ++from;
    while(from != to){
      *(from - 1) = std::move(*from);
      ++from;
    }
    (from - 1)->~Type();
  }
  
  static void moveToBuffer_(Type* it1, Type* it2, Type* it3, Type* target, int gap){
    while(it1 != it2){
      new((target++)) Type(std::move(*it1));
      it1->~Type();
      ++it1;
    }
    target += gap;
    while(it1 != it3){
      new((target++)) Type(std::move(*it1));
      it1->~Type();
      ++it1;
    }
  }
  
  static void moveToBuffer_(
    Type* begin1,
    Type* end1,
    Type* begin2,
    Type* end2,
    Type* target
  ){
    while(begin1 != end1){
      new(target) Type(std::move(*begin1));
      begin1->~Type();
      ++target;
      ++begin1;
    }
    while(begin2 != end2){
      new(target) Type(std::move(*begin2));
      begin2->~Type();
      ++target;
      ++begin2;
    }
  }
  
  void split_(int at){
    if(size_ < Size){
      moveForward_(sBuff_() + at, sBuff_() + size_);
    }else if(size_ == Size){
      char* buffer = new char[TypeSize * Size * 2];
      moveToBuffer_(sBuff_(), sBuff_() + at, sBuff_() + size_, (Type*)buffer, 1);
      ls_buffer_.buffer = buffer;
      ls_buffer_.capacity = Size * 2;
    }else if(size_ < ls_buffer_.capacity){
      moveForward_(lBuff_() + 1, lBuff_() + size_);
    }else{
      size_t new_capacity = (ls_buffer_.capacity * 3) >> 1;
      char* buffer = new char[TypeSize * new_capacity];
      moveToBuffer_(lBuff_(), lBuff_() + at, lBuff_() + size_, (Type*)buffer, 1);
      delete[] ls_buffer_.buffer;
      ls_buffer_.buffer = buffer;
      ls_buffer_.capacity = new_capacity;
    }
    --size_;
  }
  
  void split_(int from, int gap){
    size_t new_size = size_ + gap;
    if(size_ <= Size){
      if(new_size <= Size){
        moveForward_(sBuff_() + from, sBuff_() + size_, gap);
      }else{
        size_t new_capacity = Size * 2;
        while(new_capacity < new_size) new_capacity <<= 1;
        char* buffer = new char[TypeSize * new_capacity];
        moveToBuffer_(sBuff_(), sBuff_() + from, sBuff_() + size_, (Type*)buffer, gap);
        ls_buffer_.buffer = buffer;
        ls_buffer_.capacity = new_capacity;
      }
    }else{
      if(new_size <= ls_buffer_.capacity){
        moveForward_(lBuff_() + from, lBuff_() + size_, gap);
      }else{
        size_t new_capacity = ls_buffer_.capacity << 1;
        while(new_capacity < new_size) new_capacity <<= 1;
        char* buffer = new char[TypeSize * new_capacity];
        moveToBuffer_(lBuff_(), lBuff_() + from, lBuff_() + size_, (Type*)buffer, gap);
        delete[] ls_buffer_.buffer;
        ls_buffer_.buffer = buffer;
        ls_buffer_.capacity = new_capacity;
      }
    }
    size_ = new_size;
  }
  
  void join_(size_t at){
    if(at == size_ - 1);
    else if(size_ <= Size){
      moveBackward_(sBuff_() + at + 1, sBuff_() + size_);
    }
    else if(size_ == Size + 1){
      char* buffer = reinterpret_cast<char*>(lBuff_());
      moveToBuffer_(
        lBuff_(), lBuff_() + at,
        lBuff_() + at + 1, lBuff_() + ls_buffer_.capacity,
        sBuff_()
      );
      delete[] buffer;
    }else{
      moveBackward_(lBuff_() + at + 1, lBuff_() + size_);
    }
    --size_;
  }
  void join_(size_t from, size_t num){
    if(from + num >= size_ - 1);
    else if(size_ <= Size){
      moveBackward_(sBuff_() + from + num, sBuff_() + size_, num);
    }else if(size_ - num <= Size){
      Type* buffer = lBuff_();
      moveToBuffer_(
        lBuff_(), lBuff_() + from,
        lBuff_() + from + num, lBuff_() + ls_buffer_.capacity,
        sBuff_()
      );
      delete[] buffer;
    }else{
      moveBackward_(lBuff_() + from + num, lBuff_() + size_, num);
    }
    size_ -= num;
  }

public:

  //iterator types
  typedef Type* iterator;
  typedef const Type* const_iterator;
  typedef std::reverse_iterator<Type*> reverse_iterator;
  typedef std::reverse_iterator<const Type*> const_reverse_iterator;
  
  //construction and assignment
  
  SSOVector(): size_(0){}
  
  SSOVector(const SSOVector& other){
    size_ = other.size_;
    if(size_ <= Size){
      for(size_t i = 0; i < size_; ++i){
        new(ss_buffer_ + TypeSize * i) Type(osBuff_(other)[i]);
      }
    }else{
      ls_buffer_.capacity = other.ls_buffer_.capacity;
      ls_buffer_.buffer = new char[TypeSize * ls_buffer_.capacity];
      for(size_t i = 0; i < size_; ++i){
        new(ls_buffer_.buffer + TypeSize * i) Type(olBuff_(other)[i]);
      }
    }
  }
  SSOVector(SSOVector&& other){
    size_ = other.size_;
    if(size_ > Size){
      ls_buffer_.capacity = other.ls_buffer_.capacity;
      ls_buffer_.buffer = other.ls_buffer_.buffer;
      other.size_ = 0;
      //delete[] other.ls_buffer_.buffer;
      //warning: this leaves a dangling pointer in the moved from object
      //size_ being zero will prevent deleting
    }else{
      for(size_t i = 0; i < size_; ++i){
        new(ss_buffer_ + TypeSize * i) Type(osBuff_(other)[i]);
      }
      //same as copying for small objects
    }
  }
  
  SSOVector(size_t count, const value_type& value = value_type()){
    size_ = count;
    
    if(size_ > Size){
      ls_buffer_.capacity = count + 1;
      ls_buffer_.buffer = new char[TypeSize * ls_buffer_.capacity];
      for(size_t i = 0; i < size_; ++i){
        new(ls_buffer_.buffer + TypeSize * i) Type(value);
      }
    }else{
      for(size_t i = 0; i < size_; ++i){
        new(ss_buffer_ + TypeSize * i) Type(value);
      }
    }
  }
  
  template<class InputIt>
  SSOVector(InputIt b, InputIt e){
    size_ = e - b;
    char* pos;
    if(size_ > Size){
      ls_buffer_.capacity = (size_ * 3) >> 1;
      ls_buffer_.buffer = new char[ls_buffer_.capacity * TypeSize];
      pos = ls_buffer_.buffer;
    }else pos = ss_buffer_;
    for(size_t i = 0; i < size_; ++i){
      new(pos + TypeSize * i) Type(*(b++));
    }
  }
  template<class InType>
  SSOVector(const std::initializer_list<InType>& list){
    size_ = list.size();
    auto it = list.begin();
    char* pos;
    if(size_ > Size){
      ls_buffer_.capacity = (size_ * 3) >> 1;
      ls_buffer_.buffer = new char[ls_buffer_.capacity * TypeSize];
      pos = ls_buffer_.buffer;
    }else pos = ss_buffer_;
    for(size_t i = 0; i < size_; ++i){
      new(pos + TypeSize * i) Type(*(it++));
    }
  }
  template<class InType>
  SSOVector(std::initializer_list<InType>&& list){
    size_ = list.size();
    auto it = list.begin();
    char* pos;
    if(size_ > Size){
      ls_buffer_.capacity = (size_ * 3) >> 1;
      ls_buffer_.buffer = new char[ls_buffer_.capacity * TypeSize];
      pos = ls_buffer_.buffer;
    }else pos = ss_buffer_;
    for(size_t i = 0; i < size_; ++i){
      new(pos + TypeSize * i) Type(std::move(*(it++)));
    }
  }
  
  SSOVector& operator=(const SSOVector& other){
    destroyContent_();
    size_ = other.size_;
    if(size_ <= Size){
      for(size_t i = 0; i < size_; ++i){
        new(ss_buffer_ + TypeSize * i) Type(osBuff_(other)[i]);
      }
    }else{
      ls_buffer_.capacity = other.ls_buffer_.capacity;
      ls_buffer_.buffer = new char[TypeSize * ls_buffer_.capacity];
      for(size_t i = 0; i < size_; ++i){
        new(ls_buffer_.buffer + TypeSize * i) Type(olBuff_(other)[i]);
      }
    }
    return *this;
  }
  SSOVector& operator=(SSOVector&& other){
    destroyContent_();
    size_ = other.size_;
    if(size_ > Size){
      ls_buffer_.capacity = other.ls_buffer_.capacity;
      ls_buffer_.buffer = other.ls_buffer_.buffer;
      other.size_ = 0;
      //warning: same as in move constructor
    }else{
      for(size_t i = 0; i < size_; ++i){
        new(ss_buffer_ + TypeSize * i) Type(osBuff_(other)[i]);
      }
    }
    return *this;
  }
  template<class InType>
  SSOVector& operator=(const std::initializer_list<InType>& list){
    destroyContent_();
    size_ = list.size();
    auto it = list.begin();
    char* pos;
    if(size_ > Size){
      ls_buffer_.capacity = (size_ * 3) >> 1;
      ls_buffer_.buffer = new char[ls_buffer_.capacity * TypeSize];
      pos = ls_buffer_.buffer;
    }else pos = ss_buffer_;
    for(size_t i = 0; i < size_; ++i){
      new(pos + TypeSize * i) Type(*(it++));
    }
    return *this;
  }
  template<class InType>
  SSOVector& operator=(std::initializer_list<InType>&& list){
    destroyContent_();
    size_ = list.size();
    auto it = list.begin();
    char* pos;
    if(size_ > Size){
      ls_buffer_.capacity = (size_ * 3) >> 1;
      ls_buffer_.buffer = new char[ls_buffer_.capacity * TypeSize];
      pos = ls_buffer_.buffer;
    }else pos = ss_buffer_;
    for(size_t i = 0; i < size_; ++i){
      new(pos + TypeSize * i) Type(std::move(*(it++)));
    }
    return *this;
  }
  
  ~SSOVector(){
    destroyContent_();
  }
  
  //element access
  
  Type& operator[](int idx){
    return activeBuffer_()[idx];
  }
  const Type& operator[](int idx) const{
    return activeBuffer_()[idx];
  }
  
  Type& front(){
    return activeBuffer_()[0];
  }
  const Type& front() const{
    return activeBuffer_()[0];
  }
  
  Type& back(){
    return activeBuffer_()[size_ - 1];
  }
  const Type& back() const{
    return activeBuffer_()[size_ - 1];
  }
  
  Type* data(){
    return activeBuffer_();
  }
  const Type* data()const{
    return activeBuffer_();
  }
  
  //capacity
  
  bool empty()const{
    return size_ > 0;
  }
  size_t size()const{
    return size_;
  }
  size_t capacity()const{
    if(size_ > Size) return ls_buffer_.capacity;
    else return Size;
  }
  
  //modifiers
  
  void clear(){
    destroyContent_();
    size_ = 0;
  }
  
  void push_back(const Type& val){
    if(size_ == Size) makeRoomInitial_();
    else if(size_ == capacity()) makeRoomForMore_();
    ++size_;
    new(activeBuffer_() + size_ - 1) Type(val);
  }
  void push_back(Type&& val){
    if(size_ == Size){
      makeRoomInitial_();
    }else if(size_ == capacity()){
      makeRoomForMore_();
    }
    ++size_;
    new(activeBuffer_() + size_ - 1) Type(val);
  }
  template<class... Args>
  void emplace_back(Args&&... args){
    if(size_ == Size){
      makeRoomInitial_();
    }else if(size_ == capacity()){
      makeRoomForMore_();
    }
    ++size_;
    new(activeBuffer_() + size_ - 1) Type(std::forward<Args>(args)...);
  }
  void pop_back(){
    activeBuffer_()[size_ - 1].~Type();
    --size_;
    if(size_ == Size){
      moveBack_();
    }
  }
  
  iterator insert(const_iterator pos, const Type& val){
    size_t p = pos - activeBuffer_();
    split_(p);
    new(activeBuffer_() + p) Type(val);
    return activeBuffer_() + p;
  }
  iterator insert(const_iterator pos, Type&& val){
    size_t p = pos - activeBuffer_();
    split_(p);
    new(activeBuffer_() + p) Type(val);
    return activeBuffer_() + p;
  }
  iterator insert(const_iterator pos, size_t num, Type& val){
    size_t p = pos - activeBuffer_();
    split_(p, num);
    for(int i = 0; i < num; ++i){
      new(activeBuffer_() + p + i) Type(val);
    }
    return activeBuffer_() + p;
  }
  template<class InputIt>
  iterator insert(const_iterator pos, InputIt b, InputIt e){
    size_t p = pos - activeBuffer_();
    int num = e - b;
    split_(p, num);
    for(int i = 0; i < num; ++i){
      new(activeBuffer_() + p + i) Type(*(b++));
    }
    return activeBuffer_() + p;
  }
  iterator insert(const_iterator pos, const std::initializer_list<Type>& list){
    size_t p = pos - activeBuffer_();
    int num = list.size();
    split_(p, num);
    auto it = list.begin();
    for(int i = 0; i < num; ++i){
      new(activeBuffer_() + p + i) Type(*(it++));
    }
    return activeBuffer_() + p;
  }
  iterator insert(const_iterator pos, std::initializer_list<Type>&& list){
    size_t p = pos - activeBuffer_();
    int num = list.size();
    split_(p, num);
    auto it = list.begin();
    for(int i = 0; i < num; ++i){
      new(activeBuffer_() + p + i) Type(std::move(*(it++)));
    }
    return activeBuffer_() + p;
  }
  template<class... Args>
  iterator emplace(const_iterator pos, Args&&... args){
    size_t p = pos - activeBuffer_();
    split_(p);
    new(activeBuffer_() + p) Type(std::forward<Args>(args)...);
    return activeBuffer_() + p;
  }
  
  iterator erase(const_iterator pos){
    size_t p = pos - activeBuffer_();
    pos->~Type();
    join_(p);
    return activeBuffer_() + p;
  }
  iterator erase(const_iterator b, const_iterator e){
    size_t p = b - activeBuffer_();
    size_t n = e - b;
    while(b != e){
      (b++)->~Type();
    }
    join_(p, n);
    return activeBuffer_() + p;
  }
  
  //iterators
  
  iterator begin(){
    return activeBuffer_();
  }
  iterator end(){
    return activeBuffer_() + size_;
  }
  const_iterator begin()const{
    return activeBuffer_();
  }
  const_iterator end()const{
    return activeBuffer_() + size_;
  }
  const_iterator cbegin()const{
    return activeBuffer_();
  }
  const_iterator cend()const{
    return activeBuffer_() + size_;
  }
  reverse_iterator rbegin(){
    return activeBuffer_() + size_ - 1;
  }
  reverse_iterator rend(){
    return activeBuffer_() - 1;
  }
  const_reverse_iterator rbegin()const{
    return activeBuffer_() + size_ - 1;
  }
  const_reverse_iterator rend()const{
    return activeBuffer_() - 1;
  }
  const_reverse_iterator crbegin()const{
    return activeBuffer_() + size_ - 1;
  }
  const_reverse_iterator crend()const{
    return activeBuffer_() - 1;
  }
};

#endif
