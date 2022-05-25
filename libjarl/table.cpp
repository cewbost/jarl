#include "table.h"

#ifndef NDEBUG
std::string Table::toStrDebug()const{
  using namespace std::string_literals;
  std::string ret = "{";
  
  auto it = this->begin();
  if(it != this->end()){
    ret += it->first.toStrDebug() + ": "s + it->second.toStrDebug();
    ++it;
    while(it != this->end()){
      ret += ", "s + it->first.toStrDebug() + ": "s + it->second.toStrDebug();
      ++it;
    }
  }
  ret += "}";
  return ret;
}
#endif
