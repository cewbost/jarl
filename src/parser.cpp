#include "parser.h"

#include <cassert>

#ifndef NDEBUG
#include <iostream>
#endif

constexpr int def_expr_bindp  = static_cast<int>(LexemeType::RightAssocClass);
constexpr int func_bindp      = static_cast<int>(LexemeType::LeftAssocClass);

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
    return new ASTNode(ASTNodeType::Neg, this->expression_(0x81), lex.pos);
  case LexemeType::Not:
    return new ASTNode(ASTNodeType::Not, this->statement_(0x51), lex.pos);
  case LexemeType::Colon:
    return new ASTNode(
      ASTNodeType::Range,
      new ASTNode(ASTNodeType::Nop, lex.pos),
      this->expression_(0x31),
      lex.pos
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
        delete tok;
        this->errors_->emplace_back(dynSprintf(
          "line %d: Expected ')'.",
          (this->lcurrent_ - 1)->pos.first
        ));
        return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
      }else return tok;
    }
  case LexemeType::LBrace:
    {
      auto tok = this->codeBlock_();
      if(!this->checkNext_(LexemeType::RBrace)){
        delete tok;
        this->errors_->emplace_back(dynSprintf(
          "line %d: Expected '}'.",
          (this->lcurrent_ - 1)->pos.first
        ));
        return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
      }else return tok;
    }
  case LexemeType::LBracket:
    {
      auto tok = this->expression_(0x11);
      tok = new ASTNode(ASTNodeType::Array, tok, lex.pos);
      if(!this->checkNext_(LexemeType::RBracket)){
        delete tok;
        this->errors_->emplace_back(dynSprintf(
          "line %d: Expected ']'.",
          (this->lcurrent_ - 1)->pos.first
        ));
        return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
      }else return tok;
    }
  
  case LexemeType::Int:
    return new ASTNode(ASTNodeType::Int, lex.value.i, lex.pos);
  case LexemeType::Float:
    return new ASTNode(ASTNodeType::Float, lex.value.f, lex.pos);
  case LexemeType::Bool:
    return new ASTNode(ASTNodeType::Bool, lex.value.b, lex.pos);
  case LexemeType::Null:
    return new ASTNode(ASTNodeType::Null, lex.pos);
  case LexemeType::String:
    return new ASTNode(ASTNodeType::String, lex.value.s, lex.pos);
  case LexemeType::Identifier:
    return new ASTNode(ASTNodeType::Identifier, lex.value.s, lex.pos);
  
  default:
    this->errors_->emplace_back(dynSprintf(
      "line %d: Expected expression.",
      (this->lcurrent_ - 1)->pos.first
    ));
  }
}

