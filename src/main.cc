#include "ast.h"
#include "parser.h"
#include "reader.h"
#include "target-c.h"

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

int main() {
  std::string input{std::istreambuf_iterator<char>{std::cin}, {}};
  Reader reader{"stdin", input};
  Parser parser{reader};
  try {
    auto program = parser.ParseProgram();
    parser.CheckEnd();
    {
      std::ofstream output{"gel-output.c"};
      target::c::TopLevel codegen{output};
      codegen.Visit(program);
    }
    int compile_status = std::system("gcc gel-output.c -o gel-output");
    if (compile_status) return compile_status;
    return std::system("./gel-output");
  } catch (const std::exception& error) {
    std::cout << error.what() << "\n";
  }
}
