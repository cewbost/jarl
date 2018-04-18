#include "value.h"

#include "vm.h"

#include <memory>
#include <algorithm>
#include <iterator>

#include <cmath>
#include <cassert>

namespace{
  
  inline bool toInt_(String* s, Int* t){
    if(sizeof(Int) == 8){
      return s->toInt64((int64_t*)t);
    }else{
      return s->toInt32((int32_t*)t);
    }
  }
  inline bool toFloat_(String* s, Float* t){
    if(sizeof(Float) == 8){
      return s->toDouble((double*)t);
    }else{
      return s->toFloat((float*)t);
    }
  }
  
  inline Int add_(Int a, Int b){
    return a + b;
  }
  inline Float add_(Float a, Float b){
    return a + b;
  }
  inline String* add_(const String* a, const String* b){
    return make_new<String>(a, b);
  }
}

//private methods

void TypedValue::clear_(){
  switch(this->type){
  case TypeTag::String:
    this->value.string_v->decRefCount();
    break;
  case TypeTag::Func:
    this->value.func_v->decRefCount();
    break;
  case TypeTag::Partial:
    this->value.partial_v->decRefCount();
    break;
  case TypeTag::Array:
    this->value.array_v->decRefCount();
    break;
  default:
    break;
  }
}

void TypedValue::move_(TypedValue&& other)noexcept{
  this->type = other.type;
  memcpy(&this->value, &other.value, sizeof(this->value));
  other.type = TypeTag::Null;
}

void TypedValue::copy_(const TypedValue& other)noexcept{
  this->type = other.type;
  switch(other.type){
  case TypeTag::Null:
  case TypeTag::Bool:
  case TypeTag::Int:
  case TypeTag::Float:
    memcpy(&this->value, &other.value, sizeof(this->value));
    break;
  case TypeTag::String:
    this->value.string_v = other.value.string_v;
    this->value.string_v->incRefCount();
    break;
  case TypeTag::Func:
    this->value.func_v = other.value.func_v;
    this->value.func_v->incRefCount();
    break;
  case TypeTag::Partial:
    this->value.partial_v = other.value.partial_v;
    this->value.partial_v->incRefCount();
    break;
  case TypeTag::Array:
    this->value.array_v = other.value.array_v;
    this->value.array_v->incRefCount();
    break;
  case TypeTag::None:
    break;
  default:
    assert(false);
  }
}

//constructors

TypedValue::TypedValue(){
  type = TypeTag::None;
}
TypedValue::TypedValue(nullptr_t){
  type = TypeTag::Null;
  value.ptr_v = nullptr;
}
TypedValue::TypedValue(TypedValue* v){
  type = TypeTag::Rvalue;
  value.rvalue_v = v;
}
TypedValue::TypedValue(bool b){
  type = TypeTag::Bool;
  value.bool_v = b;
}
TypedValue::TypedValue(Int i){
  type = TypeTag::Int;
  value.int_v = i;
}
TypedValue::TypedValue(Float f){
  type = TypeTag::Float;
  value.float_v = f;
}
TypedValue::TypedValue(String* s){
  type = TypeTag::String;
  value.string_v = s;
  s->incRefCount();
}
TypedValue::TypedValue(Function* p){
  type = TypeTag::Func;
  value.func_v = p;
  p->incRefCount();
}
TypedValue::TypedValue(PartiallyApplied* p){
  type = TypeTag::Partial;
  value.partial_v = p;
  p->incRefCount();
}
TypedValue::TypedValue(Array* p){
  type = TypeTag::Array;
  value.array_v = p;
  p->incRefCount();
}
TypedValue::TypedValue(const void* p){
  type = TypeTag::Ptr;
  value.ptr_v = const_cast<void*>(p);
}

//assignment

