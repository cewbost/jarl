#include <jarl.h>

#include "lexer.h"
#include "parser.h"
#include "vm.h"

#include <iostream>

using jarl::vm;

vm jarl::new_vm(){
  return new VM;
}

void jarl::destroy_vm(vm v){
  delete v;
}

void jarl::execute(vm v, const char* code){
  
  Lexer lex(code);
  auto lexemes = lex.lex();
  
  Parser parser(lexemes);
  
  Procedure* proc = parser.parse();
  v->execute(*proc);
}

void jarl::set_print_func(vm v, void(*func)(const char*)){
  v->setPrintFunc(func);
}
