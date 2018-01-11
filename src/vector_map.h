#ifndef VECTOR_MAP_H_INCLUDED
#define VECTOR_MAP_H_INCLUDED

#include <vector>
#include <utility>

template<class Key, class Val, class Comp = typename std::equal_to<const Key>>
class VectorMap: public std::vector<std::pair<const Key, Val>>{
public:
  
  typedef Key key_type;
  typedef Val mapped_type;
  typedef std::pair<const Key, Val> value_type;
  
  typedef typename std::vector<std::pair<const Key, Val>>::iterator
    iterator;
  typedef typename std::vector<std::pair<const Key, Val>>::const_iterator
    const_iterator;
  
private:
  
  Comp comp_;
  
public:
  
  VectorMap(Comp c = Comp())
  : comp_(std::move(c)){}
  
  Val& operator[](const Key& key){
    for(auto& slot: *this){
      if(comp_(slot.first, key)){
        return slot.second;
      }
    }
    this->push_back(std::make_pair(key, Val()));
    return this->back().second;
  }
  Val& operator[](Key&& key){
    for(auto& slot: *this){
      if(comp_(slot.first, key)){
        return slot.second;
      }
    }
    this->push_back(std::make_pair(std::move(key), Val()));
    return this->back().second;
  }
  
  iterator find(const Key& key){
    for(auto it = this->begin(); it != this->end(); ++it){
      if(comp_(it->first, key)) return it;
    }
    return this->end();
  }
  const_iterator find(const Key& key)const{
    for(auto it = this->begin(); it != this->end(); ++it){
      if(comp_(it->first, key)) return it;
    }
    return this->end();
  }
  
  std::pair<iterator, bool> insert(const value_type& val){
    for(auto it = this->begin(); it != this->end(); ++it){
      if(comp_(it->first, val.first)){
        it->second = val.second;
        return std::make_pair(it, false);
      }
    }
    auto size = this->size();
    this->push_back(val);
    return std::make_pair(this->begin() + size, true);
  }
  std::pair<iterator, bool> insert(value_type&& val){
    for(auto it = this->begin(); it != this->end(); ++it){
      if(comp_(it->first, val.first)){
        it->second = val.second;
        return std::make_pair(it, false);
      }
    }
    auto size = this->size();
    this->push_back(std::move(val));
    return std::make_pair(this->begin() + size, true);
  }
};

#endif