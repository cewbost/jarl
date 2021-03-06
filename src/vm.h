#ifndef VM_H_INCLUDED
#define VM_H_INCLUDED

#include "rc_mixin.h"
#include "misc.h"
#include "string.h"
#include "value.h"
#include "function.h"
#include "fixed_vector.h"

#include <unordered_map>
#include <memory>

#include <csetjmp>

class VM{
public:
  struct StackFrame{
    rc_ptr<const Function> func;
    const OpCodes::Type* ip;
    unsigned bp;
    
    StackFrame(): func(nullptr), ip(nullptr), bp(0){}
    StackFrame(const Function* p, unsigned b): func(p), ip(nullptr), bp(b){}
    
    StackFrame(StackFrame&&) = default;
    StackFrame& operator=(StackFrame&&) = default;
  };
  
private:
  
  jmp_buf error_jmp_env_;
  
  std::unordered_map<
    rc_ptr<String>,
    TypedValue,
    ptr_hash<rc_ptr<String>>,
    ptr_equal_to<rc_ptr<String>>
  > global_table_;
  
  void (*print_func_)(const char*);
  void (*error_print_func_)(const char*);
  
  FixedVector<TypedValue> stack_;
  std::vector<StackFrame> call_stack_;
  
  StackFrame frame_;
  
  void doArithOp_(const OpCodes::Type**, void (TypedValue::*)(const TypedValue&));
  void doCmpOp_(const OpCodes::Type**, CmpMode);
  void pushFunction_(const Function&);
  void pushFunction_(const PartiallyApplied&);
  void pushFunction_(const PartiallyApplied&, int);
  bool popFunction_();
  
public:
  
  VM(int = 1024);
  
  void execute(const Function&);
  
  void setPrintFunc(void(*)(const char*));
  void setErrorPrintFunc(void(*)(const char*));
  void print(const char*);
  void errPrint(const char*);
  
  StackFrame* getFrame();
  
  void errorJmp(int);
  
  static void setCurrentVM(VM*);
  static VM* getCurrentVM();
};

#endif