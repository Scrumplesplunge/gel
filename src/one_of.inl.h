#pragma once

#include "one_of.h"

// Comparison between different instances of the same one_of type.
template <typename... Children>
bool operator==(const one_of<Children...>& left,
                const one_of<Children...>& right) {
  return *left.value_ == *right.value_;
}

template <typename... Children>
bool operator!=(const one_of<Children...>& left,
                const one_of<Children...>& right) {
  return *left.value_ != *right.value_;
}

template <typename... Children>
bool operator<=(const one_of<Children...>& left,
                const one_of<Children...>& right) {
  return *left.value_ <= *right.value_;
}

template <typename... Children>
bool operator<(const one_of<Children...>& left,
               const one_of<Children...>& right) {
  return *left.value_ < *right.value_;
}

template <typename... Children>
bool operator>=(const one_of<Children...>& left,
                const one_of<Children...>& right) {
  return *left.value_ >= *right.value_;
}

template <typename... Children>
bool operator>(const one_of<Children...>& left,
               const one_of<Children...>& right) {
  return *left.value_ > *right.value_;
}

template <typename... Children>
template <typename T, typename>
one_of<Children...>::one_of(T&& value)
    : value_(std::forward<T>(value)) {}

template <typename... Children>
template <typename T>
bool one_of<Children...>::is() const {
  return std::holds_alternative<T>(*value_);
}

template <typename... Children>
template <typename T>
T* one_of<Children...>::get_if() {
  return std::get_if<T>(value_.get());
}

template <typename... Children>
template <typename T>
const T* one_of<Children...>::get_if() const {
  return std::get_if<T>(value_.get());
}

template <typename... Children>
template <typename F>
auto one_of<Children...>::visit(F&& functor) const {
  return std::visit(std::forward<F>(functor), *value_);
}

template <typename... Children>
template <typename F>
auto one_of<Children...>::visit(F&& functor) {
  return std::visit(std::forward<F>(functor), *value_);
}

// Comparison against a component of the one_of type, one_of on the left.
template <typename... Children, typename T, typename>
bool operator==(const one_of<Children...>& left, const T& right) {
  return left.template is<T>() && *left.template get_if<T>() == right;
}

template <typename... Children, typename T, typename>
bool operator!=(const one_of<Children...>& left, const T& right) {
  return !(left == right);
}

// Comparison against a component of the one_of type, one_of on the right.
template <typename T, typename... Children, typename>
bool operator==(const T& left, const one_of<Children...>& right) {
  return right == left;
}

template <typename T, typename... Children, typename>
bool operator!=(const T& left, const one_of<Children...>& right) {
  return right != left;
}