TypedValue& TypedValue::operator=(nullptr_t){
  this->clear_();
  this->type = TypeTag::Null;
  this->value.ptr_v = nullptr;
  return *this;
}
TypedValue& TypedValue::operator=(TypedValue* v){
  this->clear_();
  this->type = TypeTag::Rvalue;
  this->value.rvalue_v = v;
  return *this;
}
TypedValue& TypedValue::operator=(bool val){
  this->clear_();
  this->type = TypeTag::Bool;
  this->value.bool_v = val;
  return *this;
}
TypedValue& TypedValue::operator=(Int val){
  this->clear_();
  this->type = TypeTag::Int;
  this->value.int_v = val;
  return *this;
}
TypedValue& TypedValue::operator=(Float val){
  this->clear_();
  this->type = TypeTag::Float;
  this->value.float_v = val;
  return *this;
}
TypedValue& TypedValue::operator=(String* val){
  this->clear_();
  this->type = TypeTag::String;
  this->value.string_v = val;
  val->incRefCount();
  return *this;
}
TypedValue& TypedValue::operator=(Function* val){
  this->clear_();
  this->type = TypeTag::Func;
  this->value.func_v = val;
  val->incRefCount();
  return *this;
}
TypedValue& TypedValue::operator=(PartiallyApplied* val){
  this->clear_();
  this->type = TypeTag::Partial;
  this->value.partial_v = val;
  val->incRefCount();
  return *this;
}
TypedValue& TypedValue::operator=(Array* val){
  this->clear_();
  this->type = TypeTag::Array;
  this->value.array_v = val;
  val->incRefCount();
  return *this;
}
TypedValue& TypedValue::operator=(const void* val){
  this->clear_();
  this->type = TypeTag::Ptr;
  this->value.ptr_v = const_cast<void*>(val);
  return *this;
}

//copy/move

TypedValue::TypedValue(TypedValue&& other)noexcept{
  this->move_(std::move(other));
}
TypedValue& TypedValue::operator=(TypedValue&& other)noexcept{
  this->clear_();
  this->move_(std::move(other));
  return *this;
}

TypedValue::TypedValue(const TypedValue& other)noexcept{
  this->copy_(other);
}

TypedValue& TypedValue::operator=(const TypedValue& other)noexcept{
  this->clear_();
  this->copy_(other);
  return *this;
}

//other stuff

void TypedValue::add(const TypedValue& rhs){
  if(this->type == TypeTag::Rvalue) return this->value.rvalue_v->add(rhs);
  const TypedValue* other = rhs.type == TypeTag::Rvalue? rhs.value.rvalue_v : &rhs;
  
  switch(this->type){
  case TypeTag::Int:
    switch(other->type){
    case TypeTag::Int:
      this->value.int_v += other->value.int_v;
      break;
    case TypeTag::Float:
      this->type = TypeTag::Float;
      this->value.float_v =
        static_cast<Float>(this->value.int_v)
        + other->value.float_v;
      break;
    default:
      goto error;
    }
    break;
  case TypeTag::Float:
    switch(other->type){
    case TypeTag::Int:
      this->value.float_v += static_cast<Float>(other->value.int_v);
      break;
    case TypeTag::Float:
      this->value.float_v += other->value.float_v;
      break;
    default:
      goto error;
    }
    break;
  default:
    goto error;
  }
  return;
  
error:
  VM* vm = VM::getCurrentVM();
  char* msg = dynSprintf(
    "%d: Type error. Unsupported operation %s + %s.",
    vm->getFrame()->func->getLine(vm->getFrame()->ip),
    this->typeStr(),
    other->typeStr()
  );
  vm->print(msg);
  delete[] msg;
  vm->errorJmp(1);
}

void TypedValue::sub(const TypedValue& rhs){
  if(this->type == TypeTag::Rvalue) return this->value.rvalue_v->sub(rhs);
  const TypedValue* other = rhs.type == TypeTag::Rvalue? rhs.value.rvalue_v : &rhs;
  
  switch(this->type){
  case TypeTag::Int:
    switch(other->type){
    case TypeTag::Int:
      this->value.int_v -= other->value.int_v;
      break;
    case TypeTag::Float:
      this->type = TypeTag::Float;
      this->value.float_v
        = static_cast<Float>(this->value.int_v) - other->value.float_v;
      break;
    default:
      goto error;
    }
    break;
  case TypeTag::Float:
    switch(other->type){
    case TypeTag::Int:
      this->value.float_v -= static_cast<Float>(other->value.int_v);
      break;
    case TypeTag::Float:
      this->value.float_v -= other->value.float_v;
      break;
    default:
      goto error;
    }
    break;
  default:
    goto error;
  }
  return;
  
error:
  VM* vm = VM::getCurrentVM();
  char* msg = dynSprintf(
    "%d: Type error. Unsupported operation %s - %s.",
    vm->getFrame()->func->getLine(vm->getFrame()->ip),
    this->typeStr(),
    other->typeStr()
  );
  vm->print(msg);
  delete[] msg;
  vm->errorJmp(1);
}

