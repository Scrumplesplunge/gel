#include "ast.h"
#include "parser.h"
#include "pretty.h"
#include "reader.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>
#include <iostream>

int main() {
  Reader reader{"stdin", {std::istreambuf_iterator<char>{std::cin}, {}}};
  Parser parser{reader};
  try {
    auto statement = parser.ParseStatement(0);
    parser.ConsumeNewline();
    parser.CheckEnd();
    pretty::tree::StatementPrinter printer{0, std::cout};
    statement.Visit(printer);
    std::cout << "\n";
  } catch (const SyntaxError& error) {
    std::cout << error.what() << "\n";
  }
}