ASTNode* Parser::led_(const Lexeme& lex, ASTNode* left){
  auto typen = this->bindp_(lex.type);
  if(typen >= static_cast<unsigned>(LexemeType::ExprClass)){
    if(lex.type == LexemeType::LBracket){
      auto right = this->expression_(def_expr_bindp);
      if(!this->checkNext_(LexemeType::RBracket)){
        delete right;
        delete left;
        this->errors_->emplace_back(dynSprintf(
          "line %d: Expected ']'.",
          (this->lcurrent_ - 1)->pos.first
        ));
        return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
      }else return new ASTNode(ASTNodeType::Index, left, right, lex.pos);
    }else{
      return new ASTNode(ASTNodeType::Apply, left, this->nud_(lex), lex.pos);
    }
  }else if(typen >= static_cast<unsigned>(LexemeType::LeftAssocClass)){
    auto right = this->expression_(typen);
    switch(lex.type){
    case LexemeType::Plus:
      return new ASTNode(ASTNodeType::Add, left, right, lex.pos);
    case LexemeType::Minus:
      return new ASTNode(ASTNodeType::Sub, left, right, lex.pos);
    case LexemeType::Mul:
      return new ASTNode(ASTNodeType::Mul, left, right, lex.pos);
    case LexemeType::Div:
      return new ASTNode(ASTNodeType::Div, left, right, lex.pos);
    case LexemeType::Mod:
      return new ASTNode(ASTNodeType::Mod, left, right, lex.pos);
    case LexemeType::Append:
      return new ASTNode(ASTNodeType::Append, left, right, lex.pos);
    
    case LexemeType::Cmp:
      return new ASTNode(ASTNodeType::Cmp, left, right, lex.pos);
    case LexemeType::Eq:
      return new ASTNode(ASTNodeType::Eq, left, right, lex.pos);
    case LexemeType::Neq:
      return new ASTNode(ASTNodeType::Neq, left, right, lex.pos);
    case LexemeType::Gt:
      return new ASTNode(ASTNodeType::Gt, left, right, lex.pos);
    case LexemeType::Lt:
      return new ASTNode(ASTNodeType::Lt, left, right, lex.pos);
    case LexemeType::Geq:
      return new ASTNode(ASTNodeType::Geq, left, right, lex.pos);
    case LexemeType::Leq:
      return new ASTNode(ASTNodeType::Leq, left, right, lex.pos);
    
    case LexemeType::And:
      return new ASTNode(ASTNodeType::And, left, right, lex.pos);
    case LexemeType::Or:
      return new ASTNode(ASTNodeType::Or, left, right, lex.pos);
    
    default:
      assert(false);
    }
  }else if(typen >= static_cast<unsigned>(LexemeType::RightAssocClass)){
    auto right = this->expression_(typen - 1);
    
    switch(lex.type){
    case LexemeType::Assign:
      return new ASTNode(ASTNodeType::Assign, left, right, lex.pos);
    case LexemeType::AddAssign:
      return new ASTNode(ASTNodeType::AddAssign, left, right, lex.pos);
    case LexemeType::SubAssign:
      return new ASTNode(ASTNodeType::SubAssign, left, right, lex.pos);
    case LexemeType::MulAssign:
      return new ASTNode(ASTNodeType::MulAssign, left, right, lex.pos);
    case LexemeType::DivAssign:
      return new ASTNode(ASTNodeType::DivAssign, left, right, lex.pos);
    case LexemeType::ModAssign:
      return new ASTNode(ASTNodeType::ModAssign, left, right, lex.pos);
    case LexemeType::AppendAssign:
      return new ASTNode(ASTNodeType::AppendAssign, left, right, lex.pos);
    
    case LexemeType::Colon:
      return new ASTNode(ASTNodeType::Range, left, right, lex.pos);
    case LexemeType::Comma:
      return new ASTNode(ASTNodeType::ExprList, left, right, lex.pos);
    
    default:
      assert(false);
    }
  }else assert(false);
  
  return nullptr;
}

ASTNode* Parser::statement_(int bindp){
  if(bindp >= this->bindp_(this->lcurrent_->type)){
    return new ASTNode(ASTNodeType::Nop, this->lcurrent_->pos);
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
    return new ASTNode(ASTNodeType::Nop, this->lcurrent_->pos);
  }
  auto left = this->nud_(this->nextNoNewline_());
  while(bindp < this->bindp_(this->lcurrent_->type)){
    left = this->led_(this->nextNoNewline_(), left);
  }
  return left;
}

ASTNode* Parser::ifExpr_(){
  auto pos1 = (this->lcurrent_ - 1)->pos;
  ASTNode* condition = this->statement_(func_bindp);
  auto pos2 = this->lcurrent_->pos;
  if(!this->checkNext_(LexemeType::Colon)){
    delete condition;
    this->errors_->emplace_back(dynSprintf(
      "line %d: Expected ':'.",
      (this->lcurrent_ - 1)->pos.first
    ));
    return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
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
      else_stmt? else_stmt : new ASTNode(ASTNodeType::Nop, pos2),
      pos2
    ),
    pos1
  );
}

ASTNode* Parser::whileExpr_(){
  auto pos = (this->lcurrent_ - 1)->pos;
  ASTNode* condition = this->statement_(func_bindp);
  if(!this->checkNext_(LexemeType::Colon)){
    delete condition;
    this->errors_->emplace_back(dynSprintf(
      "line %d: Expected ':'.",
      (this->lcurrent_ - 1)->pos.first
    ));
    return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
  }
  this->skipNewlines_();
  ASTNode* stmt = this->statement_(func_bindp);
  
  return new ASTNode(ASTNodeType::While, condition, stmt, pos);
}

ASTNode* Parser::identifierList_(){
  auto pos = (this->lcurrent_ - 1)->pos;
  ASTNode* node = new ASTNode(ASTNodeType::Nop, pos);
  
  if(this->lcurrent_->type == LexemeType::Identifier){
    auto next = &this->next_();
    node = new ASTNode(
      ASTNodeType::IdentifierList,
      next->value.s,
      node,
      next->pos
    );
    ASTNode** next_node = &node->string_branch.next;
    
    while(this->lcurrent_->type == LexemeType::Comma){
      ++this->lcurrent_;
      assert(this->lcurrent_->type == LexemeType::Identifier);
      if(this->lcurrent_->type != LexemeType::Identifier){
        delete node;
        this->errors_->emplace_back(dynSprintf(
          "line %d: Expected Identifier.",
          (this->lcurrent_ - 1)->pos.first
        ));
        return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
      }
      
      next = &this->next_();
      *next_node = new ASTNode(
        ASTNodeType::IdentifierList,
        next->value.s,
        *next_node,
        next->pos
      );
      next_node = &((*next_node)->string_branch.next);
    }
  }
  return node;
}

