#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include "lexer.h"
#include "token.h"
#include "procedure.h"
#include "ast.h"

#include <memory>

class Parser{
  
  using Lexit_ = std::vector<Lexeme>::const_iterator;
  
  const Lexit_ lbegin_;
  const Lexit_ lend_;
  Lexit_ lcurrent_;
  
  Lexer lexer_;
  
  Token* current_;
  
  const Lexeme& next_();
  const Lexeme& nextNoNewline_();
  bool checkNext_(LexemeType);
  void skipNextNewlines_();
  
  static int bindp_(const Lexeme&);
  
  ASTNode* nud_(const Lexeme&);
  
  ASTNode* new_statement_(int);
  ASTNode* new_expression_(int);
  ASTNode* new_ifExpr_();
  ASTNode* whileExpr_();
  ASTNode* new_funcExpr_();
  ASTNode* new_codeBlock_();
  
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
  Parser(const std::vector<Lexeme>&);
  ~Parser();
  
  Procedure* parse();
};

#endif