#ifndef TABLE_H_INCLUDED
#define TABLE_H_INCLUDED

#include "value.h"
#include "misc.h"
#include "rc_mixin.h"

#include <memory>
#include <unordered_map>

#ifndef NDEBUG
#include <string>
#endif

class TypedValue;

class Table:
  public std::unordered_map<TypedValue, TypedValue>,
  public RcDirectMixin<Table>
{
public:
  
  using std::unordered_map<TypedValue, TypedValue>::unordered_map;
  
  void operator delete(void* ptr){
    ::operator delete(ptr);
  }
  
  std::unordered_map<TypedValue, TypedValue>::const_iterator begin()const{
    return this->std::unordered_map<TypedValue, TypedValue>::begin();
  }
  std::unordered_map<TypedValue, TypedValue>::const_iterator end()const{
    return this->std::unordered_map<TypedValue, TypedValue>::end();
  }
  std::unordered_map<TypedValue, TypedValue>::iterator begin(){
    return this->std::unordered_map<TypedValue, TypedValue>::begin();
  }
  std::unordered_map<TypedValue, TypedValue>::iterator end(){
    return this->std::unordered_map<TypedValue, TypedValue>::end();
  }
  
  #ifndef NDEBUG
  std::string toStrDebug()const;
  #endif
};

#endif
