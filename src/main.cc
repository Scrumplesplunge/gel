#include "ast.h"
#include "code_generation.h"
#include "operations.h"
#include "parser.h"
#include "pretty.h"
#include "reader.h"

#include <map>
#include <stdexcept>

bool prompt(std::string_view text, std::string& line) {
  std::cout << text;
  return bool{std::getline(std::cin, line)};
}

int main() {
  std::string input{std::istreambuf_iterator<char>{std::cin}, {}};
  Reader reader{"stdin", input};
  Parser parser{reader};
  try {
    codegen::Context context;
    codegen::LexicalScope scope;
    auto statement = parser.ParseStatement(0);
    parser.ConsumeNewline();
    parser.CheckEnd();
    codegen::Statement codegen(&context, &scope);
    codegen.Visit(statement);
    pretty::OperationPrinter printer{std::cout};
    printer.Visit(codegen.ConsumeResult());
  } catch (const std::exception& error) {
    std::cout << error.what() << "\n";
  }
}
