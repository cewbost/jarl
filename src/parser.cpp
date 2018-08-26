#include "parser.h"

#include <cassert>

constexpr int def_expr_bindp      = 0x31;
constexpr int def_statement_bindp = 0x21;
constexpr int def_block_bindp     = 0x11;
constexpr int func_bindp          = static_cast<int>(LexemeType::LeftAssocClass);

int Parser::bindp_(LexemeType type){
  return static_cast<unsigned>(type) & ~0xf;
}

const Lexeme& Parser::next_(){
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

ASTNode* Parser::nud_(const Lexeme& lex){
  switch(lex.type){
  case LexemeType::Plus:
    return this->expression_(static_cast<unsigned>(LexemeType::ExprClass) + 1);
  case LexemeType::Minus:
    return new ASTNode(
      ASTNodeType::Neg,
      this->expression_(static_cast<unsigned>(LexemeType::ExprClass) + 1),
      lex.pos
    );
  case LexemeType::Not:
    return new ASTNode(
      ASTNodeType::Not,
      this->expression_(static_cast<unsigned>(LexemeType::LeftAssocClass) + 1),
      lex.pos
    );
  
  case LexemeType::Semicolon:
  case LexemeType::Newline:
    return new ASTNode(
      ASTNodeType::Seq,
      new ASTNode(ASTNodeType::Nop, lex.pos),
      this->expression_(bindp_(lex.type) + 1),
      lex.pos
    );
  case LexemeType::Comma:
    return new ASTNode(
      ASTNodeType::ExprList,
      new ASTNode(ASTNodeType::Nop, lex.pos),
      this->expression_(bindp_(lex.type) + 1),
      lex.pos
    );
  case LexemeType::Colon:
    return new ASTNode(
      ASTNodeType::KeyValuePair,
      new ASTNode(ASTNodeType::Nop, lex.pos),
      this->expression_(bindp_(lex.type) + 1),
      lex.pos
    );
  
  case LexemeType::If:
    {
      std::unique_ptr<ASTNode> tok(this->expression_(def_block_bindp));
      if(!this->checkNext_(LexemeType::Do)){
        this->errors_->emplace_back(dynSprintf(
          "line %d: Expected ':'.",
          (this->lcurrent_ - 1)->pos.first
        ));
        return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
      }else{
        return new ASTNode(
          ASTNodeType::If,
          tok.release(),
          this->expression_(def_statement_bindp),
          (this->lcurrent_ - 1)->pos
        );
      }
    }
  case LexemeType::Func:
    {
      if(!this->checkNext_(LexemeType::LParen)){
        this->errors_->emplace_back(dynSprintf(
          "line %d: Expected '('.",
          (this->lcurrent_ - 1)->pos.first
        ));
        return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
      }
      std::unique_ptr<ASTNode> cond(this->expression_(def_expr_bindp));
      if(!this->checkNext_(LexemeType::RParen)){
        this->errors_->emplace_back(dynSprintf(
          "line %d: Expected ')'.",
          (this->lcurrent_ - 1)->pos.first
        ));
        return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
      }
      return new ASTNode(
        ASTNodeType::Function,
        cond.release(),
        this->expression_(def_statement_bindp),
        (this->lcurrent_ - 1)->pos
      );
    }
  case LexemeType::While:
    {
      std::unique_ptr<ASTNode> tok(this->expression_(def_block_bindp));
      if(!this->checkNext_(LexemeType::Do)){
        this->errors_->emplace_back(dynSprintf(
          "line %d: Expected ':'.",
          (this->lcurrent_ - 1)->pos.first
        ));
        return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
      }else{
        return new ASTNode(
          ASTNodeType::While,
          tok.release(),
          this->expression_(def_statement_bindp),
          (this->lcurrent_ - 1)->pos
        );
      }
    }
  
  case LexemeType::For:
    {
      std::unique_ptr<ASTNode> tok(this->expression_(def_block_bindp));
      if(!this->checkNext_(LexemeType::Do)){
        this->errors_->emplace_back(dynSprintf(
          "line %d: Expected ':'.",
          (this->lcurrent_ - 1)->pos.first
        ));
        return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
      }else{
        return new ASTNode(
          ASTNodeType::For,
          tok.release(),
          this->expression_(def_statement_bindp),
          (this->lcurrent_ - 1)->pos
        );
      }
    }
  
  case LexemeType::LParen:
    {
      std::unique_ptr<ASTNode> tok(this->expression_(def_expr_bindp));
      if(!this->checkNext_(LexemeType::RParen)){
        this->errors_->emplace_back(dynSprintf(
          "line %d: Expected ')'.",
          (this->lcurrent_ - 1)->pos.first
        ));
        return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
      }else return tok.release();
    }
  case LexemeType::LBrace:
    {
      std::unique_ptr<ASTNode> tok(this->expression_(def_block_bindp));
      if(!this->checkNext_(LexemeType::RBrace)){
        this->errors_->emplace_back(dynSprintf(
          "line %d: Expected '}'.",
          (this->lcurrent_ - 1)->pos.first
        ));
        return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
      }else{
        if(tok->type == ASTNodeType::ExprList
        || tok->type == ASTNodeType::KeyValuePair
        || tok->type == ASTNodeType::Nop){
          return new ASTNode(ASTNodeType::Table, tok.release(), lex.pos);
        }else{
          return new ASTNode(ASTNodeType::CodeBlock, tok.release(), lex.pos);
        }
      }
    }
  case LexemeType::LBracket:
    {
      std::unique_ptr<ASTNode> tok(this->expression_(def_expr_bindp));
      if(!this->checkNext_(LexemeType::RBracket)){
        this->errors_->emplace_back(dynSprintf(
          "line %d: Expected ']'.",
          (this->lcurrent_ - 1)->pos.first
        ));
        return new ASTNode(ASTNodeType::ParseError, (this->lcurrent_ - 1)->pos);
      }else return new ASTNode(ASTNodeType::Array, tok.release(), lex.pos);
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
  
  case LexemeType::Var:
    return new ASTNode(
      ASTNodeType::Var,
      this->expression_(def_expr_bindp),
      lex.pos
    );
  case LexemeType::Print:
    return new ASTNode(
      ASTNodeType::Print,
      this->expression_(def_expr_bindp),
      lex.pos
    );
  case LexemeType::Assert:
    return new ASTNode(
      ASTNodeType::Assert,
      this->expression_(def_expr_bindp),
      lex.pos
    );
  
  default:
    this->errors_->emplace_back(dynSprintf(
      "line %d: Expected expression.",
      (this->lcurrent_ - 1)->pos.first
    ));
    return new ASTNode(ASTNodeType::ParseError, lex.pos);
  }
}

ASTNode* Parser::led_(const Lexeme& lex, ASTNode* left){
  auto typen = bindp_(lex.type);
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
    
    case LexemeType::In:
      return new ASTNode(ASTNodeType::In, left, right, lex.pos);
    
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
    case LexemeType::Insert:
      return new ASTNode(ASTNodeType::Insert, left, right, lex.pos);
    
    case LexemeType::Comma:
      return new ASTNode(ASTNodeType::ExprList, left, right, lex.pos);
    case LexemeType::Colon:
      return new ASTNode(ASTNodeType::KeyValuePair, left, right, lex.pos);
    
    default:
      assert(false);
    }
  }else if(typen >= static_cast<unsigned>(LexemeType::StopSymbolClass)){
    auto right = this->expression_(typen);
    switch(lex.type){
    case LexemeType::Semicolon:
    case LexemeType::Newline:
      return new ASTNode(ASTNodeType::Seq, left, right, lex.pos);
    case LexemeType::Comma:
      return new ASTNode(ASTNodeType::ExprList, left, right, lex.pos);
    case LexemeType::Else:
      return new ASTNode(ASTNodeType::Else, left, right, lex.pos);
      
    default:
      assert(false);
    }
  }
  
  return nullptr;
}

ASTNode* Parser::expression_(int bindp){
  if(bindp > bindp_(this->lcurrent_->type)){
    return new ASTNode(ASTNodeType::Nop, this->lcurrent_->pos);
  }
  auto left = this->nud_(this->next_());
  while(bindp < bindp_(this->lcurrent_->type)){
    left = this->led_(this->next_(), left);
  }
  return left;
}

Parser::Parser(const std::vector<Lexeme>& lexes)
: lbegin_(lexes.cbegin() + 1), lend_(lexes.cend()) {
  this->lcurrent_ = this->lbegin_;
}

std::unique_ptr<ASTNode> Parser::parse(std::vector<std::unique_ptr<char[]>>* errors){
  this->errors_ = errors;
  return std::unique_ptr<ASTNode>(this->expression_(def_block_bindp));
}
