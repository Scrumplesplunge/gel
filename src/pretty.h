#pragma once

#include "ast.h"

#include <iostream>
#include <string>

namespace pretty {
namespace tree {  // Pretty printers that show the syntax tree explicitly.

class ExpressionPrinter : public ast::ExpressionVisitor {
 public:
  ExpressionPrinter(std::ostream& output) : output_(output) {}
  void Visit(const ast::Identifier&) override;
  void Visit(const ast::Integer&) override;
  void Visit(const ast::Add&) override;
  void Visit(const ast::Subtract&) override;
  void Visit(const ast::Multiply&) override;
  void Visit(const ast::Divide&) override;
  void Visit(const ast::FunctionCall&) override;
 private:
  template <typename T>
  void VisitBinary(std::string_view type, const T& binary);

  std::ostream& output_;
};

class StatementPrinter : public ast::StatementVisitor {
 public:
  StatementPrinter(std::size_t indent, std::ostream& output)
      : indent_(indent, ' '), output_(output) {}
  void Visit(const ast::DeclareVariable&) override;
  void Visit(const ast::Assign&) override;
  void Visit(const ast::DoFunction&) override;
  void Visit(const ast::If&) override;
 private:
  void VisitBlock(const std::vector<ast::Statement>& statements);

  const std::string indent_;
  std::ostream& output_;
};

}  // namespace tree
}  // namespace pretty
