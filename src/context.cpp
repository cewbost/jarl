#include "context.h"

#include "ast.h"
#include "common.h"

#include <cassert>

using namespace Context;

struct FunctionContext {
  
  Errors* errors;
  
  std::unique_ptr<VarAllocMap> var_allocs;
  
  AttributeSet attributes;

  ASTNode* func_node = nullptr;
  FunctionContext* parentContext = nullptr;
  
  void threadAST(ASTNode*);
  
private:
  class AllocContextGuard {
    std::unique_ptr<VarAllocMap>& alloc_map_;
  public:
    AllocContextGuard(std::unique_ptr<VarAllocMap>& alloc_map)
    : alloc_map_(alloc_map){
      alloc_map_.reset(new VarAllocMap(alloc_map_.get()));
    }
    ~AllocContextGuard(){
      alloc_map_.reset(alloc_map_->steal_delegate());
    }
  };

  void threadModifyingExpr(ASTNode*, bool readwrite = false);
  bool captureVariable(String* name);
  
public:
  AllocContextGuard newScope(){
    return AllocContextGuard(this->var_allocs);
  }
};

void FunctionContext::threadAST(ASTNode* node){
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
  case ASTNodeType::OneChild:
    switch(node->type){
    case ASTNodeType::CodeBlock:
      {
        auto guard = this->newScope();
        threadAST(node->children.first);
      }
      break;
    case ASTNodeType::Var:
      {
        auto subnode = node->child;
        threadAST(subnode->children.second);
        
        assert(subnode->children.first->type == ASTNodeType::Identifier);
        auto it = var_allocs->direct()
          .find(subnode->children.first->string_value);
        ASTNode* var;
        if(it == var_allocs->direct().end()){
          var = subnode->children.first;
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
      threadAST(node->child);
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
    threadModifyingExpr(node->children.first, node->type == ASTNodeType::Assign);
    break;
  case ASTNodeType::TwoChildren:
  case ASTNodeType::BranchExpr:
    switch(node->type){
    case ASTNodeType::Seq:
      threadAST(node->children.first);
      threadAST(node->children.second);
      break;
    case ASTNodeType::While:
    case ASTNodeType::For:
      {
        threadAST(node->children.first);
        auto cloned_attributes = this->attributes;
        threadAST(node->children.second);
        threadAST(node->children.first);
        this->attributes.merge(cloned_attributes);
        cloned_attributes = this->attributes;
        threadAST(node->children.second);
        this->attributes.merge(cloned_attributes);
      }
      break;
    case ASTNodeType::If:
      {
        threadAST(node->children.first);
        auto cloned_attributes = this->attributes;
        if(node->children.second->type == ASTNodeType::Else){
          threadAST(node->children.second->children.first);
          std::swap(this->attributes, cloned_attributes);
          threadAST(node->children.second->children.second);
        }else{
          threadAST(node->children.second);
        }
        this->attributes.merge(cloned_attributes);
      }
      break;
    case ASTNodeType::And:
    case ASTNodeType::Or:
      {
        threadAST(node->children.first);
        auto cloned_attributes = this->attributes;
        threadAST(node->children.second);
        this->attributes.merge(cloned_attributes);
      }
      break;
    case ASTNodeType::Index:
    case ASTNodeType::Call:
      threadAST(node->children.first);
      threadAST(node->children.second);
      break;
    default:
      assert(false);
    }
    break;
  case ASTNodeType::FunctionNode:
    switch(node->type){
    case ASTNodeType::Function:
      {
        auto new_alloc_map = std::make_unique<VarAllocMap>(nullptr);
        for(
          auto it = node->function.data->arguments->exprListIterator();
          it != nullptr; ++it
        ){
          assert(it->type == ASTNodeType::Identifier);
          if(new_alloc_map->contains(it->string_value)){
            errors->emplace_back(dynSprintf(
              "line %d: Multiple arguments with the same name '%s'",
              node->pos.first,
              it->string_value->str()
            ));
          }else{
            new_alloc_map->direct()[it->string_value] = it.get();
          }
        }

        FunctionContext context{
          .errors         = errors,
          .var_allocs     = std::move(new_alloc_map),
          .func_node      = node,
          .parentContext  = this
        };
        context.threadAST(node->function.body);
      }
      break;
    default:
      assert(false);
    }
    break;
  default:
    assert(false);
  }
}

void FunctionContext::threadModifyingExpr(ASTNode* node, bool readwrite){
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

ASTNode* FunctionContext::captureVariable(String* name){
  if(this->parentContext == nullptr) return nullptr;

  auto allocs = this->parentContext->var_allocs.get();
  
  if(auto it = allocs->find(name); it == allocs->end()){
    //todo
  }else{
    auto var = it->second;
    auto new_var = std::make_unique<ASTNode>(
      ASTNodeType::Identifier,
      name,
      this->func_node->pos
    );
    auto res = new_var.get();
    auto& func_data = this->func_node->function.data;
    if(func_data->captures == nullptr){
      func_data->captures = std::move(new_var);
    }else{
      func_data->captures.reset(new ASTNode(
        ASTNodeType::ExprList,
        new_var.release(),
        func_data->captures.release()
      ));
    }
    return res;
  }

  return nullptr;
}

namespace Context {
  
  //AttributeSet
  AttributeSet AttributeSet::forVariable(ASTNode* var)const{
    AttributeSet ret;
    for(const Attribute& attrib: *this) if(attrib.var == var){
      ret.push_back(attrib);
    }
    return ret;
  }

  void AttributeSet::setLastRead(ASTNode* var, ASTNode* where){
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
  
  void AttributeSet::setLastWrite(ASTNode* var, ASTNode* where){
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
  
  void AttributeSet::setLastReadWrite(ASTNode* var, ASTNode* where){
    this->setLastWrite(var, where);
    this->push_back(Attribute{
      .type = Attribute::Type::LastRead,
      .var  = var,
      .node = where
    });
  }
  
  void AttributeSet::addNextRead(ASTNode* var, ASTNode* where){
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
  
  void AttributeSet::addNextWrite(ASTNode* var, ASTNode* where){
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
  
  void AttributeSet::merge(const AttributeSet& other){
    auto indexes = std::make_unique<unsigned[]>(other.size());
    auto stepper = indexes.get();
    for(int i = 0; i < other.size(); ++i){
      if(std::any_of(this->begin(), this->end(), [&](const auto& attr){
        return attr == other[i];
      })) *(stepper++) = i;
    }
    for(auto p = indexes.get(); p < stepper; ++p){
      this->push_back(other[*p]);
    }
  }
  
  //global functions
  void addContext(ASTNode* ast, std::vector<std::unique_ptr<char[]>>* errors){
    FunctionContext context{
      .errors = errors,
      .var_allocs = std::make_unique<VarAllocMap>(nullptr)
    };
    context.threadAST(ast);
  }
}
