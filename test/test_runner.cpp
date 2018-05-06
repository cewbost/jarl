#include "../bindings/jarl.h"

#include <memory>
#include <cstdio>

bool fail = false;

void print(const char* str){
  printf("%s\n", str);
}
void errorPrint(const char* str){
  printf("%s\n", str);
  fail = true;
}

int main(int argc, char** argv){
  if(argc < 2){
    printf("No argument supplied.\n");
    return 1;
  }
  
  FILE* file = fopen(argv[1], "r");
  if(file == nullptr){
    printf("unable to open file '%s'\n", argv[1]);
    return 1;
  }
  
  fseek(file, 0, SEEK_END);
  long len = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  auto buffer = std::make_unique<char[]>(len + 1);
  fread(buffer.get(), 1, len, file);
  buffer[len] = '\0';
  fclose(file);
  
  auto vm = jarl::new_vm();
  jarl::set_print_func(vm, print);
  jarl::set_error_print_func(vm, errorPrint);
  jarl::execute(vm, buffer.get());
  jarl::destroy_vm(vm);
  
  return fail? 1 : 0;
}