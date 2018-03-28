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
  
  Colon,
  
  L20 = 0x20,
  Semicolon,
  Newline,
  
  L30 = 0x30,
  Else,
  Print,
  
  L50 = 0x50,
  Comma,
  
  RightAssocClass = 0x60,
  Define,
  Assign,
  AddAssign,
  SubAssign,
  MulAssign,
  DivAssign,
  ModAssign,
  AppendAssign,
  
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
  Cmp,
  
  L90 = 0x90,
  Append,
  
  LA0 = 0xA0,
  Plus,
  Minus,
  
  LB0 = 0xB0,
  Mul,
  Div,
  Mod,
  
  ExprClass = 0xC0,
  LParen,
  LBrace,
  LBracket,
  
  LD0 = 0xD0,
  Not,
  Int,
  Float,
  Bool,
  Identifier,
  String,
  Null,
  If,
  For,
  Func,
  
  LE0 = 0xE0,
  
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
  case LexemeType::LParen:
  case LexemeType::LBrace:
  case LexemeType::LBracket:
  case LexemeType::Int:
  case LexemeType::Float:
  case LexemeType::Bool:
  case LexemeType::Identifier:
  case LexemeType::String:
  case LexemeType::Null:
    return true;
  default:
    return false;
  }
}

#endif