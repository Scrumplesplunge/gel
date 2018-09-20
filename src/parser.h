#pragma once

#include "ast.h"
#include "reader.h"

#include <stdexcept>
#include <string_view>

class SyntaxError : public std::runtime_error {
 public:
  using runtime_error::runtime_error;
};

class Parser {
 public:
  Parser(Reader& reader) : reader_(&reader) {}

  ast::Identifier ParseIdentifier();
  ast::Integer ParseInteger();
  std::vector<ast::Expression> ParseArgumentList();
  ast::Expression ParseTerm();
  ast::Expression ParseProduct();
  ast::Expression ParseExpression();

  ast::DeclareVariable ParseVariableDeclaration();
  ast::Assign ParseAssignment();
  ast::DoFunction ParseDoFunction();
  ast::If ParseIfStatement(int indent);
  ast::Statement ParseStatement(int indent);
  std::vector<ast::Statement> ParseStatementBlock(int indent);

  void CheckEnd();
  void CheckConsume(std::string_view expected);
  void ConsumeNewline();
  void ConsumeIndent(int indent);
  void CheckNotEnd();

 private:
  Reader* reader_;
};
