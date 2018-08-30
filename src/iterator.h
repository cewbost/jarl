#ifndef ITERATOR_H_INCLUDED
#define ITERATOR_H_INCLUDED

#include "rc_mixin.h"
#include "value.h"
#include "array.h"
#include "table.h"

#ifndef NDEBUG
#include <string>
#endif

class Iterator: public RcDirectMixin<Iterator> {
  
  struct ArrayIterator {
    Array* ref;
    Array::iterator current_it;
    Array::iterator end_it;
  };
  struct TableIterator {
    Table* ref;
    Table::iterator current_it;
    Table::iterator end_it;
  };
  
  TypeTag type_;
  union DataUnion_ {
    ArrayIterator array_v;
    TableIterator table_v;
    DataUnion_(){}
  }data_;
  
public:
  
  Iterator(const TypedValue&);
  ~Iterator();
  
  Iterator(const Iterator&) = delete;
  Iterator(Iterator&&) = delete;
  void operator=(const Iterator&) = delete;
  void operator=(Iterator&&) = delete;
  
  bool ended()const;
  void advance();
  TypedValue getKey()const;
  TypedValue getValue()const;
  
  void operator delete(void* ptr){
    ::operator delete(ptr);
  }
  
  #ifndef NDEBUG
  std::string toStrDebug()const;
  #endif
};

#endif