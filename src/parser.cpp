#include "parser.h"

#include "procedure.h"

#include <cassert>

#ifndef NDEBUG
#include <iostream>
#endif

constexpr int def_expr_bindp  = 15;
constexpr int func_bindp      = 45;

int Parser::bindp_(TokenType type){
  switch(type){
  case TokenType::EndToken:
    return 0;
  case TokenType::NewlineToken:
  case TokenType::SemicolonToken:
  case TokenType::RParenToken:
  case TokenType::RBraceToken:
  case TokenType::RBracketToken:
  case TokenType::ElseToken:
  case TokenType::VarToken:
  case TokenType::PrintToken:
    return 10;
  case TokenType::CommaToken:
    return 20;
  case TokenType::InsertToken:
  case TokenType::AssignToken:
  case TokenType::PlusAssignToken:
  case TokenType::MinusAssignToken:
  case TokenType::MulAssignToken:
  case TokenType::DivAssignToken:
  case TokenType::ModAssignToken:
  case TokenType::AppendAssignToken:
    return 30;
  case TokenType::ColonToken:
    return 40;
  case TokenType::ApplyToken:
    return 50;
  case TokenType::NotToken:
  case TokenType::AndToken:
  case TokenType::OrToken:
    return 60;
  case TokenType::CmpToken:
  case TokenType::EqToken:
  case TokenType::NeqToken:
  case TokenType::GtToken:
  case TokenType::LtToken:
  case TokenType::GeqToken:
  case TokenType::LeqToken:
    return 70;
  case TokenType::AppendToken:
    return 80;
  case TokenType::PlusToken:
  case TokenType::MinusToken:
    return 90;
  case TokenType::MulToken:
  case TokenType::DivToken:
  case TokenType::ModToken:
    return 100;
  case TokenType::LParenToken:
  case TokenType::LBraceToken:
  case TokenType::LBracketToken:
    return 110;
  default:
    return 120;
  }
  //NOTE: bind power is 200 for unary operators
}

Token* Parser::advanceToken_(){
  auto current = this->current_;
  this->current_ = this->lexer_.next();
  return current;
}

Token* Parser::advanceTokenNoNewline_(){
  Token* current;
  for(;;){
    current = this->current_;
    this->current_ = this->lexer_.next();
    if(current->type != TokenType::NewlineToken) break;
    delete current;
  }
  return current;
}

bool Parser::checkNextToken_(TokenType type){
  auto tok = this->advanceToken_();
  bool ret = tok->type == type;
  delete tok;
  return ret;
}

void Parser::skipNewlines_(){
  while(this->current_->type == TokenType::NewlineToken){
    delete this->current_;
    this->current_ = this->lexer_.next();
  }
}

Token* Parser::identifierList_(){
  Token* tok = new Token(TokenType::NopNode);
  
  if(this->current_->type == TokenType::IdentifierToken){
    tok = new Token(TokenType::IdentifierList, this->advanceToken_(), tok);
    Token** next_tok = &tok->childs.second;
    
    while(this->current_->type == TokenType::CommaToken){
      delete this->advanceToken_();
      assert(this->current_->type == TokenType::IdentifierToken);
      
      *next_tok = new Token(
        TokenType::IdentifierList,
        this->advanceToken_(),
        *next_tok
      );
      next_tok = &((*next_tok)->childs.second);
    }
  }
  return tok;
}

Token* Parser::statement_(int bindp){
  if(bindp >= this->bindp_(this->current_->type)){
    return new Token(TokenType::NopNode);
  }
  auto left = this->nud_(this->advanceToken_());
  while(bindp < this->bindp_(this->current_->type)){
    left = this->led_(this->advanceToken_(), left);
  }
  return left;
}

Token* Parser::expression_(int bindp){
  this->skipNewlines_();
  if(bindp > this->bindp_(this->current_->type)){
    return new Token(TokenType::NopNode);
  }
  auto left = this->nud_(this->advanceTokenNoNewline_());
  while(bindp < this->bindp_(this->current_->type)){
    left = this->led_(this->advanceTokenNoNewline_(), left);
  }
  return left;
}

