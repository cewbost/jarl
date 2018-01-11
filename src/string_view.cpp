#include "string_view.h"

#include <cstring>

size_t StringView::hash()const{
  constexpr size_t _offset = sizeof(size_t) == 8?
    0xcbf29ce484222325 : 0x811c9dc5;
  constexpr size_t _prime = sizeof(size_t) == 8?
    0x100000001b3 : 0x1000193;
  
  size_t ret = _offset;
  
  for(char ch: *this){
    ret ^= ch;
    ret *= _prime;
  }
  return ret;
}

bool StringView::cmp(const StringView& other)const{
  int s1 = this->size();
  int s2 = other.size();
  int min_s = s1 < s2? s1 : s2;
  auto res = strncmp(this->begin(), other.begin(), min_s);
  if(res == 0){
    if(s1 == s2){
      return 0;
    }else if(s1 < s2){
      return -other[s1];
    }else{
      return this->operator[](s2);
    }
  }else{
    return res;
  }
}