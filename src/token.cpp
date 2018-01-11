#include "token.h"

#include "parser.h"
#include "pool_allocator.h"

#include <iostream>

namespace{
  PoolAllocator<sizeof(Token), 256> token_pool;
  
  #ifndef NDEBUG
  inline std::string unquoteString_(const char* str){
    std::string ret(str);
    
    auto replace = [](std::string* s, std::string f, std::string r)->void{
      std::remove_const<decltype(std::string::npos)>::type stepper = 0;
      stepper = s->find(f, stepper);
      while(stepper != std::string::npos){
        s->replace(stepper, f.size(), r);
        stepper += r.size() - f.size();
        stepper = s->find(f, stepper);
      }
    };
    
    replace(&ret, "\\", "\\\\");
    replace(&ret, "\n", "\\n");
    replace(&ret, "\t", "\\t");
    
    return ret;
  }
  #endif
}

void* Token::operator new(size_t s){
  #ifdef NDEBUG
  return token_pool.allocate();
  #else
  return ::operator new(s);
  #endif
}
void Token::operator delete(void* ptr){
  #ifdef NDEBUG
  token_pool.deallocate(ptr);
  #else
  ::operator delete(ptr);
  #endif
}

Token::Token(TokenType type)
: type(type){
  this->childs.first = this->childs.second = nullptr;
}

Token::Token(TokenType type, Token* ch)
: type(type){
  this->child = ch;
}

Token::Token(TokenType type, Token* left, Token* right)
: type(type){
  this->childs.first = left;
  this->childs.second = right;
}

Token::Token(bool b)
: type(TokenType::BoolToken){
  this->bool_value = b;
}

Token::Token(Int i)
: type(TokenType::IntToken){
  this->int_value = i;
}

Token::Token(unsigned long long i)
: type(TokenType::IntToken){
  this->int_value = i;
}

Token::Token(Float f)
: type(TokenType::FloatToken){
  this->float_value = f;
}

Token::Token(String* s)
: type(TokenType::StringToken){
  this->string_value = s;
  s->incRefCount();
}

Token::Token(TokenType type, String* s)
: type(type){
  this->string_value = s;
  s->incRefCount();
}

Token::Token(TokenType type, unsigned long long i)
: type(type){
  this->int_value = i;
}

Token::~Token(){
  if(tokenFlag(this->type, ExprNodeFlag)){
    if(tokenFlag(this->type, UnaryExprFlag)){
      delete this->child;
    }else if(tokenFlag(this->type, BinaryExprFlag)){
      delete this->childs.first;
      delete this->childs.second;
    }
  }else switch(this->type){
    case TokenType::StringToken:
    case TokenType::IdentifierToken:
      this->string_value->decRefCount();
      break;
    default:
      break;
  }
}

#ifndef NDEBUG

std::string Token::toStr()const{
  using namespace std::string_literals;
  
  switch(this->type){
  case TokenType::BoolToken:
    return "bool: "s + (this->bool_value? "true"s : "false"s);
  case TokenType::IntToken:
    return "int: "s + std::to_string(this->int_value);
  case TokenType::FloatToken:
    return "float: "s + std::to_string(this->float_value);
  case TokenType::StringToken:
    return "string: \""s + unquoteString_(this->string_value->str()) + "\""s;
  case TokenType::IdentifierToken:
    return "identifier: "s + std::string(this->string_value->str());
  case TokenType::NewlineToken:
    return "newline"s;
  
  case TokenType::PlusToken: return "+"s;
  case TokenType::MinusToken: return "-"s;
  case TokenType::MulToken: return "*"s;
  case TokenType::DivToken: return "/"s;
  case TokenType::ModToken: return "%"s;
  case TokenType::AppendToken: return "++"s;
  
  case TokenType::AssignToken: return "="s;
  case TokenType::PlusAssignToken: return "+="s;
  case TokenType::MinusAssignToken: return "-="s;
  case TokenType::MulAssignToken: return "*="s;
  case TokenType::DivAssignToken: return "/="s;
  case TokenType::ModAssignToken: return "%="s;
  case TokenType::AppendAssignToken: return "++="s;
  
  case TokenType::CmpToken: return "<=>"s;
  case TokenType::EqToken: return "=="s;
  case TokenType::NeqToken: return "!="s;
  case TokenType::GtToken: return ">"s;
  case TokenType::LtToken: return "<"s;
  case TokenType::GeqToken: return ">="s;
  case TokenType::LeqToken: return "<="s;
  
  case TokenType::NotToken: return "not"s;
  case TokenType::AndToken: return "and"s;
  case TokenType::OrToken: return "or"s;
  
  case TokenType::EndToken: return "end"s;
  
  case TokenType::NopNode: return "nop"s;
  
  case TokenType::NegExpr:
    return "- ("s + this->child->toStr() + ")"s;
  case TokenType::NotExpr:
    return "not ("s + this->child->toStr() + ")"s;
  
  case TokenType::RangeExpr:
    return "range ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  
  case TokenType::ApplyExpr:
    return "apply ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  
  case TokenType::AddExpr:
    return "+ ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::SubExpr:
    return "- ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::MulExpr:
    return "* ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::DivExpr:
    return "/ ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::ModExpr:
    return "% ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::AppendExpr:
    return "++ ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  
  case TokenType::CmpExpr:
    return "<=> ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::EqExpr:
    return "== ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::NeqExpr:
    return "!= ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::GtExpr:
    return "> ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::LtExpr:
    return "< ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::GeqExpr:
    return ">= ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::LeqExpr:
    return "<= ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
    
  case TokenType::IndexExpr:
    return "[] ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  
  case TokenType::AndExpr:
    return "and ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::OrExpr:
    return "or ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  
  case TokenType::InsertExpr:
    return "<- ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::AssignExpr:
    return "= ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::AddAssignExpr:
    return "+= ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::SubAssignExpr:
    return "-= ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::MulAssignExpr:
    return "*= ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::DivAssignExpr:
    return "/= ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::ModAssignExpr:
    return "%= ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::AppendAssignExpr:
    return "++= ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  
  case TokenType::SeqNode:
    return "seq ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::IdentifierList:
    return "id list ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::ExprList:
    return "expr list ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  
  case TokenType::ConditionalExpr:
    return "if ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::ConditionalNode:
    return "choice ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  case TokenType::LoopNode:
    return "for ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  
  case TokenType::FuncNode:
    return "func ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  
  case TokenType::VarDecl:
    return "var ("s
    + this->childs.first->toStr() + ")("s
    + this->childs.second->toStr() + ")"s;
  
  case TokenType::CodeBlock:
    return "code block ("s
    + this->child->toStr() + ")"s;
  
  case TokenType::ArrayExpr:
    return "array ("s
    + this->child->toStr() + ")"s;
  
  default: return "unrecognized token"s;
  }
}

AllocMonitor<Token> token_alloc_monitor([](AllocMsg msg, const Token* tok){
  switch(msg){
  //case AllocMsg::Allocation:
  //  std::cout << "allocated string at " << str << std::endl;
  //  break;
  //case AllocMsg::Deallocation:
  //  std::cout << "deallocated string at " << str << ": " << str->str() << std::endl;
  //  break;
  case AllocMsg::DoubleAllocation:
    std::cout << "double allocated token at " << tok << std::endl;
    break;
  case AllocMsg::InvalidFree:
    std::cout << "invalid free of token at " << tok << std::endl;
    break;
  }
});

#endif