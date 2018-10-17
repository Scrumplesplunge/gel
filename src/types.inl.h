#pragma once

#include "types.h"

namespace types {

template <typename F>
void visit_children<F>::operator()(const Void&) {}

template <typename F>
void visit_children<F>::operator()(const Function& f) {
  f.return_type.visit(functor_);
  for (const auto& parameter : f.parameters) parameter.visit(functor_);
}

template <typename F>
void visit_children<F>::operator()(const Primitive&) {}

template <typename F>
void visit_children<F>::operator()(const Array& a) {
  a.element_type.visit(functor_);
}

}  // namespace types
