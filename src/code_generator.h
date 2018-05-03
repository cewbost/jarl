#ifndef CODE_GENERATOR_H_INCLUDED
#define CODE_GENERATOR_H_INCLUDED

#include "function.h"

namespace CodeGenerator {
  
  template<class Key, class Val>
  using VectorMapDefault = VectorMap<Key, Val>;
  using VarAllocMap = DelegateMap<VectorMapDefault, String*, OpCodeType>;
  using VectorMapBase = VectorMapDefault<String*, OpCodeType>::base_type;
  
  Function* generate(
    ASTNode* parse_tree,
    std::vector<std::unique_ptr<char[]>>* errors
  );
  Function* generate(
    ASTNode* parse_tree,
    std::vector<std::unique_ptr<char[]>>* errors,
    VarAllocMap* var_allocs,
    VarAllocMap* context_var_allocs = nullptr
  );
}

#endif