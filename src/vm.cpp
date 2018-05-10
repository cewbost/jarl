#include "vm.h"

#include "fixed_vector.h"

#include <algorithm>
#include <iterator>
#include <cassert>

#ifndef NDEBUG
#include <cstdio>
#define PRINT_STACK
#define PRINT_OP
#define PRINT_ERROR_JUMPS
#endif

namespace {
  thread_local VM* current_vm_ = nullptr;
}

void VM::doArithOp_(
  const OpCodeType** iptr,
  void (TypedValue::*op)(const TypedValue&)
){
  const OpCodeType*& it = *iptr;
  if(*it & Op::Extended){
    if(*it & Op::Dest){
      ++it;
      (this->stack_[this->frame_.bp + *it].*op)(this->stack_.back());
      this->stack_.pop_back();
    }else if(*it & Op::Int){
      ++it;
      (this->stack_.back().*op)(TypedValue(static_cast<Int>(*it)));
    }else{
      ++it;
      (this->stack_.back().*op)(this->stack_[this->frame_.bp + *it]);
    }
  }else{
    (this->stack_[this->stack_.size() - 2].*op)(this->stack_.back());
    this->stack_.pop_back();
  }
}

void VM::doCmpOp_(const OpCodeType** iptr, CmpMode mode){
  const OpCodeType*& it = *iptr;
  if(*it & Op::Extended){
    if(*it & Op::Dest){
      ++it;
      this->stack_[this->frame_.bp + *it].cmp(this->stack_.back(), mode);
      this->stack_.pop_back();
    }else if(*it & Op::Int){
      ++it;
      this->stack_.back().cmp(TypedValue(static_cast<Int>(*it)), mode);
    }else{
      ++it;
      this->stack_.back().cmp(this->stack_[this->frame_.bp + *it], mode);
    }
  }else{
    this->stack_[this->stack_.size() - 2].cmp(this->stack_.back(), mode);
    this->stack_.pop_back();
  }
}

void VM::pushFunction_(const Function& func){
  if(this->frame_.func){
    this->call_stack_.push_back(std::move(this->frame_));
  }
  this->frame_.func = &func;
  this->frame_.ip = func.getCode();
  this->frame_.bp = this->stack_.size() - func.arguments;
  
  std::copy(
    func.getVValues().begin(),
    func.getVValues().end(),
    std::back_inserter(stack_)
  );
}

void VM::pushFunction_(const PartiallyApplied& part){
  assert(part.nargs == 0);
  
  if(this->frame_.func){
    this->call_stack_.push_back(std::move(this->frame_));
  }
  const Function& func = *part.getFunc();
  this->frame_.func = &func;
  this->frame_.ip = func.getCode();
  this->frame_.bp = this->stack_.size();
  
  std::copy(
    part.cbegin(),
    part.cend(),
    std::back_inserter(stack_)
  );
  std::copy(
    func.getVValues().begin(),
    func.getVValues().end(),
    std::back_inserter(stack_)
  );
}

bool VM::popFunction_(){
  if(!this->call_stack_.empty()){
    if(this->stack_.size() > this->frame_.bp){
      this->stack_[this->frame_.bp - 1] = std::move(this->stack_.back());
      this->stack_.resize(this->frame_.bp);
    }
    this->frame_ = std::move(this->call_stack_.back());
    this->call_stack_.pop_back();
    return false;
  }else{
    if(this->stack_.size() > 1){
      this->stack_[0] = std::move(this->stack_.back());
      this->stack_.resize(1);
    }
    this->frame_.func = nullptr;
    return true;
  }
}

VM::VM(int stack_size)
: stack_(stack_size), print_func_(nullptr), error_print_func_(nullptr){}

