#include "ast.h"
#include "code_generation.h"
#include "operations.h"
#include "parser.h"
#include "pretty.h"
#include "reader.h"

#include <unordered_map>
#include <vector>

bool prompt(std::string_view text, std::string& line) {
  std::cout << text;
  return bool{std::getline(std::cin, line)};
}

// Assembly

class Assembler : op::Visitor {
 public:
  struct Result {
    std::unordered_map<std::string, std::size_t> label_positions;
    op::Sequence operations;
  };

  using Visitor::Visit;
  void Visit(const op::Sequence&) override;
  void Visit(const op::Integer&) override;
  void Visit(const op::Frame&) override;
  void Visit(const op::Adjust&) override;
  void Visit(const op::Load&) override;
  void Visit(const op::Store&) override;
  void Visit(const op::Add&) override;
  void Visit(const op::Subtract&) override;
  void Visit(const op::Multiply&) override;
  void Visit(const op::Divide&) override;
  void Visit(const op::CompareEq&) override;
  void Visit(const op::CompareNe&) override;
  void Visit(const op::CompareLe&) override;
  void Visit(const op::CompareLt&) override;
  void Visit(const op::CompareGe&) override;
  void Visit(const op::CompareGt&) override;
  void Visit(const op::Label&) override;
  void Visit(const op::Jump&) override;
  void Visit(const op::JumpIfZero&) override;
  void Visit(const op::JumpIfNonZero&) override;
  void Visit(const op::PrintDecimal&) override;

  Result ConsumeResult();

 private:
  Result result_;
};

// Implementation

void Assembler::Visit(const op::Sequence& sequence) {
  for (const auto& operation : sequence) Visit(operation);
}

void Assembler::Visit(const op::Integer& integer) {
  result_.operations.push_back(integer);
}

void Assembler::Visit(const op::Frame& frame) {
  result_.operations.push_back(frame);
}

void Assembler::Visit(const op::Adjust& adjust) {
  result_.operations.push_back(adjust);
}

void Assembler::Visit(const op::Load& load) {
  result_.operations.push_back(load);
}

void Assembler::Visit(const op::Store& store) {
  result_.operations.push_back(store);
}

void Assembler::Visit(const op::Add& add) {
  result_.operations.push_back(add);
}

void Assembler::Visit(const op::Subtract& subtract) {
  result_.operations.push_back(subtract);
}

void Assembler::Visit(const op::Multiply& multiply) {
  result_.operations.push_back(multiply);
}

void Assembler::Visit(const op::Divide& divide) {
  result_.operations.push_back(divide);
}

void Assembler::Visit(const op::CompareEq& compare) {
  result_.operations.push_back(compare);
}

void Assembler::Visit(const op::CompareNe& compare) {
  result_.operations.push_back(compare);
}

void Assembler::Visit(const op::CompareLe& compare) {
  result_.operations.push_back(compare);
}

void Assembler::Visit(const op::CompareLt& compare) {
  result_.operations.push_back(compare);
}

void Assembler::Visit(const op::CompareGe& compare) {
  result_.operations.push_back(compare);
}

void Assembler::Visit(const op::CompareGt& compare) {
  result_.operations.push_back(compare);
}

void Assembler::Visit(const op::Label& label) {
  result_.label_positions.emplace(label.name, result_.operations.size());
}

void Assembler::Visit(const op::Jump& jump) {
  result_.operations.push_back(jump);
}

void Assembler::Visit(const op::JumpIfZero& jump) {
  result_.operations.push_back(jump);
}

void Assembler::Visit(const op::JumpIfNonZero& jump) {
  result_.operations.push_back(jump);
}

void Assembler::Visit(const op::PrintDecimal& print) {
  result_.operations.push_back(print);
}

Assembler::Result Assembler::ConsumeResult() { return std::move(result_); }

// Runtime

class Runtime : private op::Visitor {
 public:
  Runtime(Assembler::Result result)
      : labels_(std::move(result.label_positions)),
        code_(std::move(result.operations)) {}

  void Run();

 private:
  void Push(std::int64_t value);
  std::int64_t Pop();

  using Visitor::Visit;
  void Visit(const op::Sequence&) override;
  void Visit(const op::Integer&) override;
  void Visit(const op::Frame&) override;
  void Visit(const op::Adjust&) override;
  void Visit(const op::Load&) override;
  void Visit(const op::Store&) override;
  void Visit(const op::Add&) override;
  void Visit(const op::Subtract&) override;
  void Visit(const op::Multiply&) override;
  void Visit(const op::Divide&) override;
  void Visit(const op::CompareEq&) override;
  void Visit(const op::CompareNe&) override;
  void Visit(const op::CompareLe&) override;
  void Visit(const op::CompareLt&) override;
  void Visit(const op::CompareGe&) override;
  void Visit(const op::CompareGt&) override;
  void Visit(const op::Label&) override;
  void Visit(const op::Jump&) override;
  void Visit(const op::JumpIfZero&) override;
  void Visit(const op::JumpIfNonZero&) override;
  void Visit(const op::PrintDecimal&) override;

  static constexpr auto kMemorySize = 1024;

  std::unordered_map<std::string, std::size_t> labels_;
  op::Sequence code_;

