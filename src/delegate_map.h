#ifndef DELEGATE_MAP_H_INCLUDED
#define DELEGATE_MAP_H_INCLUDED

#include <utility>
#include <cassert>

template<template<class, class> class M, class K, class V>
class DelegateMap{
public:
  
  typedef K key_type;
  typedef V mapped_type;
  typedef std::pair<const key_type, mapped_type> value_type;
  typedef M<key_type, mapped_type> map_type;
  
  typedef typename map_type::iterator iterator;
  typedef typename map_type::const_iterator const_iterator;
  
private:
  
  DelegateMap*  delegate_;
  map_type      map_;
  
public:
  
  DelegateMap()
  : delegate_(nullptr), map_(){}
  DelegateMap(DelegateMap* del)
  : delegate_(del), map_(){}
  ~DelegateMap(){
    delete delegate_;
  }
  
  mapped_type& operator[](const key_type& key){
    auto it = this->map_.find(key);
    if(it == this->map_.end()){
      if(this->delegate_){
        it = this->delegate_->map_.find(key);
        if(it == this->delegate_->map_.end()){
          return this->map_.insert(std::make_pair(key, mapped_type())).first->second;
        }else{
          return it->second;
        }
      }else{
        return this->map_.insert(std::make_pair(key, mapped_type())).first->second;
      }
    }else{
      return it->second;
    }
  }
  const mapped_type& operator[](const key_type& key)const{
    auto it = this->map_.find(key);
    if(it == this->map_.cend()){
      if(this->delegate_){
        return this->delegate_->map_.find(key)->second;
      }else{
        return this->map_.cend()->second;
      }
    }else{
      return it->second;
    }
  }
  
  iterator find(const key_type& key){
    auto it = this->map_.find(key);
    if(it == this->map_.end()){
      if(this->delegate_){
        auto del_it = this->delegate_->find(key);
        if(del_it == this->delegate_->end()){
          return this->map_.end();
        }else return del_it;
      }else{
        return it;
      }
    }else{
      return it;
    }
  }
  
  const_iterator find(const key_type& key)const{
    auto it = ((const DelegateMap*)this)->map_.find(key);
    if(it == this->map_.cend()){
      if(this->delegate_){
        auto del_it = ((const DelegateMap*)this)->delegate_->find(key);
        if(del_it == this->delegate_->end()){
          return this->map_.cend();
        }else return del_it;
      }else{
        return it;
      }
    }else{
      return it;
    }
  }

  bool contains(const key_type& key)const{
    return this->find(key) != this->end();
  }
  
  size_t size()const{
    size_t ret = this->map_.size();
    if(this->delegate_){
      ret += this->delegate_->size();
    }
    return ret;
  }
  
  void set_delegate(DelegateMap* del){
    if(this->delegate_) delete this->delegate_;
    this->delegate_ = del;
  }
  DelegateMap* get_delegate(){
    return this->delegate_.get();
  }
  DelegateMap* steal_delegate(){
    auto delegate = this->delegate_;
    this->delegate_ = nullptr;
    return delegate;
  }
  
  typename map_type::iterator begin(){return this->map_.begin();}
  typename map_type::iterator end(){return this->map_.end();}
  typename map_type::const_iterator begin()const{return this->map_.begin();}
  typename map_type::const_iterator end()const{return this->map_.end();}
  
  map_type& direct(){
    return this->map_;
  }
  const map_type& direct()const{
    return this->map_;
  }
  map_type& base(){
    if(this->delegate_){
      return this->delegate_->base();
    }else return this->map_;
  }
  const map_type& base()const{
    if(this->delegate_){
      return this->delegate_->base();
    }else return this->map_;
  }
};

#endif
