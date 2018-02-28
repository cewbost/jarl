#include "function.h"

#include "vm.h"
#include "delegate_map.h"

#include <limits>
#include <cassert>

constexpr OpCodeType stack_pos_local_ = 0x8000;
constexpr OpCodeType stack_pos_const_ = 0x4000;

namespace std{
  template<>
  struct equal_to<String*>{
    typedef ptr_equal_to<String*> type;
  };
}

void Function::putInstruction_(OpCodeType op, int pos){
  if(this->code_positions_.back().first != pos){
    this->code_positions_.emplace_back(pos, this->code_.size());
  }
  this->code_.push_back(op);
}

void Function::threadAST_(
  ASTNode* node,
  std::vector<std::unique_ptr<char[]>>* errors,
  VarAllocMap* va,
  VarAllocMap* cva,
  VectorSet<TypedValue>* vc,
  int* ss,
  bool ret
){
  switch(static_cast<ASTNodeType>(static_cast<unsigned>(node->type) & ~0xff)){
  case ASTNodeType::Error:
    {
      errors->emplace_back(
        dynSprintf("%d: Compiler error. %s", node->pos.first, node->c_str_value)
      );
    }
    break;
    
  case ASTNodeType::Value:
    if(!ret) break;
    switch(node->type){
    case ASTNodeType::Bool:
      if(node->bool_value){
        this->putInstruction_(Op::PushTrue, node->pos.first);
      }else{
        this->putInstruction_(Op::PushFalse, node->pos.first);
      }
      break;
    case ASTNodeType::Int:
      if(node->int_value <= std::numeric_limits<OpCodeSignedType>::max()
      && node->int_value >= std::numeric_limits<OpCodeSignedType>::min()){
        this->putInstruction_(Op::Push | Op::Extended | Op::Int, node->pos.first);
        this->putInstruction_(
          static_cast<OpCodeType>(node->int_value), node->pos.first
        );
      }else{
        this->putInstruction_(Op::Push | Op::Extended, node->pos.first);
        this->putInstruction_(
          (OpCodeType)(vc->emplace(node->int_value) | stack_pos_const_),
          node->pos.first
        );
      }
      break;
    case ASTNodeType::Float:
      this->putInstruction_(Op::Push | Op::Extended, node->pos.first);
      this->putInstruction_(
        (OpCodeType)(vc->emplace(node->float_value) | stack_pos_const_),
        node->pos.first
      );
      break;
    case ASTNodeType::String:
      this->putInstruction_(Op::Push | Op::Extended, node->pos.first);
      this->putInstruction_(
        (OpCodeType)(vc->emplace(node->string_value) | stack_pos_const_),
        node->pos.first
      );
      break;
    case ASTNodeType::Identifier:
      {
        this->putInstruction_(Op::Push | Op::Extended, node->pos.first);
        auto it = va->find(node->string_value);
        if(it == va->end()){
          it = cva->find(node->string_value);
          
          if(it == cva->end()){
            errors->emplace_back(
              dynSprintf(
                "%d: Compiler error. Undeclared identifier '%s'.",
                node->pos.first,
                node->string_value->str()
              )
            );
            break;
          }
          
          va->base()[node->string_value] = this->arguments;
          this->putInstruction_(this->arguments, node->pos.first);
          ++this->arguments;
        }else{
          this->putInstruction_(it->second, node->pos.first);
        }
      }
      break;
    case ASTNodeType::Null:
      this->putInstruction_(Op::Push, node->pos.first);
      break;
    default:
      assert(false);
    }
    ++(*ss);
    break;
    
  case ASTNodeType::UnaryExpr:
    this->threadAST_(node->child, errors, va, cva, vc, ss);
    switch(node->type){
    case ASTNodeType::Neg:
      this->putInstruction_(Op::Neg, node->pos.first);
      break;
    case ASTNodeType::Not:
      this->putInstruction_(Op::Not, node->pos.first);
      break;
    default:
      assert(false);
    }
    if(!ret){
      this->putInstruction_(Op::Pop, node->pos.first);
      --(*ss);
    }
    break;
    
  case ASTNodeType::BinaryExpr:
    this->threadAST_(node->children.first, errors, va, cva, vc, ss);
    this->threadAST_(node->children.second, errors, va, cva, vc, ss);
    switch(node->type){
    case ASTNodeType::Apply:
      this->putInstruction_(Op::Apply, node->pos.first);
      break;
      
    case ASTNodeType::Add:
      this->putInstruction_(Op::Add, node->pos.first);
      break;
    case ASTNodeType::Sub:
      this->putInstruction_(Op::Sub, node->pos.first);
      break;
    case ASTNodeType::Mul:
      this->putInstruction_(Op::Mul, node->pos.first);
      break;
    case ASTNodeType::Div:
      this->putInstruction_(Op::Div, node->pos.first);
      break;
    case ASTNodeType::Mod:
      this->putInstruction_(Op::Mod, node->pos.first);
      break;
    case ASTNodeType::Append:
      this->putInstruction_(Op::Append, node->pos.first);
      break;
      
    case ASTNodeType::Cmp:
      this->putInstruction_(Op::Cmp, node->pos.first);
      break;
    case ASTNodeType::Eq:
      this->putInstruction_(Op::Eq, node->pos.first);
      break;
    case ASTNodeType::Neq:
      this->putInstruction_(Op::Neq, node->pos.first);
      break;
    case ASTNodeType::Gt:
      this->putInstruction_(Op::Gt, node->pos.first);
      break;
    case ASTNodeType::Lt:
      this->putInstruction_(Op::Lt, node->pos.first);
      break;
    case ASTNodeType::Geq:
      this->putInstruction_(Op::Geq, node->pos.first);
      break;
    case ASTNodeType::Leq:
      this->putInstruction_(Op::Leq, node->pos.first);
      break;
      
    default:
      assert(false);
    }
    if(!ret){
      this->putInstruction_(Op::Pop, node->pos.first);
      *ss -= 2;
    }else --(*ss);
    break;
    
  case ASTNodeType::BranchExpr:
    {
      unsigned jmp_addr;
      this->threadAST_(node->children.first, errors, va, cva, vc, ss);
      
      switch(node->type){
      case ASTNodeType::And:
        this->putInstruction_(Op::Jfsc | Op::Extended, node->pos.first);
        jmp_addr = this->code_.size();
        this->putInstruction_((OpCodeType)0, node->pos.first);
        this->threadAST_(node->children.second, errors, va, cva, vc, ss);
        this->code_[jmp_addr] = (OpCodeType)this->code_.size() - 1;
        
        if(!ret){
          this->putInstruction_(Op::Pop, node->pos.first);
          --(*ss);
        }
        break;
      case ASTNodeType::Or:
        this->putInstruction_(Op::Jtsc | Op::Extended, node->pos.first);
        jmp_addr = this->code_.size();
        this->putInstruction_((OpCodeType)0, node->pos.first);
        this->threadAST_(node->children.second, errors, va, cva, vc, ss);
        this->code_[jmp_addr] = (OpCodeType)this->code_.size() - 1;
        
        if(!ret){
          this->putInstruction_(Op::Pop, node->pos.first);
          --(*ss);
        }
        break;
        
      case ASTNodeType::Conditional:
        assert(node->children.second->type == ASTNodeType::Branch);
        
        this->putInstruction_(Op::Jf | Op::Extended, node->pos.first);
        jmp_addr = this->code_.size();
        this->putInstruction_((OpCodeType)0, node->pos.first);
        --(*ss);
        
        this->threadAST_(
          node->children.second->children.first,
          errors, va, cva, vc, ss, ret
        );
        --(*ss);
        
        this->code_[jmp_addr] = (OpCodeType)this->code_.size() + 1;
        
        if(ret){
          this->putInstruction_(Op::Jmp | Op::Extended, node->pos.first);
          jmp_addr = this->code_.size();
          this->putInstruction_((OpCodeType)0, node->pos.first);
          if(node->children.second->children.second->type == ASTNodeType::Nop){
            this->putInstruction_(Op::Push, node->pos.first);
            ++(*ss);
          }else{
            this->threadAST_(
              node->children.second->children.second,
              errors, va, cva, vc, ss
            );
          }
          this->code_[jmp_addr] = (OpCodeType)this->code_.size() + 1;
        }else if(node->children.second->children.second->type != ASTNodeType::Nop){
          this->putInstruction_(Op::Jmp | Op::Extended, node->pos.first);
          jmp_addr = this->code_.size();
          this->putInstruction_((OpCodeType)0, node->pos.first);
          
          this->threadAST_(
            node->children.second->children.second,
            errors, va, cva, vc, ss, false
          );
          
          this->code_[jmp_addr] = (OpCodeType)this->code_.size() + 1;
        }
        break;
        
      default:
        assert(false);
      }
    }
    break;
    
  case ASTNodeType::AssignExpr:
    {
      if(node->children.first->type != ASTNodeType::Identifier){
        errors->emplace_back(
          dynSprintf("%d: Compiler error. Assignment to rvalue.", node->pos.first)
        );
      }
      
      auto stack_pos = (OpCodeType)(*va)[node->children.first->string_value];
      this->threadAST_(node->children.second, errors, va, cva, vc, ss);
      switch(node->type){
      case ASTNodeType::Assign:
        this->putInstruction_(Op::Write | Op::Extended | Op::Dest, node->pos.first);
        break;
      case ASTNodeType::AddAssign:
        this->putInstruction_(Op::Add | Op::Extended | Op::Dest, node->pos.first);
        break;
      case ASTNodeType::SubAssign:
        this->putInstruction_(Op::Sub | Op::Extended | Op::Dest, node->pos.first);
        break;
      case ASTNodeType::MulAssign:
        this->putInstruction_(Op::Mul | Op::Extended | Op::Dest, node->pos.first);
        break;
      case ASTNodeType::DivAssign:
        this->putInstruction_(Op::Div | Op::Extended | Op::Dest, node->pos.first);
        break;
      case ASTNodeType::ModAssign:
        this->putInstruction_(Op::Mod | Op::Extended | Op::Dest, node->pos.first);
        break;
      case ASTNodeType::AppendAssign:
        this->putInstruction_(Op::Append | Op::Extended | Op::Dest, node->pos.first);
        break;
        
      default:
        assert(false);
      }
      
      this->putInstruction_(stack_pos, node->pos.first);
      if(ret){
        this->putInstruction_(Op::Push | Op::Extended | Op::Dest, node->pos.first);
        this->putInstruction_(stack_pos, node->pos.first);
      }else{
        --(*ss);
      }
    }
    break;
    
  default:
    switch(node->type){
    case ASTNodeType::CodeBlock:
      {
        VarAllocMap* var_allocs = new VarAllocMap(va);
        this->threadAST_(node->children.first, errors, var_allocs, cva, vc, ss);
        var_allocs->set_delegate(nullptr);
        if(ret){
          if(var_allocs->size() > 0){
            if(var_allocs->size() == 1){
              this->putInstruction_(Op::Reduce, node->pos.first);
            }else{
              this->putInstruction_(Op::Reduce | Op::Extended, node->pos.first);
              this->putInstruction_(
                static_cast<OpCodeType>(var_allocs->size()), node->pos.first
              );
            }
            *ss -= var_allocs->size();
          }
        }else{
          if(var_allocs->size() == 0){
            this->putInstruction_(Op::Pop, node->pos.first);
          }else{
            this->putInstruction_(Op::Pop | Op::Extended, node->pos.first);
            this->putInstruction_(
              static_cast<OpCodeType>(var_allocs->size() + 1), node->pos.first
            );
          }
        }
        delete var_allocs;
      }
      break;
      
    case ASTNodeType::Array:
      {
        if(!ret) break;
        
        if(node->child->type != ASTNodeType::Nop){
          if(node->child->type == ASTNodeType::ExprList){
            ASTNode* t = node->child;
            OpCodeType elems = 0;
            
            for(;;){
              this->threadAST_(t->children.first, errors, va, cva, vc, ss);
              ++elems;
              if(t->children.second->type == ASTNodeType::ExprList){
                t = t->children.second;
              }else{
                this->threadAST_(t->children.second, errors, va, cva, vc, ss);
                ++elems;
                break;
              }
            }
            
            this->putInstruction_(
              Op::CreateArray | Op::Extended | Op::Int,
              node->pos.first
            );
            this->putInstruction_(elems, node->pos.first);
            *ss -= elems - 1;
          }else if(node->child->type == ASTNodeType::Range){
            this->threadAST_(node->child->children.first, errors, va, cva, vc, ss);
            this->threadAST_(node->child->children.second, errors, va, cva, vc, ss);
            this->putInstruction_(Op::CreateRange, node->pos.first);
            --(*ss);
          }else{
            this->threadAST_(node->child, errors, va, cva, vc, ss);
            this->putInstruction_(
              Op::CreateArray | Op::Extended | Op::Int,
              node->pos.first
            );
            this->putInstruction_(1, node->pos.first);
          }
        }else{
          this->putInstruction_(Op::CreateArray, node->pos.first);
          ++(*ss);
        }
      }
      break;
      
    case ASTNodeType::While:
      {
        if(ret){
          this->putInstruction_(Op::Push, node->pos.first);
          ++(*ss);
        }
        unsigned begin_addr = this->code_.size() - 1;
        
        this->threadAST_(node->children.first, errors, va, cva, vc, ss);
        
        this->putInstruction_(Op::Jf | Op::Extended, node->pos.first);
        --(*ss);
        unsigned end_jmp_addr = this->code_.size();
        this->putInstruction_((OpCodeType)0, node->pos.first);
        
        this->threadAST_(node->children.second, errors, va, cva, vc, ss, ret);
        if(ret){
          this->putInstruction_(Op::Reduce, node->pos.first);
          --(*ss);
        }
        this->putInstruction_(Op::Jmp | Op::Extended, node->pos.first);
        this->code_[end_jmp_addr] = (OpCodeType)this->code_.size();
        this->putInstruction_((OpCodeType)begin_addr, node->pos.first);
      }
      break;
      
    case ASTNodeType::Function:
      {
        //some checking might be nessecary for the function being well formed
        
        if(!ret) break;
        
        VarAllocMap* var_alloc = new VarAllocMap(nullptr);
        
        OpCodeType std_args = 0;
        if(node->children.first->type != ASTNodeType::Nop){
          ASTNode* t = node->children.first;
          do{
            (*var_alloc)[t->string_branch.value] = std_args;
            ++std_args;
            t = t->string_branch.next;
          }while(t->type != ASTNodeType::Nop);
        }
        
        delete node->children.first;
        node->children.first = nullptr;
        
        auto* func = new Function(node->children.second, errors, var_alloc, va);
        
        node->children.second = nullptr;
        this->putInstruction_(Op::Push | Op::Extended, node->pos.first);
        this->putInstruction_(
          (OpCodeType)vc->emplace(func) | stack_pos_const_,
          node->pos.first
        );
        ++(*ss);
        
        if(func->arguments > std_args){
          VectorMapBase& base = var_alloc->base();
          for(int i = std_args; i < func->arguments; ++i){
            this->putInstruction_(Op::Push | Op::Extended, node->pos.first);
            this->putInstruction_((*va)[base[i].first], node->pos.first);
            this->putInstruction_(Op::Apply | Op::Extended | Op::Int, node->pos.first);
            this->putInstruction_((OpCodeType)std_args, node->pos.first);
          }
        }
        delete var_alloc;
      }
      break;
      
    case ASTNodeType::Seq:
      this->threadAST_(node->children.first, errors, va, cva, vc, ss, false);
      this->threadAST_(node->children.second, errors, va, cva, vc, ss);
      break;
      
    case ASTNodeType::VarDecl:
      {
        auto stack_size = *ss;
        this->threadAST_(node->string_branch.next, errors, va, cva, vc, ss);
        va->direct()[node->string_branch.value]
          = stack_size | stack_pos_local_;
      }
      break;
      
    case ASTNodeType::Print:
      {
        this->threadAST_(node->child, errors, va, cva, vc, ss);
        this->putInstruction_(Op::Print, node->pos.first);
      }
      break;
      
    case ASTNodeType::Index:
      {
        this->threadAST_(node->children.first, errors, va, cva, vc, ss);
        
        if(node->children.second->type == ASTNodeType::Range){
          this->threadAST_(
            node->children.second->children.first,
            errors, va, cva, vc, ss
          );
          this->threadAST_(
            node->children.second->children.second,
            errors, va, cva, vc, ss
          );
          this->putInstruction_(Op::Slice, node->pos.first);
        }else{
          this->threadAST_(node->children.second, errors, va, cva, vc, ss);
          this->putInstruction_(Op::Get, node->pos.first);
        }
        
        if(!ret){
          this->putInstruction_(Op::Pop, node->pos.first);
          *ss -= 2;
        }else --(*ss);
      }
      break;
      
    case ASTNodeType::Nop:
      this->putInstruction_(Op::Push, node->pos.first);
      break;
      
    default:
      assert(false);
    }
  }
}

