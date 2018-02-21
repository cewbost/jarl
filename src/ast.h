#ifndef AST_H_INCLUDED
#define AST_H_INCLUDED

#include "string.h"

#include <jarl.h>

#include <memory>

using jarl::Int;
using jarl::Float;

enum class ASTNodeType: unsigned {
  Leaf = 0x0,
  
  Nop,
  Null,
  
  Int,
  Float,
  Bool,
  String,
  Identifier,
  
  OneChild = 0x100,
  
  Neg,
  Not,
  
  CodeBlock,
  Array,
  
  TwoChildren = 0x200,
  
  And,
  Or,
  
  Assign,
  AddAssign,
  SubAssign,
  MulAssign,
  DivAssign,
  ModAssign,
  AppendAssign,
  
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
  
  Seq,
  Conditional,
  Branch,
  While,
  
  Index,
  
  ExprList,
  Range,
  Function,
  
  StringList = 0x300,
  
  IdentifierList
};

struct ASTNode {
  
  ASTNodeType type;
  union{
    ASTNode* children[2];
    Int int_value;
    Float float_value;
    String* string_value;
    bool bool_value;
    struct {
      ASTNode* next;
      String* value;
    } string_list;
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
  std::string toStr() const;
  std::string toStr(int) const;
  #endif
};

#endif