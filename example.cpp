#define EXTISM_NO_JSON
#include "extism.hpp"

#include <cstring>
#include <fstream>
#include <iostream>

using namespace extism;

std::vector<uint8_t> read(const char *filename) {
  std::ifstream file(filename, std::ios::binary);
  return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "No input file provided" << std::endl;
    return 1;
  }

  auto wasm = read(argv[1]);
  std::string tmp = "Testing";

  // A lambda can be used as a host function
  auto hello_world = [&tmp](CurrentPlugin plugin, void *user_data) {
    std::cout << "Hello from C++" << std::endl;
    std::cout << (const char *)user_data << std::endl;
    std::cout << tmp << std::endl;
    plugin.outputVal(0).v = plugin.inputVal(0).v;
  };

  std::vector<Function> functions = {
      Function("hello_world", {ValType::I64}, {ValType::I64}, hello_world,
               (void *)"Hello again!",
               [](void *x) { std::cout << "Free user data" << std::endl; }),
  };

  Plugin plugin(wasm, true, functions);

  const char *input = argc > 1 ? argv[1] : "this is a test";
  ExtismSize length = strlen(input);

  extism::Buffer output = plugin.call("count_vowels", (uint8_t *)input, length);
  std::cout << (char *)output.data << std::endl;
  return 0;
}
