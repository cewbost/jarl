#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include "rc_mixin.h"
#include "value.h"

#include <vector>
#include <algorithm>
#include <iterator>

#ifndef NDEBUG
# include <string>
#endif

class TypedValue;

class Array: public std::vector<TypedValue>, public RcDirectMixin<Array>{
public:
  
  using std::vector<TypedValue>::vector;
  
  void operator delete(void* ptr){
    ::operator delete(ptr);
  }
  
  Array* slice(int, int) const;
  void sliceInline(int, int);
  
  std::vector<TypedValue>::const_iterator begin()const{
    return this->std::vector<TypedValue>::begin();
  }
  std::vector<TypedValue>::const_iterator end()const{
    return this->std::vector<TypedValue>::end();
  }
  std::vector<TypedValue>::iterator begin(){
    return this->std::vector<TypedValue>::begin();
  }
  std::vector<TypedValue>::iterator end(){
    return this->std::vector<TypedValue>::end();
  }
  
  #ifndef NDEBUG
  std::string toStrDebug()const;
  #endif
};

inline Array* constructArray(Array* arr){
  return arr;
}
template<class... T>
inline Array* constructArray(Array* arr, const TypedValue& val, T... args){
  arr->push_back(val);
  return constructArray(arr, args...);
}
template<class... T>
inline Array* constructArray(Array* arr, TypedValue&& val, T... args){
  arr->push_back(std::move(val));
  return constructArray(arr, args...);
}
template<class... T>
inline Array* constructArray(Array* arr, const Array& other, T... args){
  std::copy(other.begin(), other.end(), std::back_inserter(*arr));
  return constructArray(arr, args...);
}

#endif