void TypedValue::mul(const TypedValue& rhs){
  if(this->type == TypeTag::Rvalue) return this->value.rvalue_v->mul(rhs);
  const TypedValue* other = rhs.type == TypeTag::Rvalue? rhs.value.rvalue_v : &rhs;
  
  switch(this->type){
  case TypeTag::Int:
    switch(other->type){
    case TypeTag::Int:
      this->value.int_v *= other->value.int_v;
      break;
    case TypeTag::Float:
      this->type = TypeTag::Float;
      this->value.float_v
        = static_cast<Float>(this->value.int_v) * other->value.float_v;
      break;
    default:
      goto error;
    }
    break;
  case TypeTag::Float:
    switch(other->type){
    case TypeTag::Int:
      this->value.float_v *= static_cast<Float>(other->value.int_v);
      break;
    case TypeTag::Float:
      this->value.float_v *= other->value.float_v;
      break;
    default:
      goto error;
    }
    break;
  default:
    goto error;
  }
  return;
  
error:
  VM* vm = VM::getCurrentVM();
  char* msg = dynSprintf(
    "%d: Type error. Unsupported operation %s * %s.",
    vm->getFrame()->func->getLine(vm->getFrame()->ip),
    this->typeStr(),
    other->typeStr()
  );
  vm->print(msg);
  delete[] msg;
  vm->errorJmp(1);
}

void TypedValue::div(const TypedValue& rhs){
  if(this->type == TypeTag::Rvalue) return this->value.rvalue_v->div(rhs);
  const TypedValue* other = rhs.type == TypeTag::Rvalue? rhs.value.rvalue_v : &rhs;
  
  switch(this->type){
  case TypeTag::Int:
    switch(other->type){
    case TypeTag::Int:
      if(other->value.int_v == 0){
        goto div_zero_error;
      }
      this->value.int_v /= other->value.int_v;
      break;
    case TypeTag::Float:
      this->type = TypeTag::Float;
      this->value.float_v
        = static_cast<Float>(this->value.int_v) / other->value.float_v;
      break;
    default:
      goto type_error;
    }
    break;
  case TypeTag::Float:
    switch(other->type){
    case TypeTag::Int:
      this->value.float_v /= static_cast<Float>(other->value.int_v);
      break;
    case TypeTag::Float:
      this->value.float_v /= other->value.float_v;
      break;
    default:
      goto type_error;
    }
    break;
  default:
    goto type_error;
  }
  return;
  
type_error:
  {
    VM* vm = VM::getCurrentVM();
    char* msg = dynSprintf(
      "%d: Type error. Unsupported operation %s / %s.",
      vm->getFrame()->func->getLine(vm->getFrame()->ip),
      this->typeStr(),
      other->typeStr()
    );
    vm->print(msg);
    delete[] msg;
    vm->errorJmp(1);
  }
div_zero_error:
  {
    VM* vm = VM::getCurrentVM();
    char* msg = dynSprintf(
      "%d: Division by zero error.",
      vm->getFrame()->func->getLine(vm->getFrame()->ip)
    );
    vm->print(msg);
    delete[] msg;
    vm->errorJmp(2);
  }
}

void TypedValue::mod(const TypedValue& rhs){
  if(this->type == TypeTag::Rvalue) return this->value.rvalue_v->mod(rhs);
  const TypedValue* other = rhs.type == TypeTag::Rvalue? rhs.value.rvalue_v : &rhs;
  
  switch(this->type){
  case TypeTag::Int:
    switch(other->type){
    case TypeTag::Int:
      if(other->value.int_v == 0){
        goto div_zero_error;
      }
      this->value.int_v %= other->value.int_v;
      break;
    case TypeTag::Float:
      this->type = TypeTag::Float;
      this->value.float_v = 
        std::fmod(static_cast<Float>(this->value.int_v), other->value.float_v);
      break;
    default:
      goto type_error;
    }
    break;
  case TypeTag::Float:
    switch(other->type){
    case TypeTag::Int:
      this->value.float_v =
        std::fmod(this->value.float_v, static_cast<Float>(other->value.int_v));
      break;
    case TypeTag::Float:
      this->value.float_v = std::fmod(this->value.float_v, other->value.float_v);
      break;
    default:
      goto type_error;
    }
    break;
  default:
    goto type_error;
  }
  return;
  
type_error:
  {
    VM* vm = VM::getCurrentVM();
    char* msg = dynSprintf(
      "%d: Type error. Unsupported operation %s %% %s.",
      vm->getFrame()->func->getLine(vm->getFrame()->ip),
      this->typeStr(),
      other->typeStr()
    );
    vm->print(msg);
    delete[] msg;
    vm->errorJmp(1);
  }
div_zero_error:
  {
    VM* vm = VM::getCurrentVM();
    char* msg = dynSprintf(
      "%d: Division by zero error.",
      vm->getFrame()->func->getLine(vm->getFrame()->ip)
    );
    vm->print(msg);
    delete[] msg;
    vm->errorJmp(2);
  }
}

