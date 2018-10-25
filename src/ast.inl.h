#pragma once

#include "ast.h"

namespace ast {

template <typename Metadata>
const typename Metadata::Expression& Ast<Metadata>::GetMeta(
    const Expression& expression) {
  return expression.visit(
      [](const auto& x) -> const typename Metadata::Expression& {
        return x;
      });
}

template <typename Metadata>
const typename Metadata::Statement& Ast<Metadata>::GetMeta(
    const Statement& statement) {
  return statement.visit(
      [](const auto& x) -> const typename Metadata::Statement& {
        return x;
      });
}

template <typename Metadata>
const typename Metadata::TopLevel& Ast<Metadata>::GetMeta(
    const TopLevel& top_level) {
  return top_level.visit(
      [](const auto& x) -> const typename Metadata::TopLevel& {
        return x;
      });
}

}  // namespace ast
