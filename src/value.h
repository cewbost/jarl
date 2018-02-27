#ifndef VALUE_H_INCLUDED
#define VALUE_H_INCLUDED

#include "string.h"
#include "function.h"
#include "array.h"

#include <jarl.h>

#include <functional>

#ifndef NDEBUG
#include <string>
#endif

class Function;
class PartiallyApplied;
class Array;

using jarl::Int;
using jarl::Float;

constexpr Int operator"" _i(unsigned long long i){
  return static_cast<Int>(i);
}
constexpr Float operator"" _f(long double f){
  return static_cast<Float>(f);
}

enum class TypeTag: Int{
  None,
  Null,
  Ptr,
  Rvalue,
  Bool,
  Int,
  Float,
  String,
  Proc,
  Partial,
  Array
};

enum class CmpMode{
  Equal,
  NotEqual,
  Greater,
  Less,
  GreaterEqual,
  LessEqual
};

struct TypedValue;

struct Value{
  union{
    TypedValue*       rvalue_v;
    bool              bool_v;
    Int               int_v;
    Float             float_v;
    String*           string_v;
    Value*            ref_v;
    Function*         func_v;
    PartiallyApplied* partial_v;
    Array*            array_v;
    void*             ptr_v;
  };
  
  Value() = default;
  Value(bool b){
    this->bool_v = b;
  }
  Value(Int i){
    this->int_v = static_cast<Int>(i);
  }
  Value(Float f){
    this->float_v = static_cast<Float>(f);
  }
  Value(String* s){
    this->string_v = s;
  }
  Value(Value* v){
    this->ref_v = v;
  }
  Value(void* p){
    this->ptr_v = p;
  }
  
  #ifndef NDEBUG
  std::string boolToStrDebug()const{
    return std::string(this->bool_v? "true" : "false");
  }
  std::string intToStrDebug()const{
    return std::to_string(this->int_v);
  }
  std::string floatToStrDebug()const{
    return std::to_string(this->float_v);
  }
  std::string toStrDebug()const{
    return std::string(this->string_v->str());
  }
  #endif
};

class TypedValue{
  
  void clear_();
  void move_(TypedValue&&)noexcept;
  void copy_(const TypedValue&)noexcept;
  
public:
  
  TypeTag type;
  Value value;
  
  TypedValue();
  TypedValue(nullptr_t);
  TypedValue(TypedValue*);
  TypedValue(bool);
  TypedValue(Int);
  TypedValue(Float);
  TypedValue(String*);
  TypedValue(Function*);
  TypedValue(PartiallyApplied*);
  TypedValue(Array*);
  TypedValue(const void*);
  
  TypedValue& operator=(nullptr_t);
  TypedValue& operator=(TypedValue*);
  TypedValue& operator=(bool);
  TypedValue& operator=(Int);
  TypedValue& operator=(Float);
  TypedValue& operator=(String*);
  TypedValue& operator=(Function*);
  TypedValue& operator=(PartiallyApplied*);
  TypedValue& operator=(Array*);
  TypedValue& operator=(const void*);
  
  TypedValue(TypedValue&& other)noexcept;
  TypedValue& operator=(TypedValue&& other)noexcept;
  
  TypedValue(const TypedValue&)noexcept;
  TypedValue& operator=(const TypedValue&)noexcept;
  
  ~TypedValue()noexcept{
    this->clear_();
  }
  
  void add(const TypedValue&);
  void sub(const TypedValue&);
  void mul(const TypedValue&);
  void div(const TypedValue&);
  void mod(const TypedValue&);
  void append(const TypedValue&);
  void append(TypedValue&&);
  void neg();
  
  void cmp(const TypedValue&);
  void cmp(const TypedValue&, CmpMode);
  
  void boolNot();
  
  void get(const TypedValue&);
  void slice(const TypedValue&, const TypedValue&);
  
  void toBool();
  void toBool(bool*);
  void toInt();
  void toFloat();
  void toString();
  
  void toPartial();
  
  bool asBool()const{return this->value.bool_v;}
  Int asInt()const{return this->value.int_v;}
  Float asFloat()const{return this->value.float_v;}
  const String* asString()const{return this->value.string_v;}
  
  #ifndef NDEBUG
  std::string toStrDebug()const;
  #endif
};

static_assert(sizeof(TypedValue) == sizeof(void*) * 2);

namespace std{
  
  template<> struct equal_to<TypedValue>{
    constexpr bool operator()(const TypedValue& lhs, const TypedValue& rhs)const{
      if(lhs.type != rhs.type) return false;
      
      switch(lhs.type){
      case TypeTag::Null:
        return true;
      case TypeTag::Rvalue:
        return lhs.value.rvalue_v == rhs.value.rvalue_v;
      case TypeTag::Bool:
        return lhs.value.bool_v == rhs.value.bool_v;
      case TypeTag::Int:
        return lhs.value.int_v == rhs.value.int_v;
      case TypeTag::Float:
        return lhs.value.float_v == rhs.value.float_v;
      case TypeTag::String:
        return std::equal_to<String>()(*lhs.value.string_v, *rhs.value.string_v);
      default:
        return false;
      }
    }
  };
}


#endif