#ifndef STRING_VIEW_H_INCLUDED
#define STRING_VIEW_H_INCLUDED

#include "array_view.h"
#include "cmp_trait.h"

#include <functional>
#include <cstddef>

class StringView: public ArrayView<const char>, public CmpTrait<StringView>{
public:
  using ArrayView::ArrayView;
  
  size_t hash()const;
  
  bool cmp(const StringView& other)const;
};

namespace std{
  
  template<> struct hash<StringView>{
    size_t operator()(const StringView& arg)const{
      return arg.hash();
    }
  };
  
  template<> struct equal_to<StringView>{
    bool operator()(const StringView& lhs, const StringView& rhs)const{
      return lhs == rhs;
    }
  };
}

#endif