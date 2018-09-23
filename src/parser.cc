#include "parser.h"

#include <algorithm>
#include <sstream>

constexpr const char* kReservedIdentifiers[] = {
  "if",
  "else",
  "let",
};

constexpr int kSpacesPerIndent = 2;

static SyntaxError Error(Reader::Location location, std::string message) {
  std::string indicator =
      std::string(kSpacesPerIndent + location.column(), ' ');
  indicator.back() = '^';
  std::ostringstream output;
  output << "Syntax error at " << location.input_name() << ":"
         << location.line() << ":" << location.column() << ": " << message
         << "\n\n"
         << std::string(kSpacesPerIndent, ' ') << location.line_contents()
         << "\n"
         << indicator << "\n";
  return SyntaxError{output.str()};
}

ast::Identifier Parser::ParseIdentifier() {
  auto begin = std::begin(*reader_);
  auto end = std::end(*reader_);
  auto i = std::find_if_not(begin, end,
                            [](unsigned char c) { return std::isalnum(c); });
  auto length = i - begin;
  Reader::Location location = reader_->location();
  std::string_view name = reader_->prefix(length);
  auto j = std::find(std::begin(kReservedIdentifiers),
                     std::end(kReservedIdentifiers), name);
  if (j != std::end(kReservedIdentifiers)) {
    throw Error(location, "Reserved word '" + std::string{name} +
                              "' can't be used as an identifier.");
  }
  if (name.empty() || !std::isalpha(name[0]))
    throw Error(location, "Invalid identifier: " + std::string{name});
  reader_->remove_prefix(length);
  return ast::Identifier{name};
}

ast::Integer Parser::ParseInteger() {
  const bool negative = reader_->Consume("-");
  auto begin = std::begin(*reader_);
  auto end = std::end(*reader_);
  auto i = std::find_if_not(begin, end,
                            [](unsigned char c) { return std::isdigit(c); });
  auto length = i - begin;
  std::string_view integer_part = reader_->prefix(length);
  std::int64_t value = 0;
  for (char c : integer_part) {
    // Compute the value as negative and flip it subsequently, since this way we
    // correctly handle INT_MIN. Note that this still invokes UB if the input
    // value is out of range, but at least it works correctly for valid input.
    value = 10 * value - (c - '0');
  }
  if (!negative) value = -value;
  reader_->remove_prefix(length);
  return ast::Integer{value};
}

std::vector<ast::Expression> Parser::ParseArgumentList() {
  CheckConsume("(");
  if (reader_->Consume(")")) return {};
  std::vector<ast::Expression> arguments;
  while (true) {
    arguments.push_back(ParseExpression());
    CheckNotEnd();
    if (reader_->Consume(")")) return arguments;
    CheckConsume(", ");
  }
}

ast::Expression Parser::ParseTerm() {
  // A term is either an integer, an identifier, or a subexpression surrounded
  // by parentheses.
  auto location = reader_->location();
  if (reader_->Consume("(")) {
    auto expression = ParseExpression();
    if (!reader_->Consume(")"))
      throw Error(location, "No matching ')' for this '('.");
    return expression;
  }
  CheckNotEnd();
  char lookahead = reader_->front();
  if (lookahead == '-' || std::isdigit(lookahead)) {
    return ParseInteger();
  } else if (std::isalpha(lookahead)) {
    auto identifier = ParseIdentifier();
    if (!reader_->empty() && reader_->front() == '(') {
      auto arguments = ParseArgumentList();
      return ast::FunctionCall{std::move(identifier), std::move(arguments)};
    } else {
      return identifier;
    }
  } else {
    throw Error(location, "Illegal token.");
  }
}

ast::Expression Parser::ParseUnary() {
  if (reader_->Consume("!")) {
    return ast::LogicalNot{ParseUnary()};
  } else {
    return ParseTerm();
  }
}

ast::Expression Parser::ParseProduct() {
  auto left = ParseUnary();
  while (true) {
    if (reader_->Consume(" * ")) {
      left = ast::Multiply{std::move(left), ParseTerm()};
    } else if (reader_->Consume(" / ")) {
      left = ast::Divide{std::move(left), ParseTerm()};
    } else {
      return left;
    }
  }
}

ast::Expression Parser::ParseSum() {
  auto left = ParseProduct();
  while (true) {
    if (reader_->Consume(" + ")) {
      left = ast::Add{left, ParseProduct()};
    } else if (reader_->Consume(" - ")) {
      left = ast::Subtract{std::move(left), ParseProduct()};
    } else {
      return left;
    }
  }
}

