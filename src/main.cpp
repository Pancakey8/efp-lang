#include "parser.h"
#include <fstream>
#include <iostream>

int main(int, char *[]) {
  std::ifstream t("example.efp");
  std::string example;

  t.seekg(0, std::ios::end);
  example.reserve(t.tellg());
  t.seekg(0, std::ios::beg);

  example.assign((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());

  Parser p(std::move(example));
  p.parse_all();

  for (auto k : p.output) {
    print_expr(k);
    std::cout << "\n";
  }

  return 0;
}