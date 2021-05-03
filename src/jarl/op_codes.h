#ifndef OP_CODES_H_INCLUDED
#define OP_CODES_H_INCLUDED

#include <cstdint>

#ifndef NDEBUG
#include <string>
#endif

namespace OpCodes {
  
  using Type        = uint16_t;
  using SignedType  = int16_t;
  
  enum {
    Return = 0,
    Nop,
    
    Push,
    PushTrue,
    PushFalse,
    Pop,
    Reduce,
    Write,
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Append,
    Apply,
    
    Neg,
    Not,
    
    Move,
    
    Cmp,
    Eq,
    Neq,
    Gt,
    Lt,
    Geq,
    Leq,
    
    In,
    
    Get,
    Slice,
    
    Call,
    Recurse,
    
    Store,
    Borrow,
    
    BeginIter,
    NextOrJmp,
    
    Jmp,
    Jt,
    Jf,
    Jtsc,
    Jfsc,
    
    CreateArray,
    CreateTable,
    CreateRange,
    CreateClosure,
    
    Print,
    Assert,
  };
  
  constexpr Type Extended   = 0x8000;
  constexpr Type Extended2  = 0x4000;
  constexpr Type Dest       = 0x2000;
  constexpr Type Int        = 0x1000;
  constexpr Type Borrowed   = 0x0800;
  constexpr Type Alt1       = 0x0400;
  constexpr Type Alt2       = 0x0200;
  
  constexpr Type Head       = 0xfe00;
  
  #ifndef NDEBUG
  std::string opCodeToStrDebug(OpCodes::Type);
  #endif
}

#endif