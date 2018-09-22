#pragma once

#include <memory>
#include <type_traits>

namespace visitable {

template <typename Visitor>
class Node {
 public:
  // requires visitor->visit(T) to be valid.
  template <typename T, typename = std::enable_if_t<
                            !std::is_same_v<Node, std::decay_t<T>>>>
  Node(T&& value) : model_(Adapt(std::forward<T>(value))) {}

  void Visit(Visitor& visitor) const;

 private:
  struct Model;

  template <typename T>
  static std::shared_ptr<const Model> Adapt(T&& value);

  std::shared_ptr<const Model> model_;
};

}  // namespace visitable

#include "visitable.inl.h"
