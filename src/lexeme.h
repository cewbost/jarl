#ifndef LEXEME_H_INCLUDED
#define LEXEME_H_INCLUDED

#include "string.h"

#include <jarl.h>

#include <cstdint>

using jarl::Int;
using jarl::Float;

enum class LexemeType: unsigned {
  End = 0,
  Newline,
  Int,
  Float,
  Bool,
  Identifier,
  String,
  
  Plus,
  Minus,
  Mul,
  Div,
  Mod,
  Append,
  Assign,
  PlusAssign,
  MinusAssign,
  MulAssign,
  DivAssign,
  ModAssign,
  AppendAssign,
  Eq,
  Neq,
  Gt,
  Lt,
  Geq,
  Leq,
  Cmp,
  LParen,
  RParen,
  LBrace,
  RBrace,
  LBracket,
  RBracket,
  Comma,
  Semicolon,
  Colon,
  
  Var,
  Null,
  Not,
  And,
  Or,
  If,
  Else,
  While,
  Func,
  Print
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