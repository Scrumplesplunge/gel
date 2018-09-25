#include "code_generation.h"

namespace codegen {

std::string Context::Label(std::string_view prefix) {
  auto [i, j] = labels_.equal_range(prefix);
  if (i == j) i = labels_.emplace_hint(i, prefix, 0);
  return std::string{prefix} + std::to_string(i->second++);
}

LexicalScope::LexicalScope(Type type, LexicalScope& parent)
    : type_(type), parent_(&parent) {}

LexicalScope::~LexicalScope() {
  if (type_ == EXTEND) parent_->ReportOffset(current_offset_);
}

const LexicalScope::Entry& LexicalScope::Define(std::string_view name) {
  auto [i, j] = entries_.equal_range(name);
  if (i == j) {
    current_offset_--;
    ReportOffset(current_offset_);
    return entries_.emplace_hint(i, name, Entry{current_offset_})->second;
  } else {
    throw std::runtime_error("Variable '" + std::string{name} +
                             "' is already declared.");
  }
}

const LexicalScope::Entry* LexicalScope::Lookup(std::string_view name) const {
  auto i = entries_.find(name);
  return i == entries_.end() ? nullptr : &i->second;
}

op::Sequence LexicalScope::EntryCode() const {
  switch (type_) {
    case BASE:
      return {op::Adjust{min_offset_}};
    case EXTEND:
      // No code for extending; it is handled by the underlying scope.
      return {};
  }
}

void LexicalScope::ReportOffset(int offset) {
  min_offset_ = std::min(min_offset_, offset);
}

void Expression::Visit(const ast::Identifier& identifier) {
  if (auto* entry = scope_.Lookup(identifier.name)) {
    result_.push_back(op::Frame{entry->offset});
    result_.push_back(op::Load{});
  } else {
    throw std::runtime_error("No such variable '" +
                             std::string{identifier.name} + "'.");
  }
}

void Expression::Visit(const ast::Integer& integer) {
  result_.push_back(op::Integer{integer.value});
}

void Expression::Visit(const ast::Add& add) {
  Visit(add.left);
  Visit(add.right);
  result_.push_back(op::Add{});
}

void Expression::Visit(const ast::Subtract& subtract) {
  Visit(subtract.left);
  Visit(subtract.right);
  result_.push_back(op::Subtract{});
}

void Expression::Visit(const ast::Multiply& multiply) {
  Visit(multiply.left);
  Visit(multiply.right);
  result_.push_back(op::Multiply{});
}

void Expression::Visit(const ast::Divide& divide) {
  Visit(divide.left);
  Visit(divide.right);
  result_.push_back(op::Divide{});
}

void Expression::Visit(const ast::CompareEq& comparison) {
  Visit(comparison.left);
  Visit(comparison.right);
  result_.push_back(op::CompareEq{});
}

void Expression::Visit(const ast::CompareNe& comparison) {
  Visit(comparison.left);
  Visit(comparison.right);
  result_.push_back(op::CompareNe{});
}

void Expression::Visit(const ast::CompareLe& comparison) {
  Visit(comparison.left);
  Visit(comparison.right);
  result_.push_back(op::CompareLe{});
}

void Expression::Visit(const ast::CompareLt& comparison) {
  Visit(comparison.left);
  Visit(comparison.right);
  result_.push_back(op::CompareLt{});
}

void Expression::Visit(const ast::CompareGe& comparison) {
  Visit(comparison.left);
  Visit(comparison.right);
  result_.push_back(op::CompareGe{});
}

void Expression::Visit(const ast::CompareGt& comparison) {
  Visit(comparison.left);
  Visit(comparison.right);
  result_.push_back(op::CompareGt{});
}

void Expression::Visit(const ast::LogicalNot& expression) {
  Visit(expression.argument);
  result_.push_back(op::Integer{0});
  result_.push_back(op::CompareEq{});
}

void Expression::Visit(const ast::LogicalAnd& expression) {
  std::string end = context_->Label("AndEnd");
  Visit(expression.left);
  result_.push_back(op::JumpIfZero{end});
  Visit(expression.right);
  result_.push_back(op::Label{end});
}

void Expression::Visit(const ast::LogicalOr& expression) {
  std::string end = context_->Label("OrEnd");
  Visit(expression.left);
  result_.push_back(op::JumpIfNonZero{end});
  Visit(expression.right);
  result_.push_back(op::Label{end});
}

void Expression::Visit(const ast::FunctionCall&) {
  throw std::runtime_error("FunctionCall not implemented.");
}

void Statement::Visit(const ast::DeclareVariable& declaration) {
  const auto& entry = scope_->Define(declaration.identifier.name);
  Assign(entry, declaration.value);
}

void Statement::Visit(const ast::Assign& assignment) {
  if (auto* entry = scope_->Lookup(assignment.identifier.name)) {
    Assign(*entry, assignment.value);
  } else {
    throw std::runtime_error("No such variable '" +
                             std::string{assignment.identifier.name} + "'.");
  }
}

void Statement::Visit(const ast::DoFunction&) {
  throw std::runtime_error("DoFunction not implemented.");
}

void Statement::Visit(const ast::If& if_statement) {
  std::string if_false = context_->Label("IfFalse");
  std::string end = context_->Label("IfEnd");
  Expression codegen{context_, *scope_};
  if_statement.condition.Visit(codegen);
  result_.push_back(codegen.result());
  result_.push_back(op::JumpIfZero{if_false});
  {
    LexicalScope if_true_scope{LexicalScope::EXTEND, *scope_};
    Statement codegen{context_, &if_true_scope};
    codegen.Visit(if_statement.if_true);
    result_.push_back(codegen.ConsumeResult());
  }
  result_.push_back(op::Jump{end});
  result_.push_back(op::Label{if_false});
  result_.push_back(op::Adjust{1});
  {
    LexicalScope if_false_scope{LexicalScope::EXTEND, *scope_};
    Statement codegen{context_, &if_false_scope};
    codegen.Visit(if_statement.if_false);
    result_.push_back(codegen.ConsumeResult());
  }
  result_.push_back(op::Label{end});
}

op::Sequence Statement::ConsumeResult() {
  result_.insert(result_.begin(), scope_->EntryCode());
  return std::move(result_);
}

void Statement::Assign(const LexicalScope::Entry& destination,
                       const ast::Expression& value) {
  result_.push_back(op::Frame{destination.offset});
  Expression codegen{context_, *scope_};
  value.Visit(codegen);
  result_.push_back(codegen.result());
  result_.push_back(op::Store{});
}

void Statement::Visit(const std::vector<ast::Statement>& statements) {
  for (const auto& statement : statements) Visit(statement);
}

}  // namespace codegen
