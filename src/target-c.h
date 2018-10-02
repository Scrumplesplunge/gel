#pragma once

#include "ast.h"

#include <iostream>
#include <string_view>

namespace target::c {

class Expression : public ast::ExpressionVisitor {
 public:
  Expression(std::ostream& output) : output_(output) {}

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
  void VisitBinary(std::string_view op, const ast::Expression& left,
                   const ast::Expression& right);

  std::ostream& output_;
};

class Statement : public ast::StatementVisitor {
 public:
  Statement(std::ostream& output, int indent)
      : output_(output), indent_(indent) {}

  using StatementVisitor::Visit;
  void Visit(const ast::DefineVariable&) override;
  void Visit(const ast::Assign&) override;
  void Visit(const ast::DoFunction&) override;
  void Visit(const ast::If&) override;
  void Visit(const ast::While&) override;
  void Visit(const ast::ReturnVoid&) override;
  void Visit(const ast::Return&) override;
  void Visit(const std::vector<ast::Statement>& statements);

 private:
  std::ostream& output_;
  int indent_;
};

class TopLevel : public ast::TopLevelVisitor {
 public:
  TopLevel(std::ostream& output);

  using TopLevelVisitor::Visit;
  void Visit(const ast::DefineFunction&) override;
  void Visit(const std::vector<ast::DefineFunction>&) override;

 private:
  std::ostream& output_;
};

}  // namespace target::c
