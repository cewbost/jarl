#ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

#include "value.h"
#include "string_view.h"
#include "rc_mixin.h"
#include "lexeme.h"

#include <unordered_map>
#include <vector>

class Lexer{
  
  const char* mem_;
  
public:
  std::unordered_map<StringView, rc_ptr<String>> string_table;
  
  Lexer(const char*);
  
  std::vector<Lexeme> lex(std::vector<std::unique_ptr<char[]>>*);
};

#endif