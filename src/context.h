#ifndef CONTEXT_H_INCLUDED
#define CONTEXT_H_INCLUDED

#include "string.h"
#include "vector_map.h"
#include "delegate_map.h"
#include "pool_allocator.h"

#include <memory>

struct ASTNode;

namespace Context {
  
  template<class Key, class Val>
  using VectorMapDefault = VectorMap<Key, Val>;
  using VarAllocMap = DelegateMap<VectorMapDefault, String*, unsigned>;
  
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
    void setLastWrite(unsigned, ASTNode*);
    void setLastReadWrite(unsigned, ASTNode*);
    void addNextRead(unsigned, ASTNode*);
    void addNextWrite(unsigned, ASTNode*);
  };
  
  void addContext(ASTNode* ast, std::vector<std::unique_ptr<char[]>>* errors);
}

#endif