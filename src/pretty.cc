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
}  // namespace pretty
