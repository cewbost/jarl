#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include "lexer.h"
#include "ast.h"

#include <memory>

class Parser{
  
  using Lexit_ = std::vector<Lexeme>::const_iterator;
  
  const Lexit_ lbegin_;
  const Lexit_ lend_;
  Lexit_ lcurrent_;
  
  std::vector<std::unique_ptr<char[]>>* errors_;
  
  const Lexeme& next_();
  bool checkNext_(LexemeType);
  
  static int bindp_(LexemeType);
  
  ASTNode* nud_(const Lexeme&);
  ASTNode* led_(const Lexeme&, ASTNode*);
  
  ASTNode* expression_(int);
  ASTNode* ifExpr_();
  ASTNode* whileExpr_();
  ASTNode* functionExpr_();
  
public:
  
  Parser(const std::vector<Lexeme>&);
  
  ASTNode* parse(std::vector<std::unique_ptr<char[]>>*);
};

#endif