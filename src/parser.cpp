#include "parser.h"

#include "procedure.h"

#include <cassert>

#ifndef NDEBUG
#include <iostream>
#endif

constexpr int def_expr_bindp  = 15;
constexpr int func_bindp      = 45;

const Lexeme& Parser::next_(){
  const Lexeme& next = *this->lcurrent_;
  ++this->lcurrent_;
  return next;
}
const Lexeme& Parser::nextNoNewline_(){
  while(this->lcurrent_->type == LexemeType::Newline){
    ++this->lcurrent_;
  }
  const Lexeme& next = *this->lcurrent_;
  ++this->lcurrent_;
  return next;
}
bool Parser::checkNext_(LexemeType type){
  if(this->lcurrent_->type == type){
    ++this->lcurrent_;
    return true;
  }else{
    return false;
  }
}
void Parser::skipNewlines_(){
  while(this->lcurrent_->type == LexemeType::Newline){
    ++this->lcurrent_;
  }
}

int Parser::bindp_(LexemeType type){
  return static_cast<unsigned>(type) & ~0xf;
}

ASTNode* Parser::nud_(const Lexeme& lex){
  switch(lex.type){
  case LexemeType::Plus:
    return this->expression_(0x81);
  case LexemeType::Minus:
    return new ASTNode(ASTNodeType::Neg, this->expression_(0x81));
  case LexemeType::Not:
    return new ASTNode(ASTNodeType::Not, this->statement_(0x51));
  case LexemeType::Colon:
    return new ASTNode(
      ASTNodeType::Range,
      new ASTNode(ASTNodeType::Nop),
      this->expression_(0x31)
    );
  
  case LexemeType::If:
    return this->ifExpr_();
  case LexemeType::While:
    return this->whileExpr_();
  
  case LexemeType::Func:
    return this->functionExpr_();
  
  case LexemeType::LParen:
    {
      auto tok = this->expression_(def_expr_bindp);
      if(!this->checkNext_(LexemeType::RParen)){
        assert(false);
      }
      return tok;
    }
  case LexemeType::LBrace:
    {
      auto tok = this->codeBlock_();
      if(!this->checkNext_(LexemeType::RBrace)){
        assert(false);
      }
      return tok;
    }
  case LexemeType::LBracket:
    {
      auto tok = this->expression_(0x11);
      tok = new ASTNode(ASTNodeType::Array, tok);
      if(!this->checkNext_(LexemeType::RBracket)){
        assert(false);
      }
      return tok;
    }
  
  case LexemeType::Int:
    return new ASTNode(ASTNodeType::Int, lex.value.i);
  case LexemeType::Float:
    return new ASTNode(ASTNodeType::Float, lex.value.f);
  case LexemeType::Bool:
    return new ASTNode(ASTNodeType::Bool, lex.value.b);
  case LexemeType::Null:
    return new ASTNode(ASTNodeType::Null);
  case LexemeType::String:
    return new ASTNode(ASTNodeType::String, lex.value.s);
  case LexemeType::Identifier:
    return new ASTNode(ASTNodeType::Identifier, lex.value.s);
  
  default:
    assert(false);
  }
}

