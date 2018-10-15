#include "types.h"

namespace types {

bool IsValueType(const Type& type) {
  return type.is<Primitive>() || type.is<Array>();
}

// All Void values were created equal.
bool operator==(Void, Void) { return true; }
bool operator<(Void, Void) { return false; }

std::ostream& operator<<(std::ostream& output, Void) {
  return output << "void";
}

// Function types are equal if they have the same return type, the same number
// of parameters, and the same type for each parameter. The names of the
// parameters is not important.
bool operator==(const Function& left, const Function& right) {
  return left.return_type == right.return_type &&
         left.parameters == right.parameters;
}

// Order lexicographically with respect to (return_type, param1, param2, ...).
bool operator<(const Function& left, const Function& right) {
  if (left.return_type < right.return_type) return true;
  if (left.return_type != right.return_type) return false;
  return left.parameters < right.parameters;
}

std::ostream& operator<<(std::ostream& output, const Function& value) {
  output << "function (";
  bool first = true;
  for (const auto& parameter : value.parameters) {
    if (first) {
      first = false;
    } else {
      output << ", ";
    }
    output << parameter;
  }
  output << ") -> " << value.return_type;
  return output;
}

std::ostream& operator<<(std::ostream& output, Primitive value) {
  switch (value) {
    case Primitive::BOOLEAN:
      return output << "boolean";
    case Primitive::INTEGER:
      return output << "integer";
  }
  throw std::logic_error("Bad primitive value.");
}

// Array types are equal types if and only if they have the same element type.
bool operator==(const Array& left, const Array& right) {
  return left.element_type == right.element_type;
}

bool operator<(const Array& left, const Array& right) {
  return left.element_type < right.element_type;
}

std::ostream& operator<<(std::ostream& output, const Array& value) {
  return output << "[" << value.element_type << "]";
}

}  // namespace types
