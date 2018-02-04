#ifndef AST_H_INCLUDED
#define AST_H_INCLUDED

#include "string.h"

#include <jarl.h>

#include <memory>

using jarl::Int;
using jarl::Float;

enum class ASTNodeType: unsigned {
  Nop,
  
  Null,
  Int,
  Float,
  Bool,
  String,
  Identifier,
  
  Neg,
  Not,
  
  Array,
  Range
};

struct ASTNode {
  
  ASTNodeType type;
  union{
    ASTNode* children[2];
    Int int_value;
    Float float_value;
    String* string_value;
    bool bool_value;
  };
  
  ASTNode(ASTNodeType);
  ASTNode(ASTNodeType, ASTNode*);
  ASTNode(ASTNodeType, ASTNode*, ASTNode*);
  
  ASTNode(ASTNodeType, Int);
  ASTNode(ASTNodeType, Float);
  ASTNode(ASTNodeType, String*);
  ASTNode(ASTNodeType, bool);
  
  ~ASTNode();
  
  ASTNode(const ASTNode&) = delete;
  ASTNode& operator=(const ASTNode&) = delete;
};

#endif