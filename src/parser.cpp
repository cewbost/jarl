#include "parser.h"

#include "procedure.h"

#include <cassert>

#ifndef NDEBUG
#include <iostream>
#endif

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
void Parser::skipNextNewlines_(){
  while(this->lcurrent_->type == LexemeType::Newline){
    ++this->lcurrent_;
  }
}

int Parser::bindp_(LexemeType type){
  return static_cast<unsigned>(type) & ~0xf;
}

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

ASTNode* Parser::nud_(const Lexeme& lex){
  switch(lex.type){
  case LexemeType::Plus:
    return this->new_expression_(0x81);
  case LexemeType::Minus:
    return new ASTNode(ASTNodeType::Neg, this->new_expression_(0x81));
  case LexemeType::Not:
    return new ASTNode(ASTNodeType::Not, this->new_statement_(0x51));
  case LexemeType::Colon:
    return new ASTNode(
      ASTNodeType::Range,
      new ASTNode(ASTNodeType::Nop),
      this->new_expression_(0x31)
    );
  
  case LexemeType::If:
    return this->new_ifExpr_();
  case LexemeType::While:
    return this->whileExpr_();
  
  case LexemeType::Func:
    return this->functionExpr_();
  
  case LexemeType::LParen:
    {
      auto tok = this->new_expression_(def_expr_bindp);
      if(!this->checkNext_(LexemeType::RParen)){
        assert(false);
      }
      return tok;
    }
  case LexemeType::LBrace:
    {
      auto tok = this->new_codeBlock_();
      if(!this->checkNext_(LexemeType::RBrace)){
        assert(false);
      }
      return tok;
    }
  case LexemeType::LBracket:
    {
      auto tok = this->new_expression_(0x11);
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
      auto right = this->new_expression_(def_expr_bindp);
      if(!this->checkNext_(LexemeType::RBracket)){
        assert(false);
      }
      return new ASTNode(ASTNodeType::Index, left, right);
    }else{
      return new ASTNode(ASTNodeType::Apply, left, this->nud_(lex));
    }
  }else if(typen >= static_cast<unsigned>(LexemeType::LeftAssocClass)){
    auto right = this->new_expression_(typen);
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
    auto right = this->new_expression_(typen - 1);
    
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

ASTNode* Parser::new_statement_(int bindp){
  if(bindp >= this->bindp_(this->lcurrent_->type)){
    return new ASTNode(ASTNodeType::Nop);
  }
  auto left = this->nud_(this->next_());
  while(bindp < this->bindp_(this->lcurrent_->type)){
    left = this->led_(this->next_(), left);
  }
  return left;
}

ASTNode* Parser::new_expression_(int bindp){
  this->skipNextNewlines_();
  if(bindp > this->bindp_(this->lcurrent_->type)){
    return new ASTNode(ASTNodeType::Nop);
  }
  auto left = this->nud_(this->nextNoNewline_());
  while(bindp < this->bindp_(this->lcurrent_->type)){
    left = this->led_(this->nextNoNewline_(), left);
  }
  return left;
}

ASTNode* Parser::new_ifExpr_(){
  ASTNode* condition = this->new_statement_(func_bindp);
  if(!this->checkNext_(LexemeType::Colon)){
    assert(false);
  }
  this->skipNextNewlines_();
  ASTNode* stmt = this->new_statement_(func_bindp);
  ASTNode* else_stmt;
  
  auto checkpoint = this->lcurrent_;
  this->skipNextNewlines_();
  if(this->lcurrent_->type == LexemeType::Else){
    ++this->lcurrent_;
    this->skipNextNewlines_();
    else_stmt = this->new_statement_(func_bindp);
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
  ASTNode* condition = this->new_statement_(func_bindp);
  if(!this->checkNext_(LexemeType::Colon)){
    assert(false);
  }
  this->skipNextNewlines_();
  ASTNode* stmt = this->new_statement_(func_bindp);
  
  return new ASTNode(ASTNodeType::While, condition, stmt);
}

ASTNode* Parser::new_identifierList_(){
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
  ASTNode* arg_list = this->new_identifierList_();
  
  if(!this->checkNext_(LexemeType::Colon)){
    assert(false);
  }
  this->skipNextNewlines_();
  ASTNode* code = this->new_statement_(func_bindp);
  
  return new ASTNode(ASTNodeType::Function, arg_list, code);
}

ASTNode* Parser::new_varDecl_(){
  return nullptr;
}

ASTNode* Parser::new_printExpr_(){
  return nullptr;
}

ASTNode* Parser::new_codeBlock_(){
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
        auto stmt = this->new_varDecl_();
        ASTNode* seq = new ASTNode(ASTNodeType::Seq, *node, stmt);
        *node = seq;
        node = &seq->children[1];
      }
      break;
    case LexemeType::Print:
      {
        ++this->lcurrent_;
        auto stmt = this->new_printExpr_();
        ASTNode* seq = new ASTNode(ASTNodeType::Seq, *node, stmt);
        *node = seq;
        node = &seq->children[1];
      }
      break;
    default:
      {
        auto stmt = this->new_statement_(def_expr_bindp);
        ASTNode* seq = new ASTNode(ASTNodeType::Seq, *node, stmt);
        *node = seq;
        node = &seq->children[1];
      }
    }
  }
  return nullptr;
}

//old api
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

Parser::Parser(const std::vector<Lexeme>& lexes)
: lexer_(nullptr), lbegin_(lexes.cbegin()), lend_(lexes.cend()) {
  this->lcurrent_ = this->lbegin_;
}

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
