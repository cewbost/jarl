#ifndef AST_H_INCLUDED
#define AST_H_INCLUDED

#include "string.h"
#include "context.h"

#include <jarl.h>

#include <memory>

using jarl::Int;
using jarl::Float;

enum class ASTNodeType: uint16_t {
  Leaf = 0x0000,
  
  Nop,
  
  Error = 0x0100,
  
  LexError,
  ParseError,
  
  Value = 0x0200,
  
  Null,
  
  Int,
  Float,
  Bool,
  String,
  Identifier,
  
  OneChild = 0x1000,
  
  CodeBlock,
  Array,
  Table,
  Var,
  Return,
  Recurse,
  Print,
  Assert,
  
  UnaryExpr = 0x1100,
  
  Neg,
  Not,
  Move,
  
  TwoChildren = 0x2000,
  
  Seq,
  Else,
  While,
  For,
  
  Index,
  Call,
  
  ExprList,
  KeyValuePair,
  Then,
  
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
  
  In,
  
  Apply,
  
  BranchExpr = 0x2200,
  
  And,
  Or,
  
  If,

  AssignExpr = 0x2400,
  
  Assign,
  AddAssign,
  SubAssign,
  MulAssign,
  DivAssign,
  ModAssign,
  AppendAssign,
  Insert,

  FunctionNode = 0x3000,

  Function,
};

struct ASTNodeFlags {
  enum {
    None    = 0x0,
    Value   = 0x0001,
    LValue  = 0x0003
  };
};

struct ASTFunctionData;

struct ASTNode {
  
  ASTNodeType type;
  uint16_t flags;
  std::pair<uint16_t, uint16_t> pos;
  union{
    ASTNode* child;
    struct {
      ASTNode *first, *second;
    } children;

    struct {
      ASTFunctionData* data;
      ASTNode* body;
    } function;

    Int int_value;
    Float float_value;
    String* string_value;
    bool bool_value;
    const char* c_str_value;
  };
  
  std::unique_ptr<Context::AttributeSet> context;
  
  ASTNode(ASTNodeType, std::pair<uint16_t, uint16_t>);
  ASTNode(ASTNodeType, ASTNode*, std::pair<uint16_t, uint16_t>);
  ASTNode(ASTNodeType, ASTNode*, ASTNode*, std::pair<uint16_t, uint16_t>);
  ASTNode(ASTNodeType, ASTFunctionData*, ASTNode*, std::pair<uint16_t, uint16_t>);
  
  ASTNode(ASTNodeType, Int, std::pair<uint16_t, uint16_t>);
  ASTNode(ASTNodeType, Float, std::pair<uint16_t, uint16_t>);
  ASTNode(ASTNodeType, String*, std::pair<uint16_t, uint16_t>);
  ASTNode(ASTNodeType, bool, std::pair<uint16_t, uint16_t>);
  ASTNode(ASTNodeType, const char*, std::pair<uint16_t, uint16_t>);
  
  ~ASTNode();
  
  ASTNode(const ASTNode&) = delete;
  ASTNode& operator=(const ASTNode&) = delete;
  
  bool isValue(){
    return (this->flags & ASTNodeFlags::Value) == ASTNodeFlags::Value;
  }
  bool isLValue(){
    return (this->flags & ASTNodeFlags::LValue) == ASTNodeFlags::LValue;
  }
  
  //iteration
  struct ExprListIterator {
    ASTNode* current_;
    
    ExprListIterator(ASTNode*);
    bool operator==(const ExprListIterator&);
    bool operator!=(const ExprListIterator&);
    bool operator==(nullptr_t);
    bool operator!=(nullptr_t);
    ASTNode* get();
    ASTNode* operator*();
    ASTNode* operator->();
    ExprListIterator operator++();
    ExprListIterator operator++(int);
  };
  
  ExprListIterator exprListIterator(){
    return ExprListIterator(this);
  }
  ExprListIterator exprListIteratorEnd(){
    return ExprListIterator(nullptr);
  }
  
  #ifndef NDEBUG
  std::string toStrDebug() const;
  std::string toStrDebug(int) const;
  #endif
};

struct ASTFunctionData {
  std::unique_ptr<ASTNode> arguments;
  std::unique_ptr<ASTNode> captures;

  ASTFunctionData(ASTNode* arguments);
};

#endif
