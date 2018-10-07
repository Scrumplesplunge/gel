#include "util.h"

#include <algorithm>
#include <iterator>

namespace util {

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

}  // namespace util
