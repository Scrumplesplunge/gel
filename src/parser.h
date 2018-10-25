#pragma once

#include "ast.h"
#include "reader.h"

#include <stdexcept>
#include <string_view>

struct ParseMetadata {
  struct Expression {
    Reader::Location location;
  };
  struct Statement {
    Reader::Location location;
  };
  struct TopLevel {
    Reader::Location location;
  };
};

using ParsedAst = ::ast::Ast<ParseMetadata>;

class Parser {
 public:
  Parser(Reader& reader) : reader_(&reader) {}

  types::Type ParseType();
  ParsedAst::Identifier ParseIdentifier();
  ParsedAst::Integer ParseInteger();
  std::vector<ParsedAst::Expression> ParseExpressionList(std::string_view begin,
                                                         std::string_view end);
  ParsedAst::Expression ParseTerm();
  ParsedAst::Expression ParseUnary();
  ParsedAst::Expression ParseProduct();
  ParsedAst::Expression ParseSum();
  ParsedAst::Expression ParseComparison();
  ParsedAst::Expression ParseConjunction();
  ParsedAst::Expression ParseDisjunction();
  ParsedAst::Expression ParseExpression();

  ParsedAst::DefineVariable ParseVariableDefinition();
  ParsedAst::Assign ParseAssignment();
  ParsedAst::DoFunction ParseDoFunction();
  ParsedAst::If ParseIfStatement(std::size_t indent);
  ParsedAst::While ParseWhileStatement(std::size_t indent);
  ParsedAst::Statement ParseStatement(std::size_t indent);
  std::vector<ParsedAst::Statement> ParseStatementBlock(std::size_t indent);

  std::tuple<std::vector<ParsedAst::Identifier>, std::vector<types::Type>>
  ParseParameterList();
  ParsedAst::DefineFunction ParseFunctionDefinition();
  std::vector<ParsedAst::DefineFunction> ParseProgram();

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
