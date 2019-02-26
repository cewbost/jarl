#include "context.h"

#include "ast.h"
#include "common.h"

#include <cassert>

using namespace Context;

struct ThreadingContext {
  
  Errors* errors;
  
  unsigned variables;
  AttributeSet attributes;
  
  void threadAST(ASTNode*);
};

void ThreadingContext::threadAST(ASTNode* node){
  switch(static_cast<ASTNodeType>(static_cast<unsigned>(node->type) & ~0xff)){
  case ASTNodeType::Error:
    assert(false);
  case ASTNodeType::Value:
    if(node->type == ASTNodeType::Identifier){
      auto var = 0;
      node->context = std::make_unique<AttributeSet>(
        this->attributes.forVariable(var)
      );
      attributes.setLastRead(var, node);
    }
    break;
  case ASTNodeType::UnaryExpr:
    threadAST(node->child);
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
    break;
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
    if(attrib.type == Attribute::Type::LastRead && attrib.var == var){
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
  
  void AttributeSet::addNextRead(unsigned var, ASTNode* where){
    assert(
      std::none_of(this->begin(), this->end(),
      [=](const Attribute& attrib) -> bool{
      return
        attrib.var  == var &&
        attrib.node == where &&
        attrib.type == Attribute::Type::NextRead;
    }));
    this->push_back(Attribute{
      .type = Attribute::Type::NextRead,
      .var  = var,
      .node = where
    });
  }
  
  //global functions
  void addContext(ASTNode* ast, std::vector<std::unique_ptr<char[]>>* errors){
    ThreadingContext context{
      .errors = errors
    };
    context.threadAST(ast);
  }
}