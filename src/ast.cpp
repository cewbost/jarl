#include "ast.h"

#include <cassert>

ASTNode::ASTNode(ASTNodeType type, std::pair<uint16_t, uint16_t> pos)
: type(type), pos(pos){
  this->children.first = nullptr;
  this->children.second = nullptr;
}
ASTNode::ASTNode(ASTNodeType type, ASTNode* child, std::pair<uint16_t, uint16_t> pos)
: type(type), pos(pos){
  this->child = child;
}
ASTNode::ASTNode(
  ASTNodeType type,
  ASTNode* child1,
  ASTNode* child2,
  std::pair<uint16_t, uint16_t> pos
): type(type), pos(pos){
  this->children.first = child1;
  this->children.second = child2;
}

ASTNode::ASTNode(ASTNodeType type, Int i, std::pair<uint16_t, uint16_t> pos)
: type(type), pos(pos){
  this->int_value = i;
}
ASTNode::ASTNode(ASTNodeType type, Float f, std::pair<uint16_t, uint16_t> pos)
: type(type), pos(pos){
  this->float_value = f;
}
ASTNode::ASTNode(ASTNodeType type, String* s, std::pair<uint16_t, uint16_t> pos)
: type(type), pos(pos){
  this->string_value = s;
}
ASTNode::ASTNode(ASTNodeType type, bool b, std::pair<uint16_t, uint16_t> pos)
: type(type), pos(pos){
  this->bool_value = b;
}
ASTNode::ASTNode(ASTNodeType type, const char* s, std::pair<uint16_t, uint16_t> pos)
: type(type), pos(pos){
  this->c_str_value = s;
}

ASTNode::~ASTNode(){
  switch(
    static_cast<ASTNodeType>(
      static_cast<unsigned>(this->type) & ~0xfff
    )
  ){
  case ASTNodeType::Leaf:
    break;
  case ASTNodeType::OneChild:
    delete this->child;
    break;
  case ASTNodeType::TwoChildren:
    delete this->children.first;
    delete this->children.second;
    break;
  default:
    assert(false);
  }
}

//ExprListIterator
ASTNode::ExprListIterator::ExprListIterator(ASTNode* c){
  if(c && c->type == ASTNodeType::Nop)
    this->current = nullptr;
  else
    this->current = c;
}

bool ASTNode::ExprListIterator::operator!=(const ExprListIterator& other){
  return this->current != other.current;
}
ASTNode* ASTNode::ExprListIterator::operator*(){
  if(this->current->type == ASTNodeType::ExprList)
    return this->current->children.first;
  else
    return this->current;
}
ASTNode* ASTNode::ExprListIterator::operator->(){
  if(this->current->type == ASTNodeType::ExprList)
    return this->current->children.first;
  else
    return this->current;
}
ASTNode::ExprListIterator ASTNode::ExprListIterator::operator++(){
  if(this->current->type == ASTNodeType::ExprList)
    this->current = this->current->children.second;
  else
    this->current = nullptr;
  return *this;
}
ASTNode::ExprListIterator ASTNode::ExprListIterator::operator++(int){
  auto temp = *this;
  if(this->current->type == ASTNodeType::ExprList)
    this->current = this->current->children.second;
  else
    this->current = nullptr;
  return temp;
}

