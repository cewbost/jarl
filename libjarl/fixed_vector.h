#ifndef FIXED_VECTOR_H_INCLUDED
#define FIXED_VECTOR_H_INCLUDED

#include <new>
#include <cstddef>

template<class T>
class FixedVector{

public:
  
  typedef T value_type;
  
  typedef value_type* iterator;
  typedef const value_type* const_iterator;
  
  class const_reverse_iterator{
    value_type* it_;
  public:
    const_reverse_iterator() = default;
    const_reverse_iterator(const value_type* t): it_(t){}
    const_reverse_iterator& operator=(const value_type* t){
      this->it_ = t;
      return *this;
    }
    
    const_reverse_iterator& operator++(){
      --this->it_;
      return *this;
    }
    const_reverse_iterator operator++(int){
      auto copy = *this;
      --this->it_;
      return copy;
    }
    const_reverse_iterator& operator--(){
      ++this->it_;
      return *this;
    }
    const_reverse_iterator operator--(int){
      auto copy = *this;
      ++this->it_;
      return copy;
    }
    
    const_reverse_iterator operator+(int i){
      return const_reverse_iterator(this->it_ - i);
    }
    const_reverse_iterator operator-(int i){
      return const_reverse_iterator(this->it_ + i);
    }
    const_reverse_iterator& operator+=(int i){
      this->it_ -= i;
      return *this;
    }
    const_reverse_iterator& operator-=(int i){
      this->it_ += i;
      return *this;
    }
    ptrdiff_t operator-(const_reverse_iterator it){
      return -(this->it_ - it.it_);
    }
    
    bool operator==(const_reverse_iterator it){
      return this->it_ == it.it_;
    }
    bool operator!=(const_reverse_iterator it){
      return this->it_ != it.it_;
    }
    bool operator>(const_reverse_iterator it){
      return this->it_ < it.it_;
    }
    bool operator<(const_reverse_iterator it){
      return this->it_ > it.it_;
    }
    bool operator>=(const_reverse_iterator it){
      return this->it_ <= it.it_;
    }
    bool operator<=(const_reverse_iterator it){
      return this->it_ >= it.it_;
    }
    
    const value_type& operator[](int idx){
      return *(this->it_ - idx);
    }
    const value_type& operator*(){
      return *this->it_;
    }
  };
  
  class reverse_iterator: public const_reverse_iterator{
  public:
    value_type& operator[](int idx){
      return *(this->it_ - idx);
    }
    value_type& operator*(){
      return *this->it_;
    }
  };
  
private:
  
  value_type *begin_, *end_, *cap_;
  
  void moveElementsForward_(const_iterator pos){
    ::new(this->end_) value_type(std::move(*(this->end_ - 1)));
    for(auto it = this->end_ - 2; it >= pos; --it){
      it[1] = std::move(*it);
    }
    ++this->end_;
  }
  void moveElementsForward_(const_iterator pos, int num){
    const_iterator sep = this->end_ - num;
    if(sep <= pos){
      sep = pos;
      for(; sep < this->end_; ++sep){
        ::new(sep + num) value_type(std::move(*sep));
      }
    }else{
      for(auto it = sep; it < this->end_; ++it){
        ::new(it + num) value_type(std::move(*it));
      }
      for(--sep; sep >= pos; --sep){
        sep[num] = std::move(*sep);
      }
    }
    this->end_ += num;
  }
  
  void removeElement_(const_iterator pos){
    for(; pos < this->end_ - 1; ++pos){
      *pos = std::move(pos[1]);
    }
    pos->~value_type();
    --this->end_;
  }
  void removeElement_s(const_iterator pos, int num){
    const_iterator sep = this->end_ - num;
    for(; pos < sep; ++pos){
      *pos = std::move(pos[num]);
    }
    for(; pos < this->end_; ++pos){
      pos->~value_type();
    }
    this->end_ -= num;
  }
  
public:
  
  FixedVector(size_t size){
    this->begin_ = reinterpret_cast<value_type*>
      (::operator new(size * sizeof(value_type)));
    this->end_ = this->begin_;
    this->cap_ = this->begin_ + size;
  }
  ~FixedVector(){
    for(auto b = this->begin_; b < this->end_; ++b){
      b->~value_type();
    }
    ::operator delete(this->begin_);
  }
  
  FixedVector(const FixedVector& other){
    this->begin_ = reinterpret_cast<value_type*>
      (::operator new(other.capacity() * sizeof(value_type)));
    this->end_ = this->begin_ + other.size();
    this->cap_ = this->begin_ + other.capacity();
    auto it = this->begin_;
    for(const auto& t: other){
      ::new(it++) value_type(t);
    }
  }
  FixedVector(FixedVector&& other){
    this->begin_ = other.begin_;
    this->end_ = other.end_;
    this->cap_ = other.cap_;
    other.begin_ = nullptr;
  }
  
