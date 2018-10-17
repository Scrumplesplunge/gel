#include "analysis.h"
#include "ast.h"
#include "parser.h"
#include "reader.h"
#include "target-c.h"

#include <fstream>
#include <iostream>
#include <map>
#include <vector>

int main() {
  // Parse the program.
  std::string input{std::istreambuf_iterator<char>{std::cin}, {}};
  Reader reader{"stdin", input};
  Parser parser{reader};
  auto program = parser.ParseProgram();
  parser.CheckEnd();

  // Perform semantics checks.
  auto [context, scope] = analysis::DefaultState();
  auto checked = analysis::Check(program, &context, &scope);
  if (!context.diagnostics.empty()) {
    for (const auto& message : context.diagnostics) {
      std::cerr << message;
    }
    std::map<Message::Type, int> count;
    for (const auto& message : context.diagnostics) count[message.type]++;
    std::cerr << "Compile finished with " << count[Message::Type::ERROR]
              << " error(s) and " << count[Message::Type::WARNING]
              << " warning(s).\n";
    // Abort compilation if there were errors but not if there were only
    // warnings or notes.
    if (count[Message::Type::ERROR] > 0) return 1;
  }
  {
    std::ofstream output{".gel-output.c"};
    target::c::Compile(context, checked, &output);
  }
  int compile_status = std::system("gcc .gel-output.c -o .gel-output");
  if (compile_status) return compile_status;
  return std::system("./.gel-output");
}
