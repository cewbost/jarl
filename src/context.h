#ifndef CONTEXT_H_INCLUDED
#define CONTEXT_H_INCLUDED

#include "ast.h"

#include "vector_map.h"
#include "pool_allocator.h"

#include <memory>

namespace Context {
  
  constexpr unsigned NodeContextSize = 24;
  
  class NodeContext {
    
    struct VariableState {
      std::unique_ptr<VariableState> next;
      
      void* operator new(size_t);
      void operator delete(void*);
    };
    
    VectorMap<unsigned, VariableState> variable_states;
    
    void* operator new(size_t);
    void operator delete(void*);
    
  public:
    
    static const auto variable_state_size = sizeof(VariableState);
  };
  
  void addContext(ASTNode* ast, std::vector<std::unique_ptr<char[]>>* errors);
}

#endif