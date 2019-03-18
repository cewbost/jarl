#include "context.h"

#include "ast.h"
#include "common.h"

#include <cassert>

using namespace Context;

struct ThreadingContext {
  
  Errors* errors;
  
  unsigned locals;
  std::unique_ptr<VarAllocMap> var_allocs;
  
  AttributeSet attributes;
  
  void threadAST(ASTNode*);
  void threadModifyingExpr(ASTNode*, bool readwrite = false);
};

void ThreadingContext::threadAST(ASTNode* node){
  switch(static_cast<ASTNodeType>(static_cast<unsigned>(node->type) & ~0xff)){
  case ASTNodeType::Error:
    assert(false);
  case ASTNodeType::Value:
    if(node->type == ASTNodeType::Identifier){
      auto it = var_allocs->find(node->string_value);
      if(it == var_allocs->end()){
        errors->emplace_back(dynSprintf(
          "line %d: Undeclared identifier '%s'",
          node->pos.first,
          node->string_value->str()
        ));
        return;
      }
      auto var = it->second;
      attributes.setLastRead(var, node);
      node->context = std::make_unique<AttributeSet>(
        this->attributes.forVariable(var)
      );
    }
    break;
  case ASTNodeType::UnaryExpr:
    if(node->type == ASTNodeType::Move){
      threadModifyingExpr(node->child, true);
    }else{
      threadAST(node->child);
    }
    break;
  case ASTNodeType::BinaryExpr:
    threadAST(node->children.first);
    threadAST(node->children.second);
    break;
  case ASTNodeType::AssignExpr:
    threadAST(node->children.second);
    threadAST(node->children.first);
    break;
  default:
    switch(node->type){
    case ASTNodeType::Var:
      {
        auto subnode = node->child;
        threadAST(subnode->children.second);
        
        assert(subnode->children.first->type == ASTNodeType::Identifier);
        auto it = var_allocs->direct()
          .find(subnode->children.first->string_value);
        unsigned var;
        if(it == var_allocs->direct().end()){
          var = locals++;
          var_allocs->direct()[subnode->children.first->string_value] = var;
        }else{
          var = it->second;
        }
        attributes.setLastWrite(var, subnode->children.first);
        node->children.first->context = std::make_unique<AttributeSet>(
          this->attributes.forVariable(var)
        );
      }
      break;
    default:
      assert(false);
    }
    break;
  }
}

void ThreadingContext::threadModifyingExpr(ASTNode* node, bool readwrite){
  switch(node->type){
  case ASTNodeType::Identifier:
    if(
      auto it = var_allocs->find(node->string_value);
      it == var_allocs->end()
    ){
      errors->emplace_back(dynSprintf(
        "line %d: Undeclared identifier '%s'",
        node->pos,
        node->string_value->str()
      ));
    }else{
      auto var = it->second;
      if(readwrite) attributes.setLastReadWrite(var, node);
      else attributes.setLastRead(var, node);
      node->context = std::make_unique<AttributeSet>(
        this->attributes.forVariable(var)
      );
    }
    break;
  case ASTNodeType::Index:
    threadAST(node->children.second);
    threadModifyingExpr(node->children.first, readwrite);
    break;
  default:
    assert(false);
  }
}

namespace Context {
  
  //AttributeSet
  AttributeSet AttributeSet::forVariable(unsigned var){
    AttributeSet ret;
    for(const Attribute& attrib: *this) if(attrib.var == var){
      ret.push_back(attrib);
    }
    return ret;
  }
  
  void AttributeSet::setLastRead(unsigned var, ASTNode* where){
    for(const Attribute& attrib: *this)
    if(attrib.var == var){
      assert(attrib.node->context != nullptr);
      attrib.node->context->addNextRead(var, where);
    }
    this->erase(
      std::remove_if(this->begin(), this->end(), [=](const Attribute& attrib){
        return attrib.type == Attribute::Type::LastRead && attrib.var == var;
      }),
      this->end()
    );
    this->push_back(Attribute{
      .type = Attribute::Type::LastRead,
      .var  = var,
      .node = where
    });
  }
  
  void AttributeSet::setLastWrite(unsigned var, ASTNode* where){
    for(const Attribute& attrib: *this)
    if(attrib.var == var){
      assert(attrib.node->context != nullptr);
      attrib.node->context->addNextWrite(var, where);
    }
    this->erase(
      std::remove_if(this->begin(), this->end(), [=](const Attribute& attrib){
        return
          attrib.var == var && (
          attrib.type == Attribute::Type::LastWrite || 
          attrib.type == Attribute::Type::LastRead
        );
      }),
      this->end()
    );
    this->push_back(Attribute{
      .type = Attribute::Type::LastWrite,
      .var  = var,
      .node = where
    });
  }
  
  void AttributeSet::setLastReadWrite(unsigned var, ASTNode* where){
    this->setLastWrite(var, where);
    this->push_back(Attribute{
      .type = Attribute::Type::LastRead,
      .var  = var,
      .node = where
    });
  }
  
  void AttributeSet::addNextRead(unsigned var, ASTNode* where){
    assert(std::none_of(
      this->begin(), this->end(),
      [=](const Attribute& attrib)->bool{
        return
          attrib.var  == var &&
          attrib.node == where &&
          attrib.type == Attribute::Type::NextRead;
      }
    ));
    this->push_back(Attribute{
      .type = Attribute::Type::NextRead,
      .var  = var,
      .node = where
    });
  }
  
  void AttributeSet::addNextWrite(unsigned var, ASTNode* where){
    assert(std::none_of(
      this->begin(), this->end(),
      [=](const Attribute& attrib)->bool{
        return
          attrib.var  == var &&
          attrib.node == where &&
          attrib.type == Attribute::Type::NextWrite;
      }
    ));
    this->push_back(Attribute{
      .type = Attribute::Type::NextWrite,
      .var  = var,
      .node = where
    });
  }
  
  //global functions
  void addContext(ASTNode* ast, std::vector<std::unique_ptr<char[]>>* errors){
    ThreadingContext context{
      .errors = errors,
      .var_allocs = std::make_unique<VarAllocMap>(nullptr)
    };
    context.threadAST(ast);
  }
}
