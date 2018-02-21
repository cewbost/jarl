#include "vm.h"

#include "fixed_vector.h"

#include <algorithm>
#include <iterator>
#include <cassert>

#ifndef NDEBUG
#include <iostream>
#endif
  
void VM::doArithOp_(
  const OpCodeType** iptr,
  bool (TypedValue::*op)(const TypedValue&)
){
  bool success;
  const OpCodeType*& it = *iptr;
  if(*it & Op::Extended){
    if(*it & Op::Dest){
      ++it;
      success = (this->stack_[this->frame_.bp + *it].*op)(this->stack_.back());
      this->stack_.pop_back();
    }else if(*it & Op::Int){
      ++it;
      success = (this->stack_.back().*op)(TypedValue(static_cast<Int>(*it)));
    }else{
      ++it;
      success = (this->stack_.back().*op)(this->stack_[this->frame_.bp + *it]);
    }
  }else{
    success = (this->stack_[this->stack_.size() - 2].*op)(this->stack_.back());
    this->stack_.pop_back();
  }
  assert(success);
}

void VM::doCmpOp_(const OpCodeType** iptr, CmpMode mode){
  bool success;
  const OpCodeType*& it = *iptr;
  if(*it & Op::Extended){
    if(*it & Op::Dest){
      ++it;
      success = this->stack_[this->frame_.bp + *it].cmp(this->stack_.back(), mode);
      this->stack_.pop_back();
    }else if(*it & Op::Int){
      ++it;
      success = this->stack_.back().cmp(TypedValue(static_cast<Int>(*it)), mode);
    }else{
      ++it;
      success = this->stack_.back().cmp(this->stack_[this->frame_.bp + *it], mode);
    }
  }else{
    success = this->stack_[this->stack_.size() - 2].cmp(this->stack_.back(), mode);
    this->stack_.pop_back();
  }
  assert(success);
}

void VM::pushFunction_(const Procedure& proc){
  if(this->frame_.proc){
    this->call_stack_.push_back(std::move(this->frame_));
  }
  this->frame_.proc = &proc;
  this->frame_.ip = proc.getCode();
  this->frame_.bp = this->stack_.size() - proc.arguments;
  
  std::copy(
    proc.getVValues().begin(),
    proc.getVValues().end(),
    std::back_inserter(stack_)
  );
}

void VM::pushFunction_(const PartiallyApplied& part){
  assert(part.nargs == 0);
  
  if(this->frame_.proc){
    this->call_stack_.push_back(std::move(this->frame_));
  }
  const Procedure& proc = *part.getProc();
  this->frame_.proc = &proc;
  this->frame_.ip = proc.getCode();
  this->frame_.bp = this->stack_.size();
  
  std::copy(
    part.cbegin(),
    part.cend(),
    std::back_inserter(stack_)
  );
  std::copy(
    proc.getVValues().begin(),
    proc.getVValues().end(),
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
    this->frame_.proc = nullptr;
    return true;
  }
}

VM::VM(int stack_size)
: stack_(stack_size), print_func_(nullptr){}

void VM::execute(const Procedure& proc){
  
  this->pushFunction_(proc);
  
  const OpCodeType* end_it = proc.getCode() + proc.getCodeSize();
  
  while(this->frame_.ip != end_it){
    
    /*#ifndef NDEBUG
    for(auto& val: stack_){
      std::cout << "\t" << val.toStr() << std::endl;
    }
    if(*this->frame_.ip & Op::Extended){
      std::cout
        << opCodeToStr(*this->frame_.ip)
        << " "
        << *(this->frame_.ip + 1)
        << std::endl;
    }else{
      std::cout << opCodeToStr(*this->frame_.ip) << std::endl;
    }
    #endif*/
    
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
      this->frame_.ip = proc.getCode() + (OpCodeType)*this->frame_.ip;
      break;
    case Op::Jt:
      assert((*this->frame_.ip & Op::Extended) != 0);
      ++this->frame_.ip;
      stack_.back().toBool();
      if(stack_.back().value.bool_v){
        this->frame_.ip = proc.getCode() + (OpCodeType)*this->frame_.ip;
      }
      stack_.pop_back();
      break;
    case Op::Jf:
      assert((*this->frame_.ip & Op::Extended) != 0);
      ++this->frame_.ip;
      if(!stack_.back().value.bool_v){
        this->frame_.ip = proc.getCode() + (OpCodeType)*this->frame_.ip;
      }
      stack_.pop_back();
      break;
    case Op::Jtsc:
      assert((*this->frame_.ip & Op::Extended) != 0);
      ++this->frame_.ip;
      stack_.back().toBool();
      if(stack_.back().value.bool_v){
        this->frame_.ip = proc.getCode() + (OpCodeType)*this->frame_.ip;
      }else{
        stack_.pop_back();
      }
      break;
    case Op::Jfsc:
      assert((*this->frame_.ip & Op::Extended) != 0);
      ++this->frame_.ip;
      stack_.back().toBool();
      if(!stack_.back().value.bool_v){
        this->frame_.ip = proc.getCode() + (OpCodeType)*this->frame_.ip;
      }else{
        stack_.pop_back();
      }
      break;
    
    case Op::Apply: {
        
        auto& callee = stack_[stack_.size() - 2];
        int bind_pos = 0;
        
        if((*this->frame_.ip & (Op::Extended | Op::Int)) == (Op::Extended | Op::Int)){
          bind_pos = *(++this->frame_.ip);
        }
        
        if(callee.type == TypeTag::Proc){
          assert(callee.value.proc_v->arguments > bind_pos);
          
          if(callee.value.proc_v->arguments == 1){
            this->pushFunction_(*callee.value.proc_v);
            end_it = this->frame_.proc->getCode()
              + this->frame_.proc->getCodeSize();
            --this->frame_.ip;
          }else{
            callee.toPartial();
            callee.value.partial_v->apply(std::move(stack_.back()), bind_pos);
            this->stack_.pop_back();
          }
        }else if(callee.type == TypeTag::Partial){
          assert(callee.value.partial_v->nargs > bind_pos);
          
          callee.value.partial_v->apply(std::move(stack_.back()), bind_pos);
          this->stack_.pop_back();
          if(callee.value.partial_v->nargs == 0){
            this->pushFunction_(*callee.value.partial_v);
            end_it = this->frame_.proc->getCode() + this->frame_.proc->getCodeSize();
            --this->frame_.ip;
          }
        }else assert(false);
      }break;
      
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
    
    case Op::CreateRange: {
      
      Array* arr = new Array;
      
      const auto& int_1 = stack_[stack_.size() - 2];
      const auto& int_2 = stack_[stack_.size() - 1];
      
      assert(int_1.type == TypeTag::Int);
      assert(int_2.type == TypeTag::Int);
      
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
      
      break;
    }
    
    case Op::Return:
      
      if(this->popFunction_()){
        end_it = this->frame_.ip + 1;
      }else{
        end_it = this->frame_.proc->getCode() + this->frame_.proc->getCodeSize();
      }
      
      break;
    
    case Op::Print: {
        auto msg = this->stack_.back().toStr();
        this->print_func_(msg.c_str());
      }break;
    
    default:
      assert(false);
    }
    ++this->frame_.ip;
  }
  
  //std::cout << stack_.back().toStr() << std::endl;
  stack_.pop_back();
}

void VM::setPrintFunc(void(*func)(const char*)){
  this->print_func_ = func;
}