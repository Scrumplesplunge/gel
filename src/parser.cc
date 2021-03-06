#include "parser.h"

#include <algorithm>

constexpr const char* kReservedIdentifiers[] = {
    "boolean", "else",   "function", "if",   "integer",
    "let",     "return", "while",    "true", "false",
};

constexpr int kSpacesPerIndent = 2;

types::Type Parser::ParseType() {
  Reader::Location location = reader_->location();
  auto name = IdentifierPrefix();
  reader_->remove_prefix(name.length());
  if (name == "void") return types::Void{};
  if (name == "boolean") return types::Primitive::BOOLEAN;
  if (name == "integer") return types::Primitive::INTEGER;
  throw CompileError{location, "Invalid type name: " + std::string{name}};
}

ParsedAst::Identifier Parser::ParseIdentifier() {
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
  return ParsedAst::Identifier{{location}, std::string{name}};
}

ParsedAst::Integer Parser::ParseInteger() {
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
  return ParsedAst::Integer{{location}, value};
}

std::vector<ParsedAst::Expression> Parser::ParseExpressionList(
    std::string_view begin, std::string_view end) {
  CheckConsume(begin);
  if (reader_->Consume(end)) return {};
  std::vector<ParsedAst::Expression> arguments;
  while (true) {
    arguments.push_back(ParseExpression());
    CheckNotEnd();
    if (reader_->Consume(end)) return arguments;
    CheckConsume(", ");
  }
}

ParsedAst::Expression Parser::ParseTerm() {
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
  } else if (lookahead == '[') {
    return ParsedAst::ArrayLiteral{{location}, ParseExpressionList("[", "]")};
  } else if (std::isalpha(lookahead)) {
    auto candidate = IdentifierPrefix();
    if (candidate == "true" || candidate == "false") {
      reader_->remove_prefix(candidate.length());
      return ParsedAst::Boolean{{location}, candidate == "true"};
    }
    // Variables or function calls.
    auto identifier = ParseIdentifier();
    if (!reader_->empty() && reader_->front() == '(') {
      auto arguments = ParseExpressionList("(", ")");
      return ParsedAst::FunctionCall{
          {location}, std::move(identifier.name), std::move(arguments)};
    } else {
      return identifier;
    }
  } else {
    throw CompileError{location, "Illegal token."};
  }
}

ParsedAst::Expression Parser::ParseUnary() {
  auto location = reader_->location();
  if (reader_->Consume("!")) {
    return ParsedAst::LogicalNot{{location}, ParseUnary()};
  } else {
    return ParseTerm();
  }
}

ParsedAst::Expression Parser::ParseProduct() {
  auto left = ParseUnary();
  while (true) {
    if (reader_->starts_with(" * ")) {
      reader_->remove_prefix(1);
      auto location = reader_->location();
      reader_->remove_prefix(2);
      left = ParsedAst::Arithmetic{
          {location}, ast::Arithmetic::MULTIPLY, std::move(left), ParseTerm()};
    } else if (reader_->starts_with(" / ")) {
      reader_->remove_prefix(1);
      auto location = reader_->location();
      reader_->remove_prefix(2);
      left = ParsedAst::Arithmetic{
          {location}, ast::Arithmetic::DIVIDE, std::move(left), ParseTerm()};
    } else {
      return left;
    }
  }
}

ParsedAst::Expression Parser::ParseSum() {
  auto left = ParseProduct();
  while (true) {
    if (reader_->starts_with(" + ")) {
      reader_->remove_prefix(1);
      auto location = reader_->location();
      reader_->remove_prefix(2);
      left = ParsedAst::Arithmetic{
          {location}, ast::Arithmetic::ADD, left, ParseProduct()};
    } else if (reader_->starts_with(" - ")) {
      reader_->remove_prefix(1);
      auto location = reader_->location();
      reader_->remove_prefix(2);
      left = ParsedAst::Arithmetic{{location},
                                   ast::Arithmetic::SUBTRACT,
                                   std::move(left),
                                   ParseProduct()};
    } else {
      return left;
    }
  }
}

ParsedAst::Expression Parser::ParseComparison() {
  auto left = ParseSum();
  if (reader_->starts_with(" == ")) {
    reader_->remove_prefix(1);
    auto location = reader_->location();
    reader_->remove_prefix(3);
    return ParsedAst::Compare{
        {location}, ast::Compare::EQUAL, std::move(left), ParseSum()};
  } else if (reader_->starts_with(" != ")) {
    reader_->remove_prefix(1);
    auto location = reader_->location();
    reader_->remove_prefix(3);
    return ParsedAst::Compare{
        {location}, ast::Compare::NOT_EQUAL, std::move(left), ParseSum()};
  } else if (reader_->starts_with(" <= ")) {
    reader_->remove_prefix(1);
    auto location = reader_->location();
    reader_->remove_prefix(3);
    return ParsedAst::Compare{
        {location}, ast::Compare::LESS_OR_EQUAL, std::move(left), ParseSum()};
  } else if (reader_->starts_with(" < ")) {
    reader_->remove_prefix(1);
    auto location = reader_->location();
    reader_->remove_prefix(2);
    return ParsedAst::Compare{
        {location}, ast::Compare::LESS_THAN, std::move(left), ParseSum()};
  } else if (reader_->starts_with(" >= ")) {
    reader_->remove_prefix(1);
    auto location = reader_->location();
    reader_->remove_prefix(3);
    return ParsedAst::Compare{{location},
                              ast::Compare::GREATER_OR_EQUAL,
                              std::move(left),
                              ParseSum()};
  } else if (reader_->starts_with(" > ")) {
    reader_->remove_prefix(1);
    auto location = reader_->location();
    reader_->remove_prefix(2);
    return ParsedAst::Compare{
        {location}, ast::Compare::GREATER_THAN, std::move(left), ParseSum()};
  } else {
    return left;
  }
}

