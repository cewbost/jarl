#ifndef TABLE_H_INCLUDED
#define TABLE_H_INCLUDED

#include "value.h"
#include "misc.h"
#include "rc_trait.h"

#include <memory>
#include <unordered_map>

class Table: public RcTrait<Table>{
  
  std::unordered_map<
    std::unique_ptr<String>,
    TypedValue,
    ptr_hash<std::unique_ptr<String>>,
    ptr_equal_to<std::unique_ptr<String>>
  > slots_;
  
public:
  
};

#endif