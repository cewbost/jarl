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
  std::vector<OpCodes::Type>&& code,
  std::vector<TypedValue>&& values,
  std::vector<std::pair<int, int>>&& code_positions,
  unsigned arguments,
  unsigned captures,
  unsigned locals
):
  code_(std::move(code)),
  values_(std::move(values)),
  code_positions_(std::move(code_positions)),
  arguments(arguments),
  captures(captures),
  locals(locals)
{}

PartiallyApplied::PartiallyApplied(const Function* func)
: func_(func), args_(func->arguments + func->captures), nargs(func->arguments){}

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
void PartiallyApplied::capture(TypedValue* from, TypedValue* to){
  assert((to - from) == this->func_->captures);
  for(int bind_pos = this->func_->arguments; from != to; ++from, ++bind_pos){
    this->args_[bind_pos] = std::move(*from);
  }
}

int Function::getLine(const OpCodes::Type* iit) const {
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
  
  unsigned extended = 0;
  int counter = 0;
  for(auto op: this->code_){
    if(extended == 0){
      snprintf(buffer, sizeof(buffer), "%4d: ", counter);
      ret += buffer;
      ret += OpCodes::opCodeToStrDebug(op);
      if(op & OpCodes::Extended) ++extended;
      if(op & OpCodes::Extended2) ++extended;
    }else{
      ret += " "s + std::to_string(op);
      --extended;
    }
    if(extended == 0) ret += "\n"s;
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