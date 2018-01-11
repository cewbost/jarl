#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include "lexer.h"
#include "token.h"
#include "procedure.h"

#include <memory>

class Parser{
  
  Lexer lexer_;
  
  Token* current_;
  
  Token* advanceToken_();
  Token* advanceTokenNoNewline_();
  bool checkNextToken_(TokenType);
  void skipNewlines_();
  
  Token* identifierList_();
  Token* statement_(int);
  Token* expression_(int);
  Token* codeBlock_();
  Token* varDecl_();
  Token* printExpr_();
  Token* ifExpr_();
  Token* forExpr_();
  Token* funcExpr_();
  
  static int bindp_(TokenType);
  Token* nud_(Token*);
  Token* led_(Token*, Token*);
  
public:
  
  Parser(const char*);
  ~Parser();
  
  Procedure* parse();
};

#endif