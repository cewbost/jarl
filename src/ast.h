#ifndef AST_H_INCLUDED
#define AST_H_INCLUDED

#include "string.h"

#include <jarl.h>

#include <memory>

using jarl::Int;
using jarl::Float;

enum class ASTNodeType: unsigned {
  Leaf = 0x0000,
  
  Nop,
  
  Value = 0x0100,
  
  Null,
  
  Int,
  Float,
  Bool,
  String,
  Identifier,
  
  OneChild = 0x1000,
  
  CodeBlock,
  Array,
  Print,
  
  UnaryExpr = 0x1100,
  
  Neg,
  Not,
  
  TwoChildren = 0x2000,
  
  Seq,
  Branch,
  While,
  
  Index,
  
  ExprList,
  Range,
  Function,
  
  BinaryExpr = 0x2100,
  
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Append,
  
  Cmp,
  Eq,
  Neq,
  Gt,
  Lt,
  Geq,
  Leq,
  
  Apply,
  
  BranchExpr = 0x2200,
  
  And,
  Or,
  
  Conditional,
  
  AssignExpr = 0x2300,
  
  Assign,
  AddAssign,
  SubAssign,
  MulAssign,
  DivAssign,
  ModAssign,
  AppendAssign,
  
  StringBranch = 0x3000,
  
  IdentifierList,
  VarDecl
};

struct ASTNode {
  
  ASTNodeType type;
  union{
    ASTNode* child;
    struct {
      ASTNode *first, *second;
    } children;
    Int int_value;
    Float float_value;
    String* string_value;
    bool bool_value;
    struct {
      ASTNode* next;
      String* value;
    } string_branch;
  };
  
  ASTNode(ASTNodeType);
  ASTNode(ASTNodeType, ASTNode*);
  ASTNode(ASTNodeType, ASTNode*, ASTNode*);
  ASTNode(ASTNodeType, String*, ASTNode*);
  
  ASTNode(ASTNodeType, Int);
  ASTNode(ASTNodeType, Float);
  ASTNode(ASTNodeType, String*);
  ASTNode(ASTNodeType, bool);
  
  ~ASTNode();
  
  ASTNode(const ASTNode&) = delete;
  ASTNode& operator=(const ASTNode&) = delete;
  
  #ifndef NDEBUG
  std::string toStrDebug() const;
  std::string toStrDebug(int) const;
  #endif
};

#endif