Function::Function(
  ASTNode* tree,
  std::vector<std::unique_ptr<char[]>>* errors,
  VarAllocMap* var_alloc,
  VarAllocMap* context_vars
){
  //thread AST
  bool var_allocs = false;
  if(var_alloc){
    this->arguments = var_alloc->size();
    var_allocs = true;
  }else{
    this->arguments = 0;
    var_alloc = new VarAllocMap(nullptr);
  }
  
  VectorSet<TypedValue> constants;
  int stack_size = 0;
  
  auto pos = tree->pos.first;
  this->code_positions_.emplace_back(pos, 0);
  this->threadAST_(tree, errors, var_alloc, context_vars, &constants, &stack_size);
  this->putInstruction_(Op::Return, pos);
  
  delete tree;
  if(!var_allocs) delete var_alloc;
  
  //correct stack positions
  int extended = 0;
  for(auto& op: this->code_){
    if(extended == 0){
      if(op & Op::Extended){
        if(op & Op::Int){
          extended = 1;
        }else{
          extended = 2;
        }
      }
    }else if(extended == 1){
      extended = 0;
    }else{
      extended = 0;
      if(op & stack_pos_local_){
        op = (op & ~stack_pos_local_) + constants.size() + this->arguments;
      }else if(op & stack_pos_const_){
        op = (op & ~stack_pos_const_) + this->arguments;
      }
    }
  }
  
  this->values_ = std::move(constants);
}