void TypedValue::append(const TypedValue& rhs){
  if(this->type == TypeTag::Rvalue) return this->value.rvalue_v->append(rhs);
  const TypedValue* other = rhs.type == TypeTag::Rvalue? rhs.value.rvalue_v : &rhs;
  
  switch(this->type){
  case TypeTag::Bool:
    switch(other->type){
    case TypeTag::String:
      *this = make_new<String>(
        this->value.bool_v,
        (const String*)other->value.string_v
      );
      break;
    case TypeTag::Array:
      *this = constructArray(new Array, *this, *other->value.array_v);
      break;
    default:
      goto error;
    }
    break;
  case TypeTag::Int:
    switch(other->type){
    case TypeTag::String:
      *this = make_new<String>(
        this->value.int_v,
        (const String*)other->value.string_v
      );
      break;
    case TypeTag::Array:
      *this = constructArray(new Array, *this, *other->value.array_v);
      break;
    default:
      goto error;
    }
    break;
  case TypeTag::Float:
    switch(other->type){
    case TypeTag::String:
      *this = make_new<String>(
        this->value.float_v,
        (const String*)other->value.string_v
      );
      break;
    case TypeTag::Array:
      *this = constructArray(new Array, *this, *other->value.array_v);
      break;
    default:
      goto error;
    }
    break;
  case TypeTag::String:
    switch(other->type){
    case TypeTag::Bool:
      *this = make_new<String>(
        (const String*)this->value.string_v,
        other->value.bool_v
      );
      break;
    case TypeTag::Int:
      *this = make_new<String>(
        (const String*)this->value.string_v,
        other->value.int_v
      );
      break;
    case TypeTag::Float:
      *this = make_new<String>(
        (const String*)this->value.string_v,
        other->value.float_v
      );
      break;
    case TypeTag::String:
      *this = make_new<String>(
        (const String*)this->value.string_v,
        (const String*)other->value.string_v
      );
      break;
    case TypeTag::Array:
      *this = constructArray(new Array, *this, *other->value.array_v);
      break;
    default:
      goto error;
    }
    break;
  case TypeTag::Array:
    switch(other->type){
    case TypeTag::Array:
      std::copy(
        other->value.array_v->begin(),
        other->value.array_v->end(),
        std::back_inserter(*this->value.array_v)
      );
      break;
    default:
      this->value.array_v->push_back(*other);
      break;
    }
    break;
  default:
    switch(other->type){
    case TypeTag::Array:
      *this = constructArray(new Array, *this, *other->value.array_v);
      break;
    default:
      goto error;
    }
  }
  return;
  
error:
  VM* vm = VM::getCurrentVM();
  char* msg = dynSprintf(
    "%d: Type error. Unsupported operation %s ++ %s.",
    vm->getFrame()->func->getLine(vm->getFrame()->ip),
    this->typeStr(),
    other->typeStr()
  );
  vm->print(msg);
  delete[] msg;
  vm->errorJmp(1);
}

