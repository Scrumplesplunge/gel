#pragma once

#include "ast.h"
#include "reader.h"

#include <map>
#include <optional>
#include <sstream>
#include <vector>

namespace analysis {

struct Context;

class MessageBuilder {
 public:
  MessageBuilder(Context* context, Message::Type type,
                 Reader::Location location)
      : context_(context), type_(type), location_(location) {}
  ~MessageBuilder();
  MessageBuilder(const MessageBuilder&) = delete;
  MessageBuilder(MessageBuilder&&) = delete;

  template <typename T>
  MessageBuilder& operator<<(T&& value) { text_ << value; return *this; }
 private:
  Context* const context_;
  const Message::Type type_;
  const Reader::Location location_;
  std::ostringstream text_;
};

struct Context {
  std::vector<Message> diagnostics;
  MessageBuilder Error(Reader::Location location);
  MessageBuilder Warning(Reader::Location location);
  MessageBuilder Note(Reader::Location location);
};

class Scope {
 public:
  struct Entry {
    Reader::Location location;
    std::optional<ast::Type> type;
  };
  explicit Scope(Scope* parent = nullptr) : parent_(parent) {}
  bool Define(std::string name, Entry entry);
  const Entry* Lookup(std::string_view name) const;
 private:
  const Scope* parent_ = nullptr;
  std::map<std::string, Entry, std::less<>> bindings_;
};

ast::Identifier Check(const ast::Identifier&, Context*, const Scope*);
ast::Boolean Check(const ast::Boolean&, Context*, const Scope*);
ast::Integer Check(const ast::Integer&, Context*, const Scope*);
ast::Binary Check(const ast::Binary&, Context*, const Scope*);
ast::FunctionCall Check(const ast::FunctionCall&, Context*, const Scope*);
ast::LogicalNot Check(const ast::LogicalNot&, Context*, const Scope*);
ast::Expression Check(const ast::Expression&, Context*, const Scope*);

class Statement : public ast::StatementVisitor {
 public:
  Statement(Context* context, Scope* scope);
  void Visit(const ast::DefineVariable& d) override { result_ = Check(d); }
  void Visit(const ast::Assign& a) override { result_ = Check(a); }
  void Visit(const ast::DoFunction& d) override { result_ = Check(d); }
  void Visit(const ast::If& i) override { result_ = Check(i); }
  void Visit(const ast::While& w) override { result_ = Check(w); }
  void Visit(const ast::ReturnVoid& r) override { result_ = Check(r); }
  void Visit(const ast::Return& r) override { result_ = Check(r); }
  ast::DefineVariable Check(const ast::DefineVariable&) const;
  ast::Assign Check(const ast::Assign&) const;
  ast::DoFunction Check(const ast::DoFunction&) const;
  ast::If Check(const ast::If&) const;
  ast::While Check(const ast::While&) const;
  ast::ReturnVoid Check(const ast::ReturnVoid&) const;
  ast::Return Check(const ast::Return&) const;
  std::vector<ast::Statement> Check(const std::vector<ast::Statement>&) const;
  ast::Statement result() { return std::move(result_.value()); }
 private:
  Context* context_;
  Scope* scope_;
  std::optional<ast::Statement> result_;
};

ast::Statement Check(Context* context, Scope* scope,
                     const ast::Statement& statement);

class TopLevel : public ast::TopLevelVisitor {
 public:
  TopLevel(Context* context, Scope* scope);
  using TopLevelVisitor::Visit;
  void Visit(const ast::DefineFunction& d) override { result_ = Check(d); }
  void Visit(const std::vector<ast::DefineFunction>& d) override {
    result_ = Check(d);
  }
  ast::DefineFunction Check(const ast::DefineFunction&) const;
  std::vector<ast::DefineFunction> Check(
      const std::vector<ast::DefineFunction>&) const;
  ast::TopLevel result() { return std::move(result_.value()); }
 private:
  Context* context_;
  Scope* scope_;
  std::optional<ast::TopLevel> result_;
};

ast::TopLevel Check(Context* context, Scope* scope,
                    const ast::TopLevel& top_level);

}  // namespace analysis
