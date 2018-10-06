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
  void Visit(const ast::Binary&) override;
  void Visit(const ast::FunctionCall&) override;
  void Visit(const ast::LogicalNot&) override;

 private:
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
