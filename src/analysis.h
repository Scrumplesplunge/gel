#pragma once

#include "ast.h"
#include "reader.h"

#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <vector>

namespace analysis {

struct GlobalContext;

class MessageBuilder {
 public:
  MessageBuilder(GlobalContext* context, Message::Type type,
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
  GlobalContext* const context_;
  const Message::Type type_;
  const Reader::Location location_;
  std::ostringstream text_;
};

struct Operators {
  using ArithmeticKey = std::tuple<ast::Arithmetic::Operation, types::Type>;
  std::set<ArithmeticKey> arithmetic;
  std::set<types::Type> equality_comparable;
  std::set<types::Type> ordered;
};

struct GlobalContext {
  const Operators operators;
  std::vector<Message> diagnostics;
  MessageBuilder Error(Reader::Location location);
  MessageBuilder Warning(Reader::Location location);
  MessageBuilder Note(Reader::Location location);
};

struct FunctionContext {
  GlobalContext* global_context;
  ast::Identifier this_function;
  types::Function type;
};

class Scope {
 public:
  struct Entry {
    Reader::Location location;
    std::optional<types::Type> type;
  };
  explicit Scope(Scope* parent = nullptr) : parent_(parent) {}

  bool Define(std::string name, Entry entry);
  const Entry* Lookup(std::string_view name) const;

 private:
  const Scope* parent_ = nullptr;
  std::map<std::string, Entry, std::less<>> bindings_;
};

ast::Identifier Check(const ast::Identifier&, FunctionContext*, const Scope*);
ast::Boolean Check(const ast::Boolean&, FunctionContext*, const Scope*);
ast::Integer Check(const ast::Integer&, FunctionContext*, const Scope*);
ast::ArrayLiteral Check(const ast::ArrayLiteral&, FunctionContext*,
                        const Scope*);
ast::Arithmetic Check(const ast::Arithmetic&, FunctionContext*, const Scope*);
ast::Compare Check(const ast::Compare&, FunctionContext*, const Scope*);
ast::Logical Check(const ast::Logical&, FunctionContext*, const Scope*);
ast::FunctionCall Check(const ast::FunctionCall&, FunctionContext*,
                        const Scope*);
ast::LogicalNot Check(const ast::LogicalNot&, FunctionContext*, const Scope*);
ast::Expression Check(const ast::Expression&, FunctionContext*, const Scope*);

ast::DefineVariable Check(const ast::DefineVariable&, FunctionContext*, Scope*);
ast::Assign Check(const ast::Assign&, FunctionContext*, Scope*);
ast::DoFunction Check(const ast::DoFunction&, FunctionContext*, Scope*);
ast::If Check(const ast::If&, FunctionContext*, Scope*);
ast::While Check(const ast::While&, FunctionContext*, Scope*);
ast::ReturnVoid Check(const ast::ReturnVoid&, FunctionContext*, Scope*);
ast::Return Check(const ast::Return&, FunctionContext*, Scope*);
std::vector<ast::Statement> Check(const std::vector<ast::Statement>&,
                                  FunctionContext*, Scope*);
ast::Statement Check(const ast::Statement&, FunctionContext*, Scope*);

ast::DefineFunction Check(const ast::DefineFunction&, GlobalContext*, Scope*);
std::vector<ast::DefineFunction> Check(const std::vector<ast::DefineFunction>&,
                                       GlobalContext*, Scope*);
ast::TopLevel Check(const ast::TopLevel&, GlobalContext*, Scope*);

}  // namespace analysis
