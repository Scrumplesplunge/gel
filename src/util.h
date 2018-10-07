#pragma once

#include <iostream>

namespace util {

struct Spaces { int count; };
std::ostream& operator<<(std::ostream& output, Spaces);

enum class Style {
  CLEAR,
  ERROR,
  WARNING,
  NOTE,
  DETAIL,
};
std::ostream& operator<<(std::ostream& output, Style);

template <typename T>
struct Format {
  Style style;
  const T& value;
};

template <typename T>
Format<T> Error(const T& value) {
  return Format<T>{Style::ERROR, value};
}

template <typename T>
Format<T> Warning(const T& value) {
  return Format<T>{Style::WARNING, value};
}

template <typename T>
Format<T> Note(const T& value) {
  return Format<T>{Style::NOTE, value};
}

template <typename T>
Format<T> Detail(const T& value) {
  return Format<T>{Style::DETAIL, value};
}

template <typename T>
std::ostream& operator<<(std::ostream& output, Format<T> f) {
  return output << f.style << f.value << Style::CLEAR;
}

}  // namespace util
