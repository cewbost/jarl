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
class MonitoredMixin{
  MonitoredMixin(){
    monitor.push(static_cast<const Type*>(this));
  }
  MonitoredMixin(const MonitoredMixin&){
    monitor.push(static_cast<const Type*>(this));
  }
  MonitoredMixin(MonitoredMixin&&){
    monitor.push(static_cast<const Type*>(this));
  }
  ~MonitoredMixin(){
    monitor.pop(static_cast<const Type*>(this));
  }
  
  friend Type;
};

#endif