#ifndef CONTEXT_H_INCLUDED
#define CONTEXT_H_INCLUDED

#include "vector_map.h"
#include "pool_allocator.h"

#include <memory>

struct ASTNode;

namespace Context {
  
  struct Attribute {
    enum class Type: uint8_t {
      LastRead = 1,
      LastWrite,
      NextRead,
      NextWrite
    };
    Type      type;
    unsigned  var;
    ASTNode*  node;
  };
  
  class AttributeSet: std::vector<Attribute> {
  public:
    AttributeSet forVariable(unsigned);
    void setLastRead(unsigned, ASTNode*);
    void addNextRead(unsigned, ASTNode*);
  };
  
  void addContext(ASTNode* ast, std::vector<std::unique_ptr<char[]>>* errors);
}

#endif