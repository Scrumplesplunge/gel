#include "parser.h"

#include <algorithm>

ast::Identifier Parser::ParseIdentifier() {
  CheckNotEnd();
  auto i = std::find_if_not(std::begin(remaining_), std::end(remaining_),
                            [](unsigned char c) { return std::isalnum(c); });
  auto length = i - std::begin(remaining_);
  std::string_view name = remaining_.substr(0, length);
  if (name.empty() || !std::isalpha(name[0]))
    throw SyntaxError{"Invalid identifier: " + std::string{name}};
  remaining_.remove_prefix(length);
  return ast::Identifier{name};
}

ast::Integer Parser::ParseInteger() {
  CheckNotEnd();
  const bool negative = remaining_[0] == '-';
  if (negative) remaining_.remove_prefix(1);
  auto i = std::find_if_not(std::begin(remaining_), std::end(remaining_),
                            [](unsigned char c) { return std::isdigit(c); });
  auto length = i - std::begin(remaining_);
  std::string_view integer_part = remaining_.substr(0, length);
  std::int64_t value = 0;
  for (char c : integer_part) {
    // Compute the value as negative and flip it subsequently, since this way we
    // correctly handle INT_MIN. Note that this still invokes UB if the input
    // value is out of range, but at least it works correctly for valid input.
    value = 10 * value - (c - '0');
  }
  if (!negative) value = -value;
  remaining_.remove_prefix(length);
  return ast::Integer{value};
}

ast::Node Parser::ParseTerm() {
  CheckNotEnd();
  // A term is either an integer, an identifier, or a subexpression surrounded
  // by parentheses.
  char lookahead = remaining_[0];
  if (lookahead == '(') {
    remaining_.remove_prefix(1);
    auto expression = ParseExpression();
    if (remaining_.empty() || remaining_[0] != ')')
      throw SyntaxError{"Missing ')'."};
    remaining_.remove_prefix(1);
    return expression;
  } if (lookahead == '-' || std::isdigit(lookahead)) {
    return ParseInteger();
  } else if (std::isalpha(lookahead)) {
    return ParseIdentifier();
  } else {
    throw SyntaxError{"Illegal token."};
  }
}

ast::Node Parser::ParseProduct() {
  CheckNotEnd();
  auto left = ParseTerm();
  while (true) {
    std::string_view operation = remaining_.substr(0, 3);
    if (operation == " * ") {
      remaining_.remove_prefix(3);
      left = ast::Multiply{left, ParseTerm()};
    } else if (operation == " / ") {
      remaining_.remove_prefix(3);
      left = ast::Divide{left, ParseTerm()};
    } else {
      return left;
    }
  }
}

ast::Node Parser::ParseExpression() {
  CheckNotEnd();
  auto left = ParseProduct();
  while (true) {
    std::string_view operation = remaining_.substr(0, 3);
    if (operation == " + ") {
      remaining_.remove_prefix(3);
      left = ast::Add{left, ParseProduct()};
    } else if (operation == " - ") {
      remaining_.remove_prefix(3);
      left = ast::Subtract{left, ParseProduct()};
    } else {
      return left;
    }
  }
}

void Parser::CheckEnd() {
  if (!remaining_.empty())
    throw SyntaxError{"Unexpected trailing characters."};
}

void Parser::CheckNotEnd() {
  if (remaining_.empty())
    throw SyntaxError{"Unexpected end of input."};
}
