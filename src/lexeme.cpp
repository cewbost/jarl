#include "lexeme.h"

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