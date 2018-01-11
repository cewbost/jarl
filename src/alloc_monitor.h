#ifndef ALLOC_MONITOR_H_INCLUDED
#define ALLOC_MONITOR_H_INCLUDED

/*
  This class allows for monitoring of allocations/deallocations and listing all
  objects. Used for debugging.
*/

#include <vector>
#include <algorithm>
#include <functional>

enum class AllocMsg{
  Allocation,
  Deallocation,
  DoubleAllocation,
  InvalidFree
};

template<class Type>
struct AllocMonitor{
  typedef std::function<void(AllocMsg, const Type*)> CallbackType;
  
  std::vector<const Type*> allocations;
  CallbackType callback;
  
  AllocMonitor(CallbackType&& cb): callback(cb){}
  
  void push(const Type* alloc){
    auto it = std::find(this->allocations.begin(), this->allocations.end(), alloc);
    if(it != this->allocations.end()){
      callback(AllocMsg::DoubleAllocation, alloc);
    }else{
      callback(AllocMsg::Allocation, alloc);
    }
    allocations.push_back(alloc);
  }
  void pop(const Type* alloc){
    auto it = std::find(this->allocations.begin(), this->allocations.end(), alloc);
    if(it == this->allocations.end()){
      callback(AllocMsg::InvalidFree, alloc);
    }else{
      callback(AllocMsg::Deallocation, alloc);
      this->allocations.erase(it);
    }
  }
};

template<class Type, AllocMonitor<Type>& monitor>
struct MonitoredTrait{
  MonitoredTrait(){
    monitor.push(static_cast<const Type*>(this));
  }
  MonitoredTrait(const MonitoredTrait&){
    monitor.push(static_cast<const Type*>(this));
  }
  MonitoredTrait(MonitoredTrait&&){
    monitor.push(static_cast<const Type*>(this));
  }
  ~MonitoredTrait(){
    monitor.pop(static_cast<const Type*>(this));
  }
};

#endif