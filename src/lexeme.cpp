#include "lexeme.h"

namespace {
  
  #ifndef NDEBUG
  inline std::string unquoteString_(const char* str){
    std::string ret(str);
    
    auto replace = [](std::string* s, std::string f, std::string r)->void{
      std::remove_const<decltype(std::string::npos)>::type stepper = 0;
      stepper = s->find(f, stepper);
      while(stepper != std::string::npos){
        s->replace(stepper, f.size(), r);
        stepper += r.size() - f.size();
        stepper = s->find(f, stepper);
      }
    };
    
    replace(&ret, "\\", "\\\\");
    replace(&ret, "\n", "\\n");
    replace(&ret, "\t", "\\t");
    
    return ret;
  }
  #endif
}

Lexeme::Lexeme(LexemeType t, std::pair<uint16_t, uint16_t> pos){
  this->type = t;
  this->pos = pos;
}
Lexeme::Lexeme(Int i, std::pair<uint16_t, uint16_t> pos){
  this->type = LexemeType::Int;
  this->value.i = i;
  this->pos = pos;
}
Lexeme::Lexeme(unsigned long long i, std::pair<uint16_t, uint16_t> pos){
  this->type = LexemeType::Int;
  this->value.i = i;
  this->pos = pos;
}
Lexeme::Lexeme(Float f, std::pair<uint16_t, uint16_t> pos){
  this->type = LexemeType::Float;
  this->value.f = f;
  this->pos = pos;
}
Lexeme::Lexeme(bool b, std::pair<uint16_t, uint16_t> pos){
  this->type = LexemeType::Bool;
  this->value.b = b;
  this->pos = pos;
}
Lexeme::Lexeme(LexemeType t, String* s, std::pair<uint16_t, uint16_t> pos){
  this->type = t;
  this->value.s = s;
  this->pos = pos;
}
Lexeme::Lexeme(String* s, std::pair<uint16_t, uint16_t> pos){
  this->type = LexemeType::String;
  this->value.s = s;
  this->pos = pos;
}

#ifndef NDEBUG
std::string Lexeme::toStrDebug() const {
  using namespace std::string_literals;
  
  std::string ret
    = std::to_string(this->pos.first)
    + ":"s
    + std::to_string(this->pos.second)
    + " \t";
  
  switch(this->type){
  case LexemeType::Bool:
    ret += "bool: "s + (this->value.b? "true"s : "false"s); break;
  case LexemeType::Int:
    ret += "int: "s + std::to_string(this->value.i); break;
  case LexemeType::Float:
    ret += "float: "s + std::to_string(this->value.f); break;
  case LexemeType::String:
    ret += "string: \""s + unquoteString_(this->value.s->str()) + "\""s; break;
  case LexemeType::Identifier:
    ret += "identifier: "s + std::string(this->value.s->str()); break;
  
  case LexemeType::Plus: ret += "+"s; break;
  case LexemeType::Minus: ret += "-"s; break;
  case LexemeType::Mul: ret += "*"s; break;
  case LexemeType::Div: ret += "/"s; break;
  case LexemeType::Mod: ret += "%"s; break;
  case LexemeType::Append: ret += "++"s; break;
  case LexemeType::Assign: ret += "="s; break;
  case LexemeType::AddAssign: ret += "+="s; break;
  case LexemeType::SubAssign: ret += "-="s; break;
  case LexemeType::MulAssign: ret += "*="s; break;
  case LexemeType::DivAssign: ret += "/="s; break;
  case LexemeType::ModAssign: ret += "%="s; break;
  case LexemeType::AppendAssign: ret += "++="s; break;
  case LexemeType::Colon: ret += ":"s; break;
  case LexemeType::Eq: ret += "=="s; break;
  case LexemeType::Neq: ret += "!="s; break;
  case LexemeType::Gt: ret += ">"s; break;
  case LexemeType::Lt: ret += "<"s; break;
  case LexemeType::Geq: ret += ">="s; break;
  case LexemeType::Leq: ret += "<="s; break;
  case LexemeType::Cmp: ret += "<=>"s; break;
  case LexemeType::LParen: ret += "("s; break;
  case LexemeType::RParen: ret += ")"s; break;
  case LexemeType::LBrace: ret += "{"s; break;
  case LexemeType::RBrace: ret += "}"s; break;
  case LexemeType::LBracket: ret += "["s; break;
  case LexemeType::RBracket: ret += "]"s; break;
  case LexemeType::Comma: ret += ","s; break;
  case LexemeType::Semicolon: ret += ";"s; break;
  case LexemeType::Newline: ret += "\\n"; break;
  case LexemeType::Do: ret += ":"s; break;
  
  case LexemeType::Not: ret += "not"s; break;
  case LexemeType::And: ret += "and"s; break;
  case LexemeType::Or: ret += "or"s; break;
  
  case LexemeType::In: ret += "in"s; break;
  
  case LexemeType::Null: ret += "null"s; break;
  case LexemeType::If: ret += "if"s; break;
  case LexemeType::For: ret += "for"s; break;
  case LexemeType::Else: ret += "else"s; break;
  case LexemeType::Func: ret += "func"s; break;
  case LexemeType::Var: ret += "var"s; break;
  case LexemeType::Print: ret += "print"s; break;
  case LexemeType::Assert: ret += "assert"s; break;
  
  case LexemeType::End: ret += "end"s; break;
  
  case LexemeType::Error: ret += "error"s; break;
  
  default: ret += "unrecognized lexeme"s;
  }
  
  return ret;
}
#endif