Token* Parser::codeBlock_(){
  Token* ret = new Token(TokenType::NopNode);
  Token** tok = &ret;
  for(;;){
    switch(this->current_->type){
    case TokenType::EndToken:
    case TokenType::RBraceToken:
      if(ret->type == TokenType::SeqNode){
        Token* temp = ret;
        ret = ret->childs.second;
        delete temp->childs.first;
        temp->childs.first = nullptr;
        temp->childs.second = nullptr;
        delete temp;
      }
      return new Token(TokenType::CodeBlock, ret);
    case TokenType::NewlineToken:
    case TokenType::SemicolonToken:
      delete this->advanceToken_();
      break;
    case TokenType::VarToken: {
        delete this->advanceToken_();
        auto stmt = this->varDecl_();
        Token* seq = new Token(TokenType::SeqNode, *tok, stmt);
        *tok = seq;
        tok = &seq->childs.second;
      }break;
    case TokenType::PrintToken: {
        delete this->advanceToken_();
        auto stmt = this->printExpr_();
        Token* seq = new Token(TokenType::SeqNode, *tok, stmt);
        *tok = seq;
        tok = &seq->childs.second;
      }break;
    default: {
        auto stmt = this->statement_(def_expr_bindp);
        Token* seq = new Token(TokenType::SeqNode, *tok, stmt);
        *tok = seq;
        tok = &seq->childs.second;
      }
    }
  }
  return nullptr;
}

Token* Parser::varDecl_(){
  auto ret = new Token(TokenType::VarDecl);
  auto iden = this->advanceToken_();
  assert(iden->type == TokenType::IdentifierToken);
  ret->childs.first = iden;
  
  auto tok = this->advanceToken_();
  if(tok->type == TokenType::AssignToken){
    delete tok;
    ret->childs.second = this->statement_(def_expr_bindp);
  }else{
    assert(
      tok->type == TokenType::EndToken ||
      tok->type == TokenType::NewlineToken ||
      tok->type == TokenType::SemicolonToken
    );
    assert(false);
  }
  return ret;
}

Token* Parser::printExpr_(){
  auto ret = new Token(TokenType::PrintExpr);
  ret->child = this->statement_(def_expr_bindp);
  return ret;
}

Token* Parser::ifExpr_(){
  Token* condition = this->statement_(func_bindp);
  if(!this->checkNextToken_(TokenType::ColonToken)){
    assert(false);
  }
  this->skipNewlines_();
  Token* stmt = this->statement_(func_bindp);
  Token* else_stmt;
  
  this->lexer_.setCheckpoint();
  this->skipNewlines_();
  if(this->current_->type == TokenType::ElseToken){
    this->lexer_.removeCheckpoint();
    delete this->advanceToken_();
    this->skipNewlines_();
    else_stmt = this->statement_(func_bindp);
  }else{
    this->lexer_.revert();
    delete this->advanceToken_();
    else_stmt = nullptr;
  }
  
  return new Token(
    TokenType::ConditionalExpr,
    condition,
    new Token(
      TokenType::ConditionalNode,
      stmt,
      else_stmt? else_stmt : new Token(TokenType::NopNode)
    )
  );
}

Token* Parser::forExpr_(){
  Token* condition = this->statement_(func_bindp);
  if(!this->checkNextToken_(TokenType::ColonToken)){
    assert(false);
  }
  this->skipNewlines_();
  Token* stmt = this->statement_(func_bindp);
  
  return new Token(TokenType::LoopNode, condition, stmt);
}

Token* Parser::funcExpr_(){
  Token* arg_list = this->identifierList_();
  
  if(!this->checkNextToken_(TokenType::ColonToken)){
    assert(false);
  }
  this->skipNewlines_();
  Token* code = this->statement_(func_bindp);
  
  return new Token(TokenType::FuncNode, arg_list, code);
}

Token* Parser::nud_(Token* tok){
  switch(tok->type){
  //case TokenType::DelToken:
  //  delete tok;
  //  return new Token(TokenType::DelExpr, this->statement_(200));
  case TokenType::PlusToken:
    delete tok;
    return this->expression_(85);
  case TokenType::MinusToken:
    delete tok;
    return new Token(TokenType::NegExpr, this->expression_(85));
  case TokenType::NotToken:
    delete tok;
    return new Token(TokenType::NotExpr, this->statement_(55));
  case TokenType::ColonToken:
    delete tok;
    return new Token(
      TokenType::RangeExpr,
      new Token(TokenType::NopNode),
      this->expression_(35)
    );
  
  case TokenType::IfToken: {
    delete tok;
    return this->ifExpr_();
  }
  case TokenType::WhileToken: {
    delete tok;
    return this->forExpr_();
  }
  
  case TokenType::FuncToken: {
    delete tok;
    return this->funcExpr_();
  }
  
  case TokenType::LParenToken: {
      auto bindp = this->bindp_(tok->type);
      delete tok;
      tok = this->expression_(def_expr_bindp);
      if(!this->checkNextToken_(TokenType::RParenToken)){
        assert(false);
      }
      return tok;
    }
  case TokenType::LBraceToken: {
      delete tok;
      tok = this->codeBlock_();
      if(!this->checkNextToken_(TokenType::RBraceToken)){
        assert(false);
      }
      return tok;
    }
  case TokenType::LBracketToken: {
      delete tok;
      tok = this->expression_(15);
      tok = new Token(TokenType::ArrayExpr, tok);
      if(!this->checkNextToken_(TokenType::RBracketToken)){
        assert(false);
      }
      return tok;
    }
  default: 
    return tok;
  }
}

