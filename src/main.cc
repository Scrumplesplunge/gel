#include "ast.h"
#include "operations.h"
#include "parser.h"
#include "pretty.h"
#include "reader.h"

#include <map>
#include <stdexcept>

namespace codegen {

class Context {
 public:
  std::string Label(std::string_view prefix);

 private:
  std::map<std::string, int, std::less<>> labels_;
};

class LexicalScope {
 public:
  struct Entry {
    int offset = 0;
  };

  const Entry& Define(std::string_view name);
  const Entry* Lookup(std::string_view name) const;

 private:
  int current_offset_ = 0;
  std::map<std::string, Entry, std::less<>> entries_;
};

class Expression : public ast::ExpressionVisitor {
 public:
  Expression(Context* context, const LexicalScope& scope)
      : context_(context), scope_(scope) {}

  Expression(const Expression&) = delete;
  Expression(Expression&&) = delete;
  Expression& operator=(const Expression&) = delete;
  Expression& operator=(Expression&&) = delete;

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

  op::Sequence result() { return std::move(result_); };

 private:
  Context* context_;
  const LexicalScope& scope_;
  op::Sequence result_;
};

class Statement : public ast::StatementVisitor {
 public:
  Statement(Context* context, LexicalScope* scope)
      : context_(context), scope_(scope) {}

  Statement(const Statement&) = delete;
  Statement(Statement&&) = delete;
  Statement& operator=(const Statement&) = delete;
  Statement& operator=(Statement&&) = delete;

  using StatementVisitor::Visit;
  void Visit(const ast::DeclareVariable&) override;
  void Visit(const ast::Assign&) override;
  void Visit(const ast::DoFunction&) override;
  void Visit(const ast::If&) override;

  op::Sequence result() { return std::move(result_); };

 private:
  void Assign(const LexicalScope::Entry& destination,
              const ast::Expression& value);

  Context* context_;
  LexicalScope* scope_;
  op::Sequence result_;
};

// Implementation

std::string Context::Label(std::string_view prefix) {
  auto [i, j] = labels_.equal_range(prefix);
  if (i == j) i = labels_.emplace_hint(i, prefix, 0);
  return std::string{prefix} + std::to_string(i->second++);
}

const LexicalScope::Entry& LexicalScope::Define(std::string_view name) {
  auto [i, j] = entries_.equal_range(name);
  if (i == j) {
    constexpr int kIntSize = 4;
    current_offset_ -= kIntSize;
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
  std::string end = context_->Label("LogicalAnd");
  Visit(expression.left);
  result_.push_back(op::JumpIfZero{end});
  Visit(expression.right);
  result_.push_back(op::Label{end});
}

void Expression::Visit(const ast::LogicalOr& expression) {
  std::string end = context_->Label("LogicalOr");
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

void Statement::Visit(const ast::If&) {
  throw std::runtime_error("If not implemented.");
}

void Statement::Assign(const LexicalScope::Entry& destination,
                       const ast::Expression& value) {
  result_.push_back(op::Frame{destination.offset});
  Expression codegen{context_, *scope_};
  value.Visit(codegen);
  result_.push_back(codegen.result());
  result_.push_back(op::Store{});
}

}  // namespace codegen

class OperationPrinter : public op::Visitor {
 public:
  OperationPrinter(std::ostream& output) : output_(output) {}
  using Visitor::Visit;
  void Visit(const op::Sequence&) override;
  void Visit(const op::Integer&) override;
  void Visit(const op::Frame&) override;
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

void OperationPrinter::Visit(const op::Sequence& sequence) {
  for (const auto& operation : sequence) Visit(operation);
}

void OperationPrinter::Visit(const op::Integer& integer) {
  output_ << "  push " << integer.value << "\n";
}

void OperationPrinter::Visit(const op::Frame& frame) {
  output_ << "  frame " << frame.offset << "\n";
}

void OperationPrinter::Visit(const op::Load&) { output_ << "  load\n"; }
void OperationPrinter::Visit(const op::Store&) { output_ << "  store\n"; }
void OperationPrinter::Visit(const op::Add&) { output_ << "  add\n"; }
void OperationPrinter::Visit(const op::Subtract&) { output_ << "  sub\n"; }
void OperationPrinter::Visit(const op::Multiply&) { output_ << "  mul\n"; }
void OperationPrinter::Visit(const op::Divide&) { output_ << "  div\n"; }
void OperationPrinter::Visit(const op::CompareEq&) { output_ << "  ceq\n"; }
void OperationPrinter::Visit(const op::CompareNe&) { output_ << "  cne\n"; }
void OperationPrinter::Visit(const op::CompareLe&) { output_ << "  cle\n"; }
void OperationPrinter::Visit(const op::CompareLt&) { output_ << "  clt\n"; }
void OperationPrinter::Visit(const op::CompareGe&) { output_ << "  cge\n"; }
void OperationPrinter::Visit(const op::CompareGt&) { output_ << "  cgt\n"; }

void OperationPrinter::Visit(const op::Label& label) {
  output_ << label.name << ":\n";
}

void OperationPrinter::Visit(const op::Jump& jump) {
  output_ << "  jump " << jump.label << "\n";
}

void OperationPrinter::Visit(const op::JumpIfZero& jump) {
  output_ << "  jz " << jump.label << "\n";
}

void OperationPrinter::Visit(const op::JumpIfNonZero& jump) {
  output_ << "  jnz " << jump.label << "\n";
}

bool prompt(std::string_view text, std::string& line) {
  std::cout << text;
  return bool{std::getline(std::cin, line)};
}

int main() {
  codegen::Context context;
  codegen::LexicalScope scope;
  std::string line;
  while (prompt(">> ", line)) {
    Reader reader{"stdin", line};
    Parser parser{reader};
    try {
      auto statement = parser.ParseStatement(0);
      parser.CheckEnd();
      codegen::Statement codegen(&context, &scope);
      codegen.Visit(statement);
      OperationPrinter printer{std::cout};
      printer.Visit(codegen.result());
    } catch (const std::exception& error) {
      std::cout << error.what() << "\n";
    }
  }
}
