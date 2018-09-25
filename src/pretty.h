#pragma once

#include "ast.h"
#include "operations.h"

#include <iostream>
#include <string>

namespace pretty {
namespace tree {  // Pretty printers that show the syntax tree explicitly.

class ExpressionPrinter : public ast::ExpressionVisitor {
 public:
  ExpressionPrinter(std::ostream& output) : output_(output) {}
  using ExpressionVisitor::Visit;
  void Visit(const ast::Identifier&) override;
  void Visit(const ast::Integer&) override;
  void Visit(const ast::Add&) override;
  void Visit(const ast::Subtract&) override;
  void Visit(const ast::Multiply&) override;
  void Visit(const ast::Divide&) override;
  void Visit(const ast::FunctionCall&) override;
  void Visit(const ast::CompareEq&) override;
  void Visit(const ast::CompareNe&) override;
  void Visit(const ast::CompareLe&) override;
  void Visit(const ast::CompareLt&) override;
  void Visit(const ast::CompareGe&) override;
  void Visit(const ast::CompareGt&) override;
  void Visit(const ast::LogicalNot&) override;
  void Visit(const ast::LogicalAnd&) override;
  void Visit(const ast::LogicalOr&) override;
 private:
  template <typename T>
  void VisitBinary(std::string_view type, const T& binary);

  std::ostream& output_;
};

class StatementPrinter : public ast::StatementVisitor {
 public:
  StatementPrinter(std::size_t indent, std::ostream& output)
      : indent_(indent, ' '), output_(output) {}
  using StatementVisitor::Visit;
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

class OperationPrinter : public op::Visitor {
 public:
  OperationPrinter(std::ostream& output) : output_(output) {}
  using Visitor::Visit;
  void Visit(const op::Sequence&) override;
  void Visit(const op::Integer&) override;
  void Visit(const op::Frame&) override;
  void Visit(const op::Adjust&) override;
  void Visit(const op::Load&) override;
  void Visit(const op::Store&) override;
  void Visit(const op::Add&) override;
  void Visit(const op::Subtract&) override;
  void Visit(const op::Multiply&) override;
  void Visit(const op::Divide&) override;
  void Visit(const op::CompareEq&) override;
  void Visit(const op::CompareNe&) override;
  void Visit(const op::CompareLe&) override;
  void Visit(const op::CompareLt&) override;
  void Visit(const op::CompareGe&) override;
  void Visit(const op::CompareGt&) override;
  void Visit(const op::Label&) override;
  void Visit(const op::Jump&) override;
  void Visit(const op::JumpIfZero&) override;
  void Visit(const op::JumpIfNonZero&) override;
 private:
  std::ostream& output_;
};

}  // namespace pretty
