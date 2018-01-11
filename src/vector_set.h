#ifndef VECTOR_SET_H_INCLUDED
#define VECTOR_SET_H_INCLUDED

#include "misc.h"

#include <vector>
#include <functional>

template<class Key, class Comp = typename std::equal_to<Key>>
class VectorSet: public std::vector<Key>{
public:
  
  typedef Key key_type;
  typedef Key value_type;
  
private:
  
  Comp comp_;
  
public:
  
  VectorSet(Comp c = Comp())
  : comp_(std::move(c)){}
  
  const Key& at(int idx)const{
    return this->std::vector<Key>::operator[](idx);
  }
  int operator[](const Key& key)const{
    for(int i = 0; i < this->size(); ++i){
      if(comp_(this->std::vector<Key>::operator[](i), key)){
        return i;
      }
    }
    return -1;
  }
  
  int insert(const Key& key){
    int i = (*this)[key];
    if(i == -1){
      this->push_back(key);
      return this->size() - 1;
    }else return i;
  }
  
  int insert(Key&& key){
    int i = (*this)[key];
    if(i == -1){
      this->push_back(std::move(key));
      return this->size() - 1;
    }else return i;
  }
  
  template<class... T>
  int emplace(T... t){
    int idx = this->size();
    this->emplace_back(t...);
    auto& key = this->back();
    
    for(int i = 0; i < idx; ++i){
      if(comp_(this->std::vector<Key>::operator[](i), key)){
        this->pop_back();
        return i;
      }
    }
    return idx;
  }
};

#endif