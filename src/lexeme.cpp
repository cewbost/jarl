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

Lexeme::Lexeme(LexemeType t){
  this->type = t;
}
Lexeme::Lexeme(Int i){
  this->type = LexemeType::Int;
  this->value.i = i;
}
Lexeme::Lexeme(unsigned long long i){
  this->type = LexemeType::Int;
  this->value.i = i;
}
Lexeme::Lexeme(Float f){
  this->type = LexemeType::Float;
  this->value.f = f;
}
Lexeme::Lexeme(bool b){
  this->type = LexemeType::Bool;
  this->value.b = b;
}
Lexeme::Lexeme(LexemeType t, String* s){
  this->type = t;
  this->value.s = s;
}
Lexeme::Lexeme(String* s){
  this->type = LexemeType::String;
  this->value.s = s;
}

#ifndef NDEBUG
std::string Lexeme::toStr() const {
  using namespace std::string_literals;
  
  switch(this->type){
  case LexemeType::Bool:
    return "bool: "s + (this->value.b? "true"s : "false"s);
  case LexemeType::Int:
    return "int: "s + std::to_string(this->value.i);
  case LexemeType::Float:
    return "float: "s + std::to_string(this->value.f);
  case LexemeType::String:
    return "string: \""s + unquoteString_(this->value.s->str()) + "\""s;
  case LexemeType::Identifier:
    return "identifier: "s + std::string(this->value.s->str());
  case LexemeType::Newline:
    return "newline"s;
  
  case LexemeType::Plus: return "+"s;
  case LexemeType::Minus: return "-"s;
  case LexemeType::Mul: return "*"s;
  case LexemeType::Div: return "/"s;
  case LexemeType::Mod: return "%"s;
  case LexemeType::Append: return "++"s;
  case LexemeType::Assign: return "="s;
  case LexemeType::PlusAssign: return "+="s;
  case LexemeType::MinusAssign: return "-="s;
  case LexemeType::MulAssign: return "*="s;
  case LexemeType::DivAssign: return "/="s;
  case LexemeType::ModAssign: return "%="s;
  case LexemeType::AppendAssign: return "++="s;
  case LexemeType::Eq: return "=="s;
  case LexemeType::Neq: return "!="s;
  case LexemeType::Gt: return ">"s;
  case LexemeType::Lt: return "<"s;
  case LexemeType::Geq: return ">="s;
  case LexemeType::Leq: return "<="s;
  case LexemeType::Cmp: return "<=>"s;
  case LexemeType::LParen: return "("s;
  case LexemeType::RParen: return ")"s;
  case LexemeType::LBrace: return "{"s;
  case LexemeType::RBrace: return "}"s;
  case LexemeType::LBracket: return "["s;
  case LexemeType::RBracket: return "]"s;
  case LexemeType::Comma: return ","s;
  case LexemeType::Semicolon: return ";"s;
  case LexemeType::Colon: return ":"s;
  
  case LexemeType::Not: return "not"s;
  case LexemeType::And: return "and"s;
  case LexemeType::Or: return "or"s;
  
  case LexemeType::Var: return "var"s;
  case LexemeType::Null: return "null"s;
  case LexemeType::If: return "if"s;
  case LexemeType::Else: return "else"s;
  case LexemeType::While: return "while"s;
  case LexemeType::Func: return "func"s;
  case LexemeType::Print: return "print"s;
  
  case LexemeType::End: return "end"s;
  
  default: return "unrecognized lexeme"s;
  }
}
#endif