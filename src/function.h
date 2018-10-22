#ifndef PROCEDURE_H_INCLUDED
#define PROCEDURE_H_INCLUDED

#include "rc_mixin.h"
#include "value.h"
#include "ast.h"
#include "vector_map.h"
#include "vector_set.h"
#include "delegate_map.h"
#include "sso_vector.h"

#include "op_codes.h"

#include <vector>
#include <memory>
#include <cstdint>

#ifndef NDEBUG
#include <string>
#endif

/*
  Note:
    op_push with op_dest flag pushes an integer stored in the code
*/

class TypedValue;

class Function: public RcDirectMixin<Function>{
  
  std::vector<OpCodeType> code_;
  std::vector<TypedValue> values_;
  std::vector<std::pair<int, int>> code_positions_;
  
public:
  
  unsigned short arguments, captures;
  
  Function(const Function&) = delete;
  Function(Function&&) = delete;
  
  Function(
    std::vector<OpCodeType>&& code,
    std::vector<TypedValue>&& values,
    std::vector<std::pair<int, int>>&& code_positions,
    unsigned short arguments,
    unsigned short captures
  );
  
  const std::vector<OpCodeType>& getVCode()const{return this->code_;}
  const OpCodeType* getCode()const{return this->code_.data();}
  size_t getCodeSize()const{return this->code_.size();}
  
  const std::vector<TypedValue>& getVValues()const{return this->values_;}
  const TypedValue* getValues()const{return this->values_.data();}
  size_t getNumValues()const{return this->values_.size();}
  
  int getLine(const OpCodeType*) const;
  
  void operator delete(void* ptr){
    ::operator delete(ptr);
  }
  
  #ifndef NDEBUG
  std::string opcodesToStrDebug()const;
  std::string toStrDebug()const;
  #endif
};

class PartiallyApplied: public RcDirectMixin<PartiallyApplied>{
  
  typedef SSOVector<TypedValue, 8, sizeof(void*) * 2> ArgVectorType;
  
  rc_ptr<const Function> func_;
  ArgVectorType args_;
  
public:

  int nargs;
  
  PartiallyApplied(const Function*);
  
  void apply(const TypedValue&, int);
  void apply(TypedValue&&, int);
  void capture(TypedValue*, TypedValue*);
  
  const Function* getFunc()const{return this->func_.get();}
  
  ArgVectorType::const_iterator cbegin()const{return this->args_.cbegin();}
  ArgVectorType::const_iterator cend()const{return this->args_.cend();}
  ArgVectorType::iterator begin(){return this->args_.begin();}
  ArgVectorType::iterator end(){return this->args_.end();}
  
  void operator delete(void* ptr){
    ::operator delete(ptr);
  }
  
  #ifndef NDEBUG
  std::string toStrDebug()const;
  #endif
};

#endif