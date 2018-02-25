#include <jarl.h>

#include "lexer.h"
#include "parser.h"
#include "vm.h"

#include <iostream>

#ifndef NDEBUG
#define PRINT_LEXEMES
//#define PRINT_AST
//#define PRINT_CODE
#endif

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
  
  #ifdef PRINT_LEXEMES
  std::cout << "::lexemes::" << std::endl;
  for(auto& lex: lexemes){
    std::cout << lex.toStrDebug() << std::endl;
  }
  #endif
  return;
  Parser parser(lexemes);
  
  #ifdef PRINT_AST
  std::cout << "::AST::" << std::endl;
  std::cout << parse_tree->toStrDebug() << std::endl;
  #endif
  
  Procedure* proc = parser.parse();
  
  #ifndef PRINT_CODE
  std::cout << "::proc::\n";
  std::cout << proc->opcodesToStrDebug() << std::endl;
  #endif
  
  v->execute(*proc);
}

void jarl::set_print_func(vm v, void(*func)(const char*)){
  v->setPrintFunc(func);
}
