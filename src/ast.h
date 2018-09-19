#pragma once

#include <memory>
#include <type_traits>

namespace ast {

struct Visitor;

class Node {
 public:
  // requires visitor->visit(T) to be valid.
  template <typename T,
            typename = std::enable_if_t<!std::is_same_v<Node, std::decay_t<T>>>>
  Node(T&& value) : model_(Adapt(std::forward<T>(value))) {}

  void Visit(Visitor& visitor) const;

 private:
  struct Model;

  template <typename T>
  static std::shared_ptr<const Model> Adapt(T&& value);

  std::shared_ptr<const Model> model_;
};

struct Identifier { std::string_view name; };
struct Integer { std::int64_t value; };
struct Add { Node left, right; };
struct Subtract { Node left, right; };
struct Multiply { Node left, right; };
struct Divide { Node left, right; };

struct Visitor {
  virtual void Visit(const Identifier&) = 0;
  virtual void Visit(const Integer&) = 0;
  virtual void Visit(const Add&) = 0;
  virtual void Visit(const Subtract&) = 0;
  virtual void Visit(const Multiply&) = 0;
  virtual void Visit(const Divide&) = 0;
};

}  // namespace ast

#include "ast.inl.h"
