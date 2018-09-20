#include "reader.h"

#include <algorithm>

using namespace std::literals;

std::string_view Reader::Location::input_name() const {
  return reader_ ? reader_->input_name_ : ""sv;
}

std::string_view Reader::Location::line_contents() const {
  if (reader_ == nullptr) return "";
  const char* i = reader_->source_.data();
  const char* j = i + reader_->source_.length();
  const char* position = i + offset_;
  const char* line_start = position - (column_ - 1);
  const char* line_end = std::find(position, j, '\n');
  return std::string_view(line_start, line_end - line_start);
}

Reader::Location Reader::location() const {
  return Location{this, offset_, line_, column_};
}

std::string_view Reader::remaining() const {
  return std::string_view{source_}.substr(offset_);
}

std::string_view Reader::prefix(std::size_t length) const {
  return std::string_view{source_}.substr(offset_, length);
}

bool Reader::starts_with(std::string_view prefix) const {
  return std::string_view{source_}.substr(offset_, prefix.length()) == prefix;
}

void Reader::remove_prefix(std::size_t length) {
  for (char c : std::string_view{source_}.substr(offset_, length)) {
    if (c == '\n') {
      line_++;
      column_ = 1;
    } else {
      column_++;
    }
  }
  offset_ += length;
}

bool Reader::Consume(std::string_view prefix) {
  if (starts_with(prefix)) {
    remove_prefix(prefix.length());
    return true;
  } else {
    return false;
  }
}
