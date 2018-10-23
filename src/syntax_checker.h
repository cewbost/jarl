#ifndef SYNTAX_CHECKER_H_INCLUDED
#define SYNTAX_CHECKER_H_INCLUDED

#include "ast.h"

class SyntaxChecker {
  
  std::vector<std::unique_ptr<char[]>>* errors_;
  
  void expectedValueError_(ASTNode*);
  void expectedRValueError_(ASTNode*);
  void expectedKeyValuePairError_(ASTNode*);
  
public:
  void validateSyntax(ASTNode*);
};

#endif