PartiallyApplied::PartiallyApplied(const Function* func)
: func_(func), args_(func->arguments), nargs(func->arguments){}

bool PartiallyApplied::apply(const TypedValue& val, int bind_pos){
  assert(this->nargs > 0);
  for(auto& slot: this->args_){
    if(slot.type == TypeTag::None){
      if((bind_pos--) == 0){
        slot = val;
        break;
      }
    }
  }
  return --this->nargs == 0;
}
bool PartiallyApplied::apply(TypedValue&& val, int bind_pos){
  assert(this->nargs > 0);
  for(auto& slot: this->args_){
    if(slot.type == TypeTag::None){
      if((bind_pos--) == 0){
        slot = std::move(val);
        break;
      }
    }
  }
  return --this->nargs == 0;
}

int Function::getLine(const OpCodeType* iit) const {
  ptrdiff_t pos = iit - this->code_.data();
  auto it = this->code_positions_.begin();
  ptrdiff_t line = it->first;
  for(;;){
    if(it == this->code_positions_.end()){
      return this->code_positions_.back().second;
    }
    if(it->second > pos) break;
    
    line = it->first;
    ++it;
  }
  return line;
}

#ifndef NDEBUG
std::string Function::opcodesToStrDebug()const{
  using namespace std::string_literals;
  
  std::string ret = "";
  char buffer[7] = "\0";
  
  auto vit = this->values_.begin();
  
  bool extended = false;
  int counter = 0;
  for(auto op: this->code_){
    if(!extended){
      snprintf(buffer, sizeof(buffer), "%4d: ", counter);
      ret += buffer;
      ret += opCodeToStrDebug(op);
      if(op & Op::Extended) extended = true;
      else ret += "\n"s;
    }else{
      ret += " "s + std::to_string(op) + "\n"s;
      extended = false;
    }
    ++counter;
  }
  
  return ret;
}

