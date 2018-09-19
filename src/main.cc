#include "ast.h"
#include "parser.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>
#include <iostream>

class PrettyPrinter : public ast::Visitor {
 public:
  PrettyPrinter(std::ostream& output) : output_(output) {}

  void Visit(const ast::Identifier& identifier) override {
    output_ << "Identifier{\"" << identifier.name << "\"}";
  }

  void Visit(const ast::Integer& integer) override {
    output_ << "Integer{" << integer.value << "}";
  }

  template <typename T>
  void VisitBinary(std::string_view type, const T& binary) {
    output_ << type << "{";
    binary.left.Visit(*this);
    output_ << ", ";
    binary.right.Visit(*this);
    output_ << "}";
  }

  void Visit(const ast::Add& add) override { VisitBinary("Add", add); }

  void Visit(const ast::Subtract& subtract) override {
    VisitBinary("Subtract", subtract);
  }

  void Visit(const ast::Multiply& multiply) override {
    VisitBinary("Multiply", multiply);
  }

  void Visit(const ast::Divide& divide) override {
    VisitBinary("Divide", divide);
  }

 private:
  std::ostream& output_;
};

bool prompt(std::string_view text, std::string* output) {
  std::cout << text;
  return bool{std::getline(std::cin, *output)};
}

int main() {
  std::string expression;
  while (prompt(">> ", &expression)) {
    Parser parser{expression};
    try {
      auto expression = parser.ParseExpression();
      parser.CheckEnd();
      PrettyPrinter printer{std::cout};
      expression.Visit(printer);
      std::cout << "\n";
    } catch (const std::exception& error) {
      std::cout << "Error: " << error.what() << "\n";
    }
  }
}
