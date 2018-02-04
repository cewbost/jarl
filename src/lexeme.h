#ifndef LEXEME_H_INCLUDED
#define LEXEME_H_INCLUDED

#include "string.h"

#include <jarl.h>

#include <cstdint>

using jarl::Int;
using jarl::Float;

enum class LexemeType: unsigned {
  End = 0,
  
  L10 = 0x10,
  Newline,
  Semicolon,
  RParen,
  RBrace,
  RBracket,
  Var,
  Else,
  Print,
  
  L20 = 0x20,
  Comma,
  
  L30 = 0x30,
  Assign,
  PlusAssign,
  MinusAssign,
  MulAssign,
  DivAssign,
  ModAssign,
  AppendAssign,
  
  L40 = 0x40,
  Colon,
  
  L50 = 0x50,
  Not,
  And,
  Or,
  
  L60 = 0x60,
  Eq,
  Neq,
  Gt,
  Lt,
  Geq,
  Leq,
  Cmp,
  
  L70 = 0x70,
  Append,
  
  L80 = 0x80,
  Plus,
  Minus,
  
  L90 = 0x90,
  Mul,
  Div,
  Mod,
  
  LA0 = 0xa0,
  LParen,
  LBrace,
  LBracket,
  
  LB0 = 0xb0,
  Int,
  Float,
  Bool,
  Identifier,
  String,
  Null,
  If,
  While,
  Func
};

struct Lexeme {
  
  LexemeType type;
  std::pair<unsigned, unsigned> pos;
  union {
    Int i;
    Float f;
    bool b;
    String* s;
  } value;
  
  Lexeme(LexemeType);
  Lexeme(Int);
  Lexeme(unsigned long long);
  Lexeme(Float);
  Lexeme(bool);
  Lexeme(LexemeType, String*);
  Lexeme(String*);
  
  #ifndef NDEBUG
  std::string toStr()const;
  #endif
};

#endif