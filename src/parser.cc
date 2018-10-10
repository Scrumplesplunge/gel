#include "parser.h"

#include <algorithm>

constexpr const char* kReservedIdentifiers[] = {
    "boolean", "else", "function", "if", "integer", "let", "return", "while",
    "true", "false",
};

constexpr int kSpacesPerIndent = 2;

ast::Type Parser::ParseType() {
  Reader::Location location = reader_->location();
  auto name = IdentifierPrefix();
  reader_->remove_prefix(name.length());
  if (name == "void") return ast::Void{};
  if (name == "boolean") return ast::Primitive::BOOLEAN;
  if (name == "integer") return ast::Primitive::INTEGER;
  throw CompileError{location, "Invalid type name: " + std::string{name}};
}

ast::Identifier Parser::ParseIdentifier() {
  Reader::Location location = reader_->location();
  std::string_view name = IdentifierPrefix();
  auto j = std::find(std::begin(kReservedIdentifiers),
                     std::end(kReservedIdentifiers), name);
  if (j != std::end(kReservedIdentifiers)) {
    throw CompileError{location, "Reserved word '" + std::string{name} +
                                     "' can't be used as an identifier."};
  }
  if (name.empty() || !std::isalpha(name[0]))
    throw CompileError{location, "Invalid identifier: " + std::string{name}};
  reader_->remove_prefix(name.length());
  return ast::Identifier{{location}, std::string{name}};
}

ast::Integer Parser::ParseInteger() {
  auto location = reader_->location();
  const bool negative = reader_->Consume("-");
  auto begin = std::begin(*reader_);
  auto end = std::end(*reader_);
  auto i = std::find_if_not(begin, end,
                            [](unsigned char c) { return std::isdigit(c); });
  auto length = static_cast<std::string_view::size_type>(i - begin);
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
  return ast::Integer{{location}, value};
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
  // Check if this term is a nested expression.
  auto location = reader_->location();
  if (reader_->Consume("(")) {
    auto expression = ParseExpression();
    if (!reader_->Consume(")"))
      throw CompileError{location, "No matching ')' for this '('."};
    return expression;
  }
  CheckNotEnd();
  char lookahead = reader_->front();
  if (lookahead == '-' || std::isdigit(lookahead)) {
    // Positive or negative integers.
    return ParseInteger();
  } else if (std::isalpha(lookahead)) {
    auto candidate = IdentifierPrefix();
    if (candidate == "true" || candidate == "false") {
      reader_->remove_prefix(candidate.length());
      return ast::Boolean{{location}, candidate == "true"};
    }
    // Variables or function calls.
    auto identifier = ParseIdentifier();
    if (!reader_->empty() && reader_->front() == '(') {
      auto arguments = ParseArgumentList();
      return ast::FunctionCall{
          {location}, std::move(identifier), std::move(arguments)};
    } else {
      return identifier;
    }
  } else {
    throw CompileError{location, "Illegal token."};
  }
}

ast::Expression Parser::ParseUnary() {
  auto location = reader_->location();
  if (reader_->Consume("!")) {
    return ast::LogicalNot{{location}, ParseUnary()};
  } else {
    return ParseTerm();
  }
}

ast::Expression Parser::ParseProduct() {
  auto left = ParseUnary();
  while (true) {
    if (reader_->starts_with(" * ")) {
      reader_->remove_prefix(1);
      auto location = reader_->location();
      reader_->remove_prefix(2);
      left = ast::Binary{
          {location}, ast::Binary::MULTIPLY, std::move(left), ParseTerm()};
    } else if (reader_->starts_with(" / ")) {
      reader_->remove_prefix(1);
      auto location = reader_->location();
      reader_->remove_prefix(2);
      left = ast::Binary{
          {location}, ast::Binary::DIVIDE, std::move(left), ParseTerm()};
    } else {
      return left;
    }
  }
}

ast::Expression Parser::ParseSum() {
  auto left = ParseProduct();
  while (true) {
    if (reader_->starts_with(" + ")) {
      reader_->remove_prefix(1);
      auto location = reader_->location();
      reader_->remove_prefix(2);
      left = ast::Binary{{location}, ast::Binary::ADD, left, ParseProduct()};
    } else if (reader_->starts_with(" - ")) {
      reader_->remove_prefix(1);
      auto location = reader_->location();
      reader_->remove_prefix(2);
      left = ast::Binary{
          {location}, ast::Binary::SUBTRACT, std::move(left), ParseProduct()};
    } else {
      return left;
    }
  }
}

