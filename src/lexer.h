#ifndef LEXER_H_INCLUDED
#define LEXER_H_INCLUDED

#include "value.h"
#include "string_view.h"
#include "rc_trait.h"
#include "token.h"

#include <unordered_map>
#include <vector>

class Lexer{
  
  const char* mem_;
  const char* old_reader_;
  const char* reader_;
  std::vector<const char*> _checkpoints;
  
public:
  std::unordered_map<StringView, rc_ptr<String>> string_table;
  
  Lexer(const char*);
  
  void setCheckpoint();
  void removeCheckpoint();
  void revert();
  
  Token* next();
};

#endif