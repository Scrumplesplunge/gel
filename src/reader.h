#pragma once

#include <string>

class Reader {
 public:
  class Location {
   public:
    std::string_view input_name() const;
    std::string_view line_contents() const;
    int line() const { return line_; }
    int column() const { return column_; }
   private:
    friend class Reader;
    Location(const Reader* reader, std::size_t offset, int line, int column)
        : reader_(reader), offset_(offset), line_(line), column_(column) {}
    const Reader* reader_ = nullptr;
    std::size_t offset_ = 0;
    int line_ = 0;
    int column_ = 0;
  };

  Reader(std::string input_name, std::string source)
      : input_name_(std::move(input_name)), source_(std::move(source)) {}

  Location location() const;
  std::string_view remaining() const;
  std::string_view prefix(std::size_t length) const;

  bool empty() const { return remaining().empty(); }
  char front() const { return remaining().front(); }
  auto begin() const { return source_.begin() + offset_; }
  auto end() const { return source_.end(); }

  bool starts_with(std::string_view prefix) const;
  void remove_prefix(std::size_t length);
  bool Consume(std::string_view prefix);

 private:
  std::string input_name_;
  std::string source_;
  std::size_t offset_ = 0;
  int line_ = 1, column_ = 1;
};

std::string FormatMessage(std::string_view type, Reader::Location location,
                          std::string_view message);
