#pragma once

namespace ast {

template <typename Visitor>
struct AstNode<Visitor>::Model {
  virtual ~Model() = default;
  virtual void Visit(Visitor& visitor) const = 0;
};

template <typename Visitor>
void AstNode<Visitor>::Visit(Visitor& visitor) const {
  if (model_) model_->Visit(visitor);
}

template <typename Visitor>
template <typename T>
std::shared_ptr<const typename AstNode<Visitor>::Model> AstNode<Visitor>::Adapt(
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

}  // namespace ast
