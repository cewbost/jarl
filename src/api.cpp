#include <jarl.h>

#include "lexer.h"
#include "parser.h"
#include "vm.h"
#include "code_generator.h"

#ifndef NDEBUG
#include <cstdio>
//#define PRINT_LEXEMES
//#define PRINT_AST
//#define NO_GENERATE
//#define PRINT_CODE
//#define NO_EXECUTE
#else
#undef PRINT_LEXEMES
#undef PRINT_AST
#undef NO_GENERATE
#undef PRINT_CODE
#undef NO_EXECUTE
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
  
  #ifdef PRINT_LEXEMES
  fprintf(stderr, "::lexemes::\n");
  for(auto& lex: lexemes){
    fprintf(stderr, "%s\n", lex.toStrDebug().c_str());
  }
  #endif
  
  if(errors.size() > 0){
    for(auto& error: errors){
      v->errPrint(error.get());
    }
    return;
  }
  
  Parser parser(lexemes);
  auto parse_tree = parser.parse(&errors);
  
  #ifdef PRINT_AST
  fprintf(stderr, "::AST::\n");
  fprintf(stderr, "%s\n", parse_tree->toStrDebug().c_str());
  #endif
  
  if(errors.size() > 0){
    for(auto& error: errors){
      v->errPrint(error.get());
    }
    return;
  }
  
  #ifdef NO_GENERATE
  return;
  #endif
  
  Function* proc = CodeGenerator::generate(std::move(parse_tree), &errors);
  
  if(errors.size() > 0){
    for(auto& error: errors){
      v->errPrint(error.get());
    }
    delete proc;
    return;
  }
  
  #ifdef NO_EXECUTE
  delete proc;
  #else
  v->execute(*proc);
  #endif
}

void jarl::set_print_func(vm v, void(*func)(const char*)){
  v->setPrintFunc(func);
}
void jarl::set_error_print_func(vm v, void(*func)(const char*)){
  v->setErrorPrintFunc(func);
}
