#include "reader.h"

#include <algorithm>
#include <sstream>

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

std::ostream& operator<<(std::ostream& output, Reader::Location location) {
  return output << location.input_name() << ":" << location.line() << ":"
                << location.column();
}

std::ostream& operator<<(std::ostream& output, Message::Type type) {
  constexpr char kRed[] = "\x1b[31;1m";
  constexpr char kYellow[] = "\x1b[33m";
  constexpr char kCyan[] = "\x1b[36m";
  constexpr char kReset[] = "\x1b[0m";
  switch (type) {
    case Message::Type::ERROR:
      return output << kRed << "error" << kReset;
    case Message::Type::WARNING:
      return output << kYellow << "warning" << kReset;
    case Message::Type::NOTE:
      return output << kCyan << "note" << kReset;
  }
}

std::ostream& operator<<(std::ostream& output, const Message& message) {
  constexpr int kSourceIndent = 2;
  return output << message.location << ": " << message.type << ": "
                << message.text << "\n\n"
                << std::string(kSourceIndent, ' ')
                << message.location.line_contents() << "\n"
                << std::string(kSourceIndent + message.location.column() - 1,
                               ' ')
                << "^\n";
}

Message Message::Error(Reader::Location location, std::string message) {
  return Message{Message::Type::ERROR, location, std::move(message)};
}

Message Message::Warning(Reader::Location location, std::string message) {
  return Message{Message::Type::WARNING, location, std::move(message)};
}

Message Message::Note(Reader::Location location, std::string message) {
  return Message{Message::Type::NOTE, location, std::move(message)};
}

CompileError::CompileError(Reader::Location location, std::string_view text)
    : message_(Message::Error(location, std::string{text})) {
  std::ostringstream output;
  output << message_;
  formatted_ = output.str();
}
