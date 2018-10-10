#include "target-c.h"

#include "util.h"

#include <algorithm>
#include <iterator>

namespace target::c {
namespace {

constexpr char kHeader[] = R"(
// Generated by the gel compiler.
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

void gel_print(int_least64_t number) { printf("%d\n", number); }

// Start of user code.
)";

constexpr char kFooter[] = R"(
// End of user code.

int main() { return gel_main(); }
)";

void CompileExpression(const ast::Identifier&, std::ostream*);
void CompileExpression(const ast::Boolean&, std::ostream*);
void CompileExpression(const ast::Integer&, std::ostream*);
void CompileExpression(const ast::Arithmetic&, std::ostream*);
void CompileExpression(const ast::Compare&, std::ostream*);
void CompileExpression(const ast::Logical&, std::ostream*);
void CompileExpression(const ast::FunctionCall&, std::ostream*);
void CompileExpression(const ast::LogicalNot&, std::ostream*);
void CompileExpression(const ast::Expression&, std::ostream*);

void CompileStatement(const ast::DefineVariable&, std::ostream*, int indent);
void CompileStatement(const ast::Assign&, std::ostream*, int indent);
void CompileStatement(const ast::DoFunction&, std::ostream*, int indent);
void CompileStatement(const ast::If&, std::ostream*, int indent);
void CompileStatement(const ast::While&, std::ostream*, int indent);
void CompileStatement(const ast::ReturnVoid&, std::ostream*, int indent);
void CompileStatement(const ast::Return&, std::ostream*, int indent);
void CompileStatement(const std::vector<ast::Statement>&, std::ostream*,
                      int indent);
void CompileStatement(const ast::Statement&, std::ostream*, int indent);

void CompileTopLevel(const ast::DefineFunction&, std::ostream*);
void CompileTopLevel(const std::vector<ast::DefineFunction>&, std::ostream*);
void CompileTopLevel(const ast::TopLevel&, std::ostream*);

void PrintType(const ast::Type& type, std::ostream* output) {
  type.visit([&](const auto& node) {
    using value_type = std::decay_t<decltype(node)>;
    if constexpr (std::is_same_v<value_type, ast::Void>) {
      *output << "void";
    } else if constexpr (std::is_same_v<value_type, ast::Primitive>) {
      switch (node) {
        case ast::Primitive::BOOLEAN:
          *output << "bool";
          break;
        case ast::Primitive::INTEGER:
          *output << "int_least64_t";
          break;
      }
    } else if constexpr (std::is_same_v<value_type, ast::Function>) {
      throw std::logic_error(
          "No function types should have to be visited when compiling.");
    }
  });
}

void CompileExpression(const ast::Identifier& identifier,
                       std::ostream* output) {
  *output << "gel_" << identifier.name;
}

void CompileExpression(const ast::Boolean& boolean, std::ostream* output) {
  *output << (boolean.value ? "true" : "false");
}

void CompileExpression(const ast::Integer& integer, std::ostream* output) {
  *output << integer.value;
}

void CompileExpression(const ast::Arithmetic& binary, std::ostream* output) {
  *output << "(";
  CompileExpression(binary.left, output);
  *output << " ";
  switch (binary.operation) {
    case ast::Arithmetic::ADD:
      *output << "+";
      break;
    case ast::Arithmetic::DIVIDE:
      *output << "/";
      break;
    case ast::Arithmetic::MULTIPLY:
      *output << "*";
      break;
    case ast::Arithmetic::SUBTRACT:
      *output << "-";
      break;
  }
  *output << " ";
  CompileExpression(binary.right, output);
  *output << ")";
}

void CompileExpression(const ast::Compare& binary, std::ostream* output) {
  *output << "(";
  CompileExpression(binary.left, output);
  *output << " ";
  switch (binary.operation) {
    case ast::Compare::EQUAL:
      *output << "==";
      break;
    case ast::Compare::GREATER_OR_EQUAL:
      *output << ">=";
      break;
    case ast::Compare::GREATER_THAN:
      *output << ">";
      break;
    case ast::Compare::LESS_OR_EQUAL:
      *output << "<=";
      break;
    case ast::Compare::LESS_THAN:
      *output << "<";
      break;
    case ast::Compare::NOT_EQUAL:
      *output << "!=";
      break;
  }
  *output << " ";
  CompileExpression(binary.right, output);
  *output << ")";
}

