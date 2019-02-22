#include "array.h"

#include <algorithm>
#include <iterator>

#ifndef NDEBUG
#include "alloc_monitor.h"
#include <cstdio>
#else
#undef MONITOR_ARRAY_ALLOCS
#endif

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

AllocMonitor<Array> array_alloc_monitor([](AllocMsg msg, const Array* arr){
  switch(msg){
  #ifdef MONITOR_ARRAY_ALLOCS
  case AllocMsg::Allocation:
    fprintf(stderr, "allocated Array at %p\n", arr);
    break;
  case AllocMsg::Deallocation:
    fprintf(stderr, "deallocated Array at %p\n", arr);
    break;
  #endif
  case AllocMsg::DoubleAllocation:
    fprintf(stderr, "double allocated Array at %p\n", arr);
    break;
  case AllocMsg::InvalidFree:
    fprintf(stderr, "invalid free of Array at %p\n", arr);
    break;
  }
});
#endif