Token* Parser::led_(Token* tok, Token* left){
  if(tokenFlag(tok->type, ExprNodeFlag)){
    return new Token(TokenType::ApplyExpr, left, this->nud_(tok));
  }else{
    if(tokenFlag(tok->type, ValueTokenFlag)){
      if(tok->type == TokenType::LBracketToken){
        auto right = this->expression_(def_expr_bindp);
        if(!this->checkNextToken_(TokenType::RBracketToken)){
          assert(false);
        }
        delete tok;
        return new Token(TokenType::IndexExpr, left, right);
      }
      return new Token(TokenType::ApplyExpr, left, this->nud_(tok));
    }else{
      if(tokenFlag(tok->type, OperatorTokenFlag)){
        if(tokenFlag(tok->type, RightAssocFlag)){
          auto right = this->expression_(this->bindp_(tok->type) - 1);
          auto type = tok->type;
          delete tok;
          switch(type){
          case TokenType::InsertToken:
            return new Token(TokenType::InsertExpr, left, right);
          case TokenType::AssignToken:
            return new Token(TokenType::AssignExpr, left, right);
          case TokenType::PlusAssignToken:
            return new Token(TokenType::AddAssignExpr, left, right);
          case TokenType::MinusAssignToken:
            return new Token(TokenType::SubAssignExpr, left, right);
          case TokenType::MulAssignToken:
            return new Token(TokenType::MulAssignExpr, left, right);
          case TokenType::DivAssignToken:
            return new Token(TokenType::DivAssignExpr, left, right);
          case TokenType::ModAssignToken:
            return new Token(TokenType::ModAssignExpr, left, right);
          case TokenType::AppendAssignToken:
            return new Token(TokenType::AppendAssignExpr, left, right);
          
          case TokenType::ColonToken:
            return new Token(TokenType::RangeExpr, left, right);
          case TokenType::CommaToken:
            return new Token(TokenType::ExprList, left, right);
          
          default:
            assert(false);
          }
        }else{
          auto right = this->expression_(this->bindp_(tok->type));
          auto type = tok->type;
          delete tok;
          switch(type){
          case TokenType::PlusToken:
            return new Token(TokenType::AddExpr, left, right);
          case TokenType::MinusToken:
            return new Token(TokenType::SubExpr, left, right);
          case TokenType::MulToken:
            return new Token(TokenType::MulExpr, left, right);
          case TokenType::DivToken:
            return new Token(TokenType::DivExpr, left, right);
          case TokenType::ModToken:
            return new Token(TokenType::ModExpr, left, right);
          case TokenType::AppendToken:
            return new Token(TokenType::AppendExpr, left, right);
          case TokenType::ApplyToken:
            return new Token(TokenType::ApplyExpr, left, right);
          
          case TokenType::CmpToken:
            return new Token(TokenType::CmpExpr, left, right);
          case TokenType::EqToken:
            return new Token(TokenType::EqExpr, left, right);
          case TokenType::NeqToken:
            return new Token(TokenType::NeqExpr, left, right);
          case TokenType::GtToken:
            return new Token(TokenType::GtExpr, left, right);
          case TokenType::LtToken:
            return new Token(TokenType::LtExpr, left, right);
          case TokenType::GeqToken:
            return new Token(TokenType::GeqExpr, left, right);
          case TokenType::LeqToken:
            return new Token(TokenType::LeqExpr, left, right);
          
          case TokenType::AndToken:
            return new Token(TokenType::AndExpr, left, right);
          case TokenType::OrToken:
            return new Token(TokenType::OrExpr, left, right);
          
          default:
            assert(false);
          }
        }
      }else{
        //error?
        return left;
      }
    }
  }
}

Parser::Parser(const char* code)
: lexer_(code), current_(nullptr){}

Procedure* Parser::parse(){
  delete this->advanceToken_();
  auto tok = this->codeBlock_();
  
  #ifndef NDEBUG
  //std::cout << "::AST::\n";
  
  //std::cout << tok->toStr() << std::endl;
  
  //std::cout << "::proc::\n";
  #endif
  
  Procedure* proc = new Procedure(tok);
  
  return proc;
  //return nullptr;
}

Parser::~Parser(){
  delete this->current_;
}
