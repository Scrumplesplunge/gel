#pragma once

namespace ast {

struct Node::Model {
  virtual ~Model() = default;
  virtual void Visit(Visitor& visitor) const = 0;
};

template <typename T>
std::shared_ptr<const Node::Model> Node::Adapt(T&& value) {
  struct Adaptor final : Model {
    Adaptor(T&& value) : value(std::move(value)) {}
    void Visit(Visitor& visitor) const override {
      visitor.Visit(value);
    }
    T value;
  };
  return std::make_shared<Adaptor>(std::forward<T>(value));
}

}  // namespace ast
