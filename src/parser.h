#pragma once

#include "ast.h"
#include "reader.h"

#include <stdexcept>
#include <string_view>

class Parser {
 public:
  Parser(Reader& reader) : reader_(&reader) {}

  ast::Type ParseType();
  ast::Identifier ParseIdentifier();
  ast::Integer ParseInteger();
  std::vector<ast::Expression> ParseArgumentList();
  ast::Expression ParseTerm();
  ast::Expression ParseUnary();
  ast::Expression ParseProduct();
  ast::Expression ParseSum();
  ast::Expression ParseComparison();
  ast::Expression ParseConjunction();
  ast::Expression ParseDisjunction();
  ast::Expression ParseExpression();

  ast::DefineVariable ParseVariableDefinition();
  ast::Assign ParseAssignment();
  ast::DoFunction ParseDoFunction();
  ast::If ParseIfStatement(std::size_t indent);
  ast::While ParseWhileStatement(std::size_t indent);
  ast::Statement ParseStatement(std::size_t indent);
  std::vector<ast::Statement> ParseStatementBlock(std::size_t indent);

  std::vector<ast::Identifier> ParseParameterList();
  ast::DefineFunction ParseFunctionDefinition();
  std::vector<ast::DefineFunction> ParseProgram();

  void ParseComment(std::size_t indent);

  void CheckEnd();
  void CheckConsume(std::string_view expected);
  void ConsumeNewline();
  void ConsumeIndent(std::size_t indent);
  void CheckNotEnd();

 private:
  std::string_view IdentifierPrefix() const;

  Reader* reader_;
};
