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
  
  RParen,
  RBrace,
  RBracket,
  
  Do,
  
  L20 = 0x20,
  Semicolon,
  Newline,
  
  L30 = 0x30,
  Else,
  Var,
  Return,
  Print,
  Assert,
  
  RightAssocClass = 0x50,
  Comma,
  
  L60 = 0x60,
  Assign,
  AddAssign,
  SubAssign,
  MulAssign,
  DivAssign,
  ModAssign,
  AppendAssign,
  
  Insert,
  
  Colon,
  
  LeftAssocClass = 0x70,
  And,
  Or,
  
  L80 = 0x80,
  Eq,
  Neq,
  Gt,
  Lt,
  Geq,
  Leq,
  In,
  
  L90 = 0x90,
  Append,
  
  LA0 = 0xA0,
  
  Cmp,
  
  LB0 = 0xB0,
  Plus,
  Minus,
  
  LC0 = 0xC0,
  Mul,
  Div,
  Mod,
  
  ExprClass = 0xD0,
  LParen,
  LBrace,
  LBracket,
  
  LE0 = 0xE0,
  Not,
  Int,
  Float,
  Bool,
  Identifier,
  String,
  Null,
  If,
  While,
  For,
  Func,
  
  LF0 = 0xF0,
  
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

inline bool isStopLexeme(const Lexeme& lex){
  switch(lex.type){
  case LexemeType::RParen:
  case LexemeType::RBrace:
  case LexemeType::RBracket:
  case LexemeType::Int:
  case LexemeType::Float:
  case LexemeType::Bool:
  case LexemeType::Identifier:
  case LexemeType::String:
  case LexemeType::Null:
  case LexemeType::Print:
  case LexemeType::Assert:
  case LexemeType::Return:
    return true;
  default:
    return false;
  }
}

#endif