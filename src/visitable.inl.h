#pragma once

namespace visitable {

template <typename Visitor>
struct Node<Visitor>::Model {
  virtual ~Model() = default;
  virtual void Visit(Visitor& visitor) const = 0;
};

template <typename Visitor>
void Node<Visitor>::Visit(Visitor& visitor) const {
  if (model_) model_->Visit(visitor);
}

template <typename Visitor>
template <typename T>
std::shared_ptr<const typename Node<Visitor>::Model> Node<Visitor>::Adapt(
    T&& value) {
  struct Adaptor final : Model {
    Adaptor(T&& value) : value(std::move(value)) {}
    void Visit(Visitor& visitor) const override {
      visitor.Visit(value);
    }
    T value;
  };
  return std::make_shared<Adaptor>(std::forward<T>(value));
}

}  // namespace visitable
