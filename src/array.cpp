#include "array.h"

#include <algorithm>
#include <iterator>

Array* Array::slice(int first, int second) const{
  Array* res = new Array;
  res->reserve(second - first);
  std::copy(this->begin() + first, this->begin() + second, std::back_inserter(*res));
  return res;
}

void Array::sliceInline(int first, int second){
  this->erase(this->begin() + second, this->end());
  this->erase(this->begin(), this->begin() + first);
}

#ifndef NDEBUG
std::string Array::toStrDebug()const{
  std::string ret = "[";
  
  auto it = this->begin();
  if(it != this->end()){
    ret += it->toStrDebug();
    ++it;
    while(it != this->end()){
      ret += ", ";
      ret += it->toStrDebug();
      ++it;
    }
  }
  
  ret += "]";
  return ret;
}
#endif