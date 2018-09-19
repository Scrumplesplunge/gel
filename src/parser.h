#pragma once

#include "ast.h"

#include <stdexcept>
#include <string_view>

class SyntaxError : public std::runtime_error {
 public:
  using runtime_error::runtime_error;
};

class Parser {
 public:
  Parser(std::string_view source) : remaining_(source) {}

  ast::Identifier ParseIdentifier();
  ast::Integer ParseInteger();
  ast::Node ParseTerm();
  ast::Node ParseProduct();
  ast::Node ParseExpression();
  void CheckEnd();

 private:
  void CheckNotEnd();

  std::string_view remaining_;
};
