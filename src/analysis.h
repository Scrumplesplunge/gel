#pragma once

#include "ast.h"
#include "parser.h"
#include "reader.h"

#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <vector>

namespace analysis {

struct AnnotatedMetadata {
  struct Expression {
    types::Type type;
  };
  struct Statement {};
  struct TopLevel {};
};

using AnnotatedAst = ::ast::Ast<AnnotatedMetadata>;

class Checker;

class MessageBuilder {
 public:
  MessageBuilder(Checker* checker, Message::Type type,
                 Reader::Location location)
      : checker_(checker), type_(type), location_(location) {}
  ~MessageBuilder();
  MessageBuilder(const MessageBuilder&) = delete;
  MessageBuilder(MessageBuilder&&) = delete;

  template <typename T>
  MessageBuilder& operator<<(T&& value) {
    text_ << value;
    return *this;
  }

 private:
  Checker* const checker_;
  const Message::Type type_;
  const Reader::Location location_;
  std::ostringstream text_;
};

struct Operators {
  using ArithmeticKey = std::tuple<ast::Arithmetic, types::Type>;
  std::set<ArithmeticKey> arithmetic;
  std::set<types::Type> equality_comparable;
  std::set<types::Type> ordered;
};

class Scope {
 public:
  struct Entry {
    Reader::Location location;
    // The type is present unless the expression that defined this variable
    // contained an an error.
    std::optional<types::Type> type;
  };
  explicit Scope(Scope* parent = nullptr) : parent_(parent) {}

  bool Define(std::string name, Entry entry);
  const Entry* Lookup(std::string_view name) const;

 private:
  const Scope* parent_ = nullptr;
  std::map<std::string, Entry, std::less<>> bindings_;
};

class Checker {
 public:
  Checker();

  std::optional<AnnotatedAst::DefineFunction> CheckTopLevel(
      const ParsedAst::DefineFunction&);
  std::optional<std::vector<AnnotatedAst::DefineFunction>> CheckTopLevel(
      const std::vector<ParsedAst::DefineFunction>&);
  std::optional<AnnotatedAst::TopLevel> CheckAnyTopLevel(
      const ParsedAst::TopLevel&);

  void AddType(const types::Type& type);
  MessageBuilder Error(Reader::Location location);
  MessageBuilder Warning(Reader::Location location);
  MessageBuilder Note(Reader::Location location);

  std::vector<Message> ConsumeDiagnostics() { return std::move(diagnostics_); }
  std::vector<types::Type> ConsumeTypes() { return std::move(types_); }

 private:
  friend class MessageBuilder;
  friend class FunctionChecker;

  const Operators operators_;
  std::vector<Message> diagnostics_;
  std::vector<types::Type> types_;
  Scope scope_;
};

class FunctionChecker {
 public:
  FunctionChecker(types::Function type, std::string this_function,
                  Checker* checker, Scope* scope)
      : type_(std::move(type)),
        this_function_(std::move(this_function)),
        checker_(checker), scope_(scope) {}

  std::optional<AnnotatedAst::Identifier> CheckExpression(
      const ParsedAst::Identifier&);
  std::optional<AnnotatedAst::Boolean> CheckExpression(
      const ParsedAst::Boolean&);
  std::optional<AnnotatedAst::Integer> CheckExpression(
      const ParsedAst::Integer&);
  std::optional<AnnotatedAst::ArrayLiteral> CheckExpression(
      const ParsedAst::ArrayLiteral&);
  std::optional<AnnotatedAst::Arithmetic> CheckExpression(
      const ParsedAst::Arithmetic&);
  std::optional<AnnotatedAst::Compare> CheckExpression(
      const ParsedAst::Compare&);
  std::optional<AnnotatedAst::Logical> CheckExpression(
      const ParsedAst::Logical&);
  std::optional<AnnotatedAst::FunctionCall> CheckExpression(
      const ParsedAst::FunctionCall&);
  std::optional<AnnotatedAst::LogicalNot> CheckExpression(
      const ParsedAst::LogicalNot&);
  std::optional<AnnotatedAst::Expression> CheckAnyExpression(
      const ParsedAst::Expression&);

  std::optional<AnnotatedAst::DefineVariable> CheckStatement(
      const ParsedAst::DefineVariable&);
  std::optional<AnnotatedAst::Assign> CheckStatement(const ParsedAst::Assign&);
  std::optional<AnnotatedAst::DoFunction> CheckStatement(
      const ParsedAst::DoFunction&);
  std::optional<AnnotatedAst::If> CheckStatement(const ParsedAst::If&);
  std::optional<AnnotatedAst::While> CheckStatement(const ParsedAst::While&);
  std::optional<AnnotatedAst::ReturnVoid> CheckStatement(
      const ParsedAst::ReturnVoid&);
  std::optional<AnnotatedAst::Return> CheckStatement(const ParsedAst::Return&);
  std::optional<std::vector<AnnotatedAst::Statement>> CheckStatement(
      const std::vector<ParsedAst::Statement>&);
  std::optional<AnnotatedAst::Statement> CheckAnyStatement(
      const ParsedAst::Statement&);

 private:
  types::Function type_;
  std::string this_function_;
  Checker* checker_;
  Scope* scope_;
};

struct Result {
  std::vector<types::Type> required_types;
  std::optional<AnnotatedAst::TopLevel> annotated_ast;
  std::vector<Message> diagnostics;
};
Result Check(const ParsedAst::TopLevel&);

}  // namespace analysis
