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
  MessageBuilder& operator<<(T&& value) {
    text_ << value;
    return *this;
  }

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

ast::DefineVariable Check(const ast::DefineVariable&, Context*, Scope*);
ast::Assign Check(const ast::Assign&, Context*, Scope*);
ast::DoFunction Check(const ast::DoFunction&, Context*, Scope*);
ast::If Check(const ast::If&, Context*, Scope*);
ast::While Check(const ast::While&, Context*, Scope*);
ast::ReturnVoid Check(const ast::ReturnVoid&, Context*, Scope*);
ast::Return Check(const ast::Return&, Context*, Scope*);
std::vector<ast::Statement> Check(const std::vector<ast::Statement>&, Context*,
                                  Scope*);
ast::Statement Check(const ast::Statement&, Context*, Scope*);

ast::DefineFunction Check(const ast::DefineFunction&, Context*, Scope*);
std::vector<ast::DefineFunction> Check(const std::vector<ast::DefineFunction>&,
                                       Context*, Scope*);
ast::TopLevel Check(const ast::TopLevel&, Context*, Scope*);

}  // namespace analysis
