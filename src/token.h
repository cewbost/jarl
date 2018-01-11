#ifndef TOKEN_H_INCLUDED
#define TOKEN_H_INCLUDED

#include "value.h"

#include <jarl.h>

#include <utility>
#include <string>

#ifndef NDEBUG
#include "alloc_monitor.h"
#endif

using jarl::Int;
using jarl::Float;

typedef unsigned short TokenTypeType;

constexpr TokenTypeType ExprNodeFlag      = 0x8000;
constexpr TokenTypeType UnaryExprFlag     = 0x4000;
constexpr TokenTypeType BinaryExprFlag    = 0x2000;
constexpr TokenTypeType AssignExprFlag    = 0x1000;
constexpr TokenTypeType BooleanExprFlag   = 0x0800;
constexpr TokenTypeType MiscExprFlag      = 0x0400;

constexpr TokenTypeType StatementNodeFlag = 0x4000;

constexpr TokenTypeType ValueTokenFlag    = 0x2000;

constexpr TokenTypeType OperatorTokenFlag = 0x1000;
constexpr TokenTypeType RightAssocFlag    = 0x0800;

constexpr TokenTypeType KeywordTokenFlag  = 0x0800;

enum class TokenType: TokenTypeType{
  
  EndToken = 0,
  
  NewlineToken,
  SemicolonToken,
  
  ValueToken = ValueTokenFlag,
  
  NullToken,
  BoolToken,
  IntToken,
  FloatToken,
  StringToken,
  IdentifierToken,
  ParamToken,
  
  LParenToken,
  RParenToken,
  LBraceToken,
  RBraceToken,
  LBracketToken,
  RBracketToken,
  
  LeftAssocOperator = OperatorTokenFlag,
  
  ApplyToken,
  PlusToken,
  MinusToken,
  MulToken,
  DivToken,
  ModToken,
  AppendToken,
  
  NotToken,
  AndToken,
  OrToken,
  
  CmpToken,
  EqToken,
  NeqToken,
  GtToken,
  LtToken,
  GeqToken,
  LeqToken,
  
  RightAssocOperator = OperatorTokenFlag | RightAssocFlag,
  
  ColonToken,
  CommaToken,
  
  InsertToken,
  AssignToken,
  PlusAssignToken,
  MinusAssignToken,
  MulAssignToken,
  DivAssignToken,
  ModAssignToken,
  AppendAssignToken,
  
  KeywordToken = KeywordTokenFlag,
  
  VarToken,
  IfToken,
  ElseToken,
  WhileToken,
  FuncToken,
  PrintToken,
  
  UnaryExprNode = ExprNodeFlag | UnaryExprFlag,
  
  NegExpr,
  NotExpr,
  
  BinaryExprNode = ExprNodeFlag | BinaryExprFlag,
  
  RangeExpr,
  
  ApplyExpr,
  
  AddExpr,
  SubExpr,
  MulExpr,
  DivExpr,
  ModExpr,
  AppendExpr,
  
  CmpExpr,
  EqExpr,
  NeqExpr,
  GtExpr,
  LtExpr,
  GeqExpr,
  LeqExpr,
  
  BooleanExprNode = ExprNodeFlag | BinaryExprFlag | BooleanExprFlag,
  
  AndExpr,
  OrExpr,
  ConditionalExpr,
  ConditionalNode,
  
  AssignExprNode = ExprNodeFlag | BinaryExprFlag | AssignExprFlag,
  
  InsertExpr,
  AssignExpr,
  AddAssignExpr,
  SubAssignExpr,
  MulAssignExpr,
  DivAssignExpr,
  ModAssignExpr,
  AppendAssignExpr,
  
  MiscUnaryExprNode = ExprNodeFlag | MiscExprFlag | UnaryExprFlag,
  
  CodeBlock,
  ArrayExpr,
  
  MiscBinaryExprNode = ExprNodeFlag | MiscExprFlag | BinaryExprFlag,
  
  LoopNode,
  FuncNode,
  SeqNode,
  VarDecl,
  IdentifierList,
  ExprList,
  IndexExpr,
  PrintExpr,
  
  MiscEndNode = ExprNodeFlag | MiscExprFlag,
  
  NopNode
};

constexpr TokenType tokenMajorType(TokenType t){
  return TokenType((int)t & 0xfc00);
}
constexpr bool tokenFlag(TokenType t, TokenTypeType flag){
  return static_cast<TokenTypeType>(t) & flag;
}

#ifndef NDEBUG
struct Token;
extern AllocMonitor<Token> token_alloc_monitor;
#endif

struct Token
#ifndef NDEBUG
: MonitoredTrait<Token, token_alloc_monitor>
#endif
{
  
  void* operator new(size_t);
  void operator delete(void*);
  
  const TokenType type;
  union{
    bool bool_value;
    Int int_value;
    Float float_value;
    String* string_value;
    Token* child;
    std::pair<Token*, Token*> childs;
  };
  
  Token(TokenType);
  Token(TokenType, Token*);
  Token(TokenType, Token*, Token*);
  Token(bool);
  Token(Int);
  Token(unsigned long long);
  Token(Float);
  Token(String*);
  Token(TokenType, String*);
  Token(TokenType, unsigned long long);
  
  ~Token();
  
  Token(const Token&) = delete;
  Token(Token&&) = delete;
  void operator=(const Token&) = delete;
  void operator=(Token&&) = delete;
  
  #ifndef NDEBUG
  std::string toStr()const;
  #endif
};

#endif