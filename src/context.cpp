#include "context.h"

using namespace Context;

namespace {
  PoolAllocator<sizeof(NodeContext), 64> node_context_pool_;
  PoolAllocator<NodeContext::variable_state_size, 64> variable_state_pool_;
}

namespace Context {
  
  void* NodeContext::operator new(size_t){
    return node_context_pool_.allocate();
  }
  void NodeContext::operator delete(void* ptr){
    node_context_pool_.deallocate(ptr);
  }
  
  void* NodeContext::VariableState::operator new(size_t){
    return variable_state_pool_.allocate();
  }
  void NodeContext::VariableState::operator delete(void* ptr){
    variable_state_pool_.deallocate(ptr);
  }
  
  void addContext(ASTNode* ast, std::vector<std::unique_ptr<char[]>>* errors){
    (void)ast;
    (void)errors;
  }
}