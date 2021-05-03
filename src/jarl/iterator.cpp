#include "iterator.h"

Iterator::Iterator(const TypedValue& src){
  switch(src.type){
  case TypeTag::Array:
    this->data_.array_v.ref = src.value.array_v;
    src.value.array_v->incRefCount();
    this->data_.array_v.current_it = src.value.array_v->begin();
    this->data_.array_v.end_it = src.value.array_v->end();
    break;
  case TypeTag::Table:
    this->data_.table_v.ref = src.value.table_v;
    src.value.table_v->incRefCount();
    this->data_.table_v.current_it = src.value.table_v->begin();
    this->data_.table_v.end_it = src.value.table_v->end();
    break;
  default:
    assert(false);
  }
  this->type_ = src.type;
}

Iterator::~Iterator(){
  switch(this->type_){
  case TypeTag::Array:
    this->data_.array_v.ref->decRefCount();
    break;
  case TypeTag::Table:
    this->data_.table_v.ref->decRefCount();
    break;
  default:
    assert(false);
  }
}

bool Iterator::ended()const{
  switch(this->type_){
  case TypeTag::Array:
    return this->data_.array_v.current_it == this->data_.array_v.end_it;
  case TypeTag::Table:
    return this->data_.table_v.current_it == this->data_.table_v.end_it;
  default:
    assert(false);
  }
  return true;
}

void Iterator::advance(){
  switch(this->type_){
  case TypeTag::Array:
    this->data_.array_v.current_it =
      std::next(this->data_.array_v.current_it);
    break;
  case TypeTag::Table:
    this->data_.table_v.current_it =
      std::next(this->data_.table_v.current_it);
    break;
  default:
    assert(false);
  }
}

TypedValue Iterator::getKey()const{
  switch(this->type_){
  case TypeTag::Array:
    return TypedValue(
      this->data_.array_v.current_it -
      this->data_.array_v.ref->begin()
    );
  case TypeTag::Table:
    return this->data_.table_v.current_it->first;
  default:
    assert(false);
  }
  return nullptr;
}

TypedValue Iterator::getValue()const{
  switch(this->type_){
  case TypeTag::Array:
    return *this->data_.array_v.current_it;
  case TypeTag::Table:
    return this->data_.table_v.current_it->second;
  default:
    assert(false);
  }
  return nullptr;
}

#ifndef NDEBUG
std::string Iterator::toStrDebug()const{
  using namespace std::string_literals;
  
  std::string ret;
  switch(this->type_){
  case TypeTag::Array:
    ret = "array"s;
    break;
  case TypeTag::Table:
    ret = "table"s;
    break;
  default:
    assert(false);
  }
  ret += " iterator ["s;
  if(this->ended()){
    ret += "ended"s;
  }else{
    ret += this->getKey().toStrDebug();
    ret += ": "s;
    ret += this->getValue().toStrDebug();
  }
  ret += "]"s;
  return ret;
}
#endif

#ifndef NDEBUG
std::string Table::toStrDebug()const{
  using namespace std::string_literals;
  std::string ret = "{";
  
  auto it = this->begin();
  if(it != this->end()){
    ret += it->first.toStrDebug() + ": "s + it->second.toStrDebug();
    ++it;
    while(it != this->end()){
      ret += ", "s + it->first.toStrDebug() + ": "s + it->second.toStrDebug();
      ++it;
    }
  }
  ret += "}";
  return ret;
}
#endif
