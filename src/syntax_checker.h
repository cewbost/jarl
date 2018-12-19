#ifndef SYNTAX_CHECKER_H_INCLUDED
#define SYNTAX_CHECKER_H_INCLUDED

#include "ast.h"

class SyntaxChecker {
  
  std::vector<std::unique_ptr<char[]>>* errors_;
  
  void expectValue_(ASTNode*);
  void expectValueNop_(ASTNode*);
  void expectLValue_(ASTNode*);
  void expectIdentifier_(ASTNode*);
  void expectValueList_(ASTNode*);
  
  void expectedValueError_(ASTNode*);
  void expectedKeyValuePairError_(ASTNode*);
  
public:
  void validateSyntax(ASTNode*);
  
  SyntaxChecker(std::vector<std::unique_ptr<char[]>>* errors)
  : errors_(errors){}
};

#endif