#include <jarl.h>

#include "lexer.h"
#include "parser.h"
#include "syntax_checker.h"
#include "context.h"
#include "vm.h"
#include "code_generator.h"
#include "common.h"

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

namespace {
  
  inline bool printErrors_(vm v, const Errors& errors){
    if(errors.size() > 0){
      for(auto& error: errors){
        v->errPrint(error.get());
      }
      return true;
    }else return false;
  }
}

vm jarl::new_vm(){
  return new VM;
}

void jarl::destroy_vm(vm v){
  delete v;
}

void jarl::execute(vm v, const char* code){
  
  Errors errors;
  
  //lexer stage
  Lexer lex(code);
  auto lexemes = lex.lex(&errors);
  
  #ifdef PRINT_LEXEMES
  fprintf(stderr, "::lexemes::\n");
  for(auto& lex: lexemes){
    fprintf(stderr, "%s\n", lex.toStrDebug().c_str());
  }
  #endif
  
  if(printErrors_(v, errors)) return;
  
  //parse stage
  Parser parser(lexemes);
  auto parse_tree = parser.parse(&errors);
  
  #ifdef PRINT_AST
  fprintf(stderr, "::AST::\n");
  fprintf(stderr, "%s\n", parse_tree->toStrDebug().c_str());
  #endif
  
  if(printErrors_(v, errors)) return;
  
  //syntax validation stage
  SyntaxChecker syn_checker(&errors);
  syn_checker.validateSyntax(parse_tree.get());
  
  //context computing stage
  Context::addContext(parse_tree.get(), &errors);
  
  //code generation stage
  #ifdef NO_GENERATE
  return;
  #endif
  
  std::unique_ptr<Function> proc(
    CodeGenerator::generate(std::move(parse_tree), &errors)
  );
  
  if(printErrors_(v, errors)) return;
  
  //execution
  #ifndef NO_EXECUTE
  v->execute(*proc.release());
  #endif
}

void jarl::set_print_func(vm v, void(*func)(const char*)){
  v->setPrintFunc(func);
}
void jarl::set_error_print_func(vm v, void(*func)(const char*)){
  v->setErrorPrintFunc(func);
}