ast::Expression Parser::ParseComparison() {
  auto left = ParseSum();
  if (reader_->Consume(" == ")) {
    return ast::CompareEq{std::move(left), ParseSum()};
  } else if (reader_->Consume(" != ")) {
    return ast::CompareNe{std::move(left), ParseSum()};
  } else if (reader_->Consume(" <= ")) {
    return ast::CompareLe{std::move(left), ParseSum()};
  } else if (reader_->Consume(" < ")) {
    return ast::CompareLt{std::move(left), ParseSum()};
  } else if (reader_->Consume(" >= ")) {
    return ast::CompareGe{std::move(left), ParseSum()};
  } else if (reader_->Consume(" > ")) {
    return ast::CompareGt{std::move(left), ParseSum()};
  } else {
    return left;
  }
}

ast::Expression Parser::ParseConjunction() {
  auto left = ParseComparison();
  while (true) {
    if (reader_->Consume(" && ")) {
      left = ast::LogicalAnd{std::move(left), ParseComparison()};
    } else {
      return left;
    }
  }
}

ast::Expression Parser::ParseDisjunction() {
  auto left = ParseConjunction();
  while (true) {
    if (reader_->Consume(" || ")) {
      left = ast::LogicalOr{std::move(left), ParseConjunction()};
    } else {
      return left;
    }
  }
}

ast::Expression Parser::ParseExpression() { return ParseDisjunction(); }

ast::DeclareVariable Parser::ParseVariableDeclaration() {
  CheckConsume("let ");
  auto identifier = ParseIdentifier();
  CheckConsume(" = ");
  auto value = ParseExpression();
  return ast::DeclareVariable{std::move(identifier), std::move(value)};
}

ast::Assign Parser::ParseAssignment() {
  auto identifier = ParseIdentifier();
  CheckConsume(" = ");
  auto value = ParseExpression();
  return ast::Assign{std::move(identifier), std::move(value)};
}

ast::DoFunction Parser::ParseDoFunction() {
  CheckConsume("do ");
  auto function = ParseIdentifier();
  auto arguments = ParseArgumentList();
  return ast::DoFunction{
      ast::FunctionCall{std::move(function), std::move(arguments)}};
}

ast::If Parser::ParseIfStatement(int indent) {
  CheckConsume("if (");
  auto condition = ParseExpression();
  CheckConsume(") ");
  auto statements = ParseStatementBlock(indent);
  if (reader_->starts_with(" else if (")) {
    CheckConsume(" else ");
    return ast::If{std::move(condition), std::move(statements),
                   {ParseIfStatement(indent)}};
  } else if (reader_->Consume(" else ")) {
    return ast::If{std::move(condition), std::move(statements),
                   ParseStatementBlock(indent)};
  } else {
    return ast::If{std::move(condition), std::move(statements), {}};
  }
}

ast::Statement Parser::ParseStatement(int indent) {
  if (reader_->starts_with("let ")) {
    return ParseVariableDeclaration();
  } else if (reader_->starts_with("do ")) {
    return ParseDoFunction();
  } else if (reader_->starts_with("if ")) {
    return ParseIfStatement(indent);
  } else {
    return ParseAssignment();
  }
}

std::vector<ast::Statement> Parser::ParseStatementBlock(int indent) {
  CheckConsume("{");
  CheckNotEnd();
  // Empty statement blocks are just "{}", ie. without a newline.
  if (reader_->Consume("}")) return {};
  // All other blocks have multiple lines and at least one statement.
  std::vector<ast::Statement> statements;
  while (true) {
    ConsumeNewline();
    ConsumeIndent(indent);
    if (reader_->Consume("}")) return statements;
    ConsumeIndent(kSpacesPerIndent);
    statements.push_back(ParseStatement(indent + kSpacesPerIndent));
  }
}

void Parser::CheckEnd() {
  if (!reader_->empty())
    throw Error(reader_->location(), "Unexpected trailing characters.");
}

void Parser::CheckConsume(std::string_view expected) {
  if (!reader_->Consume(expected)) {
    throw Error(reader_->location(),
                "Expected '" + std::string{expected} + "'.");
  }
}

void Parser::ConsumeNewline() {
  if (!reader_->Consume("\n"))
    throw Error(reader_->location(), "Expected '\\n'.");
}

void Parser::ConsumeIndent(int indent) {
  auto has_indent = [&] {
    std::string_view prefix = reader_->prefix(indent);
    if (static_cast<int>(prefix.length()) < indent) return false;
    for (int i = 0; i < indent; i++) {
      if (prefix[i] != ' ') return false;
    }
    return true;
  };

  if (has_indent()) {
    reader_->remove_prefix(indent);
  } else {
    throw Error(
        reader_->location(),
        "Expected at least " + std::to_string(indent) + " spaces of indent.");
  }
}

void Parser::CheckNotEnd() {
  if (reader_->empty())
    throw Error(reader_->location(), "Unexpected end of input.");
}