#ifndef NDEBUG
std::string ASTNode::toStrDebug() const {
  return this->toStrDebug(0);
}
std::string ASTNode::toStrDebug(int indent) const {
  
  std::string ret = "";
  for(int i = 0; i < indent; ++i) ret += "| ";
  
  switch(this->type){
  case ASTNodeType::Nop:
    ret += "nop"; break;
  case ASTNodeType::Null:
    ret += "null"; break;
  case ASTNodeType::Int:
    ret += "int"; break;
  case ASTNodeType::Float:
    ret += "float"; break;
  case ASTNodeType::Bool:
    ret += "bool"; break;
  case ASTNodeType::String:
    ret += "string"; break;
  case ASTNodeType::Identifier:
    ret += "identifier"; break;
  case ASTNodeType::Neg:
    ret += "-"; break;
  case ASTNodeType::Not:
    ret += "not"; break;
  case ASTNodeType::And:
    ret += "and"; break;
  case ASTNodeType::Or:
    ret += "or"; break;
  case ASTNodeType::Assign:
    ret += "="; break;
  case ASTNodeType::AddAssign:
    ret += "+="; break;
  case ASTNodeType::SubAssign:
    ret += "-="; break;
  case ASTNodeType::MulAssign:
    ret += "*="; break;
  case ASTNodeType::DivAssign:
    ret += "/="; break;
  case ASTNodeType::ModAssign:
    ret += "%="; break;
  case ASTNodeType::AppendAssign:
    ret += "++="; break;
  case ASTNodeType::Insert:
    ret += "<-"; break;
  case ASTNodeType::Add:
    ret += "+"; break;
  case ASTNodeType::Sub:
    ret += "-"; break;
  case ASTNodeType::Mul:
    ret += "*"; break;
  case ASTNodeType::Div:
    ret += "/"; break;
  case ASTNodeType::Mod:
    ret += "%"; break;
  case ASTNodeType::Append:
    ret += "++"; break;
  case ASTNodeType::Cmp:
    ret += "<=>"; break;
  case ASTNodeType::Eq:
    ret += "=="; break;
  case ASTNodeType::Neq:
    ret += "!="; break;
  case ASTNodeType::Gt:
    ret += ">"; break;
  case ASTNodeType::Lt:
    ret += "<"; break;
  case ASTNodeType::Geq:
    ret += ">="; break;
  case ASTNodeType::Leq:
    ret += "<="; break;
  case ASTNodeType::In:
    ret += "in"; break;
  case ASTNodeType::Apply:
    ret += "apply"; break;
  case ASTNodeType::Seq:
    ret += "seq"; break;
  case ASTNodeType::If:
    ret += "if"; break;
  case ASTNodeType::Else:
    ret += "else"; break;
  case ASTNodeType::While:
    ret += "while"; break;
  case ASTNodeType::For:
    ret += "for"; break;
  case ASTNodeType::CodeBlock:
    ret += "code block"; break;
  case ASTNodeType::Index:
    ret += "[]"; break;
  case ASTNodeType::ExprList:
    ret += "expr list"; break;
  case ASTNodeType::KeyValuePair:
    ret += "key value pair"; break;
  case ASTNodeType::Array:
    ret += "array"; break;
  case ASTNodeType::Table:
    ret += "table"; break;
  case ASTNodeType::Range:
    ret += "range"; break;
  case ASTNodeType::Function:
    ret += "function"; break;
  case ASTNodeType::Var:
    ret += "var"; break;
  case ASTNodeType::Print:
    ret += "print"; break;
  case ASTNodeType::Assert:
    ret += "assert"; break;
  case ASTNodeType::LexError:
    ret += "lex error"; break;
  case ASTNodeType::ParseError:
    ret += "parse error"; break;
  default:
    ret += "unidentified ast node"; break;
  }
  
  switch(
    static_cast<ASTNodeType>(
      static_cast<unsigned>(this->type) & ~0xfff
    )
  ){
  case ASTNodeType::Leaf:
    switch(this->type){
    case ASTNodeType::Int:
      ret += ": ";
      ret += std::to_string(this->int_value);
      break;
    case ASTNodeType::Float:
      ret += ": ";
      ret += std::to_string(this->float_value);
      break;
    case ASTNodeType::Bool:
      ret += ": ";
      ret += this->bool_value? "true" : "false";
      break;
    case ASTNodeType::String:
      ret += ": ";
      ret += unlexString(this->string_value->str());
      break;
    case ASTNodeType::Identifier:
      ret += ": ";
      ret += this->string_value->str();
      break;
    default:
      break;
    }
    ret += "\n";
    break;
  case ASTNodeType::OneChild:
    ret += "\n";
    ret += this->child->toStrDebug(indent + 1);
    break;
  case ASTNodeType::TwoChildren:
    ret += "\n";
    ret += this->children.first->toStrDebug(indent + 1);
    ret += this->children.second->toStrDebug(indent + 1);
    break;
  default:
    assert(false);
  }
  return ret;
}
#endif
