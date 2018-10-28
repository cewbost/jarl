#include "op_codes.h"

#include "misc.h"

#ifndef NDEBUG
#include <string>
#endif

#ifndef NDEBUG
std::string OpCodes::opCodeToStrDebug(OpCodes::Type op){
  using namespace std::string_literals;
  
  std::string ret = "";
  
  switch(op & ~OpCodes::Head){
  case OpCodes::Return:
    ret += "return"s;
    break;
  case OpCodes::Nop:
    ret += "nop"s;
    break;
  case OpCodes::Reduce:
    ret += "reduce"s;
    break;
  case OpCodes::Push:
    ret += "push"s;
    break;
  case OpCodes::PushTrue:
    ret += "push true"s;
    break;
  case OpCodes::PushFalse:
    ret += "push false"s;
    break;
  case OpCodes::CreateArray:
    ret += "create array"s;
    break;
  case OpCodes::CreateTable:
    ret += "create table"s;
    break;
  case OpCodes::CreateRange:
    ret += "create range"s;
    break;
  case OpCodes::CreateClosure:
    ret += "create closure"s;
    break;
  case OpCodes::Pop:
    ret += "pop"s;
    break;
  case OpCodes::Write:
    ret += "write"s;
    break;
  case OpCodes::Add:
    ret += "add"s;
    break;
  case OpCodes::Sub:
    ret += "sub"s;
    break;
  case OpCodes::Mul:
    ret += "mul"s;
    break;
  case OpCodes::Div:
    ret += "div"s;
    break;
  case OpCodes::Mod:
    ret += "mod"s;
    break;
  case OpCodes::Append:
    ret += "append"s;
    break;
  case OpCodes::Apply:
    ret += "apply"s;
    break;
  case OpCodes::Neg:
    ret += "neg"s;
    break;
  case OpCodes::Not:
    ret += "not"s;
    break;
  case OpCodes::Cmp:
    ret += "cmp"s;
    break;
  case OpCodes::Eq:
    ret += "eq"s;
    break;
  case OpCodes::Neq:
    ret += "neq"s;
    break;
  case OpCodes::Gt:
    ret += "gt"s;
    break;
  case OpCodes::Lt:
    ret += "lt"s;
    break;
  case OpCodes::Geq:
    ret += "geq"s;
    break;
  case OpCodes::Leq:
    ret += "leq"s;
    break;
  case OpCodes::In:
    ret += "in"s;
    break;
  case OpCodes::Get:
    ret += "get"s;
    break;
  case OpCodes::Slice:
    ret += "slice"s;
    break;
  case OpCodes::Call:
    ret += "call"s;
    break;
  case OpCodes::Recurse:
    ret += "recurse"s;
    break;
  case OpCodes::Borrow:
    ret += "borrow"s;
    break;
  case OpCodes::BeginIter:
    ret += "begin iteration"s;
    break;
  case OpCodes::NextOrJmp:
    ret += "next or jmp"s;
    break;
  case OpCodes::Jmp:
    ret += "jmp"s;
    break;
  case OpCodes::Jt:
    ret += "jt"s;
    break;
  case OpCodes::Jf:
    ret += "jf"s;
    break;
  case OpCodes::Jtsc:
    ret += "jtsc"s;
    break;
  case OpCodes::Jfsc:
    ret += "jfsc"s;
    break;
  case OpCodes::Print:
    ret += "print"s;
    break;
  case OpCodes::Assert:
    ret += "assert"s;
    break;
  default:
    {
      ret += "unknown op code: "s;
      std::unique_ptr<char[]> val(dynSprintf("%x", op & ~OpCodes::Head));
      ret += val.get();
    }
    break;
  }
  
  if(op & OpCodes::Dest)     ret += " dest"s;
  if(op & OpCodes::Int)      ret += " int"s;
  if(op & OpCodes::Borrowed) ret += " borrowed"s;
  if(op & OpCodes::Alt1)     ret += " alt1"s;
  if(op & OpCodes::Alt2)     ret += " alt2"s;
  
  return ret;
}
#endif