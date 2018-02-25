#ifndef VM_H_INCLUDED
#define VM_H_INCLUDED

#include "rc_trait.h"
#include "misc.h"
#include "string.h"
#include "value.h"
#include "function.h"
#include "fixed_vector.h"

#include <unordered_map>
#include <memory>

class VM{
  
  std::vector<std::unique_ptr<char[]>> errors_;
  
  struct StackFrame{
    rc_ptr<const Function> proc;
    const OpCodeType* ip;
    unsigned bp;
    
    StackFrame(): proc(nullptr), ip(nullptr), bp(0){}
    StackFrame(const Function* p, unsigned b): proc(p), ip(nullptr), bp(b){}
    
    StackFrame(StackFrame&&) = default;
    StackFrame& operator=(StackFrame&&) = default;
  };
  
  std::unordered_map<
    rc_ptr<String>,
    TypedValue,
    ptr_hash<rc_ptr<String>>,
    ptr_equal_to<rc_ptr<String>>
  > global_table_;
  
  void (*print_func_)(const char*);
  
  FixedVector<TypedValue> stack_;
  std::vector<StackFrame> call_stack_;
  
  StackFrame frame_;
  
  void doArithOp_(const OpCodeType**, bool (TypedValue::*)(const TypedValue&));
  void doCmpOp_(const OpCodeType**, CmpMode);
  void pushFunction_(const Function&);
  void pushFunction_(const PartiallyApplied&);
  bool popFunction_();
  
public:
  
  VM(int = 1024);
  
  void execute(const Function&);
  
  void setPrintFunc(void(*)(const char*));
  
  void reportError(std::unique_ptr<char[]>);
  int errors() const;
  std::unique_ptr<char[]> getError();
};

#endif