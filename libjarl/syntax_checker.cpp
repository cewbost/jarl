#include "syntax_checker.h"

#include <cassert>

void SyntaxChecker::expectValue_(ASTNode* node){
  if(!node->isValue()){
    expectedValueError_(node);
  }
}
void SyntaxChecker::expectValueNop_(ASTNode* node){
  if(node->type != ASTNodeType::Nop && !node->isValue()){
    expectedValueError_(node);
  }
}
void SyntaxChecker::expectLValue_(ASTNode* node){
  if(!node->isLValue()){
    errors_->emplace_back(dynSprintf(
      "line %d: Expected l-value expression.",
      node->pos.first
    ));
  }
}
void SyntaxChecker::expectIdentifier_(ASTNode* node){
  if(node->type != ASTNodeType::Identifier){
    errors_->emplace_back(dynSprintf(
      "line %d: Expected identifier.",
      node->pos.first
    ));
  }
}
void SyntaxChecker::expectValueList_(ASTNode* node){
  for(auto it = node->exprListIterator(); it != nullptr; ++it){
    expectValue_(it.get());
  }
}

void SyntaxChecker::expectedValueError_(ASTNode* node){
  errors_->emplace_back(dynSprintf(
    "line %d: Expected value expression.",
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
      node->flags = ASTNodeFlags::LValue;
    }else{
      node->flags = ASTNodeFlags::Value;
    }
    break;
  
  case ASTNodeType::UnaryExpr:
    if(node->type == ASTNodeType::Move){
      expectLValue_(node->child);
    }else{
      expectValue_(node->child);
    }
    node->flags = ASTNodeFlags::Value;
    break;
  
  case ASTNodeType::BinaryExpr:
    expectValue_(node->children.first);
    expectValue_(node->children.second);
    node->flags = ASTNodeFlags::Value;
    break;
  
  case ASTNodeType::BranchExpr:
    switch(node->type){
    case ASTNodeType::And:
    case ASTNodeType::Or:
      expectValue_(node->children.first);
      expectValue_(node->children.second);
      node->flags = ASTNodeFlags::Value;
      break;
    case ASTNodeType::If:
      expectValue_(node->children.first);
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
    expectLValue_(node->children.first);
    expectValue_(node->children.second);
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
        expectValueNop_(it.get());
      }
      node->flags = ASTNodeFlags::Value;
      break;
    case ASTNodeType::Table:
      for(auto it = node->child->exprListIterator(); it != nullptr; ++it){
        if(it->type == ASTNodeType::KeyValuePair){
          expectValue_(it->children.first);
          expectValue_(it->children.second);
        }else if(it->type == ASTNodeType::Nop) continue;
        else expectedKeyValuePairError_(it.get());
      }
      node->flags = ASTNodeFlags::Value;
      break;
    case ASTNodeType::Function:
      for(auto it = node->children.first->exprListIterator(); it != nullptr; ++it){
        expectIdentifier_(it.get());
      }
      node->flags = ASTNodeFlags::Value;
      break;
    case ASTNodeType::Recurse:
      expectValueList_(node->child);
      node->flags = ASTNodeFlags::Value;
      break;
    case ASTNodeType::Call:
      expectValue_(node->children.first);
      expectValueList_(node->children.second);
      node->flags = ASTNodeFlags::Value;
      break;
    case ASTNodeType::Index:
      expectValue_(node->children.first);
      if(node->children.second->type == ASTNodeType::ExprList){
        if(node->children.second->children.first->type == ASTNodeType::ExprList){
          expectedValueError_(node->children.second->children.first);
        }
        if(node->children.second->children.second->type == ASTNodeType::ExprList){
          expectedValueError_(node->children.second->children.second);
        }
        expectValueNop_(node->children.second->children.first);
        expectValueNop_(node->children.second->children.second);
      }else expectValue_(node->children.second);
      node->flags = ASTNodeFlags::LValue;
      break;
    case ASTNodeType::While:
      expectValue_(node->children.first);
      node->flags = ASTNodeFlags::None;
      break;
    case ASTNodeType::For: {
        ASTNode* stepper = node->children.first;
        if(stepper->type == ASTNodeType::ExprList){
          expectIdentifier_(stepper->children.first);
          stepper = stepper->children.second;
        }
        if(stepper->type != ASTNodeType::In){
          errors_->emplace_back(dynSprintf(
            "line %d: Expected in expression.",
            stepper
          ));
          break;
        }
        expectIdentifier_(stepper->children.first);
        expectValue_(stepper->children.second);
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
      expectLValue_(node->child->children.first);
      expectValue_(node->child->children.second);
      node->flags = ASTNodeFlags::None;
      break;
    case ASTNodeType::Return:
      expectValueNop_(node->child);
      node->flags = ASTNodeFlags::None;
      break;
    case ASTNodeType::Print:
      expectValueList_(node->child);
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
