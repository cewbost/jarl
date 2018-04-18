#ifndef CODE_GENERATOR_H_INCLUDED
#define CODE_GENERATOR_H_INCLUDED

#include "function.h"

namespace CodeGenerator {
  
  template<class Key, class Val>
  using VectorMapDefault = VectorMap<Key, Val>;
  using VarAllocMap = DelegateMap<VectorMapDefault, String*, OpCodeType>;
  using VectorMapBase = std::vector<std::pair<String* const, OpCodeType>>;
  
  Function* generate(
    std::unique_ptr<ASTNode> parse_tree,
    std::vector<std::unique_ptr<char[]>>* errors,
    std::unique_ptr<VarAllocMap> var_allocs = nullptr,
    std::unique_ptr<VarAllocMap> context_var_allocs = nullptr
  );
}

#endif