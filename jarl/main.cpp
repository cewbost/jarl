#include <jarl.h>

#include <cstdio>

#include <memory>

void stdoutPrint(const char* msg) {
  printf(msg);
}

void stderrPrint(const char* msg) {
  fprintf(stderr, msg);
}

std::unique_ptr<char[]> execFile(const char* filename) {
  FILE* file = fopen(filename, "rb");
  if(file == nullptr) {
    fprintf(stderr, "Unable to open file '%s'\n", filename);
    return nullptr;
  }

  fseek(file, 0, SEEK_END);
  auto size = ftell(file);
  fseek(file, 0, SEEK_SET);

  auto script = std::make_unique<char[]>(size);

  if(int read = fread(script.get(), 1, size, file); read != size) {
    fprintf(stderr, "Error reading file '%s'\n", filename);
    fclose(file);
    return nullptr;
  }

  fclose(file);
  return script;
}

int main(int argc, char** argv) {
  auto vm = jarl::new_vm();

  jarl::set_print_func(vm, stdoutPrint);
  jarl::set_print_func(vm, stderrPrint);

  for(int i = 1; i < argc; ++i) {
    if(auto script = execFile(argv[i]); !script) {
      goto terminate;
    } else {
      jarl::execute(vm, script.get());
    }
  }

terminate:
  jarl::destroy_vm(vm);

  return 0;
}
