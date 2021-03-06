#ifndef CODE_GENERATOR_H_INCLUDED
#define CODE_GENERATOR_H_INCLUDED

#include "function.h"

namespace CodeGenerator {
  
  template<class Key, class Val>
  using VectorMapDefault = VectorMap<Key, Val>;
  using VarAllocMap = DelegateMap<VectorMapDefault, String*, OpCodes::Type>;
  using VectorMapBase = VectorMapDefault<String*, OpCodes::Type>::base_type;
  
  Function* generate(
    std::unique_ptr<ASTNode>&& parse_tree,
    std::vector<std::unique_ptr<char[]>>* errors
  );
}

#endif