void TypedValue::append(TypedValue&& rhs){
  if(rhs.type == TypeTag::Rvalue)
    return this->append(rhs.value.rvalue_v);
  if(this->type == TypeTag::Rvalue)
    return this->value.rvalue_v->append(std::move(rhs));
  TypedValue* other = &rhs;
  
  switch(this->type){
  case TypeTag::Bool:
    switch(other->type){
    case TypeTag::String:
      *this = make_new<String>(
        this->value.bool_v,
        (const String*)other->value.string_v
      );
      break;
    case TypeTag::Array:
      other->value.array_v->insert(other->value.array_v->begin(), std::move(*this));
      *this = std::move(*other);
      break;
    default:
      goto error;
    }
  case TypeTag::Int:
    switch(other->type){
    case TypeTag::String:
      *this = make_new<String>(
        this->value.int_v,
        (const String*)other->value.string_v
      );
      break;
    case TypeTag::Array:
      other->value.array_v->insert(other->value.array_v->begin(), std::move(*this));
      *this = std::move(*other);
      break;
    default:
      goto error;
    }
    break;
  case TypeTag::Float:
    switch(other->type){
    case TypeTag::String:
      *this = make_new<String>(
        this->value.float_v,
        (const String*)other->value.string_v
      );
      break;
    case TypeTag::Array:
      other->value.array_v->insert(other->value.array_v->begin(), std::move(*this));
      *this = std::move(*other);
      break;
    default:
      goto error;
    }
    break;
  case TypeTag::String:
    switch(other->type){
    case TypeTag::Bool:
      *this = make_new<String>(
        (const String*)this->value.string_v,
        other->value.bool_v
      );
      break;
    case TypeTag::Int:
      *this = make_new<String>(
        (const String*)this->value.string_v,
        other->value.int_v
      );
      break;
    case TypeTag::Float:
      *this = make_new<String>(
        (const String*)this->value.string_v,
        other->value.float_v
      );
      break;
    case TypeTag::String:
      *this = make_new<String>(
        (const String*)this->value.string_v,
        (const String*)other->value.string_v
      );
      break;
    case TypeTag::Array:
      other->value.array_v->insert(other->value.array_v->begin(), std::move(*this));
      *this = std::move(*other);
      break;
    default:
      goto error;
    }
    break;
  case TypeTag::Array:
    switch(other->type){
    case TypeTag::Array:
      std::copy(
        other->value.array_v->begin(),
        other->value.array_v->end(),
        std::back_inserter(*this->value.array_v)
      );
      other->value.array_v->decRefCount();
      other->type = TypeTag::Null;
      break;
    default:
      this->value.array_v->push_back(std::move(*other));
      break;
    }
  default:
    switch(other->type){
    case TypeTag::Array:
      *this = constructArray(new Array, *this, *other->value.array_v);
      break;
    default:
      goto error;
    }
  }
  return;
  
error:
  VM* vm = VM::getCurrentVM();
  char* msg = dynSprintf(
    "%d: Type error. Unsupported operation %s ++ %s.",
    vm->getFrame()->func->getLine(vm->getFrame()->ip),
    this->typeStr(),
    other->typeStr()
  );
  vm->print(msg);
  delete[] msg;
  vm->errorJmp(1);
}

void TypedValue::neg(){
  if(this->type == TypeTag::Rvalue) return this->value.rvalue_v->neg();
  
  switch(this->type){
  case TypeTag::Int:
    this->value.int_v = -this->value.int_v;
    break;
  case TypeTag::Float:
    this->value.float_v = -this->value.float_v;
    break;
  default:
    goto error;
  }
  return;
  
error:
  VM* vm = VM::getCurrentVM();
  char* msg = dynSprintf(
    "%d: Type error. Unsupported operation -%s.",
    vm->getFrame()->func->getLine(vm->getFrame()->ip),
    this->typeStr()
  );
  vm->print(msg);
  delete[] msg;
  vm->errorJmp(1);
}

void TypedValue::cmp(const TypedValue& rhs){
  if(this->type == TypeTag::Rvalue) return this->value.rvalue_v->cmp(rhs);
  const TypedValue* other = rhs.type == TypeTag::Rvalue? rhs.value.rvalue_v : &rhs;
  
  switch(this->type){
  case TypeTag::Bool:
    if(other->type == TypeTag::Bool){
      this->type = TypeTag::Int;
      this->value.int_v = (Int)this->value.bool_v - (Int)other->value.bool_v;
    }else goto error;
    break;
  case TypeTag::Int:
    if(other->type == TypeTag::Int){
      Int t = this->value.int_v - other->value.int_v;
      this->value.int_v = t > 0? 1 : (t < 0? -1 : 0);
    }else if(other->type == TypeTag::Float){
      Float t = (Float)this->value.int_v - other->value.float_v;
      this->value.int_v = t > 0? 1 : (t < 0? -1 : 0);
    }else goto error;
    break;
  case TypeTag::Float:
    if(other->type == TypeTag::Float){
      Float t = this->value.float_v - other->value.float_v;
      this->type = TypeTag::Int;
      this->value.int_v = t > 0? 1 : (t < 0? -1 : 0);
    }else if(other->type == TypeTag::Int){
      Float t = this->value.float_v - (Float)other->value.int_v;
      this->type = TypeTag::Int;
      this->value.int_v = t > 0? 1 : (t < 0? -1 : 0);
    }else goto error;
    break;
  case TypeTag::String:
    if(other->type == TypeTag::String){
      this->type = TypeTag::Int;
      this->value.int_v = this->value.string_v->cmp(*other->value.string_v);
    }else goto error;
    break;
  default:
    goto error;
  }
  return;
  
error:
  VM* vm = VM::getCurrentVM();
  char* msg = dynSprintf(
    "%d: Type error. Unsupported operation %s <=> %s.",
    vm->getFrame()->func->getLine(vm->getFrame()->ip),
    this->typeStr(),
    other->typeStr()
  );
  vm->print(msg);
  delete[] msg;
  vm->errorJmp(1);
}

