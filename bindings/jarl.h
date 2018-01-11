#ifndef JARL_H_INCLUDED
#define JARL_H_INCLUDED

#include <type_traits>
#include <cstdint>

class VM;

namespace jarl{
  
  using Int = intptr_t;
  using Float = std::conditional<sizeof(void*) == 8, double, float>::type;
  
  typedef VM* vm;
  
  vm new_vm();
  void destroy_vm(vm);
  void execute(vm, const char*);
  void set_print_func(vm, void(*)(const char*));
}

#endif