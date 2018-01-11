#include "procedure.h"

#include "token.h"
#include "delegate_map.h"

#include <limits>
#include <cassert>

constexpr OpCodeType _stack_pos_local = 0x8000;
constexpr OpCodeType _stack_pos_const = 0x4000;

namespace std{
  template<>
  struct equal_to<String*>{
    typedef ptr_equal_to<String*> type;
  };
}

void Procedure::threadAST_(
  Token* tok,
  VarAllocMap* va,
  VarAllocMap* cva,
  VectorSet<TypedValue>* vc,
  int* ss,
  bool ret
){
  switch(tokenMajorType(tok->type)){
  case TokenType::ValueToken:
    if(!ret) break;
    switch(tok->type){
    case TokenType::BoolToken:
      if(tok->bool_value){
        this->code_.push_back(Op::PushTrue);
      }else{
        this->code_.push_back(Op::PushFalse);
      }
      break;
    case TokenType::IntToken:
      if(tok->int_value <= std::numeric_limits<OpCodeSignedType>::max()
      && tok->int_value >= std::numeric_limits<OpCodeSignedType>::min()){
        this->code_.push_back(Op::Push | Op::Extended | Op::Int);
        this->code_.push_back(static_cast<OpCodeType>(tok->int_value));
      }else{
        this->code_.push_back(Op::Push | Op::Extended);
        this->code_.push_back(
          (OpCodeType)(vc->emplace(tok->int_value) | _stack_pos_const)
        );
      }
      break;
    case TokenType::FloatToken:
      this->code_.push_back(Op::Push | Op::Extended);
      this->code_.push_back(
        (OpCodeType)(vc->emplace(tok->float_value) | _stack_pos_const)
      );
      break;
    case TokenType::StringToken:
      this->code_.push_back(Op::Push | Op::Extended);
      this->code_.push_back(
        (OpCodeType)(vc->emplace(tok->string_value) | _stack_pos_const)
      );
      break;
    case TokenType::IdentifierToken: {
        this->code_.push_back(Op::Push | Op::Extended);
        //this->code_.push_back((OpCodeType)(*va)[tok->string_value]);
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
      }break;
    case TokenType::NullToken:
      this->code_.push_back(Op::Push);
      break;
    default:
      assert(false);
      break;
    }
    ++(*ss);
    break;
  
  case TokenType::UnaryExprNode:
    this->threadAST_(tok->child, va, cva, vc, ss);
    switch(tok->type){
    case TokenType::NegExpr:
      this->code_.push_back(Op::Neg);
      break;
    case TokenType::NotExpr:
      this->code_.push_back(Op::Not);
      break;
    default:
      assert(false);
      break;
    }
    if(!ret){
      this->code_.push_back(Op::Pop);
      --(*ss);
    }
    break;
  
  case TokenType::BinaryExprNode:
    this->threadAST_(tok->childs.first, va, cva, vc, ss);
    this->threadAST_(tok->childs.second, va, cva, vc, ss);
    switch(tok->type){
    case TokenType::ApplyExpr:
      this->code_.push_back(Op::Apply);
      break;
    
    case TokenType::AddExpr:
      this->code_.push_back(Op::Add);
      break;
    case TokenType::SubExpr:
      this->code_.push_back(Op::Sub);
      break;
    case TokenType::MulExpr:
      this->code_.push_back(Op::Mul);
      break;
    case TokenType::DivExpr:
      this->code_.push_back(Op::Div);
      break;
    case TokenType::ModExpr:
      this->code_.push_back(Op::Mod);
      break;
    case TokenType::AppendExpr:
      this->code_.push_back(Op::Append);
      break;
    
    case TokenType::CmpExpr:
      this->code_.push_back(Op::Cmp);
      break;
    case TokenType::EqExpr:
      this->code_.push_back(Op::Eq);
      break;
    case TokenType::NeqExpr:
      this->code_.push_back(Op::Neq);
      break;
    case TokenType::GtExpr:
      this->code_.push_back(Op::Gt);
      break;
    case TokenType::LtExpr:
      this->code_.push_back(Op::Lt);
      break;
    case TokenType::GeqExpr:
      this->code_.push_back(Op::Geq);
      break;
    case TokenType::LeqExpr:
      this->code_.push_back(Op::Leq);
      break;
    
    default:
      assert(false);
      break;
    }
    if(!ret){
      this->code_.push_back(Op::Pop);
      *ss -= 2;
    }else --(*ss);
    break;
  
  case TokenType::BooleanExprNode: {
      unsigned jmp_addr;
      this->threadAST_(tok->childs.first, va, cva, vc, ss);
      
      switch(tok->type){
      case TokenType::AndExpr:
        this->code_.push_back(Op::Jfsc | Op::Extended);
        jmp_addr = this->code_.size();
        this->code_.push_back((OpCodeType)0);
        this->threadAST_(tok->childs.second, va, cva, vc, ss);
        this->code_[jmp_addr] = (OpCodeType)this->code_.size() - 1;
        
        if(!ret){
          this->code_.push_back(Op::Pop);
          --(*ss);
        }
        break;
      case TokenType::OrExpr:
        this->code_.push_back(Op::Jtsc | Op::Extended);
        jmp_addr = this->code_.size();
        this->code_.push_back((OpCodeType)0);
        this->threadAST_(tok->childs.second, va, cva, vc, ss);
        this->code_[jmp_addr] = (OpCodeType)this->code_.size() - 1;
        
        if(!ret){
          this->code_.push_back(Op::Pop);
          --(*ss);
        }
        break;
      
      case TokenType::ConditionalExpr:
        assert(tok->childs.second->type == TokenType::ConditionalNode);
        
        this->code_.push_back(Op::Jf | Op::Extended);
        jmp_addr = this->code_.size();
        this->code_.push_back((OpCodeType)0);
        --(*ss);
        
        this->threadAST_(tok->childs.second->childs.first, va, cva, vc, ss, ret);
        --(*ss);
        
        this->code_[jmp_addr] = (OpCodeType)this->code_.size() + 1;
        
        if(ret){
          this->code_.push_back(Op::Jmp | Op::Extended);
          jmp_addr = this->code_.size();
          this->code_.push_back((OpCodeType)0);
          if(tok->childs.second->childs.second->type == TokenType::NopNode){
            this->code_.push_back(Op::Push);
            ++(*ss);
          }else{
            this->threadAST_(tok->childs.second->childs.second, va, cva, vc, ss);
          }
          this->code_[jmp_addr] = (OpCodeType)this->code_.size() + 1;
        }else if(tok->childs.second->childs.second->type != TokenType::NopNode){
          this->code_.push_back(Op::Jmp | Op::Extended);
          jmp_addr = this->code_.size();
          this->code_.push_back((OpCodeType)0);
          
          this->threadAST_(tok->childs.second->childs.second, va, cva, vc, ss, false);
          
          this->code_[jmp_addr] = (OpCodeType)this->code_.size() + 1;
        }
        break;
        
      case TokenType::ConditionalNode:
        assert(false);
        break;
      
      default:
        assert(false);
        break;
      }
    }
    break;
  
  case TokenType::AssignExprNode: {
      assert(tok->childs.first->type == TokenType::IdentifierToken);
      auto stack_pos = (OpCodeType)(*va)[tok->childs.first->string_value];
      this->threadAST_(tok->childs.second, va, cva, vc, ss);
      switch(tok->type){
      case TokenType::AssignExpr:
        this->code_.push_back(Op::Write | Op::Extended | Op::Dest);
        break;
      case TokenType::AddAssignExpr:
        this->code_.push_back(Op::Add | Op::Extended | Op::Dest);
        break;
      case TokenType::SubAssignExpr:
        this->code_.push_back(Op::Sub | Op::Extended | Op::Dest);
        break;
      case TokenType::MulAssignExpr:
        this->code_.push_back(Op::Mul | Op::Extended | Op::Dest);
        break;
      case TokenType::DivAssignExpr:
        this->code_.push_back(Op::Div | Op::Extended | Op::Dest);
        break;
      case TokenType::ModAssignExpr:
        this->code_.push_back(Op::Mod | Op::Extended | Op::Dest);
        break;
      case TokenType::AppendAssignExpr:
        this->code_.push_back(Op::Append | Op::Extended | Op::Dest);
        break;
      default:
        assert(false);
        break;
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
  
  case TokenType::MiscUnaryExprNode:
    switch(tok->type){
    case TokenType::CodeBlock: {
        VarAllocMap* var_allocs = new VarAllocMap(va);
        this->threadAST_(tok->childs.first, var_allocs, cva, vc, ss);
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
      }break;
    case TokenType::ArrayExpr: {
        if(!ret) break;
        
        if(tok->child->type != TokenType::NopNode){
          if(tok->child->type == TokenType::ExprList){
            Token* t = tok->child;
            OpCodeType elems = 0;
            
            for(;;){
              this->threadAST_(t->childs.first, va, cva, vc, ss);
              ++elems;
              if(t->childs.second->type == TokenType::ExprList){
                t = t->childs.second;
              }else{
                this->threadAST_(t->childs.second, va, cva, vc, ss);
                ++elems;
                break;
              }
            }
            
            this->code_.push_back(Op::CreateArray | Op::Extended | Op::Int);
            this->code_.push_back(elems);
            *ss -= elems - 1;
          }else if(tok->child->type == TokenType::RangeExpr){
            this->threadAST_(tok->child->childs.first, va, cva, vc, ss);
            this->threadAST_(tok->child->childs.second, va, cva, vc, ss);
            this->code_.push_back(Op::CreateRange);
            --(*ss);
          }else{
            this->threadAST_(tok->child, va, cva, vc, ss);
            this->code_.push_back(Op::CreateArray | Op::Extended | Op::Int);
            this->code_.push_back(1);
          }
        }else{
          this->code_.push_back(Op::CreateArray);
          ++(*ss);
        }
      }break;
    default:
      assert(false);
    }
    break;
    
  case TokenType::MiscBinaryExprNode:
    switch(tok->type){
    case TokenType::LoopNode: {
        if(ret){
          this->code_.push_back(Op::Push);
          ++(*ss);
        }
        unsigned begin_addr = this->code_.size() - 1;
        
        this->threadAST_(tok->childs.first, va, cva, vc, ss);
        
        this->code_.push_back(Op::Jf | Op::Extended);
        --(*ss);
        unsigned end_jmp_addr = this->code_.size();
        this->code_.push_back((OpCodeType)0);
        
        this->threadAST_(tok->childs.second, va, cva, vc, ss, ret);
        if(ret){
          this->code_.push_back(Op::Reduce);
          --(*ss);
        }
        this->code_.push_back(Op::Jmp | Op::Extended);
        this->code_[end_jmp_addr] = (OpCodeType)this->code_.size();
        this->code_.push_back((OpCodeType)begin_addr);
      }break;
    
    case TokenType::FuncNode: {
        //some checking might be nessecary for the procedure being well formed
        
        if(!ret) break;
        
        VarAllocMap* var_alloc = new VarAllocMap(nullptr);
        
        OpCodeType std_args = 0;
        if(tok->childs.first->type != TokenType::NopNode){
          Token* t = tok->childs.first;
          do{
            assert(t->childs.first->type == TokenType::IdentifierToken);
            
            (*var_alloc)[t->childs.first->string_value] = std_args;
            ++std_args;
            t = t->childs.second;
          }while(t->type != TokenType::NopNode);
        }
        
        delete tok->childs.first;
        tok->childs.first = nullptr;
        
        auto* proc = new Procedure(tok->childs.second, var_alloc, va);
        
        tok->childs.second = nullptr;
        this->code_.push_back(Op::Push | Op::Extended);
        this->code_.push_back((OpCodeType)vc->emplace(proc) | _stack_pos_const);
        ++(*ss);
        
        if(proc->arguments > std_args){
          //auto& base = *((VectorMapBase*)(&var_alloc->base()));
          VectorMapBase& base = var_alloc->base();
          for(int i = std_args; i < proc->arguments; ++i){
            this->code_.push_back(Op::Push | Op::Extended);
            this->code_.push_back((*va)[base[i].first]);
            this->code_.push_back(Op::Apply | Op::Extended | Op::Int);
            this->code_.push_back((OpCodeType)std_args);
          }
        }
        delete var_alloc;
      }break;
    case TokenType::SeqNode:
      threadAST_(tok->childs.first, va, cva, vc, ss, false);
      threadAST_(tok->childs.second, va, cva, vc, ss);
      break;
    case TokenType::VarDecl: {
        assert(tok->childs.first->type == TokenType::IdentifierToken);
        auto stack_size = *ss;
        threadAST_(tok->childs.second, va, cva, vc, ss);
        va->direct()[tok->childs.first->string_value]
          = stack_size | _stack_pos_local;
      }break;
    case TokenType::PrintExpr: {
        this->threadAST_(tok->child, va, cva, vc, ss);
        this->code_.push_back(Op::Print);
      }break;
    case TokenType::IndexExpr: {
        this->threadAST_(tok->childs.first, va, cva, vc, ss);
        
        if(tok->childs.second->type == TokenType::RangeExpr){
          this->threadAST_(tok->childs.second->childs.first, va, cva, vc, ss);
          this->threadAST_(tok->childs.second->childs.second, va, cva, vc, ss);
          this->code_.push_back(Op::Slice);
        }else{
          this->threadAST_(tok->childs.second, va, cva, vc, ss);
          this->code_.push_back(Op::Get);
        }
        
        if(!ret){
          this->code_.push_back(Op::Pop);
          *ss -= 2;
        }else --(*ss);
      }break;
    default:
      assert(false);
    }
    break;
    
  case TokenType::MiscEndNode:
    //NopNode
    this->code_.push_back(Op::Push);
    break;
  default:
    assert(false);
  }
}

Procedure::Procedure(Token* tree, VarAllocMap* var_alloc, VarAllocMap* context_vars){
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
  
  this->threadAST_(tree, var_alloc, context_vars, &constants, &stack_size);
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
      if(op & _stack_pos_local){
        op = (op & ~_stack_pos_local) + constants.size() + this->arguments;
      }else if(op & _stack_pos_const){
        op = (op & ~_stack_pos_const) + this->arguments;
      }
    }
  }
  
  this->values_ = std::move(constants);
  
  #ifndef NDEBUG
  //std::cout << this->opcodesToStr() << std::endl;
  #endif
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
std::string Procedure::opcodesToStr()const{
  using namespace std::string_literals;
  
  std::string ret = "";
  
  auto vit = this->values_.begin();
  
  bool extended = false;
  for(auto op: this->code_){
    if(!extended){
      ret += opCodeToStr(op);
      if(op & Op::Extended) extended = true;
      else ret += "\n"s;
    }else{
      ret += " "s + std::to_string(op) + "\n"s;
      extended = false;
    }
  }
  
  return ret;
}

std::string opCodeToStr(OpCodeType op){
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
  default:
    ret += "[unknown op code]"s;
    break;
  }
  
  if(op & Op::Dest)  ret += " dest"s;
  if(op & Op::Int)   ret += " int"s;
  
  return ret;
}

std::string Procedure::toStr()const{
  char buffer[20] = "";
  std::sprintf(buffer, "%p", (const void*)this);
  return buffer;
}

std::string PartiallyApplied::toStr()const{
  char buffer[20] = "";
  std::sprintf(buffer, "%p", (const void*)this);
  return buffer;
}
#endif