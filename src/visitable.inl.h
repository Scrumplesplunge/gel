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
      // Oh boy, this is possibly my favourite crazy C++ thing that I've come
      // across in all my time using the language. If I don't explicitly force
      // the Visit function to perfectly match the signature here by creating
      // a member function pointer, we might resolve an overload which takes
      // Node<Visitor>. If this can happen, suddenly every single type will
      // successfully assign into a Node<Visitor> and compile successfully, but
      // will result in infinite recursion the moment Visit is called.
      using VisitFunction = void (Visitor::*)(const T&);
      constexpr VisitFunction visit = &Visitor::Visit;
      (visitor.*visit)(value);
    }
    T value;
  };
  return std::make_shared<Adaptor>(std::forward<T>(value));
}

}  // namespace visitable
