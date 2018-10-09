#pragma once

#include "value.h"

template <typename T>
value<T>::value() : value_(std::make_unique<T>()) {}

template <typename T>
template <typename... Args, typename>
value<T>::value(Args&&... args)
    : value_(std::make_unique<T>(std::forward<Args>(args)...)) {}

template <typename T>
value<T>::value(const T& value) : value_(std::make_unique<T>(value)) {}

template <typename T>
value<T>::value(T&& value)
    : value_(std::make_unique<T>(std::move(value))) {}

template <typename T>
value<T>::value(const value& other)
    : value_(std::make_unique<T>(*other.value_)) {}

template <typename T>
value<T>::value(value&& other)
    : value_(std::make_unique<T>(std::move(*other.value_))) {}

template <typename T>
value<T>& value<T>::operator=(const value& other) {
  *value_ = *other.value_;
  return *this;
}

template <typename T>
value<T>& value<T>::operator=(value&& other) {
  *value_ = std::move(*other.value_);
  return *this;
}

template <typename T>
void value<T>::reset() {
  value_ = std::make_unique<T>();
}

template <typename T>
T* value<T>::get() {
  return value_.get();
}

template <typename T>
const T* value<T>::get() const {
  return value_.get();
}

template <typename T>
T& value<T>::operator*() { return *value_; }

template <typename T>
const T& value<T>::operator*() const { return *value_; }

template <typename T>
T* value<T>::operator->() { return value_.get(); }

template <typename T>
const T* value<T>::operator->() const { return value_.get(); }

template <typename T, typename U>
bool operator==(const value<T>& left, const value<U>& right) {
  return *left == *right;
}

template <typename T, typename U>
bool operator!=(const value<T>& left, const value<U>& right) {
  return *left != *right;
}

template <typename T, typename U>
bool operator<=(const value<T>& left, const value<U>& right) {
  return *left <= *right;
}

template <typename T, typename U>
bool operator<(const value<T>& left, const value<U>& right) {
  return *left < *right;
}

template <typename T, typename U>
bool operator>=(const value<T>& left, const value<U>& right) {
  return *left >= *right;
}

template <typename T, typename U>
bool operator>(const value<T>& left, const value<U>& right) {
  return *left > *right;
}