void CompileExpression(const ast::Logical& binary, std::ostream* output) {
  *output << "(";
  CompileExpression(binary.left, output);
  *output << " ";
  switch (binary.operation) {
    case ast::Logical::AND:
      *output << "&&";
      break;
    case ast::Logical::OR:
      *output << "||";
      break;
  }
  *output << " ";
  CompileExpression(binary.right, output);
  *output << ")";
}

void CompileExpression(const ast::FunctionCall& call, std::ostream* output) {
  CompileExpression(call.function, output);
  *output << "(";
  bool first = true;
  for (const auto& argument : call.arguments) {
    if (first) {
      first = false;
    } else {
      *output << ", ";
    }
    CompileExpression(argument, output);
  }
  *output << ")";
}

void CompileExpression(const ast::LogicalNot& op, std::ostream* output) {
  *output << "!";
  CompileExpression(op.argument, output);
}

void CompileExpression(const ast::Expression& expression,
                       std::ostream* output) {
  expression.visit([&](const auto& node) { CompileExpression(node, output); });
}

void CompileStatement(const ast::DefineVariable& definition,
                      std::ostream* output, int indent) {
  *output << util::Spaces{indent};
  PrintType(definition.variable.type.value(), output);
  *output << " ";
  CompileExpression(definition.variable, output);
  *output << " = ";
  CompileExpression(definition.value, output);
  *output << ";\n";
}

void CompileStatement(const ast::Assign& assignment, std::ostream* output,
                      int indent) {
  *output << util::Spaces{indent};
  CompileExpression(assignment.variable, output);
  *output << " = ";
  CompileExpression(assignment.value, output);
  *output << ";\n";
}

void CompileStatement(const ast::DoFunction& do_function, std::ostream* output,
                      int indent) {
  *output << util::Spaces{indent};
  CompileExpression(do_function.function_call, output);
  *output << ";\n";
}

void CompileStatement(const ast::If& if_statement, std::ostream* output,
                      int indent) {
  *output << util::Spaces{indent} << "if (";
  CompileExpression(if_statement.condition, output);
  *output << ") {\n";
  CompileStatement(if_statement.if_true, output, indent + 2);
  *output << util::Spaces{indent} << "} else {\n";
  CompileStatement(if_statement.if_false, output, indent + 2);
  *output << util::Spaces{indent} << "}\n";
}

void CompileStatement(const ast::While& while_statement, std::ostream* output,
                      int indent) {
  *output << util::Spaces{indent} << "while (";
  CompileExpression(while_statement.condition, output);
  *output << ") {\n";
  CompileStatement(while_statement.body, output, indent + 2);
  *output << util::Spaces{indent} << "}\n";
}

void CompileStatement(const ast::ReturnVoid&, std::ostream* output,
                      int indent) {
  *output << util::Spaces{indent} << "return;\n";
}

void CompileStatement(const ast::Return& return_statement, std::ostream* output,
                      int indent) {
  *output << util::Spaces{indent} << "return ";
  CompileExpression(return_statement.value, output);
  *output << ";\n";
}

void CompileStatement(const std::vector<ast::Statement>& statements,
                      std::ostream* output, int indent) {
  for (const auto& statement : statements)
    CompileStatement(statement, output, indent);
}

void CompileStatement(const ast::Statement& statement, std::ostream* output,
                      int indent) {
  statement.visit([&](const auto& x) { CompileStatement(x, output, indent); });
}

void CompileTopLevel(const ast::DefineFunction& definition,
                     std::ostream* output) {
  const ast::Function* type =
      definition.function.type.value().get_if<ast::Function>();
  PrintType(type->return_type, output);
  *output << " ";
  CompileExpression(definition.function, output);
  *output << "(";
  bool first = true;
  for (const auto& parameter : definition.parameters) {
    if (first) {
      first = false;
    } else {
      *output << ", ";
    }
    PrintType(parameter.type.value(), output);
    *output << " ";
    CompileExpression(parameter, output);
  }
  *output << ") {\n";
  CompileStatement(definition.body, output, 2);
  *output << "}\n";
}

void CompileTopLevel(const std::vector<ast::DefineFunction>& definitions,
                     std::ostream* output) {
  bool first = true;
  for (const auto& definition : definitions) {
    if (first) {
      first = false;
    } else {
      *output << "\n";
    }
    CompileTopLevel(definition, output);
  }
}

void CompileTopLevel(const ast::TopLevel& top_level, std::ostream* output) {
  top_level.visit([&](const auto& x) { CompileTopLevel(x, output); });
}

}  // namespace

void Compile(const ast::TopLevel& top_level, std::ostream* output) {
  *output << kHeader;
  CompileTopLevel(top_level, output);
  *output << kFooter;
}

}  // namespace target::c