void TypedValue::cmp(const TypedValue& rhs, CmpMode mode){
  if(this->type == TypeTag::Rvalue) return this->value.rvalue_v->cmp(rhs, mode);
  const TypedValue* other = rhs.type == TypeTag::Rvalue? rhs.value.rvalue_v : &rhs;
  
  if(this->type != other->type) goto error;
  
  int cmp;
  switch(this->type){
  case TypeTag::Bool:
    cmp = this->value.bool_v - other->value.bool_v;
    break;
  case TypeTag::Int:
    cmp = this->value.int_v - other->value.int_v;
    break;
  case TypeTag::Float:
    cmp = this->value.float_v - other->value.float_v;
    break;
  case TypeTag::String:
    cmp = this->value.string_v->cmp(*other->value.string_v);
    break;
  default:
    goto error;
  }
  
  this->type = TypeTag::Bool;
  
  switch(mode){
  case CmpMode::Equal:
    this->value.bool_v = cmp == 0;
    break;
  case CmpMode::NotEqual:
    this->value.bool_v = cmp != 0;
    break;
  case CmpMode::Greater:
    this->value.bool_v = cmp > 0;
    break;
  case CmpMode::Less:
    this->value.bool_v = cmp < 0;
    break;
  case CmpMode::GreaterEqual:
    this->value.bool_v = cmp >= 0;
    break;
  case CmpMode::LessEqual:
    this->value.bool_v = cmp <= 0;
    break;
  }
  return;
  
error:
  VM* vm = VM::getCurrentVM();
  char* msg = dynSprintf(
    "%d: Type error. Unsupported operation %s <=> %s.",
    vm->getFrame()->func->getLine(vm->getFrame()->ip),
    this->typeStr(),
    other->typeStr()
  );
  vm->print(msg);
  delete[] msg;
  vm->errorJmp(1);
}

void TypedValue::boolNot(){
  if(this->type == TypeTag::Rvalue) return this->value.rvalue_v->boolNot();
  
  if(this->type != TypeTag::Bool){
    this->toBool();
  }
  this->value.bool_v = !this->value.bool_v;
  return;
}

void TypedValue::get(const TypedValue& rhs){
  if(this->type == TypeTag::Rvalue) return this->value.rvalue_v->get(rhs);
  const TypedValue* other = rhs.type == TypeTag::Rvalue? rhs.value.rvalue_v : &rhs;
  
  Int index;
  if(other->type == TypeTag::Int){
    index = other->value.int_v;
  }else goto type_error;
  
  switch(this->type){
  case TypeTag::Array:
    if(index < 0) index = this->value.array_v->size() + index;
    if(index >= this->value.array_v->size()) goto index_error;
    *this = this->value.array_v->operator[](index);
    break;
  case TypeTag::String:
    {
      if(index < 0) index = this->value.string_v->utf8Len() + index;
      auto glyph = this->value.string_v->utf8Get(index);
      if(glyph == 0xffffffff) goto index_error;
      *this = make_new<String, uint32_t>(glyph);
    }
    break;
  default:
    goto type_error;
  }
  return;
  
type_error:
  {
    VM* vm = VM::getCurrentVM();
    char* msg = dynSprintf(
      "%d: Type error. Unsupported operation %s[%s].",
      vm->getFrame()->func->getLine(vm->getFrame()->ip),
      this->typeStr(),
      other->typeStr()
    );
    vm->print(msg);
    delete[] msg;
    vm->errorJmp(1);
  }
index_error:
  {
    VM* vm = VM::getCurrentVM();
    char* msg = dynSprintf(
      "%d: Index out of range error: %lld.",
      vm->getFrame()->func->getLine(vm->getFrame()->ip),
      (long long)index
    );
    vm->print(msg);
    delete[] msg;
    vm->errorJmp(3);
  }
}

