#include "util.h"

#include <algorithm>
#include <iterator>

namespace util {
namespace internal {

enum class Error {
  INCOMPLETE_SUBSTITUTION,
  UNDEFINED_VARIABLE,
};

class format_error : std::exception {
 public:
  format_error(Error type) : type_(type) {}
  const char* what() const noexcept override;
 private:
  const Error type_;
};

const char* format_error::what() const noexcept {
  switch (type_) {
    case Error::INCOMPLETE_SUBSTITUTION:
      return "Incomplete substitution in format string.";
    case Error::UNDEFINED_VARIABLE:
      return "Variable in format string is not provided in substitution "
             "container.";
  }
}

}  // namespace internal

std::ostream& operator<<(std::ostream& output, Spaces spaces) {
  std::fill_n(std::ostream_iterator<char>(output), spaces.count, ' ');
  return output;
}

std::ostream& operator<<(std::ostream& output, Style style) {
  switch (style) {
    case Style::CLEAR:
      return output << "\x1b[0m";
    case Style::ERROR:
      return output << "\x1b[31;1m";
    case Style::WARNING:
      return output << "\x1b[33m";
    case Style::NOTE:
      return output << "\x1b[36m";
    case Style::DETAIL:
      return output << "\x1b[37;1m";
  }
}

void substitute(std::ostream& output, std::string_view format,
                const std::initializer_list<Substitution>& substitutions) {
  const char* i = format.data();
  const char* const end = i + format.length();
  while (true) {
    const char* j = std::find(i, end, '$');
    output.write(i, j - i);
    if (j == end) break;
    if (end - j < 2)
      throw internal::format_error(internal::Error::INCOMPLETE_SUBSTITUTION);
    if (j[1] == '$') {
      // Literal '$'.
      output.put('$');
      i = j + 2;
    } else if (j[1] == '{') {
      // Substitution ${foo}.
      const char* start = j + 2;
      const char* k = std::find(start, end, '}');
      if (k == end)
        throw internal::format_error(internal::Error::INCOMPLETE_SUBSTITUTION);
      std::string_view variable{
          start, static_cast<std::string_view::size_type>(k - start)};
      auto entry =
          std::find_if(std::begin(substitutions), std::end(substitutions),
                       [&](const auto& x) { return x.variable == variable; });
      if (entry == std::end(substitutions))
        throw internal::format_error(internal::Error::UNDEFINED_VARIABLE);
      output << entry->value;
      i = k + 1;
    }
  }
}

}  // namespace util