void VM::execute(const Function& func){
  
  VM::setCurrentVM(this);
  
  this->pushFunction_(func);
  
  const OpCodeType* end_it = func.getCode() + func.getCodeSize();
  
  if(setjmp(this->error_jmp_env_) == 0){
    
  loop_start:
    while(this->frame_.ip != end_it){
      
      #ifdef PRINT_STACK
      {
        auto it = stack_.begin();
        int s_pos = 0;
        if(it != stack_.end()){
          fprintf(stderr, "stack [%2d] %s\n", s_pos, it->toStrDebug().c_str());
          for(++it, ++s_pos; it != stack_.end(); ++it, ++s_pos){
            fprintf(stderr, "      [%2d] %s\n", s_pos, it->toStrDebug().c_str());
          }
        }
      }
      #endif
      #ifdef PRINT_OP
      fprintf(stderr, "op%5d: ", this->frame_.ip - this->frame_.func->getCode());
      if(*this->frame_.ip & Op::Extended){
        fprintf(stderr, "%s %d\n",
          opCodeToStrDebug(*this->frame_.ip).c_str(),
          *(this->frame_.ip + 1)
        );
      }else{
        fprintf(stderr, "%s\n", opCodeToStrDebug(*this->frame_.ip).c_str());
      }
      #endif
      
      switch(*this->frame_.ip & ~Op::Head){
      case Op::Nop:
        break;
      case Op::Push:
        if(*this->frame_.ip & Op::Extended){
          if(*this->frame_.ip & Op::Int){
            ++this->frame_.ip;
            stack_.emplace_back(static_cast<Int>(*this->frame_.ip));
          }else{
            ++this->frame_.ip;
            stack_.push_back(stack_[this->frame_.bp + *this->frame_.ip]);
          }
        }else{
          stack_.emplace_back(nullptr);
        }
        break;
      case Op::PushTrue:
        assert((*this->frame_.ip & Op::Extended) == 0);
        stack_.emplace_back(true);
        break;
      case Op::PushFalse:
        assert((*this->frame_.ip & Op::Extended) == 0);
        stack_.emplace_back(false);
        break;
      case Op::Pop:
        if(*this->frame_.ip & Op::Extended){
          ++this->frame_.ip;
          stack_.resize(stack_.size() - *this->frame_.ip);
        }else{
          stack_.pop_back();
        }
        break;
      case Op::Reduce:
        if(*this->frame_.ip & Op::Extended){
          ++this->frame_.ip;
          int num = *this->frame_.ip;
          stack_[stack_.size() - num - 1] = std::move(stack_.back());
          stack_.resize(stack_.size() - num);
        }else{
          stack_[stack_.size() - 2] = std::move(stack_.back());
          stack_.pop_back();
        }
        break;
      case Op::Write:
        assert((*this->frame_.ip & Op::Extended) != 0);
        ++this->frame_.ip;
        stack_[this->frame_.bp + *this->frame_.ip] = std::move(stack_.back());
        stack_.pop_back();
        break;
      
      case Op::Add:
        this->doArithOp_(&this->frame_.ip, &TypedValue::add);
        break;
      case Op::Sub:
        this->doArithOp_(&this->frame_.ip, &TypedValue::sub);
        break;
      case Op::Mul:
        this->doArithOp_(&this->frame_.ip, &TypedValue::mul);
        break;
      case Op::Div:
        this->doArithOp_(&this->frame_.ip, &TypedValue::div);
        break;
      case Op::Mod:
        this->doArithOp_(&this->frame_.ip, &TypedValue::mod);
        break;
      case Op::Append:
        this->doArithOp_(&this->frame_.ip, &TypedValue::append);
        break;
      
      case Op::Neg:
        stack_.back().neg();
        break;
      case Op::Not:
        stack_.back().boolNot();
        break;
      
      case Op::Cmp:
        this->doArithOp_(&this->frame_.ip, &TypedValue::cmp);
        break;
      
      case Op::Eq:
        this->doCmpOp_(&this->frame_.ip, CmpMode::Equal);
        break;
      case Op::Neq:
        this->doCmpOp_(&this->frame_.ip, CmpMode::NotEqual);
        break;
      case Op::Gt:
        this->doCmpOp_(&this->frame_.ip, CmpMode::Greater);
        break;
      case Op::Lt:
        this->doCmpOp_(&this->frame_.ip, CmpMode::Less);
        break;
      case Op::Geq:
        this->doCmpOp_(&this->frame_.ip, CmpMode::GreaterEqual);
        break;
      case Op::Leq:
        this->doCmpOp_(&this->frame_.ip, CmpMode::LessEqual);
        break;
      
      case Op::Get:
        this->doArithOp_(&this->frame_.ip, &TypedValue::get);
        break;
      
      case Op::Slice:
        assert(!(*this->frame_.ip & Op::Extended));
        this->stack_[this->stack_.size() - 3].slice(
          this->stack_[this->stack_.size() - 2],
          this->stack_.back()
        );
        this->stack_.pop_back();
        this->stack_.pop_back();
        break;
      
      case Op::Jmp:
        assert((*this->frame_.ip & Op::Extended) != 0);
        ++this->frame_.ip;
        this->frame_.ip = func.getCode() + (OpCodeType)*this->frame_.ip;
        goto loop_start;
      case Op::Jt:
        assert((*this->frame_.ip & Op::Extended) != 0);
        ++this->frame_.ip;
        stack_.back().toBool();
        if(stack_.back().value.bool_v){
          this->frame_.ip = func.getCode() + (OpCodeType)*this->frame_.ip;
          stack_.pop_back();
          goto loop_start;
        }
        stack_.pop_back();
        break;
      case Op::Jf:
        assert((*this->frame_.ip & Op::Extended) != 0);
        ++this->frame_.ip;
        stack_.back().toBool();
        if(!stack_.back().value.bool_v){
          this->frame_.ip = func.getCode() + (OpCodeType)*this->frame_.ip;
          stack_.pop_back();
          goto loop_start;
        }
        stack_.pop_back();
        break;
      case Op::Jtsc:
        assert((*this->frame_.ip & Op::Extended) != 0);
        ++this->frame_.ip;
        stack_.back().toBool();
        if(stack_.back().value.bool_v){
          this->frame_.ip = func.getCode() + (OpCodeType)*this->frame_.ip;
          goto loop_start;
        }else{
          stack_.pop_back();
          break;
        }
      case Op::Jfsc:
        assert((*this->frame_.ip & Op::Extended) != 0);
        ++this->frame_.ip;
        stack_.back().toBool();
        if(!stack_.back().value.bool_v){
          this->frame_.ip = func.getCode() + (OpCodeType)*this->frame_.ip;
          goto loop_start;
        }else{
          stack_.pop_back();
          break;
        }
      
      case Op::Apply:
        {
          auto& callee = stack_[stack_.size() - 2];
          int bind_pos = 0;
          
          if(
            (*this->frame_.ip & (Op::Extended | Op::Int))
            == (Op::Extended | Op::Int)
          ){
            bind_pos = *(++this->frame_.ip);
          }
          
          if(callee.type == TypeTag::Func){
            if(callee.value.func_v->arguments <= bind_pos){
              char* msg = dynSprintf(
                "%d: Unable to bind argument to index %d.",
                this->getFrame()->func->getLine(this->getFrame()->ip),
                bind_pos
              );
              this->errPrint(msg);
              delete[] msg;
              this->errorJmp(1);
            }
            
            if(callee.value.func_v->arguments == 1){
              this->pushFunction_(*callee.value.func_v);
              end_it = this->frame_.func->getCode()
                + this->frame_.func->getCodeSize();
              --this->frame_.ip;
            }else{
              callee.toPartial();
              callee.value.partial_v->apply(std::move(stack_.back()), bind_pos);
              this->stack_.pop_back();
            }
          }else if(callee.type == TypeTag::Partial){
            if(callee.value.partial_v->nargs <= bind_pos){
              char* msg = dynSprintf(
                "%d: Unable to bind argument to index %d.",
                this->getFrame()->func->getLine(this->getFrame()->ip),
                bind_pos
              );
              this->errPrint(msg);
              delete[] msg;
              this->errorJmp(1);
            }
            
            callee.value.partial_v->apply(std::move(stack_.back()), bind_pos);
            this->stack_.pop_back();
            if(callee.value.partial_v->nargs == 0){
              this->pushFunction_(*callee.value.partial_v);
              end_it =
                this->frame_.func->getCode() + this->frame_.func->getCodeSize();
              --this->frame_.ip;
            }
          }else{
            char* msg = dynSprintf(
              "%d: Type error. Cannot bind argument to %s.",
              this->getFrame()->func->getLine(this->getFrame()->ip),
              callee.typeStr()
            );
            this->errPrint(msg);
            delete[] msg;
            this->errorJmp(1);
          }
        }
        break;
        
      case Op::CreateArray:
        if(*this->frame_.ip & Op::Extended){
          assert((*this->frame_.ip & Op::Int) == Op::Int);
          Array* arr = new Array;
          
          auto stack_pos = stack_.size() - *(++this->frame_.ip);
          
          for(auto i = stack_pos; i < stack_.size(); ++i){
            arr->push_back(std::move(stack_[i]));
          }
          stack_.resize(stack_pos + 1);
          stack_.back() = arr;
        }else{
          stack_.push_back(new Array);
        }
        break;
      
      case Op::CreateRange:
        {
          
          Array* arr = new Array;
          
          const auto& int_1 = stack_[stack_.size() - 2];
          const auto& int_2 = stack_[stack_.size() - 1];
          
          if(int_1.type != TypeTag::Int || int_2.type != TypeTag::Int){
            char* msg = dynSprintf(
              "%d: Type error. Create range from %s to %s.",
              this->getFrame()->func->getLine(this->getFrame()->ip),
              int_1.typeStr(),
              int_2.typeStr()
            );
            this->errPrint(msg);
            delete[] msg;
            this->errorJmp(1);
          }
          
          Int begin = int_1.value.int_v;
          Int end = int_2.value.int_v;
          
          while(begin < end){
            arr->emplace_back(begin++);
          }
          while(begin > end){
            arr->emplace_back(begin--);
          }
          
          stack_.resize(stack_.size() - 1);
          stack_.back() = TypedValue(arr);
        }
        break;
      
      case Op::Return:
        
        if(this->popFunction_()){
          end_it = this->frame_.ip + 1;
        }else{
          end_it = this->frame_.func->getCode() + this->frame_.func->getCodeSize();
        }
        
        break;
      
      case Op::Print:
        {
          int num;
          if(*this->frame_.ip & Op::Extended){
            assert(*this->frame_.ip & Op::Int);
            ++this->frame_.ip;
            num = *this->frame_.ip;
          }else num = 1;
          
          int it = stack_.size() - num;
          auto msg = stack_[it].toCStr();
          for(++it; it < stack_.size(); ++it){
            char* new_msg = dynSprintf("%s%s",
              msg.get(),
              stack_[it].toCStr().release()
            );
            msg.reset(new_msg);
          }
          this->print(msg.get());
          stack_.resize(stack_.size() - num);
        }
        break;
        
      case Op::Assert:
        stack_.back().toBool();
        if(*this->frame_.ip & Op::Bool){
          if(!stack_.back().value.bool_v){
            auto str = stack_[stack_.size() - 2].toCStr();
            VM* vm = VM::getCurrentVM();
            char* msg = dynSprintf(
              "%d: Assertion failed. %s.",
              vm->getFrame()->func->getLine(vm->getFrame()->ip),
              str.get()
            );
            str = nullptr;
            vm->errPrint(msg);
            delete[] msg;
            vm->errorJmp(1);
          }else{
            stack_[stack_.size() - 2] = std::move(stack_.back());
            stack_.pop_back();
          }
        }else{
          if(!stack_.back().value.bool_v){
            VM* vm = VM::getCurrentVM();
            char* msg = dynSprintf(
              "%d: Assertion failed.",
              vm->getFrame()->func->getLine(vm->getFrame()->ip)
            );
            vm->errPrint(msg);
            delete[] msg;
            vm->errorJmp(1);
          }
        }
        break;
      
      default:
        assert(false);
      }
      ++this->frame_.ip;
    }
  }else{
    //nop for now
  }
  
  stack_.pop_back();
}

void VM::setPrintFunc(void(*func)(const char*)){
  this->print_func_ = func;
}
void VM::setErrorPrintFunc(void(*func)(const char*)){
  this->error_print_func_ = func;
}
void VM::print(const char* msg){
  this->print_func_(msg);
}
void VM::errPrint(const char* msg){
  this->error_print_func_(msg);
}

VM::StackFrame* VM::getFrame(){
  return &this->frame_;
}

void VM::errorJmp(int val){
  #ifdef PRINT_ERROR_JUMPS
  fprintf(stderr, "error jump!\n");
  #endif
  longjmp(this->error_jmp_env_, val);
}

void VM::setCurrentVM(VM* vm){
  current_vm_ = vm;
}

VM* VM::getCurrentVM(){
  return current_vm_;
}
