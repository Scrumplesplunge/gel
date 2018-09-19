#include "ast.h"

namespace ast {

void Node::Visit(Visitor& visitor) const {
  if (model_) model_->Visit(visitor);
}

}  // namespace ast
