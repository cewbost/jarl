#ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

#include "value.h"
#include "string_view.h"
#include "rc_trait.h"
#include "lexeme.h"

#include <unordered_map>
#include <vector>

class Lexer{
  
  const char* mem_;
  
public:
  std::unordered_map<StringView, rc_ptr<String>> string_table;
  
  Lexer(const char*);
  
  void setCheckpoint();
  void removeCheckpoint();
  void revert();
  
  std::vector<Lexeme> lex();
};

#endif