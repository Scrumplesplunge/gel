#pragma once

#include "util.h"
#include "value.h"

#include <variant>

template <typename... Children>
class one_of;

template <typename... Children>
bool operator==(const one_of<Children...>&, const one_of<Children...>&);
template <typename... Children>
bool operator!=(const one_of<Children...>&, const one_of<Children...>&);
template <typename... Children>
bool operator<=(const one_of<Children...>&, const one_of<Children...>&);
template <typename... Children>
bool operator<(const one_of<Children...>&, const one_of<Children...>&);
template <typename... Children>
bool operator>=(const one_of<Children...>&, const one_of<Children...>&);
template <typename... Children>
bool operator>(const one_of<Children...>&, const one_of<Children...>&);

template <typename... Children>
class one_of {
 public:
  template <typename T,
            typename = std::enable_if_t<util::is_one_of_v<T, Children...>>>
  one_of(T&& value);

  template <typename T>
  bool is() const;

  template <typename T>
  T* get_if();
  template <typename T>
  const T* get_if() const;

  template <typename F>
  auto visit(F&& functor) const
      -> decltype(std::visit(std::forward<F>(functor),
                             std::declval<const std::variant<Children...>>()));

 private:
  // Comparison between different instances of the same one_of type.
  friend bool operator== <>(const one_of& left, const one_of& right);
  friend bool operator!= <>(const one_of& left, const one_of& right);
  friend bool operator<= <>(const one_of& left, const one_of& right);
  friend bool operator< <>(const one_of& left, const one_of& right);
  friend bool operator>= <>(const one_of& left, const one_of& right);
  friend bool operator> <>(const one_of& left, const one_of& right);

  value<std::variant<Children...>> value_;
};

// Comparison against a component of the one_of type, one_of on the left.
template <typename... Children, typename T,
          typename = std::enable_if_t<util::is_one_of_v<T, Children...>>>
bool operator==(const one_of<Children...>& left, const T& right);

template <typename... Children, typename T,
          typename = std::enable_if_t<util::is_one_of_v<T, Children...>>>
bool operator!=(const one_of<Children...>& left, const T& right);

// Comparison against a component of the one_of type, one_of on the right.
template <typename T, typename... Children,
          typename = std::enable_if_t<util::is_one_of_v<T, Children...>>>
bool operator==(const T& left, const one_of<Children...>& right);

template <typename T, typename... Children,
          typename = std::enable_if_t<util::is_one_of_v<T, Children...>>>
bool operator!=(const T& left, const one_of<Children...>& right);

#include "one_of.inl.h"
