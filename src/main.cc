#include "ast.h"
#include "parser.h"
#include "pretty.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>
#include <iostream>

int main() {
  std::string input{std::istreambuf_iterator<char>{std::cin}, {}};
  Parser parser{input};
  auto statement = parser.ParseStatement(0);
  parser.ConsumeNewline();
  parser.CheckEnd();
  pretty::tree::StatementPrinter printer{0, std::cout};
  statement.Visit(printer);
  std::cout << "\n";
}
