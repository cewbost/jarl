#include "syntax_checker.h"

#include <cassert>

void SyntaxChecker::expectedValueError_(ASTNode* node){
  errors_->emplace_back(dynSprintf(
    "line %d: Expected value expression.",
    node->pos.first
  ));
}
void SyntaxChecker::expectedRValueError_(ASTNode* node){
  errors_->emplace_back(dynSprintf(
    "line %d: Expected r-value expression.",
    node->pos.first
  ));
}
void SyntaxChecker::expectedKeyValuePairError_(ASTNode* node){
  errors_->emplace_back(dynSprintf(
    "line %d: Expected key-value-pair.",
    node->pos.first
  ));
}

void SyntaxChecker::validateSyntax(ASTNode* node){
  switch(
    static_cast<ASTNodeType>(
      static_cast<unsigned>(node->type) & ~0xfff
    )
  ){
  case ASTNodeType::Leaf:
    break;
  case ASTNodeType::OneChild:
    validateSyntax(node->child);
    break;
  case ASTNodeType::TwoChildren:
    validateSyntax(node->children.first);
    validateSyntax(node->children.second);
    break;
  }
  
  switch(static_cast<ASTNodeType>(static_cast<unsigned>(node->type) & ~0xff)){
  case ASTNodeType::Error:
    assert(false);
    break;
  
  case ASTNodeType::Value:
    if(node->type == ASTNodeType::Identifier){
      node->flags = ASTNodeFlags::RValue;
    }else{
      node->flags = ASTNodeFlags::Value;
    }
    break;
  
  case ASTNodeType::UnaryExpr:
    if(!node->child->isValue()){
      expectedValueError_(node->child);
    }
    node->flags = ASTNodeFlags::Value;
    break;
  
  case ASTNodeType::BinaryExpr:
    if(!node->children.first->isValue()){
      expectedValueError_(node->children.first);
      break;
    }
    if(!node->children.second->isValue()){
      expectedValueError_(node->children.second);
      break;
    }
    node->flags = ASTNodeFlags::Value;
    break;
  
  case ASTNodeType::BranchExpr:
    switch(node->type){
    case ASTNodeType::And:
    case ASTNodeType::Or:
      if(!node->children.first->isValue()){
        expectedValueError_(node->children.first);
      }
      if(!node->children.second->isValue()){
        expectedValueError_(node->children.second);
      }
      node->flags = ASTNodeFlags::Value;
      break;
    case ASTNodeType::If:
      if(!node->children.first->isValue()){
        expectedValueError_(node->children.first);
      }
      if(node->children.second->type == ASTNodeType::Else){
        if(node->children.second->children.first->isValue()
        && node->children.second->children.second->isValue()){
          node->flags = ASTNodeFlags::Value;
        }else{
          node->flags = ASTNodeFlags::None;
        }
      }else{
        node->flags = ASTNodeFlags::None;
      }
      break;
    }
    break;
  
  case ASTNodeType::AssignExpr:
    if(!node->children.first->isRValue()){
      expectedRValueError_(node->children.first);
    }
    if(!node->children.second->isValue()){
      expectedValueError_(node->children.second);
    }
    node->flags = ASTNodeFlags::None;
    break;
  
  default:
    switch(node->type){
    case ASTNodeType::CodeBlock:
      if(node->child->isValue()){
        node->flags = ASTNodeFlags::Value;
      }else{
        node->flags = ASTNodeFlags::None;
      }
      break;
    case ASTNodeType::Seq:
      if(node->children.second->isValue()){
        node->flags = ASTNodeFlags::Value;
      }else{
        node->flags = ASTNodeFlags::None;
      }
      break;
    case ASTNodeType::Array:
      if(node->child->type != ASTNodeType::Nop){
        for(auto it = node->child->exprListIterator(); it != nullptr; ++it){
          if(it->type == ASTNodeType::Nop) continue;
          else if(!it->isValue()) expectedValueError_(it.get());
        }
      }
      node->flags = ASTNodeFlags::Value;
      break;
    case ASTNodeType::Table:
      if(node->child->type != ASTNodeType::Nop){
        for(auto it = node->child->exprListIterator(); it != nullptr; ++it){
          if(it->type == ASTNodeType::KeyValuePair){
            if(!it->children.first->isValue()){
              expectedValueError_(it->children.first);
            }
            if(!it->children.second->isValue()){
              expectedValueError_(it->children.second);
            }
          }else if(it->type == ASTNodeType::Nop) continue;
          else expectedKeyValuePairError_(it.get());
        }
      }
      node->flags = ASTNodeFlags::Value;
      break;
    case ASTNodeType::Function:
    case ASTNodeType::Recurse:
    case ASTNodeType::Call:
      node->flags = ASTNodeFlags::Value;
      break;
    case ASTNodeType::Index:
      node->flags = ASTNodeFlags::RValue;
      break;
    case ASTNodeType::While:
    case ASTNodeType::For:
    case ASTNodeType::Var:
    case ASTNodeType::Return:
    case ASTNodeType::Print:
    case ASTNodeType::Assert:
    case ASTNodeType::Nop:
      node->flags = ASTNodeFlags::None;
      break;
    }
  }
}
