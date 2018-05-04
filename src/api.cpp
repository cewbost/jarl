#include <jarl.h>

#include "lexer.h"
#include "parser.h"
#include "vm.h"
#include "code_generator.h"

#ifndef NDEBUG
#include <iostream>
//#define PRINT_LEXEMES
#define PRINT_AST
#define PRINT_CODE
#endif

using jarl::vm;

vm jarl::new_vm(){
  return new VM;
}

void jarl::destroy_vm(vm v){
  delete v;
}

void jarl::execute(vm v, const char* code){
  
  std::vector<std::unique_ptr<char[]>> errors;
  
  Lexer lex(code);
  auto lexemes = lex.lex(&errors);
  
  if(errors.size() > 0){
    for(auto& error: errors){
      v->print(error.get());
    }
    return;
  }
  
  #ifdef PRINT_LEXEMES
  std::cout << "::lexemes::" << std::endl;
  for(auto& lex: lexemes){
    std::cout << lex.toStrDebug() << std::endl;
  }
  #endif
  
  Parser parser(lexemes);
  auto parse_tree = parser.parse(&errors);
  
  if(errors.size() > 0){
    for(auto& error: errors){
      v->print(error.get());
    }
    return;
  }
  
  #ifdef PRINT_AST
  std::cout << "::AST::" << std::endl;
  std::cout << parse_tree->toStrDebug() << std::endl;
  #endif
  
  Function* proc = CodeGenerator::generate(parse_tree, &errors);
  
  if(errors.size() > 0){
    for(auto& error: errors){
      v->print(error.get());
    }
    delete proc;
    return;
  }
  
  #ifdef PRINT_CODE
  std::cout << "::proc::\n";
  std::cout << proc->opcodesToStrDebug() << std::endl;
  #endif
  
  v->execute(*proc);
  delete proc;
}

void jarl::set_print_func(vm v, void(*func)(const char*)){
  v->setPrintFunc(func);
}
