#include "op_codes.h"

#include "misc.h"

#ifndef NDEBUG
#include <string>
#endif

#ifndef NDEBUG
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
  case Op::Assert:
    ret += "assert"s;
    break;
  default:
    {
      ret += "unknown op code: "s;
      std::unique_ptr<char[]> val(dynSprintf("%x", op & ~Op::Head));
      ret += val.get();
    }
    break;
  }
  
  if(op & Op::Dest)   ret += " dest"s;
  if(op & Op::Int)    ret += " int"s;
  if(op & Op::Bool)   ret += " bool"s;
  
  return ret;
}
#endif