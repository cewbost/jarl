#include "function.h"

#include "vm.h"
#include "delegate_map.h"

#include <limits>
#include <cassert>

namespace std{
  template<>
  struct equal_to<String*>{
    typedef ptr_equal_to<String*> type;
  };
}

Function::Function(
  std::vector<OpCodeType>&& code,
  std::vector<TypedValue>&& values,
  std::vector<std::pair<int, int>>&& code_positions
):
  code_(std::move(code)),
  values_(std::move(values)),
  code_positions_(std::move(code_positions))
{}

PartiallyApplied::PartiallyApplied(const Function* func)
: func_(func), args_(func->arguments), nargs(func->arguments){}

bool PartiallyApplied::apply(const TypedValue& val, int bind_pos){
  assert(this->nargs > 0);
  for(auto& slot: this->args_){
    if(slot.type == TypeTag::None){
      if((bind_pos--) == 0){
        slot = val;
        break;
      }
    }
  }
  return --this->nargs == 0;
}
bool PartiallyApplied::apply(TypedValue&& val, int bind_pos){
  assert(this->nargs > 0);
  for(auto& slot: this->args_){
    if(slot.type == TypeTag::None){
      if((bind_pos--) == 0){
        slot = std::move(val);
        break;
      }
    }
  }
  return --this->nargs == 0;
}

int Function::getLine(const OpCodeType* iit) const {
  ptrdiff_t pos = iit - this->code_.data();
  auto it = this->code_positions_.begin();
  ptrdiff_t line = it->first;
  for(;;){
    if(it == this->code_positions_.end()){
      return this->code_positions_.back().second;
    }
    if(it->second > pos) break;
    
    line = it->first;
    ++it;
  }
  return line;
}

#ifndef NDEBUG
std::string Function::opcodesToStrDebug()const{
  using namespace std::string_literals;
  
  std::string ret = "";
  char buffer[7] = "\0";
  
  auto vit = this->values_.begin();
  
  bool extended = false;
  int counter = 0;
  for(auto op: this->code_){
    if(!extended){
      snprintf(buffer, sizeof(buffer), "%4d: ", counter);
      ret += buffer;
      ret += opCodeToStrDebug(op);
      if(op & Op::Extended) extended = true;
      else ret += "\n"s;
    }else{
      ret += " "s + std::to_string(op) + "\n"s;
      extended = false;
    }
    ++counter;
  }
  
  return ret;
}

std::string opCodeToStrDebug(OpCodeType op){
  using namespace std::string_literals;
  
  std::string ret = "";
  
  switch(op & ~Op::Head){
  case Op::Return:
    ret += "return"s;
    break;
  case Op::Nop:
    ret += "nop"s;
    break;
  case Op::Reduce:
    ret += "reduce"s;
    break;
  case Op::Push:
    ret += "push"s;
    break;
  case Op::PushTrue:
    ret += "push true"s;
    break;
  case Op::PushFalse:
    ret += "push false"s;
    break;
  case Op::CreateArray:
    ret += "create array"s;
    break;
  case Op::CreateRange:
    ret += "create range"s;
    break;
  case Op::Pop:
    ret += "pop"s;
    break;
  case Op::Write:
    ret += "write"s;
    break;
  case Op::Add:
    ret += "add"s;
    break;
  case Op::Sub:
    ret += "sub"s;
    break;
  case Op::Mul:
    ret += "mul"s;
    break;
  case Op::Div:
    ret += "div"s;
    break;
  case Op::Mod:
    ret += "mod"s;
    break;
  case Op::Append:
    ret += "append"s;
    break;
  case Op::Apply:
    ret += "apply"s;
    break;
  case Op::Neg:
    ret += "neg"s;
    break;
  case Op::Not:
    ret += "not"s;
    break;
  case Op::Cmp:
    ret += "cmp"s;
    break;
  case Op::Eq:
    ret += "eq"s;
    break;
  case Op::Neq:
    ret += "neq"s;
    break;
  case Op::Gt:
    ret += "gt"s;
    break;
  case Op::Lt:
    ret += "lt"s;
    break;
  case Op::Geq:
    ret += "geq"s;
    break;
  case Op::Leq:
    ret += "leq"s;
    break;
  case Op::Get:
    ret += "get"s;
    break;
  case Op::Slice:
    ret += "slice"s;
    break;
  case Op::Jmp:
    ret += "jmp"s;
    break;
  case Op::Jt:
    ret += "jt"s;
    break;
  case Op::Jf:
    ret += "jf"s;
    break;
  case Op::Jtsc:
    ret += "jtsc"s;
    break;
  case Op::Jfsc:
    ret += "jfsc"s;
    break;
  case Op::Print:
    ret += "print"s;
    break;
  default:
    ret += "[unknown op code]"s;
    break;
  }
  
  if(op & Op::Dest)  ret += " dest"s;
  if(op & Op::Int)   ret += " int"s;
  
  return ret;
}

std::string Function::toStrDebug()const{
  char buffer[20] = "";
  std::sprintf(buffer, "%p", (const void*)this);
  return buffer;
}

std::string PartiallyApplied::toStrDebug()const{
  char buffer[20] = "";
  std::sprintf(buffer, "%p", (const void*)this);
  return buffer;
}
#endif