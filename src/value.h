#pragma once

#include <memory>

template <typename T>
class value {
 public:
  value();
  template <typename... Args,
            typename = std::enable_if_t<
                !(sizeof...(Args) == 1 &&
                  std::is_same_v<std::decay_t<Args>..., value>)>>
  value(Args&&... args);
  explicit value(const T& value);
  explicit value(T&& value);
  value(const value& other);
  value(value&& other);
  value& operator=(const value& other);
  value& operator=(value&& other);
  void reset();
  T* get();
  const T* get() const;
  T& operator*();
  const T& operator*() const;
  T* operator->();
  const T* operator->() const;
 private:
  std::unique_ptr<T> value_;  // Never nullptr.
};

template <typename T, typename U>
bool operator==(const value<T>& left, const value<U>& right);
template <typename T, typename U>
bool operator!=(const value<T>& left, const value<U>& right);
template <typename T, typename U>
bool operator<=(const value<T>& left, const value<U>& right);
template <typename T, typename U>
bool operator<(const value<T>& left, const value<U>& right);
template <typename T, typename U>
bool operator>=(const value<T>& left, const value<U>& right);
template <typename T, typename U>
bool operator>(const value<T>& left, const value<U>& right);

#include "value.inl.h"
