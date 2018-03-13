#ifndef LEXEME_H_INCLUDED
#define LEXEME_H_INCLUDED

#include "string.h"

#include <jarl.h>

#include <cstdint>

using jarl::Int;
using jarl::Float;

enum class LexemeType: unsigned {
  End = 0,
  
  StopSymbolClass = 0x10,
  Newline,
  RParen,
  RBrace,
  RBracket,
  
  Colon,
  Else,
  
  L20 = 0x20,
  Semicolon,
  
  L30 = 0x30,
  Var,
  Print,
  While,
  
  L40 = 0x40,
  Comma,
  
  RightAssocClass = 0x50,
  Assign,
  AddAssign,
  SubAssign,
  MulAssign,
  DivAssign,
  ModAssign,
  AppendAssign,
  
  LeftAssocClass = 0x60,
  And,
  Or,
  
  L70 = 0x70,
  Eq,
  Neq,
  Gt,
  Lt,
  Geq,
  Leq,
  Cmp,
  
  L80 = 0x80,
  Append,
  
  L90 = 0x90,
  Plus,
  Minus,
  
  LA0 = 0xA0,
  Mul,
  Div,
  Mod,
  
  ExprClass = 0xB0,
  LParen,
  LBrace,
  LBracket,
  
  LC0 = 0xC0,
  Not,
  Int,
  Float,
  Bool,
  Identifier,
  String,
  Null,
  If,
  Func,
  
  LD0 = 0xD0,
  
  Error
};

struct Lexeme {
  
  LexemeType type;
  std::pair<uint16_t, uint16_t> pos;
  union {
    Int i;
    Float f;
    bool b;
    String* s;
  } value;
  
  
  
  Lexeme(LexemeType, std::pair<uint16_t, uint16_t>);
  Lexeme(Int, std::pair<uint16_t, uint16_t>);
  Lexeme(unsigned long long, std::pair<uint16_t, uint16_t>);
  Lexeme(Float, std::pair<uint16_t, uint16_t>);
  Lexeme(bool, std::pair<uint16_t, uint16_t>);
  Lexeme(LexemeType, String*, std::pair<uint16_t, uint16_t>);
  Lexeme(String*, std::pair<uint16_t, uint16_t>);
  
  #ifndef NDEBUG
  std::string toStrDebug() const;
  #endif
};

#endif