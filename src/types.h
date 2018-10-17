#pragma once

#include "one_of.h"

#include <vector>

namespace types {

struct Void;
struct Function;
enum class Primitive;
struct Array;
using Type = one_of<Void, Function, Primitive, Array>;

bool IsValueType(const Type& type);

// Empty type.
struct Void {};

bool operator==(Void left, Void right);
bool operator<(Void left, Void right);
std::ostream& operator<<(std::ostream& output, Void value);

// Functions are not values, so they cannot be assigned, copied, etc.
struct Function {
  Type return_type;
  std::vector<Type> parameters;
};

bool operator==(const Function& left, const Function& right);
bool operator<(const Function& left, const Function& right);
std::ostream& operator<<(std::ostream& output, const Function& value);

// Primitive (scalar) types.
enum class Primitive {
  BOOLEAN,
  INTEGER,
};

std::ostream& operator<<(std::ostream& output, Primitive value);

// Unlike C, gel arrays are proper value types.
struct Array { Type element_type; };

bool operator==(const Array& left, const Array& right);
bool operator<(const Array& left, const Array& right);
std::ostream& operator<<(std::ostream& output, const Array& value);

// All types are comparable, but the comparisons are implemented in terms of ==
// and < only; the rest are derived from these.
template <typename T>
bool operator!=(const T& left, const T& right) { return !(left == right); }

template <typename T>
bool operator<=(const T& left, const T& right) { return !(right < left); }

template <typename T>
bool operator>=(const T& left, const T& right) { return !(left < right); }

template <typename T>
bool operator>(const T& left, const T& right) { return right < left; }

template <typename F>
class visit_children {
 public:
  visit_children(F&& functor) : functor_(functor) {}
  void operator()(const Void& v);
  void operator()(const Function&);
  void operator()(const Primitive&);
  void operator()(const Array&);
 private:
  F functor_;
};
template <typename F> visit_children(F) -> visit_children<F>;

}  // namespace types

#include "types.inl.h"
