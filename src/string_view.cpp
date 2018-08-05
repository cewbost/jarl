#include "string_view.h"

#include "misc.h"

#include <cstring>

size_t StringView::hash()const{
  return defaultHash(this->begin(), this->end());
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