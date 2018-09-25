#include "pretty.h"

namespace pretty {
namespace tree {

void ExpressionPrinter::Visit(const ast::Identifier& identifier) {
  output_ << "Identifier{\"" << identifier.name << "\"}";
}

void ExpressionPrinter::Visit(const ast::Integer& integer) {
  output_ << "Integer{" << integer.value << "}";
}

template <typename T>
void ExpressionPrinter::VisitBinary(std::string_view type, const T& binary) {
  output_ << type << "{";
  Visit(binary.left);
  output_ << ", ";
  Visit(binary.right);
  output_ << "}";
}

void ExpressionPrinter::Visit(const ast::Add& add) {
  VisitBinary("Add", add);
}

void ExpressionPrinter::Visit(const ast::Subtract& subtract) {
  VisitBinary("Subtract", subtract);
}

void ExpressionPrinter::Visit(const ast::Multiply& multiply) {
  VisitBinary("Multiply", multiply);
}

void ExpressionPrinter::Visit(const ast::Divide& divide) {
  VisitBinary("Divide", divide);
}

void ExpressionPrinter::Visit(const ast::FunctionCall& function_call) {
  output_ << "FunctionCall{\"" << function_call.function.name << "\", {";
  bool first = true;
  for (auto argument : function_call.arguments) {
    if (first) {
      first = false;
    } else {
      output_ << ", ";
    }
    Visit(argument);
  }
  output_ << "}}";
}

void ExpressionPrinter::Visit(const ast::CompareEq& comparison) {
  VisitBinary("CompareEq", comparison);
}

void ExpressionPrinter::Visit(const ast::CompareNe& comparison) {
  VisitBinary("CompareNe", comparison);
}

void ExpressionPrinter::Visit(const ast::CompareLe& comparison) {
  VisitBinary("CompareLe", comparison);
}

void ExpressionPrinter::Visit(const ast::CompareLt& comparison) {
  VisitBinary("CompareLt", comparison);
}

void ExpressionPrinter::Visit(const ast::CompareGe& comparison) {
  VisitBinary("CompareGe", comparison);
}

void ExpressionPrinter::Visit(const ast::CompareGt& comparison) {
  VisitBinary("CompareGt", comparison);
}

void ExpressionPrinter::Visit(const ast::LogicalNot& expression) {
  output_ << "LogicalNot{";
  Visit(expression.argument);
  output_ << "}";
}

void ExpressionPrinter::Visit(const ast::LogicalAnd& expression) {
  VisitBinary("LogicalAnd", expression);
}

void ExpressionPrinter::Visit(const ast::LogicalOr& expression) {
  VisitBinary("LogicalOr", expression);
}

void StatementPrinter::Visit(const ast::DeclareVariable& declaration) {
  output_ << "DeclareVariable {\n"
          << indent_ << "  .identifier = \"" << declaration.identifier.name
          << "\"\n"
          << indent_ << "  .value = ";
  ExpressionPrinter printer{output_};
  declaration.value.Visit(printer);
  output_ << "\n" << indent_ << "}";
}

void StatementPrinter::Visit(const ast::Assign& assignment) {
  output_ << "Assign {\n"
          << indent_ << "  .identifier = \"" << assignment.identifier.name
          << "\"\n"
          << indent_ << "  .value = ";
  ExpressionPrinter printer{output_};
  assignment.value.Visit(printer);
  output_ << "\n" << indent_ << "}";
}

void StatementPrinter::Visit(const ast::DoFunction& do_function) {
  output_ << "DoFunction {\n" << indent_ << "  .function_call = ";
  ExpressionPrinter printer{output_};
  printer.Visit(do_function.function_call);
  output_ << "\n" << indent_ << "}";
}

void StatementPrinter::VisitBlock(
    const std::vector<ast::Statement>& statements) {
  output_ << "{";
  if (statements.empty()) {
    output_ << "}";
  } else {
    bool first = true;
    StatementPrinter nested_printer{indent_.length() + 2, output_};
    for (const auto& statement : statements) {
      if (first) {
        first = false;
      } else {
        output_ << ",";
      }
      output_ << "\n" << indent_ << "  ";
      statement.Visit(nested_printer);
    }
    output_ << "\n" << indent_ << "}";
  }
}

void StatementPrinter::Visit(const ast::If& if_statement) {
  output_ << "If {\n" << indent_ << "  .condition = ";
  ExpressionPrinter printer{output_};
  if_statement.condition.Visit(printer);
  output_ << "\n" << indent_ << "  .if_true = ";
  StatementPrinter nested_printer{indent_.length() + 2, output_};
  nested_printer.VisitBlock(if_statement.if_true);
  output_ << ",\n" << indent_ << "  .if_false = ";
  nested_printer.VisitBlock(if_statement.if_false);
  output_ << "\n" << indent_ << "}";
}

}  // namespace tree

void OperationPrinter::Visit(const op::Sequence& sequence) {
  for (const auto& operation : sequence) Visit(operation);
}

void OperationPrinter::Visit(const op::Integer& integer) {
  output_ << "  push " << integer.value << "\n";
}

void OperationPrinter::Visit(const op::Frame& frame) {
  output_ << "  frame " << frame.offset << "\n";
}

void OperationPrinter::Visit(const op::Adjust& adjust) {
  output_ << "  adjust " << adjust.size << "\n";
}

void OperationPrinter::Visit(const op::Load&) { output_ << "  load\n"; }
void OperationPrinter::Visit(const op::Store&) { output_ << "  store\n"; }
void OperationPrinter::Visit(const op::Add&) { output_ << "  add\n"; }
void OperationPrinter::Visit(const op::Subtract&) { output_ << "  sub\n"; }
void OperationPrinter::Visit(const op::Multiply&) { output_ << "  mul\n"; }
void OperationPrinter::Visit(const op::Divide&) { output_ << "  div\n"; }
void OperationPrinter::Visit(const op::CompareEq&) { output_ << "  ceq\n"; }
void OperationPrinter::Visit(const op::CompareNe&) { output_ << "  cne\n"; }
void OperationPrinter::Visit(const op::CompareLe&) { output_ << "  cle\n"; }
void OperationPrinter::Visit(const op::CompareLt&) { output_ << "  clt\n"; }
void OperationPrinter::Visit(const op::CompareGe&) { output_ << "  cge\n"; }
void OperationPrinter::Visit(const op::CompareGt&) { output_ << "  cgt\n"; }

void OperationPrinter::Visit(const op::Label& label) {
  output_ << label.name << ":\n";
}

void OperationPrinter::Visit(const op::Jump& jump) {
  output_ << "  jump " << jump.label << "\n";
}

void OperationPrinter::Visit(const op::JumpIfZero& jump) {
  output_ << "  jz " << jump.label << "\n";
}

void OperationPrinter::Visit(const op::JumpIfNonZero& jump) {
  output_ << "  jnz " << jump.label << "\n";
}

}  // namespace pretty