ast::Expression Parser::ParseComparison() {
  auto left = ParseSum();
  if (reader_->starts_with(" == ")) {
    reader_->remove_prefix(1);
    auto location = reader_->location();
    reader_->remove_prefix(3);
    return ast::Binary{
        {location}, ast::Binary::COMPARE_EQ, std::move(left), ParseSum()};
  } else if (reader_->starts_with(" != ")) {
    reader_->remove_prefix(1);
    auto location = reader_->location();
    reader_->remove_prefix(3);
    return ast::Binary{
        {location}, ast::Binary::COMPARE_NE, std::move(left), ParseSum()};
  } else if (reader_->starts_with(" <= ")) {
    reader_->remove_prefix(1);
    auto location = reader_->location();
    reader_->remove_prefix(3);
    return ast::Binary{
        {location}, ast::Binary::COMPARE_LE, std::move(left), ParseSum()};
  } else if (reader_->starts_with(" < ")) {
    reader_->remove_prefix(1);
    auto location = reader_->location();
    reader_->remove_prefix(2);
    return ast::Binary{
        {location}, ast::Binary::COMPARE_LT, std::move(left), ParseSum()};
  } else if (reader_->starts_with(" >= ")) {
    reader_->remove_prefix(1);
    auto location = reader_->location();
    reader_->remove_prefix(3);
    return ast::Binary{
        {location}, ast::Binary::COMPARE_GE, std::move(left), ParseSum()};
  } else if (reader_->starts_with(" > ")) {
    reader_->remove_prefix(1);
    auto location = reader_->location();
    reader_->remove_prefix(2);
    return ast::Binary{
        {location}, ast::Binary::COMPARE_GT, std::move(left), ParseSum()};
  } else {
    return left;
  }
}

ast::Expression Parser::ParseConjunction() {
  auto left = ParseComparison();
  while (true) {
    if (reader_->starts_with(" && ")) {
      reader_->remove_prefix(1);
      auto location = reader_->location();
      reader_->remove_prefix(3);
      left = ast::Binary{{location},
                         ast::Binary::LOGICAL_AND,
                         std::move(left),
                         ParseComparison()};
    } else {
      return left;
    }
  }
}

ast::Expression Parser::ParseDisjunction() {
  auto left = ParseConjunction();
  while (true) {
    if (reader_->starts_with(" || ")) {
      reader_->remove_prefix(1);
      auto location = reader_->location();
      reader_->remove_prefix(3);
      left = ast::Binary{{location},
                         ast::Binary::LOGICAL_OR,
                         std::move(left),
                         ParseConjunction()};
    } else {
      return left;
    }
  }
}

ast::Expression Parser::ParseExpression() { return ParseDisjunction(); }

ast::DefineVariable Parser::ParseVariableDefinition() {
  CheckConsume("let ");
  auto identifier = ParseIdentifier();
  CheckConsume(" ");
  auto location = reader_->location();
  CheckConsume("= ");
  auto value = ParseExpression();
  return ast::DefineVariable{
      {location}, std::move(identifier), std::move(value)};
}

ast::Assign Parser::ParseAssignment() {
  auto identifier = ParseIdentifier();
  CheckConsume(" ");
  auto location = reader_->location();
  CheckConsume("= ");
  auto value = ParseExpression();
  return ast::Assign{{location}, std::move(identifier), std::move(value)};
}

ast::DoFunction Parser::ParseDoFunction() {
  auto do_location = reader_->location();
  CheckConsume("do ");
  auto call_location = reader_->location();
  auto function = ParseIdentifier();
  auto arguments = ParseArgumentList();
  return ast::DoFunction{
      {do_location},
      ast::FunctionCall{
          {call_location}, std::move(function), std::move(arguments)}};
}

ast::If Parser::ParseIfStatement(std::size_t indent) {
  auto location = reader_->location();
  CheckConsume("if (");
  auto condition = ParseExpression();
  CheckConsume(") ");
  auto statements = ParseStatementBlock(indent);
  if (reader_->starts_with(" else if (")) {
    CheckConsume(" else ");
    return ast::If{{location},
                   std::move(condition),
                   std::move(statements),
                   {ParseIfStatement(indent)}};
  } else if (reader_->Consume(" else ")) {
    return ast::If{{location},
                   std::move(condition),
                   std::move(statements),
                   ParseStatementBlock(indent)};
  } else {
    return ast::If{{location}, std::move(condition), std::move(statements), {}};
  }
}