void TypedValue::slice(const TypedValue& rhs1, const TypedValue& rhs2){
  if(this->type == TypeTag::Rvalue) return this->value.rvalue_v->slice(rhs1, rhs2);
  const TypedValue* other1 =
    rhs1.type == TypeTag::Rvalue? rhs1.value.rvalue_v : &rhs1;
  const TypedValue* other2 =
    rhs2.type == TypeTag::Rvalue? rhs2.value.rvalue_v : &rhs2;
  
  Int index1, index2;
  if(other1->type == TypeTag::Int){
    index1 = other1->value.int_v;
  }else if(other1->type == TypeTag::Null){
    index1 = 0;
  }else goto error;
  if(other2->type == TypeTag::Int){
    index2 = other2->value.int_v;
  }else if(other2->type == TypeTag::Null){
    index2 = std::numeric_limits<Int>::max();
  }else goto error;
  
  switch(this->type){
  case TypeTag::Array:
    {
      int size = this->value.array_v->size();
      
      if(index1 < 0) index1 = size + index1;
      if(index2 < 0) index2 = size + index2;
      
      if(index1 < 0) index1 = 0;
      else if(index1 >= size) index1 = size;
      if(index2 < 0) index2 = 0;
      else if(index2 >= size) index2 = size;
    }
    if(index2 <= index1){
      *this = new Array;
    }else{
      *this = this->value.array_v->slice(index1, index2);
    }
    break;
  case TypeTag::String:
    {
      int size = this->value.string_v->utf8Len();
      
      if(index1 < 0) index1 = size + index1;
      if(index2 < 0) index2 = size + index2;
      
      if(index1 < 0) index1 = 0;
      else if(index1 >= size) index1 = size;
      if(index2 < 0) index2 = 0;
      else if(index2 >= size) index2 = size;
    }
    if(index2 <= index1){
      *this = make_new<String>();
    }else{
      *this = make_new<String>(
        this->value.string_v->str() + index1,
        (int)(index2 - index1)
      );
    }
    break;
  default:
    goto error;
  }
  return;
  
error:
  VM* vm = VM::getCurrentVM();
  char* msg = dynSprintf(
    "%d: Type error. Unsupported operation %s[%s:%s].",
    vm->getFrame()->func->getLine(vm->getFrame()->ip),
    this->typeStr(),
    rhs1.typeStr(),
    rhs2.typeStr()
  );
  vm->print(msg);
  delete[] msg;
  vm->errorJmp(1);
}

void TypedValue::toBool(){
  TypedValue* value = this->type == TypeTag::Rvalue? this->value.rvalue_v : this;
  
  switch(value->type){
  case TypeTag::Bool:
    break;
  case TypeTag::Int:
    value->value.bool_v = static_cast<bool>(value->value.int_v);
    break;
  case TypeTag::Float:
    value->value.bool_v = static_cast<bool>(value->value.float_v);
    break;
  default:
    goto error;
  }
  value->type = TypeTag::Bool;
  return;
  
error:
  VM* vm = VM::getCurrentVM();
  char* msg = dynSprintf(
    "%d: Type error. Unsupported operation %s to bool.",
    vm->getFrame()->func->getLine(vm->getFrame()->ip),
    this->typeStr()
  );
  vm->print(msg);
  delete[] msg;
  vm->errorJmp(1);
}

void TypedValue::toBool(bool* ret){
  TypedValue* value = this->type == TypeTag::Rvalue? this->value.rvalue_v : this;
  
  switch(value->type){
  case TypeTag::Bool:
    *ret = value->value.bool_v;
    break;
  case TypeTag::Int:
    *ret = (bool)value->value.int_v;
    break;
  case TypeTag::Float:
    *ret = (bool)value->value.float_v;
    break;
  default:
    goto error;
  }
  return;
  
error:
  VM* vm = VM::getCurrentVM();
  char* msg = dynSprintf(
    "%d: Type error. Unsupported operation %s to bool.",
    vm->getFrame()->func->getLine(vm->getFrame()->ip),
    this->typeStr()
  );
  vm->print(msg);
  delete[] msg;
  vm->errorJmp(1);
}

void TypedValue::toInt(){
  TypedValue* value = this->type == TypeTag::Rvalue? this->value.rvalue_v : this;
  
  switch(value->type){
  case TypeTag::Bool:
    value->value.int_v = static_cast<Int>(value->value.bool_v);
    break;
  case TypeTag::Int:
    break;
  case TypeTag::Float:
    value->value.int_v = static_cast<Int>(value->value.float_v);
    break;
  case TypeTag::String: {
      Int i;
      if(!toInt_(value->value.string_v, &i)){
        goto bad_op_error;
      }
      value->value.string_v->decRefCount();
      value->value.int_v = i;
    }break;
  default:
    goto type_error;
  }
  value->type = TypeTag::Int;
  return;
  
type_error:
  {
    VM* vm = VM::getCurrentVM();
    char* msg = dynSprintf(
      "%d: Type error. Unsupported operation %s to int.",
      vm->getFrame()->func->getLine(vm->getFrame()->ip),
      this->typeStr()
    );
    vm->print(msg);
    delete[] msg;
    vm->errorJmp(1);
  }
bad_op_error:
  {
    VM* vm = VM::getCurrentVM();
    char* msg = dynSprintf(
      "%d: Bad conversion error.",
      vm->getFrame()->func->getLine(vm->getFrame()->ip)
    );
    vm->print(msg);
    delete[] msg;
    vm->errorJmp(4);
  }
}

