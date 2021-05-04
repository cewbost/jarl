#include "../bindings/jarl.h"

#include <memory>
#include <cstdio>

bool fail = false;

void print(const char* str) {
	printf("%s\n", str);
}

void errorPrint(const char* str) {
	printf("%s\n", str);
	fail = true;
}

std::unique_ptr<char[]> readFile(const char* filename) {
	FILE* file = fopen(filename, "r");
	if (file == nullptr) {
		perror("unable to open file");
		return nullptr;
	}
	
	std::unique_ptr<char[]> buffer;
	long len;

	if (fseek(file, 0, SEEK_END) != 0) goto error;
	len = ftell(file);
	if (len < 0) goto error;
	if (fseek(file, 0, SEEK_SET) != 0) goto error;

	buffer = std::make_unique<char[]>(len + 1);

	len = fread(buffer.get(), 1, len, file);
	if (ferror(file) != 0) goto error;
	buffer[len] = '\0';

	fclose(file);
	return buffer;

error:
	perror("unable to read file");
	fclose(file);
	return nullptr;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("No argument supplied.\n");
		return 1;
	}

	auto buffer = readFile(argv[1]);
	if (!buffer) {
		return 1;
	}

	auto vm = jarl::new_vm();
	jarl::set_print_func(vm, print);
	jarl::set_error_print_func(vm, errorPrint);
	jarl::execute(vm, buffer.get());
	jarl::destroy_vm(vm);

	return fail? 1 : 2;
}
