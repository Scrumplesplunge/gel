#pragma once

#include "ast.h"
#include "reader.h"

#include <stdexcept>
#include <string_view>

class Parser {
 public:
  Parser(Reader& reader) : reader_(&reader) {}

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
  ast::If ParseIfStatement(int indent);
  ast::While ParseWhileStatement(int indent);
  ast::Statement ParseStatement(int indent);
  std::vector<ast::Statement> ParseStatementBlock(int indent);

  std::vector<std::string> ParseParameterList();
  ast::DefineFunction ParseFunctionDefinition();
  std::vector<ast::DefineFunction> ParseProgram();

  void ParseComment(int indent);

  void CheckEnd();
  void CheckConsume(std::string_view expected);
  void ConsumeNewline();
  void ConsumeIndent(int indent);
  void CheckNotEnd();

 private:
  Reader* reader_;
};
