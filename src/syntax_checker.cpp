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
void SyntaxChecker::expectedIdentifierError_(ASTNode* node){
  errors_->emplace_back(dynSprintf(
    "line %d: Expected identifier.",
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
      for(auto it = node->child->exprListIterator(); it != nullptr; ++it){
        if(it->type == ASTNodeType::Nop) continue;
        else if(!it->isValue()) expectedValueError_(it.get());
      }
      node->flags = ASTNodeFlags::Value;
      break;
    case ASTNodeType::Table:
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
      node->flags = ASTNodeFlags::Value;
      break;
    case ASTNodeType::Function:
      for(auto it = node->children.first->exprListIterator(); it != nullptr; ++it){
        if(it->type != ASTNodeType::Identifier){
          expectedIdentifierError_(it.get());
        }
      }
      node->flags = ASTNodeFlags::Value;
      break;
    case ASTNodeType::Recurse:
      for(auto it = node->child->exprListIterator(); it != nullptr; ++it){
        if(!it->isValue()){
          expectedValueError_(it.get());
        }
      }
      node->flags = ASTNodeFlags::Value;
      break;
    case ASTNodeType::Call:
      if(!node->children.first->isValue()){
        expectedValueError_(node->children.first);
      }
      for(auto it = node->child->exprListIterator(); it != nullptr; ++it){
        if(!it->isValue()){
          expectedValueError_(it.get());
        }
      }
      node->flags = ASTNodeFlags::Value;
      break;
    case ASTNodeType::Index:
      if(!node->children.first->isValue()){
        expectedValueError_(node->children.first);
      }
      if(node->children.second->type == ASTNodeType::ExprList){
        if(node->children.second->children.first->type != ASTNodeType::Nop
        && !node->children.second->children.first->isValue()){
          expectedValueError_(node->children.second->children.first);
        }
        if(node->children.second->children.second->type != ASTNodeType::Nop
        && !node->children.second->children.second->isValue()){
          expectedValueError_(node->children.second->children.second);
        }
      }else if(!node->children.second->isValue()){
        expectedValueError_(node->children.second);
      }
      node->flags = ASTNodeFlags::RValue;
      break;
    case ASTNodeType::While:
      if(!node->children.first->isValue()){
        expectedValueError_(node->children.first);
      }
      node->flags = ASTNodeFlags::None;
      break;
    case ASTNodeType::For: {
        ASTNode* stepper = node->children.first;
        if(stepper->type == ASTNodeType::ExprList){
          if(stepper->children.first->type != ASTNodeType::Identifier){
            expectedIdentifierError_(stepper->children.first);
          }
          stepper = stepper->children.second;
        }
        if(stepper->type != ASTNodeType::In){
          errors_->emplace_back(dynSprintf(
            "line %d: Expected in expression.",
            stepper
          ));
          break;
        }
        if(stepper->children.first->type != ASTNodeType::Identifier){
          expectedIdentifierError_(stepper->children.first);
        }
        if(!stepper->children.second->isValue()){
          expectedValueError_(stepper->children.second);
        }
      }
      node->flags = ASTNodeFlags::None;
      break;
    case ASTNodeType::Var:
      if(node->child->type != ASTNodeType::Assign){
        errors_->emplace_back(dynSprintf(
          "line %d: Expected assignment expression.",
          node->child
        ));
        break;
      }
      if(!node->child->children.first->isRValue()){
        expectedRValueError_(node->child->children.first);
      }
      if(!node->child->children.first->isValue()){
        expectedValueError_(node->child->children.second);
      }
      node->flags = ASTNodeFlags::None;
      break;
    case ASTNodeType::Return:
      if(node->child->type != ASTNodeType::Nop
      && !node->child->isValue()){
        expectedValueError_(node->child);
      }
      node->flags = ASTNodeFlags::None;
      break;
    case ASTNodeType::Print:
      for(auto it = node->child->exprListIterator(); it != nullptr; ++it){
        if(!it->isValue()){
          expectedValueError_(it.get());
        }
      }
      node->flags = ASTNodeFlags::None;
      break;
    case ASTNodeType::Assert:
      if(node->child->type != ASTNodeType::ExprList){
        errors_->emplace_back(dynSprintf(
          "line %d: Expected expression list",
          node->child
        ));
        break;
      }
      if(node->child->children.first->type == ASTNodeType::ExprList
      || !node->child->children.first->isValue()){
        expectedValueError_(node->child->children.first);
      }
      if(node->child->children.second->type == ASTNodeType::ExprList
      || !node->child->children.second->isValue()){
        expectedValueError_(node->child->children.second);
      }
      node->flags = ASTNodeFlags::None;
      break;
    case ASTNodeType::Nop:
      node->flags = ASTNodeFlags::None;
      break;
    }
  }
}