ast::While Parser::ParseWhileStatement(std::size_t indent) {
  auto location = reader_->location();
  CheckConsume("while (");
  auto condition = ParseExpression();
  CheckConsume(") ");
  auto statements = ParseStatementBlock(indent);
  return ast::While{{location}, std::move(condition), std::move(statements)};
}

ast::Statement Parser::ParseStatement(std::size_t indent) {
  ParseComment(indent);
  if (reader_->starts_with("let ")) {
    return ParseVariableDefinition();
  } else if (reader_->starts_with("do ")) {
    return ParseDoFunction();
  } else if (reader_->starts_with("if ")) {
    return ParseIfStatement(indent);
  } else if (reader_->starts_with("while ")) {
    return ParseWhileStatement(indent);
  } else if (reader_->starts_with("return\n")) {
    auto location = reader_->location();
    CheckConsume("return");
    return ast::ReturnVoid{{location}};
  } else if (reader_->Consume("return ")) {
    auto location = reader_->location();
    return ast::Return{{location}, ParseExpression()};
  } else {
    return ParseAssignment();
  }
}

std::vector<ast::Statement> Parser::ParseStatementBlock(std::size_t indent) {
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

std::vector<ast::Identifier> Parser::ParseParameterList() {
  CheckConsume("(");
  if (reader_->Consume(")")) return {};
  std::vector<ast::Identifier> parameters;
  while (true) {
    parameters.push_back(ParseIdentifier());
    CheckConsume(" : ");
    parameters.back().type = ParseType();
    CheckNotEnd();
    if (reader_->Consume(")")) return parameters;
    CheckConsume(", ");
  }
}

ast::DefineFunction Parser::ParseFunctionDefinition() {
  ParseComment(0);
  auto location = reader_->location();
  CheckConsume("function ");
  auto identifier = ParseIdentifier();
  auto parameters = ParseParameterList();
  CheckConsume(" : ");
  auto return_type = ParseType();
  identifier.type = ast::Function{std::move(return_type), parameters};
  CheckConsume(" ");
  auto body = ParseStatementBlock(0);
  ConsumeNewline();
  return ast::DefineFunction{{location},
                             std::move(identifier),
                             std::move(parameters),
                             std::move(body)};
}

std::vector<ast::DefineFunction> Parser::ParseProgram() {
  std::vector<ast::DefineFunction> definitions;
  definitions.push_back(ParseFunctionDefinition());
  while (!reader_->empty()) {
    ConsumeNewline();
    definitions.push_back(ParseFunctionDefinition());
  }
  return definitions;
}

void Parser::ParseComment(std::size_t indent) {
  while (reader_->Consume("#")) {
    auto i = std::find(std::begin(*reader_), std::end(*reader_), '\n');
    reader_->remove_prefix(static_cast<std::size_t>(i - std::begin(*reader_)));
    ConsumeNewline();
    ConsumeIndent(indent);
  }
}

void Parser::CheckEnd() {
  if (!reader_->empty())
    throw CompileError{reader_->location(), "Unexpected trailing characters."};
}

void Parser::CheckConsume(std::string_view expected) {
  if (!reader_->Consume(expected)) {
    throw CompileError{reader_->location(),
                       "Expected '" + std::string{expected} + "'."};
  }
}

void Parser::ConsumeNewline() {
  if (!reader_->Consume("\n"))
    throw CompileError{reader_->location(), "Expected '\\n'."};
}

void Parser::ConsumeIndent(std::size_t indent) {
  auto has_indent = [&] {
    std::string_view prefix = reader_->prefix(indent);
    if (prefix.length() < indent) return false;
    for (std::size_t i = 0; i < indent; i++) {
      if (prefix[i] != ' ') return false;
    }
    return true;
  };

  if (has_indent()) {
    reader_->remove_prefix(indent);
  } else {
    throw CompileError{
        reader_->location(),
        "Expected at least " + std::to_string(indent) + " spaces of indent."};
  }
}

void Parser::CheckNotEnd() {
  if (reader_->empty())
    throw CompileError{reader_->location(), "Unexpected end of input."};
}

std::string_view Parser::IdentifierPrefix() const {
  auto begin = std::begin(*reader_);
  auto end = std::end(*reader_);
  auto i = std::find_if_not(begin, end,
                            [](unsigned char c) { return std::isalnum(c); });
  return reader_->prefix(static_cast<std::size_t>(i - begin));
}
