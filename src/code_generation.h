#pragma once

#include "ast.h"
#include "operations.h"
#include "visitable.h"

#include <map>
#include <string>

namespace codegen {

// The context holds state which is shared across a whole code generation pass.
class Context {
 public:
  std::string Label(std::string_view prefix);

 private:
  std::map<std::string, int, std::less<>> labels_;
};

// A LexicalScope manages allocation of variables.
class LexicalScope {
 public:
  enum Type {
    BASE,
    // Allocate within the same frame as the parent scope. This is used for
    // nested blocks such as if-statements.
    EXTEND,
  };

  LexicalScope() = default;
  LexicalScope(Type type, LexicalScope& parent);
  ~LexicalScope();

  LexicalScope(const LexicalScope&) = delete;
  LexicalScope(LexicalScope&&) = delete;
  LexicalScope& operator=(const LexicalScope&) = delete;
  LexicalScope& operator=(LexicalScope&&) = delete;

  struct Entry {
    int offset = 0;
  };

  const Entry& Define(std::string_view name);
  const Entry* Lookup(std::string_view name) const;

  // This is code which should be performed on entry to the scope or on exit
  // from the scope. These functions will not work properly if they are used
  // before all variables in the scope have been defined.
  op::Sequence EntryCode() const;

 private:
  void ReportOffset(int offset);

  Type type_ = BASE;
  LexicalScope* parent_ = nullptr;
  int min_offset_ = 0;
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

  op::Sequence ConsumeResult();

 private:
  void Assign(const LexicalScope::Entry& destination,
              const ast::Expression& value);

  void Visit(const std::vector<ast::Statement>& statements);

  Context* context_;
  LexicalScope* scope_;
  op::Sequence result_;
};

}  // namespace codegen
