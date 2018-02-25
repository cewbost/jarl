#include "procedure.h"

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

void Procedure::threadAST_(
  VM* vm,
  ASTNode* tok,
  VarAllocMap* va,
  VarAllocMap* cva,
  VectorSet<TypedValue>* vc,
  int* ss,
  bool ret
){
  switch(static_cast<ASTNodeType>(static_cast<unsigned>(tok->type) & ~0xff)){
  case ASTNodeType::Error:
    switch(tok->type){
    case ASTNodeType::LexError:
      {
        char* buffer = new char[64];
        sprintf(buffer, "line %d: Unrecognized lexeme.\n");
        vm->reportError(std::unique_ptr<char[]>(buffer));
      }
      break;
    case ASTNodeType::ParseError:
      {
        char* buffer = new char[64];
        sprintf(buffer, "line %d: Parser error.\n");
        vm->reportError(std::unique_ptr<char[]>(buffer));
      }
      break;
    default:
      assert(false);
    }
  case ASTNodeType::Value:
    if(!ret) break;
    switch(tok->type){
    case ASTNodeType::Bool:
      if(tok->bool_value){
        this->code_.push_back(Op::PushTrue);
      }else{
        this->code_.push_back(Op::PushFalse);
      }
      break;
    case ASTNodeType::Int:
      if(tok->int_value <= std::numeric_limits<OpCodeSignedType>::max()
      && tok->int_value >= std::numeric_limits<OpCodeSignedType>::min()){
        this->code_.push_back(Op::Push | Op::Extended | Op::Int);
        this->code_.push_back(static_cast<OpCodeType>(tok->int_value));
      }else{
        this->code_.push_back(Op::Push | Op::Extended);
        this->code_.push_back(
          (OpCodeType)(vc->emplace(tok->int_value) | stack_pos_const_)
        );
      }
      break;
    case ASTNodeType::Float:
      this->code_.push_back(Op::Push | Op::Extended);
      this->code_.push_back(
        (OpCodeType)(vc->emplace(tok->float_value) | stack_pos_const_)
      );
      break;
    case ASTNodeType::String:
      this->code_.push_back(Op::Push | Op::Extended);
      this->code_.push_back(
        (OpCodeType)(vc->emplace(tok->string_value) | stack_pos_const_)
      );
      break;
    case ASTNodeType::Identifier:
      {
        this->code_.push_back(Op::Push | Op::Extended);
        auto it = va->find(tok->string_value);
        if(it == va->end()){
          assert((bool)cva);
          it = cva->find(tok->string_value);
          assert(it != cva->end());
          
          va->base()[tok->string_value] = this->arguments;
          this->code_.push_back(this->arguments);
          ++this->arguments;
        }else{
          this->code_.push_back(it->second);
        }
      }
      break;
    case ASTNodeType::Null:
      this->code_.push_back(Op::Push);
      break;
    default:
      assert(false);
    }
    ++(*ss);
    break;
    
  case ASTNodeType::UnaryExpr:
    this->threadAST_(vm, tok->child, va, cva, vc, ss);
    switch(tok->type){
    case ASTNodeType::Neg:
      this->code_.push_back(Op::Neg);
      break;
    case ASTNodeType::Not:
      this->code_.push_back(Op::Not);
      break;
    default:
      assert(false);
    }
    if(!ret){
      this->code_.push_back(Op::Pop);
      --(*ss);
    }
    break;
    
  case ASTNodeType::BinaryExpr:
    this->threadAST_(vm, tok->children.first, va, cva, vc, ss);
    this->threadAST_(vm, tok->children.second, va, cva, vc, ss);
    switch(tok->type){
    case ASTNodeType::Apply:
      this->code_.push_back(Op::Apply);
      break;
      
    case ASTNodeType::Add:
      this->code_.push_back(Op::Add);
      break;
    case ASTNodeType::Sub:
      this->code_.push_back(Op::Sub);
      break;
    case ASTNodeType::Mul:
      this->code_.push_back(Op::Mul);
      break;
    case ASTNodeType::Div:
      this->code_.push_back(Op::Div);
      break;
    case ASTNodeType::Mod:
      this->code_.push_back(Op::Mod);
      break;
    case ASTNodeType::Append:
      this->code_.push_back(Op::Append);
      break;
      
    case ASTNodeType::Cmp:
      this->code_.push_back(Op::Cmp);
      break;
    case ASTNodeType::Eq:
      this->code_.push_back(Op::Eq);
      break;
    case ASTNodeType::Neq:
      this->code_.push_back(Op::Neq);
      break;
    case ASTNodeType::Gt:
      this->code_.push_back(Op::Gt);
      break;
    case ASTNodeType::Lt:
      this->code_.push_back(Op::Lt);
      break;
    case ASTNodeType::Geq:
      this->code_.push_back(Op::Geq);
      break;
    case ASTNodeType::Leq:
      this->code_.push_back(Op::Leq);
      break;
      
    default:
      assert(false);
    }
    if(!ret){
      this->code_.push_back(Op::Pop);
      *ss -= 2;
    }else --(*ss);
    break;
    
  case ASTNodeType::BranchExpr:
    {
      unsigned jmp_addr;
      this->threadAST_(vm, tok->children.first, va, cva, vc, ss);
      
      switch(tok->type){
      case ASTNodeType::And:
        this->code_.push_back(Op::Jfsc | Op::Extended);
        jmp_addr = this->code_.size();
        this->code_.push_back((OpCodeType)0);
        this->threadAST_(vm, tok->children.second, va, cva, vc, ss);
        this->code_[jmp_addr] = (OpCodeType)this->code_.size() - 1;
        
        if(!ret){
          this->code_.push_back(Op::Pop);
          --(*ss);
        }
        break;
      case ASTNodeType::Or:
        this->code_.push_back(Op::Jtsc | Op::Extended);
        jmp_addr = this->code_.size();
        this->code_.push_back((OpCodeType)0);
        this->threadAST_(vm, tok->children.second, va, cva, vc, ss);
        this->code_[jmp_addr] = (OpCodeType)this->code_.size() - 1;
        
        if(!ret){
          this->code_.push_back(Op::Pop);
          --(*ss);
        }
        break;
        
      case ASTNodeType::Conditional:
        assert(tok->children.second->type == ASTNodeType::Branch);
        
        this->code_.push_back(Op::Jf | Op::Extended);
        jmp_addr = this->code_.size();
        this->code_.push_back((OpCodeType)0);
        --(*ss);
        
        this->threadAST_(vm,
          tok->children.second->children.first,
          va, cva, vc, ss, ret
        );
        --(*ss);
        
        this->code_[jmp_addr] = (OpCodeType)this->code_.size() + 1;
        
        if(ret){
          this->code_.push_back(Op::Jmp | Op::Extended);
          jmp_addr = this->code_.size();
          this->code_.push_back((OpCodeType)0);
          if(tok->children.second->children.second->type == ASTNodeType::Nop){
            this->code_.push_back(Op::Push);
            ++(*ss);
          }else{
            this->threadAST_(vm,
              tok->children.second->children.second,
              va, cva, vc, ss
            );
          }
          this->code_[jmp_addr] = (OpCodeType)this->code_.size() + 1;
        }else if(tok->children.second->children.second->type != ASTNodeType::Nop){
          this->code_.push_back(Op::Jmp | Op::Extended);
          jmp_addr = this->code_.size();
          this->code_.push_back((OpCodeType)0);
          
          this->threadAST_(vm,
            tok->children.second->children.second,
            va, cva, vc, ss, false
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
      assert(tok->children.first->type == ASTNodeType::Identifier);
      auto stack_pos = (OpCodeType)(*va)[tok->children.first->string_value];
      this->threadAST_(vm, tok->children.second, va, cva, vc, ss);
      switch(tok->type){
      case ASTNodeType::Assign:
        this->code_.push_back(Op::Write | Op::Extended | Op::Dest);
        break;
      case ASTNodeType::AddAssign:
        this->code_.push_back(Op::Add | Op::Extended | Op::Dest);
        break;
      case ASTNodeType::SubAssign:
        this->code_.push_back(Op::Sub | Op::Extended | Op::Dest);
        break;
      case ASTNodeType::MulAssign:
        this->code_.push_back(Op::Mul | Op::Extended | Op::Dest);
        break;
      case ASTNodeType::DivAssign:
        this->code_.push_back(Op::Div | Op::Extended | Op::Dest);
        break;
      case ASTNodeType::ModAssign:
        this->code_.push_back(Op::Mod | Op::Extended | Op::Dest);
        break;
      case ASTNodeType::AppendAssign:
        this->code_.push_back(Op::Append | Op::Extended | Op::Dest);
        break;
        
      default:
        assert(false);
      }
      
      this->code_.push_back(stack_pos);
      if(ret){
        this->code_.push_back(Op::Push | Op::Extended | Op::Dest);
        this->code_.push_back(stack_pos);
      }else{
        --(*ss);
      }
    }
    break;
    
  default:
    switch(tok->type){
    case ASTNodeType::CodeBlock:
      {
        VarAllocMap* var_allocs = new VarAllocMap(va);
        this->threadAST_(vm, tok->children.first, var_allocs, cva, vc, ss);
        var_allocs->set_delegate(nullptr);
        if(ret){
          if(var_allocs->size() > 0){
            if(var_allocs->size() == 1){
              this->code_.push_back(Op::Reduce);
            }else{
              this->code_.push_back(Op::Reduce | Op::Extended);
              this->code_.push_back(static_cast<OpCodeType>(var_allocs->size()));
            }
            *ss -= var_allocs->size();
          }
        }else{
          if(var_allocs->size() == 0){
            this->code_.push_back(Op::Pop);
          }else{
            this->code_.push_back(Op::Pop | Op::Extended);
            this->code_.push_back(static_cast<OpCodeType>(var_allocs->size() + 1));
          }
        }
        delete var_allocs;
      }
      break;
      
    case ASTNodeType::Array:
      {
        if(!ret) break;
        
        if(tok->child->type != ASTNodeType::Nop){
          if(tok->child->type == ASTNodeType::ExprList){
            ASTNode* t = tok->child;
            OpCodeType elems = 0;
            
            for(;;){
              this->threadAST_(vm, t->children.first, va, cva, vc, ss);
              ++elems;
              if(t->children.second->type == ASTNodeType::ExprList){
                t = t->children.second;
              }else{
                this->threadAST_(vm, t->children.second, va, cva, vc, ss);
                ++elems;
                break;
              }
            }
            
            this->code_.push_back(Op::CreateArray | Op::Extended | Op::Int);
            this->code_.push_back(elems);
            *ss -= elems - 1;
          }else if(tok->child->type == ASTNodeType::Range){
            this->threadAST_(vm, tok->child->children.first, va, cva, vc, ss);
            this->threadAST_(vm, tok->child->children.second, va, cva, vc, ss);
            this->code_.push_back(Op::CreateRange);
            --(*ss);
          }else{
            this->threadAST_(vm, tok->child, va, cva, vc, ss);
            this->code_.push_back(Op::CreateArray | Op::Extended | Op::Int);
            this->code_.push_back(1);
          }
        }else{
          this->code_.push_back(Op::CreateArray);
          ++(*ss);
        }
      }
      break;
      
    case ASTNodeType::While:
      {
        if(ret){
          this->code_.push_back(Op::Push);
          ++(*ss);
        }
        unsigned begin_addr = this->code_.size() - 1;
        
        this->threadAST_(vm, tok->children.first, va, cva, vc, ss);
        
        this->code_.push_back(Op::Jf | Op::Extended);
        --(*ss);
        unsigned end_jmp_addr = this->code_.size();
        this->code_.push_back((OpCodeType)0);
        
        this->threadAST_(vm, tok->children.second, va, cva, vc, ss, ret);
        if(ret){
          this->code_.push_back(Op::Reduce);
          --(*ss);
        }
        this->code_.push_back(Op::Jmp | Op::Extended);
        this->code_[end_jmp_addr] = (OpCodeType)this->code_.size();
        this->code_.push_back((OpCodeType)begin_addr);
      }
      break;
      
    case ASTNodeType::Function:
      {
        //some checking might be nessecary for the procedure being well formed
        
        if(!ret) break;
        
        VarAllocMap* var_alloc = new VarAllocMap(nullptr);
        
        OpCodeType std_args = 0;
        if(tok->children.first->type != ASTNodeType::Nop){
          ASTNode* t = tok->children.first;
          do{
            (*var_alloc)[t->string_branch.value] = std_args;
            ++std_args;
            t = t->string_branch.next;
          }while(t->type != ASTNodeType::Nop);
        }
        
        delete tok->children.first;
        tok->children.first = nullptr;
        
        auto* proc = new Procedure(vm, tok->children.second, var_alloc, va);
        
        tok->children.second = nullptr;
        this->code_.push_back(Op::Push | Op::Extended);
        this->code_.push_back((OpCodeType)vc->emplace(proc) | stack_pos_const_);
        ++(*ss);
        
        if(proc->arguments > std_args){
          VectorMapBase& base = var_alloc->base();
          for(int i = std_args; i < proc->arguments; ++i){
            this->code_.push_back(Op::Push | Op::Extended);
            this->code_.push_back((*va)[base[i].first]);
            this->code_.push_back(Op::Apply | Op::Extended | Op::Int);
            this->code_.push_back((OpCodeType)std_args);
          }
        }
        delete var_alloc;
      }
      break;
      
    case ASTNodeType::Seq:
      this->threadAST_(vm, tok->children.first, va, cva, vc, ss, false);
      this->threadAST_(vm, tok->children.second, va, cva, vc, ss);
      break;
      
    case ASTNodeType::VarDecl:
      {
        auto stack_size = *ss;
        this->threadAST_(vm, tok->string_branch.next, va, cva, vc, ss);
        va->direct()[tok->string_branch.value]
          = stack_size | stack_pos_local_;
      }
      break;
      
    case ASTNodeType::Print:
      {
        this->threadAST_(vm, tok->child, va, cva, vc, ss);
        this->code_.push_back(Op::Print);
      }
      break;
      
    case ASTNodeType::Index:
      {
        this->threadAST_(vm, tok->children.first, va, cva, vc, ss);
        
        if(tok->children.second->type == ASTNodeType::Range){
          this->threadAST_(vm,
            tok->children.second->children.first,
            va, cva, vc, ss
          );
          this->threadAST_(vm,
            tok->children.second->children.second,
            va, cva, vc, ss
          );
          this->code_.push_back(Op::Slice);
        }else{
          this->threadAST_(vm, tok->children.second, va, cva, vc, ss);
          this->code_.push_back(Op::Get);
        }
        
        if(!ret){
          this->code_.push_back(Op::Pop);
          *ss -= 2;
        }else --(*ss);
      }
      break;
      
    case ASTNodeType::Nop:
      this->code_.push_back(Op::Push);
      break;
      
    default:
      assert(false);
    }
  }
}

Procedure::Procedure(
  VM* vm,
  ASTNode* tree,
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
  
  this->threadAST_(vm, tree, var_alloc, context_vars, &constants, &stack_size);
  this->code_.push_back(Op::Return);
  
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

PartiallyApplied::PartiallyApplied(const Procedure* proc)
: proc_(proc), args_(proc->arguments), nargs(proc->arguments){}

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

#ifndef NDEBUG
std::string Procedure::opcodesToStrDebug()const{
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

std::string Procedure::toStrDebug()const{
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