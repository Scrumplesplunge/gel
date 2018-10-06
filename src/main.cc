#include "ast.h"
#include "parser.h"
#include "reader.h"
#include "target-c.h"

#include <fstream>
#include <iostream>
#include <map>
#include <vector>

namespace analysis {

class SemanticError : public std::runtime_error {
 public:
  using runtime_error::runtime_error;
};

static SemanticError Error(Reader::Location location,
                           std::string_view message) {
  return SemanticError{FormatMessage("Error", location, message)};
}

class Scope {
 public:
  struct Entry {
    Reader::Location location;
    ast::Type type;
  };
  explicit Scope(Scope* parent = nullptr) : parent_(parent) {}
  bool Define(std::string name, Entry entry);
  const Entry* Lookup(std::string_view name) const;
 private:
  const Scope* parent_ = nullptr;
  std::map<std::string, Entry, std::less<>> bindings_;
};

class Expression : public ast::ExpressionVisitor {
 public:
  Expression(const Scope* scope);
  using ExpressionVisitor::Visit;
  void Visit(const ast::Identifier& i) override { result_ = Check(i); }
  void Visit(const ast::Integer& i) override { result_ = Check(i); }
  void Visit(const ast::Binary& b) override { result_ = Check(b); }
  void Visit(const ast::FunctionCall& f) override { result_ = Check(f); }
  void Visit(const ast::LogicalNot& l) override { result_ = Check(l); }
  ast::Identifier Check(const ast::Identifier&) const;
  ast::Integer Check(const ast::Integer&) const;
  ast::Binary Check(const ast::Binary&) const;
  ast::FunctionCall Check(const ast::FunctionCall&) const;
  ast::LogicalNot Check(const ast::LogicalNot&) const;
  ast::Expression result() { return std::move(result_.value()); }
 private:
  const Scope* scope_;
  std::optional<ast::Expression> result_;
};

ast::Expression Check(const Scope& scope, const ast::Expression& expression) {
  Expression checker{&scope};
  expression.Visit(checker);
  return checker.result();
}

class Statement : public ast::StatementVisitor {
 public:
  Statement(Scope* scope);
  void Visit(const ast::DefineVariable& d) override { result_ = Check(d); }
  void Visit(const ast::Assign& a) override { result_ = Check(a); }
  void Visit(const ast::DoFunction& d) override { result_ = Check(d); }
  void Visit(const ast::If& i) override { result_ = Check(i); }
  void Visit(const ast::While& w) override { result_ = Check(w); }
  void Visit(const ast::ReturnVoid& r) override { result_ = Check(r); }
  void Visit(const ast::Return& r) override { result_ = Check(r); }
  ast::DefineVariable Check(const ast::DefineVariable&) const;
  ast::Assign Check(const ast::Assign&) const;
  ast::DoFunction Check(const ast::DoFunction&) const;
  ast::If Check(const ast::If&) const;
  ast::While Check(const ast::While&) const;
  ast::ReturnVoid Check(const ast::ReturnVoid&) const;
  ast::Return Check(const ast::Return&) const;
  std::vector<ast::Statement> Check(const std::vector<ast::Statement>&) const;
  ast::Statement result() { return std::move(result_.value()); }
 private:
  Scope* scope_;
  std::optional<ast::Statement> result_;
};

ast::Statement Check(Scope* scope, const ast::Statement& statement) {
  Statement checker{scope};
  statement.Visit(checker);
  return checker.result();
}

class TopLevel : public ast::TopLevelVisitor {
 public:
  TopLevel(Scope* scope);
  using TopLevelVisitor::Visit;
  void Visit(const ast::DefineFunction& d) override { result_ = Check(d); }
  void Visit(const std::vector<ast::DefineFunction>& d) override {
    result_ = Check(d);
  }
  ast::DefineFunction Check(const ast::DefineFunction&) const;
  std::vector<ast::DefineFunction> Check(
      const std::vector<ast::DefineFunction>&) const;
  ast::TopLevel result() { return std::move(result_.value()); }
 private:
  Scope* scope_;
  std::optional<ast::TopLevel> result_;
};

ast::TopLevel Check(Scope* scope, const ast::TopLevel& top_level) {
  TopLevel checker{scope};
  top_level.Visit(checker);
  return checker.result();
}

// Implementation

bool Scope::Define(std::string name, Scope::Entry entry) {
  auto [i, j] = bindings_.equal_range(name);
  if (i != j) return false;
  bindings_.emplace_hint(i, std::move(name), entry);
  return true;
}

const Scope::Entry* Scope::Lookup(std::string_view name) const {
  auto i = bindings_.find(name);
  if (i != bindings_.end()) return &i->second;
  if (parent_ == nullptr) return nullptr;
  return parent_->Lookup(name);
}

Expression::Expression(const Scope* scope) : scope_(scope) {}

ast::Identifier Expression::Check(const ast::Identifier& identifier) const {
  auto* entry = scope_->Lookup(identifier.name);
  if (entry == nullptr) {
    throw Error(identifier.location,
                "Undefined identifier '" + identifier.name + "'.");
  }
  auto copy = identifier;
  copy.type = entry->type;
  return copy;
}

ast::Integer Expression::Check(const ast::Integer& integer) const {
  auto copy = integer;
  copy.type = ast::Primitive::INTEGER;
  return copy;
}

ast::Binary Expression::Check(const ast::Binary& binary) const {
  Expression left_visit{scope_};
  left_visit.Visit(binary.left);
  auto left = left_visit.result();
  auto left_type = GetMeta(left).type.value();
  Expression right_visit{scope_};
  right_visit.Visit(binary.left);
  auto right = right_visit.result();
  auto right_type = GetMeta(right).type.value();

  auto copy = binary;
  switch (binary.operation) {
    case ast::Binary::ADD:
    case ast::Binary::DIVIDE:
    case ast::Binary::MULTIPLY:
    case ast::Binary::SUBTRACT:
      // Both arguments should be integers.
      if (!IsArithmeticType(left_type)) {
        throw Error(
            binary.location,
            "Left argument of arithmetic operator is not of arithmetic type.");
      }
      if (!IsArithmeticType(right_type)) {
        throw Error(
            binary.location,
            "Right argument of arithmetic operator is not of arithmetic type.");
      }
      if (!(left_type == right_type)) {
        throw Error(
            binary.location,
            "Arguments of arithmetic operator are of different types.");
      }
      copy.type = left_type;
      break;
    case ast::Binary::LOGICAL_AND:
    case ast::Binary::LOGICAL_OR:
      // Both arguments should be booleans.
      if (!(left_type == ast::Primitive::BOOLEAN)) {
        throw Error(
            binary.location,
            "Left argument to logical operation is not of boolean type.");
      }
      if (!(right_type == ast::Primitive::BOOLEAN)) {
        throw Error(
            binary.location,
            "Right argument to logical operation is not of boolean type.");
      }
      copy.type = ast::Primitive::BOOLEAN;
      break;
    case ast::Binary::COMPARE_EQ:
    case ast::Binary::COMPARE_GE:
    case ast::Binary::COMPARE_GT:
    case ast::Binary::COMPARE_LE:
    case ast::Binary::COMPARE_LT:
    case ast::Binary::COMPARE_NE:
      if (!(left_type == right_type)) {
        throw Error(
            binary.location,
            "Arguments of comparison operator are of different types.");
      }
      copy.type = ast::Primitive::BOOLEAN;
      break;
  }
  return copy;
}

ast::FunctionCall Expression::Check(const ast::FunctionCall& call) const {
  auto* entry = scope_->Lookup(call.function.name);
  if (entry == nullptr) {
    throw Error(call.function.location,
                "Undefined identifier '" + call.function.name + "'.");
  }
  const ast::Function* type = GetFunctionType(entry->type);
  if (type == nullptr) {
    throw Error(call.function.location,
                "'" + call.function.name + "' is not of function type.");
  }
  if (call.arguments.size() != type->parameters.size()) {
    throw Error(call.function.location,
                "'" + call.function.name + "' expects " +
                    std::to_string(type->parameters.size()) +
                    " arguments but " + std::to_string(call.arguments.size()) +
                    " were provided.");
  }
  auto copy = call;
  copy.type = type->return_type;
  copy.arguments.clear();
  for (std::size_t i = 0; i < call.arguments.size(); i++) {
    Expression visitor{scope_};
    visitor.Visit(call.arguments[i]);
    auto arg_copy = visitor.result();
    if (!(GetMeta(arg_copy).type == type->parameters[i])) {
      throw Error(GetMeta(call.arguments[i]).location,
                  "Type mismatch for argument " + std::to_string(i) +
                      " of call to '" + call.function.name + "'.");
    }
    copy.arguments.push_back(std::move(arg_copy));
  }
  return copy;
}

ast::LogicalNot Expression::Check(const ast::LogicalNot& logical_not) const {
  Expression visitor{scope_};
  visitor.Visit(logical_not.argument);
  auto value_copy = visitor.result();
  if (!(GetMeta(value_copy).type == ast::Primitive::BOOLEAN)) {
    throw Error(logical_not.location,
                "Argument to logical negation is not of boolean type.");
  }
  auto copy = logical_not;
  copy.type = ast::Primitive::BOOLEAN;
  return copy;
}

Statement::Statement(Scope* scope) : scope_(scope) {}

ast::DefineVariable Statement::Check(
    const ast::DefineVariable& definition) const {
  Expression visitor{scope_};
  visitor.Visit(definition.value);
  auto value_copy = visitor.result();
  Scope::Entry entry{definition.location, GetMeta(value_copy).type.value()};
  if (!scope_->Define(definition.name, entry)) {
    throw Error(definition.location,
                "Redefinition of variable '" + definition.name + "'.");
  }
  return definition;
}

ast::Assign Statement::Check(const ast::Assign& assignment) const {
  Expression visitor{scope_};
  visitor.Visit(assignment.value);
  auto value_copy = visitor.result();
  auto* entry = scope_->Lookup(assignment.variable);
  if (entry == nullptr) {
    throw Error(assignment.location, "Assignment to undefined variable '" +
                                         assignment.variable + "'.");
  }
  if (!(entry->type == GetMeta(value_copy).type.value())) {
    throw Error(assignment.location,
                "Type mismatch in assignment to variable '" +
                    assignment.variable + "'.");
  }
  return assignment;
}

ast::DoFunction Statement::Check(const ast::DoFunction& do_function) const {
  Expression visitor{scope_};
  auto copy = do_function;
  copy.function_call = visitor.Check(do_function.function_call);
  return copy;
}

ast::If Statement::Check(const ast::If& if_statement) const {
  auto copy = if_statement;
  copy.condition = analysis::Check(*scope_, if_statement.condition);
  if (!(GetMeta(copy.condition).type.value() == ast::Primitive::BOOLEAN)) {
    throw Error(GetMeta(copy.condition).location,
                "Condition for if statement is not of boolean type.");
  }
  Scope true_scope{scope_};
  Statement true_checker{&true_scope};
  copy.if_true = true_checker.Check(if_statement.if_true);
  Scope false_scope{scope_};
  Statement false_checker{&false_scope};
  copy.if_false = false_checker.Check(if_statement.if_false);
  return copy;
}

ast::While Statement::Check(const ast::While& while_statement) const {
  auto copy = while_statement;
  copy.condition = analysis::Check(*scope_, while_statement.condition);
  if (!(GetMeta(copy.condition).type.value() == ast::Primitive::BOOLEAN)) {
    throw Error(GetMeta(copy.condition).location,
                "Condition for while statement is not of boolean type.");
  }
  Scope body_scope{scope_};
  Statement body_checker{&body_scope};
  copy.body = body_checker.Check(while_statement.body);
  return copy;
}

ast::ReturnVoid Statement::Check(
    const ast::ReturnVoid& return_statement) const {
  return return_statement;
}

ast::Return Statement::Check(const ast::Return& return_statement) const {
  auto copy = return_statement;
  copy.value = analysis::Check(*scope_, return_statement.value);
  return copy;
}

std::vector<ast::Statement> Statement::Check(
    const std::vector<ast::Statement>& statements) const {
  std::vector<ast::Statement> result;
  for (const auto& statement : statements)
    result.push_back(analysis::Check(scope_, statement));
  return result;
}

TopLevel::TopLevel(Scope* scope) : scope_(scope) {}

ast::DefineFunction TopLevel::Check(
    const ast::DefineFunction& definition) const {
  // For now, treat all input/outputs as integers.
  ast::Type type =
      ast::Function{ast::Primitive::INTEGER,
                    std::vector<ast::Type>(definition.parameters.size(),
                                           ast::Primitive::INTEGER)};
  if (!scope_->Define(definition.name,
                      Scope::Entry{definition.location, type})) {
    throw Error(definition.location,
                "Redefinition of name '" + definition.name + "'.");
  }
  Scope function_scope{scope_};
  for (const auto& parameter : definition.parameters) {
    if (!function_scope.Define(
            parameter,
            Scope::Entry{definition.location, ast::Primitive::INTEGER})) {
      throw Error(definition.location,
                  "Multiple parameters called '" + parameter + "'.");
    }
  }
  Statement checker{&function_scope};
  auto copy = definition;
  copy.body = checker.Check(definition.body);
  return copy;
}

std::vector<ast::DefineFunction> TopLevel::Check(
    const std::vector<ast::DefineFunction>& definitions) const {
  std::vector<ast::DefineFunction> result;
  for (const auto& definition : definitions) {
    result.push_back(Check(definition));
  }
  return result;
}

}  // namespace analysis

int main() {
  Reader builtins{"builtin", "<native code>"};
  std::string input{std::istreambuf_iterator<char>{std::cin}, {}};
  Reader reader{"stdin", input};
  Parser parser{reader};
  auto program = parser.ParseProgram();
  parser.CheckEnd();
  analysis::Scope scope;
  scope.Define("print", analysis::Scope::Entry{
                            builtins.location(),
                            ast::Function{ast::Primitive::INTEGER,
                                          {ast::Primitive::INTEGER}}});
  analysis::TopLevel checker{&scope};
  checker.Visit(program);
  auto checked = checker.result();
  {
    std::ofstream output{".gel-output.c"};
    target::c::TopLevel codegen{output};
    checked.Visit(codegen);
  }
  int compile_status = std::system("gcc .gel-output.c -o .gel-output");
  if (compile_status) return compile_status;
  return std::system("./.gel-output");
}
