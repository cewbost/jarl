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
  std::vector<std::pair<int, int>>&& code_positions,
  int arguments
):
  code_(std::move(code)),
  values_(std::move(values)),
  code_positions_(std::move(code_positions)),
  arguments(arguments)
{}

PartiallyApplied::PartiallyApplied(const Function* func)
: func_(func), args_(func->arguments), nargs(func->arguments){}

void PartiallyApplied::apply(const TypedValue& val, int bind_pos){
  assert(this->nargs > 0);
  for(auto& slot: this->args_){
    if(slot.type == TypeTag::None){
      if((bind_pos--) == 0){
        slot = val;
        break;
      }
    }
  }
  --this->nargs;
}
void PartiallyApplied::apply(TypedValue&& val, int bind_pos){
  assert(this->nargs > 0);
  for(auto& slot: this->args_){
    if(slot.type == TypeTag::None){
      if((bind_pos--) == 0){
        slot = std::move(val);
        break;
      }
    }
  }
  --this->nargs;
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

std::string Function::toStrDebug()const{
  char buffer[32] = "";
  std::sprintf(buffer, "function %p", this);
  return buffer;
}

std::string PartiallyApplied::toStrDebug()const{
  auto it = this->args_.begin();
  std::unique_ptr<char[]> buffer(dynSprintf("%s", it->toStrDebug().c_str()));
  for(++it; it != this->args_.end(); ++it){
    buffer.reset(dynSprintf("%s, %s", buffer.get(), it->toStrDebug().c_str()));
  }
  buffer.reset(dynSprintf("function %p (%s)", this->getFunc(), buffer.get()));
  return buffer.get();
}
#endif