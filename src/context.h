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
  using VarAllocMap = DelegateMap<VectorMapDefault, String*, ASTNode*>;
  
  struct Attribute {
    enum class Type: uint8_t {
      LastRead = 1,
      LastWrite,
      NextRead,
      NextWrite
    };
    Type      type;
    ASTNode*  var;
    ASTNode*  node;
    
    bool operator==(const Attribute& other)const{
      return this->type == other.type && this->var == other.var;
    }
  };
  
  class AttributeSet: public std::vector<Attribute> {
  public:
    AttributeSet forVariable(ASTNode*)const;
    void setLastRead(ASTNode*, ASTNode*);
    void setLastWrite(ASTNode*, ASTNode*);
    void setLastReadWrite(ASTNode*, ASTNode*);
    void addNextRead(ASTNode*, ASTNode*);
    void addNextWrite(ASTNode*, ASTNode*);
    
    void merge(const AttributeSet&);
  };
  
  void addContext(ASTNode* ast, std::vector<std::unique_ptr<char[]>>* errors);
}

#endif