  std::array<std::int64_t, kMemorySize> memory_ = {};
  std::int64_t stack_index_ = kMemorySize;
  std::int64_t frame_index_ = kMemorySize;
  std::size_t instruction_index_ = 0;
};

// Implementation

void Runtime::Run() {
  stack_index_ = kMemorySize;
  frame_index_ = kMemorySize;
  instruction_index_ = 0;
  while (instruction_index_ < code_.size()) {
    std::cout << "@" << instruction_index_ << ": ";
    std::size_t i = instruction_index_++;
    pretty::OperationPrinter printer{std::cout};
    printer.Visit(code_[i]);
    Visit(code_[i]);
  }
  std::cout << "Finished at @" << instruction_index_ << "\n";
}

void Runtime::Push(std::int64_t value) {
  if (stack_index_ == 0) throw std::runtime_error("Stack overflow.");
  memory_[--stack_index_] = value;
}

std::int64_t Runtime::Pop() {
  if (stack_index_ + 1 == kMemorySize)
    throw std::runtime_error("Stack underflow.");
  return memory_[stack_index_++];
}

void Runtime::Visit(const op::Sequence&) {
  throw std::logic_error(
      "There should be no sequence nodes in assembler output.");
}

void Runtime::Visit(const op::Integer& integer) {
  Push(integer.value);
}

void Runtime::Visit(const op::Frame& frame) {
  Push(frame_index_ + frame.offset);
}

void Runtime::Visit(const op::Adjust& adjust) {
  stack_index_ += adjust.size;
  if (stack_index_ < 0) throw std::runtime_error("Stack overflow.");
  if (stack_index_ > kMemorySize) throw std::runtime_error("Stack underflow.");
}

void Runtime::Visit(const op::Load&) {
  Push(memory_[Pop()]);
}

void Runtime::Visit(const op::Store&) {
  std::int64_t value = Pop();
  std::int64_t address = Pop();
  if (address < 0 || address >= kMemorySize)
    throw std::runtime_error("Address out of bounds.");
  memory_[address] = value;
}

void Runtime::Visit(const op::Add&) {
  std::int64_t b = Pop();
  std::int64_t a = Pop();
  Push(a + b);
}

void Runtime::Visit(const op::Subtract&) {
  std::int64_t b = Pop();
  std::int64_t a = Pop();
  Push(a - b);
}

void Runtime::Visit(const op::Multiply&) {
  std::int64_t b = Pop();
  std::int64_t a = Pop();
  Push(a * b);
}

void Runtime::Visit(const op::Divide&) {
  std::int64_t b = Pop();
  std::int64_t a = Pop();
  Push(a / b);
}

void Runtime::Visit(const op::CompareEq&) {
  std::int64_t b = Pop();
  std::int64_t a = Pop();
  Push(a == b);
}

void Runtime::Visit(const op::CompareNe&) {
  std::int64_t b = Pop();
  std::int64_t a = Pop();
  Push(a != b);
}

void Runtime::Visit(const op::CompareLe&) {
  std::int64_t b = Pop();
  std::int64_t a = Pop();
  Push(a <= b);
}

void Runtime::Visit(const op::CompareLt&) {
  std::int64_t b = Pop();
  std::int64_t a = Pop();
  Push(a < b);
}

void Runtime::Visit(const op::CompareGe&) {
  std::int64_t b = Pop();
  std::int64_t a = Pop();
  Push(a >= b);
}

void Runtime::Visit(const op::CompareGt&) {
  std::int64_t b = Pop();
  std::int64_t a = Pop();
  Push(a > b);
}

void Runtime::Visit(const op::Label&) {
  throw std::logic_error("Assembler output should have no label definitions.");
}

void Runtime::Visit(const op::Jump& jump) {
  instruction_index_ = labels_.at(jump.label);
}

void Runtime::Visit(const op::JumpIfZero& jump) {
  if (stack_index_ == kMemorySize) throw std::logic_error("Stack underflow.");
  if (memory_[stack_index_] == 0) {
    instruction_index_ = labels_.at(jump.label);
  } else {
    stack_index_++;
  }
}

void Runtime::Visit(const op::JumpIfNonZero& jump) {
  if (stack_index_ == kMemorySize) throw std::logic_error("Stack underflow.");
  if (memory_[stack_index_] != 0) {
    instruction_index_ = labels_.at(jump.label);
  } else {
    stack_index_++;
  }
}

void Runtime::Visit(const op::PrintDecimal&) {
  std::cout << " => " << Pop() << "\n";
}

int main() {
  std::string input{std::istreambuf_iterator<char>{std::cin}, {}};
  Reader reader{"stdin", input};
  Parser parser{reader};
  try {
    codegen::Context context;
    codegen::LexicalScope scope;
    auto statement = parser.ParseStatement(0);
    parser.ConsumeNewline();
    parser.CheckEnd();
    codegen::Statement codegen(&context, &scope);
    codegen.Visit(statement);
    Assembler assembler;
    auto code = codegen.ConsumeResult();
    pretty::OperationPrinter printer{std::cout};
    printer.Visit(code);
    assembler.Visit(std::move(code));
    Runtime runtime{assembler.ConsumeResult()};
    std::cout << "Starting runtime...\n";
    runtime.Run();
  } catch (const std::exception& error) {
    std::cout << error.what() << "\n";
  }
}