ASTNode* Parser::functionExpr_(){
  auto pos = (this->lcurrent_ - 1)->pos;
  ASTNode* arg_list = this->identifierList_();
  
  if(!this->checkNext_(LexemeType::Colon)){
    delete arg_list;
    this->errors_->emplace_back(dynSprintf(
      "line %d: Expected ':'.",
      (this->lcurrent_ - 1)->pos.first
    ));
    return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
  }
  this->skipNewlines_();
  ASTNode* code = this->statement_(func_bindp);
  
  return new ASTNode(ASTNodeType::Function, arg_list, code, pos);
}

ASTNode* Parser::varDecl_(){
  auto pos = (this->lcurrent_ - 1)->pos;
  auto ret = new ASTNode(ASTNodeType::VarDecl, pos);
  if(this->lcurrent_->type != LexemeType::Identifier){
    delete ret;
    this->errors_->emplace_back(dynSprintf(
      "line %d: Expected Identifier.",
      this->lcurrent_->pos.first
    ));
    return new ASTNode(ASTNodeType::ParseError, this->lcurrent_->pos);
  }
  auto iden = this->next_();
  ret->string_branch.value = iden.value.s;
  
  auto next = this->next_();
  if(next.type != LexemeType::Assign){
    delete ret;
    this->errors_->emplace_back(dynSprintf(
      "line %d: Expected '='.",
      (this->lcurrent_ - 1)->pos.first
    ));
    return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
  }else ret->string_branch.next = this->statement_(def_expr_bindp);
  
  return ret;
}

ASTNode* Parser::printExpr_(){
  auto pos = (this->lcurrent_ - 1)->pos;
  auto ret = new ASTNode(ASTNodeType::Print, pos);
  ret->child = this->statement_(def_expr_bindp);
  return ret;
}

ASTNode* Parser::codeBlock_(){
  auto pos = this->lcurrent_->pos;
  ASTNode* ret = new ASTNode(ASTNodeType::Nop, this->lcurrent_->pos);
  ASTNode** node = &ret;
  
  for(;;){
    switch(this->lcurrent_->type){
    case LexemeType::End:
    case LexemeType::RBrace:
      if(ret->type == ASTNodeType::Seq){
        ASTNode* temp = ret;
        ret = ret->children.second;
        delete temp->children.first;
        temp->children.first = nullptr;
        temp->children.second = nullptr;
        delete temp;
      }
      return new ASTNode(ASTNodeType::CodeBlock, ret, pos);
    case LexemeType::Newline:
    case LexemeType::Semicolon:
      ++this->lcurrent_;
      break;
    case LexemeType::Var:
      {
        ++this->lcurrent_;
        auto stmt = this->varDecl_();
        ASTNode* seq = new ASTNode(
          ASTNodeType::Seq,
          *node,
          stmt,
          this->lcurrent_->pos
        );
        *node = seq;
        node = &seq->children.second;
      }
      break;
    case LexemeType::Print:
      {
        ++this->lcurrent_;
        auto stmt = this->printExpr_();
        ASTNode* seq = new ASTNode(
          ASTNodeType::Seq,
          *node,
          stmt,
          this->lcurrent_->pos
        );
        *node = seq;
        node = &seq->children.second;
      }
      break;
    default:
      {
        auto stmt = this->statement_(def_expr_bindp);
        ASTNode* seq = new ASTNode(
          ASTNodeType::Seq,
          *node,
          stmt,
          this->lcurrent_->pos
        );
        *node = seq;
        node = &seq->children.second;
      }
    }
  }
  return nullptr;
}

Parser::Parser(const std::vector<Lexeme>& lexes)
: lbegin_(lexes.cbegin()), lend_(lexes.cend()) {
  this->lcurrent_ = this->lbegin_;
}

ASTNode* Parser::parse(std::vector<std::unique_ptr<char[]>>* errors){
  this->errors_ = errors;
  return this->codeBlock_();
}
