#ifndef OP_CODES_H_INCLUDED
#define OP_CODES_H_INCLUDED

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
    
    Get,
    Slice,
    
    Jmp,
    Jt,
    Jf,
    Jtsc,
    Jfsc,
    
    CreateArray,
    CreateRange,
    
    Print,
    Assert,
    
    Extended = 0x8000,
    Dest     = 0x4000,
    Int      = 0x2000,
    Bool     = 0x1000,
    
    Head     = 0xf000
  };
};

#endif