ASTNode* Parser::led_(const Lexeme& lex, ASTNode* left){
  auto typen = this->bindp_(lex.type);
  if(typen >= static_cast<unsigned>(LexemeType::ExprClass)){
    if(lex.type == LexemeType::LBracket){
      auto right = this->expression_(def_expr_bindp);
      if(!this->checkNext_(LexemeType::RBracket)){
        assert(false);
      }
      return new ASTNode(ASTNodeType::Index, left, right);
    }else{
      return new ASTNode(ASTNodeType::Apply, left, this->nud_(lex));
    }
  }else if(typen >= static_cast<unsigned>(LexemeType::LeftAssocClass)){
    auto right = this->expression_(typen);
    switch(lex.type){
    case LexemeType::Plus:
      return new ASTNode(ASTNodeType::Add, left, right);
    case LexemeType::Minus:
      return new ASTNode(ASTNodeType::Sub, left, right);
    case LexemeType::Mul:
      return new ASTNode(ASTNodeType::Mul, left, right);
    case LexemeType::Div:
      return new ASTNode(ASTNodeType::Div, left, right);
    case LexemeType::Mod:
      return new ASTNode(ASTNodeType::Mod, left, right);
    case LexemeType::Append:
      return new ASTNode(ASTNodeType::Append, left, right);
    
    case LexemeType::Cmp:
      return new ASTNode(ASTNodeType::Cmp, left, right);
    case LexemeType::Eq:
      return new ASTNode(ASTNodeType::Eq, left, right);
    case LexemeType::Neq:
      return new ASTNode(ASTNodeType::Neq, left, right);
    case LexemeType::Gt:
      return new ASTNode(ASTNodeType::Gt, left, right);
    case LexemeType::Lt:
      return new ASTNode(ASTNodeType::Lt, left, right);
    case LexemeType::Geq:
      return new ASTNode(ASTNodeType::Geq, left, right);
    case LexemeType::Leq:
      return new ASTNode(ASTNodeType::Leq, left, right);
    
    case LexemeType::And:
      return new ASTNode(ASTNodeType::And, left, right);
    case LexemeType::Or:
      return new ASTNode(ASTNodeType::Or, left, right);
    
    default:
      assert(false);
    }
  }else if(typen >= static_cast<unsigned>(LexemeType::RightAssocClass)){
    auto right = this->expression_(typen - 1);
    
    switch(lex.type){
    case LexemeType::Assign:
      return new ASTNode(ASTNodeType::Assign, left, right);
    case LexemeType::AddAssign:
      return new ASTNode(ASTNodeType::AddAssign, left, right);
    case LexemeType::SubAssign:
      return new ASTNode(ASTNodeType::SubAssign, left, right);
    case LexemeType::MulAssign:
      return new ASTNode(ASTNodeType::MulAssign, left, right);
    case LexemeType::DivAssign:
      return new ASTNode(ASTNodeType::DivAssign, left, right);
    case LexemeType::ModAssign:
      return new ASTNode(ASTNodeType::ModAssign, left, right);
    case LexemeType::AppendAssign:
      return new ASTNode(ASTNodeType::AppendAssign, left, right);
    
    case LexemeType::Colon:
      return new ASTNode(ASTNodeType::Range, left, right);
    case LexemeType::Comma:
      return new ASTNode(ASTNodeType::ExprList, left, right);
    
    default:
      assert(false);
    }
  }else assert(false);
  
  return nullptr;
}

ASTNode* Parser::statement_(int bindp){
  if(bindp >= this->bindp_(this->lcurrent_->type)){
    return new ASTNode(ASTNodeType::Nop);
  }
  auto left = this->nud_(this->next_());
  while(bindp < this->bindp_(this->lcurrent_->type)){
    left = this->led_(this->next_(), left);
  }
  return left;
}

ASTNode* Parser::expression_(int bindp){
  this->skipNewlines_();
  if(bindp > this->bindp_(this->lcurrent_->type)){
    return new ASTNode(ASTNodeType::Nop);
  }
  auto left = this->nud_(this->nextNoNewline_());
  while(bindp < this->bindp_(this->lcurrent_->type)){
    left = this->led_(this->nextNoNewline_(), left);
  }
  return left;
}

ASTNode* Parser::ifExpr_(){
  ASTNode* condition = this->statement_(func_bindp);
  if(!this->checkNext_(LexemeType::Colon)){
    assert(false);
  }
  this->skipNewlines_();
  ASTNode* stmt = this->statement_(func_bindp);
  ASTNode* else_stmt;
  
  auto checkpoint = this->lcurrent_;
  this->skipNewlines_();
  if(this->lcurrent_->type == LexemeType::Else){
    ++this->lcurrent_;
    this->skipNewlines_();
    else_stmt = this->statement_(func_bindp);
  }else{
    this->lcurrent_ = checkpoint;
    ++this->lcurrent_;
    else_stmt = nullptr;
  }
  
  return new ASTNode(
    ASTNodeType::Conditional,
    condition,
    new ASTNode(
      ASTNodeType::Branch,
      stmt,
      else_stmt? else_stmt : new ASTNode(ASTNodeType::Nop)
    )
  );
}

