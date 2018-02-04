#include "ast.h"

ASTNode::ASTNode(ASTNodeType type): type(type){
  this->children[0] = this->children[1] = nullptr;
}
ASTNode::ASTNode(ASTNodeType type, ASTNode* child): type(type){
  this->children[0] = child;
  this->children[1] = nullptr;
}
ASTNode::ASTNode(ASTNodeType type, ASTNode* child1, ASTNode* child2): type(type){
  this->children[0] = child1;
  this->children[1] = child2;
}

ASTNode::ASTNode(ASTNodeType type, Int i): type(type){
  this->int_value = i;
}
ASTNode::ASTNode(ASTNodeType type, Float f): type(type){
  this->float_value = f;
}
ASTNode::ASTNode(ASTNodeType type, String* s): type(type){
  this->string_value = s;
}
ASTNode::ASTNode(ASTNodeType type, bool b): type(type){
  this->bool_value = b;
}

ASTNode::~ASTNode(){
  switch(this->type){
  case ASTNodeType::Int:
  case ASTNodeType::Float:
  case ASTNodeType::Bool:
    break;
  case ASTNodeType::String:
    this->string_value->decRefCount();
    break;
  default:
    delete this->children[0];
    delete this->children[1];
  }
}
