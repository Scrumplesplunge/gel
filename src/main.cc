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
  analysis::Operators operators = {
    {
      {ast::Arithmetic::ADD, ast::Primitive::INTEGER},
      {ast::Arithmetic::DIVIDE, ast::Primitive::INTEGER},
      {ast::Arithmetic::MULTIPLY, ast::Primitive::INTEGER},
      {ast::Arithmetic::SUBTRACT, ast::Primitive::INTEGER},
    },
    {ast::Primitive::BOOLEAN, ast::Primitive::INTEGER},
    {ast::Primitive::INTEGER},
  };
  analysis::GlobalContext context{std::move(operators), {}};
  analysis::Scope scope;
  Reader builtins{"builtin", "<native code>"};
  scope.Define(
      "print",
      analysis::Scope::Entry{
          builtins.location(),
          ast::Function{
              ast::Void{},
              {ast::Identifier{{builtins.location(), ast::Primitive::INTEGER},
                               "number"}}}});
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
    target::c::Compile(checked, &output);
  }
  int compile_status = std::system("gcc .gel-output.c -o .gel-output");
  if (compile_status) return compile_status;
  return std::system("./.gel-output");
}
