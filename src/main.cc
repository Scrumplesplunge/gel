#include "ast.h"
#include "operations.h"
#include "parser.h"
#include "pretty.h"
#include "reader.h"

#include <map>
#include <stdexcept>

namespace codegen {

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
  Expression(const LexicalScope& scope) : scope_(scope) {}

  using ExpressionVisitor::Visit;
  void Visit(const ast::Identifier&) override;
  void Visit(const ast::Integer&) override;
  void Visit(const ast::Add&) override;
  void Visit(const ast::Subtract&) override;
  void Visit(const ast::Multiply&) override;
  void Visit(const ast::Divide&) override;
  void Visit(const ast::FunctionCall&) override;

  op::Sequence result() { return std::move(result_); };

 private:
  const LexicalScope& scope_;
  op::Sequence result_;
};

class Statement : public ast::StatementVisitor {
 public:
  Statement(LexicalScope* scope) : scope_(scope) {}

  using StatementVisitor::Visit;
  void Visit(const ast::DeclareVariable&) override;
  void Visit(const ast::Assign&) override;
  void Visit(const ast::DoFunction&) override;
  void Visit(const ast::If&) override;

  op::Sequence result() { return std::move(result_); };

 private:
  void Assign(const LexicalScope::Entry& destination,
              const ast::Expression& value);

  LexicalScope* scope_;
  op::Sequence result_;
};

// Implementation

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
  Expression codegen{*scope_};
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

bool prompt(std::string_view text, std::string& line) {
  std::cout << text;
  return bool{std::getline(std::cin, line)};
}

int main() {
  codegen::LexicalScope scope;
  std::string line;
  while (prompt(">> ", line)) {
    Reader reader{"stdin", line};
    Parser parser{reader};
    try {
      auto statement = parser.ParseStatement(0);
      parser.CheckEnd();
      codegen::Statement codegen(&scope);
      codegen.Visit(statement);
      OperationPrinter printer{std::cout};
      printer.Visit(codegen.result());
    } catch (const std::exception& error) {
      std::cout << error.what() << "\n";
    }
  }
}
