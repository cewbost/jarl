#ifndef DELEGATE_VECTOR_SET_H_INCLUDED
#define DELEGATE_VECTOR_SET_H_INCLUDED

#include <utility>
#include <cassert>

template<class Key, class Comp = typename std::equal_to<const Key>>
class DelegateVectorSet: private std::vector<Key>{
public:
  
  typedef Key key_type;
  typedef Key value_type;
  
private:
  
  Comp comp_;
  DelegateVectorSet<Key, Comp>* delegate_;
  
  int findKey_(const Key& key)const{
    for(int i = 0; i < this->size(); ++i){
      if(comp_(this->std::vector<Key>::operator[](i), key)){
        return i;
      }
    }
    return -1;
  }
  
public:
  
  DelegateVectorSet(Comp c = Comp())
  : comp_(std::move(c)), delegate_(nullptr){}
  
  DelegateVectorSet(DelegateVectorSet* del)
  : comp_(del->comp_), delegate_(del){}
  
  int size()const{
    if(delegate_) return this->delegate_->size() + this->std::vector<Key>::size();
    else return this->std::vector<Key>::size();
  }
  
  const Key& at(int idx)const{
    if(delegate_){
      int delegate_size = this->delegate_->size();
      if(delegate_size > idx) return this->delegate_->at(idx);
      else return this->std::vector<Key>::operator[](idx - delegate_size);
    }else return this->std::vector<Key>::operator[](idx);
  }
  int operator[](const Key& key)const{
    for(int i = 0; i < this->size(); ++i){
      if(comp_(this->std::vector<Key>::operator[](i), key)){
        if(delegate_) return i + this->delegate_->size();
        else return i;
      }
    }
    if(delegate_) return this->delegate_->operator[](key);
    else return -1;
  }
  
  int insert(const Key& key){
    int i = this->findKey_(key);
    if(i == -1){
      this->push_back(key);
      return this->size() - 1;
    }else{
      if(delegate_) return this->delegate_->size() + i;
      else return i;
    }
  }
  
  int insert(Key&& key){
    int i = this->findKey_(key);
    if(i == -1){
      this->push_back(std::move(key));
      return this->size() - 1;
    }else{
      if(delegate_) return this->delegate_->size() + i;
      else return i;
    }
  }
  
  template<class... T>
  int emplace(T... t){
    int idx = this->std::vector<Key>::size();
    this->emplace_back(t...);
    auto& key = this->back();
    
    for(int i = 0; i < idx; ++i){
      if(comp_(this->std::vector<Key>::operator[](i), key)){
        this->pop_back();
        if(delegate_) return this->delegate_->size() + i;
        else return i;
      }
    }
    return this->size() - 1;
  }
  
  void set_delegate(DelegateVectorSet* del){
    this->delegate_ = del;
  }
};

#endif