ParsedAst::Expression Parser::ParseConjunction() {
  auto left = ParseComparison();
  while (true) {
    if (reader_->starts_with(" && ")) {
      reader_->remove_prefix(1);
      auto location = reader_->location();
      reader_->remove_prefix(3);
      left = ParsedAst::Logical{
          {location}, ast::Logical::AND, std::move(left), ParseComparison()};
    } else {
      return left;
    }
  }
}

ParsedAst::Expression Parser::ParseDisjunction() {
  auto left = ParseConjunction();
  while (true) {
    if (reader_->starts_with(" || ")) {
      reader_->remove_prefix(1);
      auto location = reader_->location();
      reader_->remove_prefix(3);
      left = ParsedAst::Logical{
          {location}, ast::Logical::OR, std::move(left), ParseConjunction()};
    } else {
      return left;
    }
  }
}

ParsedAst::Expression Parser::ParseExpression() { return ParseDisjunction(); }

ParsedAst::DefineVariable Parser::ParseVariableDefinition() {
  CheckConsume("let ");
  auto identifier = ParseIdentifier();
  CheckConsume(" ");
  auto location = reader_->location();
  CheckConsume("= ");
  auto value = ParseExpression();
  return ParsedAst::DefineVariable{
      {location}, std::move(identifier), std::move(value)};
}

ParsedAst::Assign Parser::ParseAssignment() {
  auto identifier = ParseIdentifier();
  CheckConsume(" ");
  auto location = reader_->location();
  CheckConsume("= ");
  auto value = ParseExpression();
  return ParsedAst::Assign{{location}, std::move(identifier), std::move(value)};
}

ParsedAst::DoFunction Parser::ParseDoFunction() {
  auto do_location = reader_->location();
  CheckConsume("do ");
  auto call_location = reader_->location();
  auto function = ParseIdentifier();
  auto arguments = ParseExpressionList("(", ")");
  return ParsedAst::DoFunction{
      {do_location},
      ParsedAst::FunctionCall{
          {call_location}, std::move(function.name), std::move(arguments)}};
}

ParsedAst::If Parser::ParseIfStatement(std::size_t indent) {
  auto location = reader_->location();
  CheckConsume("if (");
  auto condition = ParseExpression();
  CheckConsume(") ");
  auto statements = ParseStatementBlock(indent);
  if (reader_->starts_with(" else if (")) {
    CheckConsume(" else ");
    return ParsedAst::If{{location},
                         std::move(condition),
                         std::move(statements),
                         {ParseIfStatement(indent)}};
  } else if (reader_->Consume(" else ")) {
    return ParsedAst::If{{location},
                         std::move(condition),
                         std::move(statements),
                         ParseStatementBlock(indent)};
  } else {
    return ParsedAst::If{
        {location}, std::move(condition), std::move(statements), {}};
  }
}

ParsedAst::While Parser::ParseWhileStatement(std::size_t indent) {
  auto location = reader_->location();
  CheckConsume("while (");
  auto condition = ParseExpression();
  CheckConsume(") ");
  auto statements = ParseStatementBlock(indent);
  return ParsedAst::While{
      {location}, std::move(condition), std::move(statements)};
}

ParsedAst::Statement Parser::ParseStatement(std::size_t indent) {
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
    return ParsedAst::ReturnVoid{{location}};
  } else if (reader_->Consume("return ")) {
    auto location = reader_->location();
    return ParsedAst::Return{{location}, ParseExpression()};
  } else {
    return ParseAssignment();
  }
}

std::vector<ParsedAst::Statement> Parser::ParseStatementBlock(
    std::size_t indent) {
  CheckConsume("{");
  CheckNotEnd();
  // Empty statement blocks are just "{}", ie. without a newline.
  if (reader_->Consume("}")) return {};
  // All other blocks have multiple lines and at least one statement.
  std::vector<ParsedAst::Statement> statements;
  while (true) {
    ConsumeNewline();
    ConsumeIndent(indent);
    if (reader_->Consume("}")) return statements;
    ConsumeIndent(kSpacesPerIndent);
    statements.push_back(ParseStatement(indent + kSpacesPerIndent));
  }
}

std::tuple<std::vector<ParsedAst::Identifier>, std::vector<types::Type>>
Parser::ParseParameterList() {
  CheckConsume("(");
  if (reader_->Consume(")")) return {};
  std::vector<ParsedAst::Identifier> parameters;
  std::vector<types::Type> parameter_types;
  while (true) {
    parameters.push_back(ParseIdentifier());
    CheckConsume(" : ");
    parameter_types.push_back(ParseType());
    CheckNotEnd();
    if (reader_->Consume(")"))
      return std::tuple{std::move(parameters), std::move(parameter_types)};
    CheckConsume(", ");
  }
}

ParsedAst::DefineFunction Parser::ParseFunctionDefinition() {
  ParseComment(0);
  auto location = reader_->location();
  CheckConsume("function ");
  auto identifier = ParseIdentifier();
  auto [parameters, parameter_types] = ParseParameterList();
  CheckConsume(" : ");
  auto return_type = ParseType();
  CheckConsume(" ");
  auto body = ParseStatementBlock(0);
  ConsumeNewline();
  return ParsedAst::DefineFunction{
      {location},
      types::Function{std::move(return_type), std::move(parameter_types)},
      std::move(identifier.name), std::move(parameters), std::move(body)};
}

std::vector<ParsedAst::DefineFunction> Parser::ParseProgram() {
  std::vector<ParsedAst::DefineFunction> definitions;
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