ASTNode* Parser::whileExpr_(){
  ASTNode* condition = this->statement_(func_bindp);
  if(!this->checkNext_(LexemeType::Colon)){
    assert(false);
  }
  this->skipNewlines_();
  ASTNode* stmt = this->statement_(func_bindp);
  
  return new ASTNode(ASTNodeType::While, condition, stmt);
}

ASTNode* Parser::identifierList_(){
  ASTNode* node = new ASTNode(ASTNodeType::Nop);
  
  if(this->lcurrent_->type == LexemeType::Identifier){
    node = new ASTNode(
      ASTNodeType::IdentifierList,
      this->next_().value.s,
      node
    );
    ASTNode** next_node = &node->string_list.next;
    
    while(this->lcurrent_->type == LexemeType::Comma){
      ++this->lcurrent_;
      assert(this->lcurrent_->type == LexemeType::Identifier);
      
      *next_node = new ASTNode(
        ASTNodeType::IdentifierList,
        this->next_().value.s,
        *next_node
      );
      next_node = &((*next_node)->string_list.next);
    }
  }
  return node;
}

ASTNode* Parser::functionExpr_(){
  ASTNode* arg_list = this->identifierList_();
  
  if(!this->checkNext_(LexemeType::Colon)){
    assert(false);
  }
  this->skipNewlines_();
  ASTNode* code = this->statement_(func_bindp);
  
  return new ASTNode(ASTNodeType::Function, arg_list, code);
}

ASTNode* Parser::varDecl_(){
  return nullptr;
}

ASTNode* Parser::printExpr_(){
  return nullptr;
}

ASTNode* Parser::codeBlock_(){
  ASTNode* ret = new ASTNode(ASTNodeType::Nop);
  ASTNode** node = &ret;
  
  for(;;){
    switch(this->lcurrent_->type){
    case LexemeType::End:
    case LexemeType::RBrace:
      if(ret->type == ASTNodeType::Seq){
        ASTNode* temp = ret;
        ret = ret->children[1];
        delete temp->children[0];
        temp->children[0] = nullptr;
        temp->children[1] = nullptr;
        delete temp;
      }
      return new ASTNode(ASTNodeType::CodeBlock, ret);
    case LexemeType::Newline:
    case LexemeType::Semicolon:
      ++this->lcurrent_;
      break;
    case LexemeType::Var:
      {
        ++this->lcurrent_;
        auto stmt = this->varDecl_();
        ASTNode* seq = new ASTNode(ASTNodeType::Seq, *node, stmt);
        *node = seq;
        node = &seq->children[1];
      }
      break;
    case LexemeType::Print:
      {
        ++this->lcurrent_;
        auto stmt = this->printExpr_();
        ASTNode* seq = new ASTNode(ASTNodeType::Seq, *node, stmt);
        *node = seq;
        node = &seq->children[1];
      }
      break;
    default:
      {
        auto stmt = this->statement_(def_expr_bindp);
        ASTNode* seq = new ASTNode(ASTNodeType::Seq, *node, stmt);
        *node = seq;
        node = &seq->children[1];
      }
    }
  }
  return nullptr;
}

Parser::Parser(const std::vector<Lexeme>& lexes)
: lbegin_(lexes.cbegin()), lend_(lexes.cend()) {
  this->lcurrent_ = this->lbegin_;
}

Procedure* Parser::parse(){
  auto parse_tree = this->codeBlock_();
  
  #ifndef NDEBUG
  //std::cout << "::AST::\n";
  
  //std::cout << tok->toStr() << std::endl;
  
  //std::cout << "::proc::\n";
  #endif
  
  //Procedure* proc = new Procedure(parse_tree);
  
  //return proc;
  return nullptr;
}