  FixedVector& operator=(const FixedVector& other){
    this->clear();
    this->end_ = this->begin_ + other.size();
    auto it = this->begin_;
    for(const auto& t: other){
      ::new(it++) value_type(t);
    }
    return *this;
  }
  FixedVector& operator=(FixedVector&& other){
    this->clear();
    ::operator delete(this->begin_);
    this->begin_ = other.begin_;
    this->end_ = other.end_;
    this->cap_ = other.cap_;
    other.begin_ = nullptr;
  }
  
  void clear(){
    for(auto& t: *this){
      t.~value_type();
    }
    this->end_ = this->begin_;
  }
  
  value_type& operator[](size_t idx){
    return this->begin_[idx];
  }
  const value_type& operator[](size_t idx)const{
    return this->begin_[idx];
  }
  value_type& front(){
    return *this->begin_;
  }
  const value_type& front()const{
    return *this->begin_();
  }
  value_type& back(){
    return *(this->end_ - 1);
  }
  const value_type& back()const{
    return *(this->end_ - 1);
  }
  value_type* data(){
    return this->begin_;
  }
  const value_type* data()const{
    return this->begin_;
  }
  
  bool empty()const{
    return this->begin_ == this->end_;
  }
  size_t size()const{
    return this->end_ - this->begin_;
  }
  size_t capacity()const{
    return this->cap_ - this->begin_;
  }
  
  iterator insert(const_iterator pos, const value_type& val){
    this->moveElementsForward_(pos);
    *pos = val;
    return pos;
  }
  iterator insert(const_iterator pos, value_type&& val){
    this->moveElementsForward_(pos);
    *pos = std::move(val);
    return pos;
  }
  template<class... Args>
  iterator emplace(const_iterator pos, Args&&... args){
    this->moveElementsForward_(pos);
    pos->~value_type();
    ::new(pos) value_type(args...);
    return pos;
  }
  iterator erase(const_iterator pos){
    this->removeElement_(pos);
    return pos;
  }
  
  void push_back(const value_type& val){
    ::new(this->end_) value_type(val);
    ++this->end_;
  }
  void push_back(value_type&& val){
    ::new(this->end_) value_type(std::move(val));
    ++this->end_;
  }
  template<class... Args>
  void emplace_back(Args... args){
    ::new(this->end_) value_type(args...);
    ++this->end_;
  }
  void pop_back(){
    --this->end_;
    this->end_->~value_type();
  }
  
  void resize(size_t count){
    if(count > this->size()){
      auto targ = this->begin_ + count;
      for(auto it = this->end_; it < targ; ++it){
        ::new(it) value_type;
      }
      this->end_ = targ;
    }else if(count < this->size()){
      for(auto it = this->begin_ + count; it < this->end_; ++it){
        it->~value_type();
      }
      this->end_ = this->begin_ + count;
    }
  }
  void resize(size_t count, const value_type& value){
    if(count > this->size()){
      auto targ = this->begin_ + count;
      for(auto it = this->end_; it < targ; ++it){
        ::new(it) value_type(value);
      }
      this->end_ = targ;
    }else if(count < this->size()){
      for(auto it = this->begin_ + count; it < this->end_; ++it){
        it->~value_type();
      }
      this->end_ = this->begin_ + count;
    }
  }
  
  void swap(FixedVector& other){
    value_type *t_begin, *t_end, *t_cap;
    t_begin = other.begin_;
    t_end = other.end_;
    t_cap = other.cap_;
    other.begin_ = this->begin_;
    other.end_ = this->end_;
    other.cap_ = this->cap_;
    this->begin_ = t_begin;
    this->end_ = t_end;
    this->cap_ = t_cap;
  }
  
  iterator begin(){
    return iterator(this->begin_);
  }
  iterator end(){
    return iterator(this->end_);
  }
  const_iterator cbegin()const{
    return const_iterator(this->begin_);
  }
  const_iterator cend()const{
    return const_iterator(this->end_);
  }
  reverse_iterator rbegin(){
    return reverse_iterator(this->end_ - 1);
  }
  reverse_iterator rend(){
    return reverse_iterator(this->begin_ - 1);
  }
  const_reverse_iterator crbegin()const{
    return const_reverse_iterator(this->end_ - 1);
  }
  const_reverse_iterator crend()const{
    return const_reverse_iterator(this->begin_ - 1);
  }
};

namespace std{
  template<class value_type>
  void swap(FixedVector<value_type>& lhs, FixedVector<value_type>& rhs){
    lhs.swap(rhs);
  }
}

#endif