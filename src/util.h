#pragma once

#include <iostream>
#include <string_view>

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

template <typename T, typename... Options>
struct is_one_of {
  static constexpr bool value =
      (std::is_same_v<std::decay_t<T>, Options> || ...);
};

template <typename T, typename... Options>
constexpr bool is_one_of_v = is_one_of<T, Options...>::value;

struct Substitution {
  std::string_view variable;
  std::string_view value;
};

void substitute(std::ostream& output, std::string_view format,
                const std::initializer_list<Substitution>& substitutions);

}  // namespace util
