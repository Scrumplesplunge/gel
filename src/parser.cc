#include "parser.h"

#include <algorithm>

constexpr const char* kReservedIdentifiers[] = {
  "if",
  "else",
  "let",
};

constexpr int kSpacesPerIndent = 2;

ast::Identifier Parser::ParseIdentifier() {
  CheckNotEnd();
  auto i = std::find_if_not(std::begin(remaining_), std::end(remaining_),
                            [](unsigned char c) { return std::isalnum(c); });
  auto length = i - std::begin(remaining_);
  std::string_view name = remaining_.substr(0, length);
  auto j = std::find(std::begin(kReservedIdentifiers),
                     std::end(kReservedIdentifiers), name);
  if (j != std::end(kReservedIdentifiers)) {
    throw SyntaxError{"Reserved word '" + std::string{name} +
                      "' can't be used as an identifier."};
  }
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

std::vector<ast::Expression> Parser::ParseArgumentList() {
  CheckNotEnd();
  Consume("(");
  CheckNotEnd();
  if (remaining_[0] == ')') {
    Consume(")");
    return {};  // Empty argument list.
  }
  std::vector<ast::Expression> arguments;
  while (true) {
    arguments.push_back(ParseExpression());
    CheckNotEnd();
    if (remaining_[0] == ')') {
      Consume(")");
      return arguments;
    }
    Consume(", ");
  }
}

ast::Expression Parser::ParseTerm() {
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
    auto identifier = ParseIdentifier();
    if (!remaining_.empty() && remaining_[0] == '(') {
      auto arguments = ParseArgumentList();
      return ast::FunctionCall{std::move(identifier), std::move(arguments)};
    } else {
      return identifier;
    }
  } else {
    throw SyntaxError{"Illegal token."};
  }
}

ast::Expression Parser::ParseProduct() {
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

ast::Expression Parser::ParseExpression() {
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

ast::DeclareVariable Parser::ParseVariableDeclaration() {
  Consume("let ");
  auto identifier = ParseIdentifier();
  Consume(" = ");
  auto value = ParseExpression();
  return ast::DeclareVariable{std::move(identifier), std::move(value)};
}

ast::Assign Parser::ParseAssignment() {
  auto identifier = ParseIdentifier();
  Consume(" = ");
  auto value = ParseExpression();
  return ast::Assign{std::move(identifier), std::move(value)};
}

ast::DoFunction Parser::ParseDoFunction() {
  Consume("do ");
  auto function = ParseIdentifier();
  auto arguments = ParseArgumentList();
  return ast::DoFunction{
      ast::FunctionCall{std::move(function), std::move(arguments)}};
}

ast::If Parser::ParseIfStatement(int indent) {
  Consume("if (");
  auto condition = ParseExpression();
  Consume(") ");
  auto statements = ParseStatementBlock(indent);
  if (remaining_.substr(0, 9) == " else if (") {
    Consume(" else ");
    return ast::If{std::move(condition), std::move(statements),
                   {ParseIfStatement(indent)}};
  } else if (remaining_.substr(0, 6) == " else ") {
    Consume(" else ");
    return ast::If{std::move(condition), std::move(statements),
                   ParseStatementBlock(indent)};
  } else {
    return ast::If{std::move(condition), std::move(statements), {}};
  }
}

ast::Statement Parser::ParseStatement(int indent) {
  if (remaining_.substr(0, 4) == "let ") {
    return ParseVariableDeclaration();
  } else if (remaining_.substr(0, 3) == "do ") {
    return ParseDoFunction();
  } else if (remaining_.substr(0, 3) == "if ") {
    return ParseIfStatement(indent);
  } else {
    return ParseAssignment();
  }
}

std::vector<ast::Statement> Parser::ParseStatementBlock(int indent) {
  Consume("{");
  CheckNotEnd();
  // Empty statement blocks are just "{}", ie. without a newline.
  if (remaining_[0] == '}') {
    Consume("}");
    return {};
  }
  // All other blocks have multiple lines and at least one statement.
  std::vector<ast::Statement> statements;
  while (true) {
    ConsumeNewline();
    ConsumeIndent(indent);
    CheckNotEnd();
    if (remaining_[0] == '}') {
      Consume("}");
      return statements;
    }
    ConsumeIndent(kSpacesPerIndent);
    statements.push_back(ParseStatement(indent + kSpacesPerIndent));
  }
}

void Parser::CheckEnd() {
  if (!remaining_.empty()) {
    throw SyntaxError{"Unexpected trailing characters: " +
                      std::to_string(remaining_.length())};
  }
}

void Parser::Consume(std::string_view expected) {
  if (remaining_.substr(0, expected.length()) != expected)
    throw SyntaxError{"Expected '" + std::string{expected} + "'."};
  remaining_.remove_prefix(expected.length());
}

void Parser::ConsumeNewline() {
  if (remaining_.empty() || remaining_[0] != '\n')
    throw SyntaxError{"Expected '\\n'."};
  remaining_.remove_prefix(1);
}

void Parser::ConsumeIndent(int indent) {
  auto has_indent = [&] {
    if (remaining_.length() < static_cast<std::size_t>(indent)) return false;
    for (int i = 0; i < indent; i++) {
      if (remaining_[i] != ' ') return false;
    }
    return true;
  };

  if (has_indent()) {
    remaining_.remove_prefix(indent);
  } else {
    throw SyntaxError{"Expected at least " + std::to_string(indent) +
                      " spaces of indent."};
  }
}

void Parser::CheckNotEnd() {
  if (remaining_.empty())
    throw SyntaxError{"Unexpected end of input."};
}
