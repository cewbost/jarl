#ifndef ARRAY_VIEW_H_INCLUDED
#define ARRAY_VIEW_H_INCLUDED

#include <utility>

template<class T>
struct ArrayView: std::pair<T*, T*>{
  
  ArrayView() = default;
  ArrayView(T* b, T* e)
  : std::pair<T*, T*>(b, e){}
  ArrayView(T* b, int s)
  : std::pair<T*, T*>(b, b + s){}
  
  template<class Y>
  ArrayView(const ArrayView<Y>& other)
  : std::pair<T*, T*>(static_cast<T*>(other.first), static_cast<T*>(other.second)){}
  
  int size()const{
    return this->second - this->first;
  }
  bool empty()const{
    return this->first == this->second;
  }
  T pop_front(){
    T ret = *this->first;
    ++this->first;
    return ret;
  }
  
  T* begin()const{
    return this->first < this->second? this->first : this->second;
  }
  T* end()const{
    return this->second;
  }
  
  T& operator[](int idx){
    return this->first[idx];
  }
  const T& operator[](int idx)const{
    return this->first[idx];
  }
  
  template<class Y>
  friend struct ArrayView;
};

#endif