std::string opCodeToStrDebug(OpCodeType op){
  using namespace std::string_literals;
  
  std::string ret = "";
  
  switch(op & ~Op::Head){
  case Op::Return:
    ret += "return"s;
    break;
  case Op::Nop:
    ret += "nop"s;
    break;
  case Op::Reduce:
    ret += "reduce"s;
    break;
  case Op::Push:
    ret += "push"s;
    break;
  case Op::PushTrue:
    ret += "push true"s;
    break;
  case Op::PushFalse:
    ret += "push false"s;
    break;
  case Op::CreateArray:
    ret += "create array"s;
    break;
  case Op::CreateRange:
    ret += "create range"s;
    break;
  case Op::Pop:
    ret += "pop"s;
    break;
  case Op::Write:
    ret += "write"s;
    break;
  case Op::Add:
    ret += "add"s;
    break;
  case Op::Sub:
    ret += "sub"s;
    break;
  case Op::Mul:
    ret += "mul"s;
    break;
  case Op::Div:
    ret += "div"s;
    break;
  case Op::Mod:
    ret += "mod"s;
    break;
  case Op::Append:
    ret += "append"s;
    break;
  case Op::Apply:
    ret += "apply"s;
    break;
  case Op::Neg:
    ret += "neg"s;
    break;
  case Op::Not:
    ret += "not"s;
    break;
  case Op::Cmp:
    ret += "cmp"s;
    break;
  case Op::Eq:
    ret += "eq"s;
    break;
  case Op::Neq:
    ret += "neq"s;
    break;
  case Op::Gt:
    ret += "gt"s;
    break;
  case Op::Lt:
    ret += "lt"s;
    break;
  case Op::Geq:
    ret += "geq"s;
    break;
  case Op::Leq:
    ret += "leq"s;
    break;
  case Op::Get:
    ret += "get"s;
    break;
  case Op::Slice:
    ret += "slice"s;
    break;
  case Op::Jmp:
    ret += "jmp"s;
    break;
  case Op::Jt:
    ret += "jt"s;
    break;
  case Op::Jf:
    ret += "jf"s;
    break;
  case Op::Jtsc:
    ret += "jtsc"s;
    break;
  case Op::Jfsc:
    ret += "jfsc"s;
    break;
  case Op::Print:
    ret += "print"s;
    break;
  default:
    ret += "[unknown op code]"s;
    break;
  }
  
  if(op & Op::Dest)  ret += " dest"s;
  if(op & Op::Int)   ret += " int"s;
  
  return ret;
}

std::string Function::toStrDebug()const{
  char buffer[20] = "";
  std::sprintf(buffer, "%p", (const void*)this);
  return buffer;
}

std::string PartiallyApplied::toStrDebug()const{
  char buffer[20] = "";
  std::sprintf(buffer, "%p", (const void*)this);
  return buffer;
}
#endif