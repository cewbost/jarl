#ifndef OP_CODES_H_INCLUDED
#define OP_CODES_H_INCLUDED

#include <cstdint>

#ifndef NDEBUG
#include <string>
#endif

using OpCodeType        = uint16_t;
using OpCodeSignedType  = int16_t;

struct Op{
  enum{
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
    
    Print,
    Assert,
    
    Extended  = 0x8000,
    Dest      = 0x4000,
    Int       = 0x2000,
    Borrowed  = 0x1000,
    Alt1      = 0x0800,
    Alt2      = 0x0400,
    
    Head      = 0xf800
  };
};

#ifndef NDEBUG
std::string opCodeToStrDebug(OpCodeType);
#endif

#endif