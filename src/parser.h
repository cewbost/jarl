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
  
  const Lexeme& next_();
  const Lexeme& nextNoNewline_();
  bool checkNext_(LexemeType);
  void skipNewlines_();
  
  static int bindp_(LexemeType);
  
  ASTNode* nud_(const Lexeme&);
  ASTNode* led_(const Lexeme&, ASTNode*);
  
  ASTNode* statement_(int);
  ASTNode* expression_(int);
  ASTNode* ifExpr_();
  ASTNode* whileExpr_();
  ASTNode* identifierList_();
  ASTNode* functionExpr_();
  ASTNode* varDecl_();
  ASTNode* printExpr_();
  ASTNode* codeBlock_();
  
public:
  
  Parser(const std::vector<Lexeme>&);
  
  Procedure* parse();
};

#endif