void TypedValue::toFloat(){
  TypedValue* value = this->type == TypeTag::Rvalue? this->value.rvalue_v : this;
  
  switch(value->type){
  case TypeTag::Bool:
    value->value.float_v = static_cast<Float>(value->value.bool_v);
    break;
  case TypeTag::Int:
    value->value.float_v = static_cast<Float>(value->value.int_v);
    break;
  case TypeTag::Float:
    break;
  case TypeTag::String: {
      Float f;
      if(!toFloat_(value->value.string_v, &f)){
        goto bad_op_error;
      }
      value->value.string_v->decRefCount();
      value->value.float_v = f;
    }break;
  default:
    goto type_error;
  }
  value->type = TypeTag::Float;
  return;
  
type_error:
  {
    VM* vm = VM::getCurrentVM();
    char* msg = dynSprintf(
      "%d: Type error. Unsupported operation %s to float.",
      vm->getFrame()->func->getLine(vm->getFrame()->ip),
      this->typeStr()
    );
    vm->print(msg);
    delete[] msg;
    vm->errorJmp(1);
  }
bad_op_error:
  {
    VM* vm = VM::getCurrentVM();
    char* msg = dynSprintf(
      "%d: Bad conversion error.",
      vm->getFrame()->func->getLine(vm->getFrame()->ip)
    );
    vm->print(msg);
    delete[] msg;
    vm->errorJmp(4);
  }
}

void TypedValue::toString(){
  TypedValue* value = this->type == TypeTag::Rvalue? this->value.rvalue_v : this;
  
  String* s;
  switch(value->type){
  case TypeTag::Bool:
    s = make_new<String>(value->value.bool_v? "true" : "false");
    break;
  case TypeTag::Int:
    s = make_new<String>(value->value.int_v);
    break;
  case TypeTag::Float:
    s = make_new<String>(value->value.float_v);
    break;
  case TypeTag::String:
    return;
  default:
    goto error;
  }
  value->value.string_v = s;
  s->incRefCount();
  return;
  
error:
  VM* vm = VM::getCurrentVM();
  char* msg = dynSprintf(
    "%d: Type error. Unsupported operation %s to string.",
    vm->getFrame()->func->getLine(vm->getFrame()->ip),
    this->typeStr()
  );
  vm->print(msg);
  delete[] msg;
  vm->errorJmp(1);
}

void TypedValue::toPartial(){
  if(this->type != TypeTag::Func){
    VM* vm = VM::getCurrentVM();
    char* msg = dynSprintf(
      "%d: Type error. Unsupported operation %s to function.",
      vm->getFrame()->func->getLine(vm->getFrame()->ip),
      this->typeStr()
    );
    vm->print(msg);
    delete[] msg;
    vm->errorJmp(1);
  }
  auto proc = this->value.func_v;
  this->value.partial_v = new PartiallyApplied(proc);
  this->value.partial_v->incRefCount();
  proc->decRefCount();
  this->type = TypeTag::Partial;
}

const char* TypedValue::typeStr() const {
  switch(this->type){
  case TypeTag::Null:
    return "null";
  case TypeTag::Bool:
    return "bool";
  case TypeTag::Int:
    return "int";
  case TypeTag::Float:
    return "float";
  case TypeTag::String:
    return "string";
  case TypeTag::Func:
  case TypeTag::Partial:
    return "function";
  case TypeTag::Array:
    return "array";
  default:
    assert(false);
  }
}

#ifndef NDEBUG
std::string TypedValue::toStrDebug()const{
  using namespace std::string_literals;
  
  switch(this->type){
  case TypeTag::None:
    return "_"s;
  case TypeTag::Null:
    return "null"s;
  case TypeTag::Bool:
    return this->value.boolToStrDebug();
  case TypeTag::Int:
    return this->value.intToStrDebug();
  case TypeTag::Float:
    return this->value.floatToStrDebug();
  case TypeTag::String:
    return this->value.toStrDebug();
  case TypeTag::Rvalue:
    return this->value.rvalue_v->toStrDebug();
  case TypeTag::Func:
    return this->value.func_v->toStrDebug();
  case TypeTag::Partial:
    return this->value.partial_v->toStrDebug();
  case TypeTag::Array:
    return this->value.array_v->toStrDebug();
  default:
    return std::to_string((uintptr_t)this->value.ptr_v);
  }
}
#endif
