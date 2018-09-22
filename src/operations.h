#pragma once

#include "visitable.h"

#include <cstdint>
#include <vector>

namespace op {

struct Visitor;
using Node = visitable::Node<Visitor>;
using Sequence = std::vector<Node>;

// Push an integer.
struct Integer { std::int64_t value; };

// Push an address relative to the stack frame.
struct Frame { int offset; };

// Pop an address, push the value stored at that address.
struct Load {};

// Pop a value, pop an address, store the value to the address.
struct Store {};

// Binary operations that consume two stack entries and produce one.
struct Add {};
struct Subtract {};
struct Multiply {};
struct Divide {};

struct Visitor {
  void Visit(const Node& node) { node.Visit(*this); }
  virtual void Visit(const Sequence&) = 0;
  virtual void Visit(const Integer&) = 0;
  virtual void Visit(const Frame&) = 0;
  virtual void Visit(const Load&) = 0;
  virtual void Visit(const Store&) = 0;
  virtual void Visit(const Add&) = 0;
  virtual void Visit(const Subtract&) = 0;
  virtual void Visit(const Multiply&) = 0;
  virtual void Visit(const Divide&) = 0;
};

}  // namespace op

extern template class visitable::Node<op::Visitor>;
