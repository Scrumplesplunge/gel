#include "analysis.h"

#include "util.h"

#include <algorithm>
#include <cassert>

namespace analysis {
namespace {

const Reader::Location BuiltinLocation() {
  static const auto* const reader = new Reader{"builtin", "<native code>"};
  return reader->location();
}

std::optional<types::Type> GetType(
    const std::optional<AnnotatedAst::Expression>& expression) {
  if (!expression.has_value()) return std::nullopt;
  return AnnotatedAst::GetMeta(*expression).type;
}

}  // namespace

MessageBuilder::~MessageBuilder() {
  checker_->diagnostics_.push_back(Message{type_, location_, text_.str()});
}

Checker::Checker()
    : operators_{
          {
              {ast::Arithmetic::ADD, types::Primitive::INTEGER},
              {ast::Arithmetic::DIVIDE, types::Primitive::INTEGER},
              {ast::Arithmetic::MULTIPLY, types::Primitive::INTEGER},
              {ast::Arithmetic::SUBTRACT, types::Primitive::INTEGER},
          },
          {types::Primitive::BOOLEAN, types::Primitive::INTEGER},
          {types::Primitive::INTEGER},
      },
      types_{
          types::Void{},
          types::Primitive::BOOLEAN,
          types::Primitive::INTEGER,
      } {
  scope_.Define("print", analysis::Scope::Entry{
                             BuiltinLocation(),
                             types::Function{types::Void{},
                                             {types::Primitive::INTEGER}}});
}

void Checker::AddType(const types::Type& type) {
  auto i = std::find(types_.begin(), types_.end(), type);
  if (i != types_.end()) return;
  // Add all child types first.
  type.visit(types::visit_children{[this](auto subtype) { AddType(subtype); }});
  types_.emplace_back(type);
}

MessageBuilder Checker::Error(Reader::Location location) {
  return MessageBuilder{this, Message::Type::ERROR, location};
}

MessageBuilder Checker::Warning(Reader::Location location) {
  return MessageBuilder{this, Message::Type::WARNING, location};
}

MessageBuilder Checker::Note(Reader::Location location) {
  return MessageBuilder{this, Message::Type::NOTE, location};
}

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

std::optional<AnnotatedAst::Identifier> FunctionChecker::CheckExpression(
    const ParsedAst::Identifier& identifier) {
  auto* entry = scope_->Lookup(identifier.name);
  if (entry == nullptr) {
    checker_->Error(identifier.location)
        << "Undefined identifier " << util::Detail(identifier.name) << ".";
    return std::nullopt;
  }
  if (!entry->type.has_value()) return std::nullopt;
  return AnnotatedAst::Identifier{{*entry->type}, identifier.name};
}

std::optional<AnnotatedAst::Boolean> FunctionChecker::CheckExpression(
    const ParsedAst::Boolean& boolean) {
  checker_->AddType(types::Primitive::BOOLEAN);
  return AnnotatedAst::Boolean{{types::Primitive::BOOLEAN}, boolean.value};
}

std::optional<AnnotatedAst::Integer> FunctionChecker::CheckExpression(
    const ParsedAst::Integer& integer) {
  checker_->AddType(types::Primitive::INTEGER);
  return AnnotatedAst::Integer{{types::Primitive::INTEGER}, integer.value};
}

std::optional<AnnotatedAst::ArrayLiteral> FunctionChecker::CheckExpression(
    const ParsedAst::ArrayLiteral& array) {
  std::map<types::Type, Reader::Location> type_exemplars;
  std::vector<AnnotatedAst::Expression> parts;
  bool error = false;
  for (const auto& entry : array.parts) {
    auto result = CheckAnyExpression(entry);
    if (result.has_value()) {
      type_exemplars.emplace(AnnotatedAst::GetMeta(*result).type,
                             ParsedAst::GetMeta(entry).location);
      parts.emplace_back(std::move(*result));
    } else {
      error = true;
    }
  }
  if (error) return std::nullopt;
  assert(type_exemplars.size() >= 1);
  assert(parts.size() == array.parts.size());
  if (type_exemplars.size() == 1) {
    auto type = types::Array{type_exemplars.begin()->first};
    checker_->AddType(type);
    return AnnotatedAst::ArrayLiteral{{type}, std::move(parts)};
  } else {
    checker_->Error(ParsedAst::GetMeta(array).location)
        << "Ambiguous type for array.";
    for (const auto& [type, location] : type_exemplars) {
      checker_->Note(location) << "Expression of type " << type << ".";
    }
    return std::nullopt;
  }
}

std::optional<AnnotatedAst::Arithmetic> FunctionChecker::CheckExpression(
    const ParsedAst::Arithmetic& binary) {
  auto left = CheckAnyExpression(binary.left);
  auto right = CheckAnyExpression(binary.right);

  if (!left.has_value() || !right.has_value()) {
    // It's not possible to infer the result type without an argument type.
    return std::nullopt;
  }

  const auto& left_type = AnnotatedAst::GetMeta(*left).type;
  const auto& right_type = AnnotatedAst::GetMeta(*right).type;
  if (left_type != right_type) {
    checker_->Error(binary.location)
        << "Mismatched arguments to arithmetic operator. "
        << "Left argument has type " << util::Detail(left_type)
        << ", but right argument has type " << util::Detail(right_type) << ".";
    return std::nullopt;
  }
  const auto& inferred_type = left_type;

  const auto& operators = checker_->operators_.arithmetic;
  auto i = operators.find({binary.operation, inferred_type});
  if (i == operators.end()) {
    checker_->Error(binary.location) << "Cannot use this operator with "
                                     << util::Detail(inferred_type) << ".";
    return std::nullopt;
  }

  return AnnotatedAst::Arithmetic{
      {inferred_type}, binary.operation, *left, *right};
}

std::optional<AnnotatedAst::Compare> FunctionChecker::CheckExpression(
    const ParsedAst::Compare& binary) {
  auto left = CheckAnyExpression(binary.left);
  auto right = CheckAnyExpression(binary.right);

  if (!left.has_value() || !right.has_value()) {
    // It's not possible to infer the result type without an argument type.
    return std::nullopt;
  }

  const auto& left_type = AnnotatedAst::GetMeta(*left).type;
  const auto& right_type = AnnotatedAst::GetMeta(*right).type;
  if (left_type != right_type) {
    checker_->Error(binary.location)
        << "Mismatched arguments to comparison operator. "
        << "Left argument has type " << util::Detail(left_type)
        << ", but right argument has type " << util::Detail(right_type) << ".";
    return std::nullopt;
  }
  const auto& inferred_type = left_type;

  if (binary.operation == ast::Compare::EQUAL ||
      binary.operation == ast::Compare::NOT_EQUAL) {
    const auto& types = checker_->operators_.equality_comparable;
    auto i = types.find(inferred_type);
    if (i == types.end()) {
      checker_->Error(binary.location)
          << util::Detail(inferred_type) << " is not equality comparable.";
      return std::nullopt;
    }
  } else {
    const auto& types = checker_->operators_.ordered;
    auto i = types.find(inferred_type);
    if (i == types.end()) {
      checker_->Error(binary.location)
          << util::Detail(inferred_type) << " is not an ordered type.";
      return std::nullopt;
    }
  }

  return AnnotatedAst::Compare{
      {types::Primitive::BOOLEAN}, binary.operation, *left, *right};
}

std::optional<AnnotatedAst::Logical> FunctionChecker::CheckExpression(
    const ParsedAst::Logical& binary) {
  auto left = CheckAnyExpression(binary.left);
  auto right = CheckAnyExpression(binary.right);

  // Both arguments should be booleans.
  if (left.has_value()) {
    const auto& type = AnnotatedAst::GetMeta(*left).type;
    if (type != types::Primitive::BOOLEAN) {
      checker_->Error(ParsedAst::GetMeta(binary.left).location)
          << "Expression should be " << util::Detail(types::Primitive::BOOLEAN)
          << ", actual type is " << util::Detail(type) << ".";
      return std::nullopt;
    }
  }
  if (right.has_value()) {
    const auto& type = AnnotatedAst::GetMeta(*right).type;
    if (type != types::Primitive::BOOLEAN) {
      checker_->Error(ParsedAst::GetMeta(binary.right).location)
          << "Expression should be " << util::Detail(types::Primitive::BOOLEAN)
          << ", actual type is " << util::Detail(type) << ".";
      return std::nullopt;
    }
  }
  if (!left.has_value() || !right.has_value()) return std::nullopt;
  return AnnotatedAst::Logical{
      {types::Primitive::BOOLEAN}, binary.operation, *left, *right};
}

std::optional<AnnotatedAst::FunctionCall> FunctionChecker::CheckExpression(
    const ParsedAst::FunctionCall& call) {
  std::vector<AnnotatedAst::Expression> arguments;
  {
    bool error = false;
    for (const auto& argument : call.arguments) {
      auto result = CheckAnyExpression(argument);
      if (result.has_value()) {
        arguments.emplace_back(std::move(*result));
      } else {
        error = true;
      }
    }
    if (error) return std::nullopt;
  }
  assert(arguments.size() == call.arguments.size());

  auto* entry = scope_->Lookup(call.function);
  if (entry == nullptr) {
    checker_->Error(call.location)
        << "Undefined identifier " << util::Detail(call.function) << ".";
    return std::nullopt;
  }

  if (!entry->type.has_value()) return std::nullopt;

  const types::Function* type = entry->type->get_if<types::Function>();
  if (type == nullptr) {
    checker_->Error(call.location)
        << util::Detail(call.function) << " is not of function type.";
    checker_->Note(entry->location)
        << util::Detail(call.function) << " is declared here.";
    return std::nullopt;
  }

  if (call.arguments.size() != type->parameters.size()) {
    checker_->Error(call.location)
        << util::Detail(call.function) << " expects "
        << util::Detail(type->parameters.size()) << " arguments but "
        << util::Detail(call.arguments.size()) << " were provided.";
    checker_->Note(entry->location)
        << util::Detail(call.function) << " is declared here.";
    return std::nullopt;
  }

  {
    bool error = false;
    for (std::size_t i = 0; i < arguments.size(); i++) {
      auto arg_type = AnnotatedAst::GetMeta(arguments[i]).type;
      if (arg_type != type->parameters[i]) {
        checker_->Error(ParsedAst::GetMeta(call.arguments[i]).location)
            << "Type mismatch for parameter " << util::Detail(i)
            << " of call to " << util::Detail(call.function)
            << ". Expected type is " << util::Detail(type->parameters[i])
            << " but the actual type is " << util::Detail(arg_type) << ".";
      }
    }
    if (error) return std::nullopt;
  }
  return AnnotatedAst::FunctionCall{
      {type->return_type}, call.function, std::move(arguments)};
}

std::optional<AnnotatedAst::LogicalNot> FunctionChecker::CheckExpression(
    const ParsedAst::LogicalNot& logical_not) {
  checker_->AddType(types::Primitive::BOOLEAN);
  auto argument = CheckAnyExpression(logical_not.argument);
  if (!argument.has_value()) return std::nullopt;
  const auto& type = AnnotatedAst::GetMeta(*argument).type;
  if (type != types::Primitive::BOOLEAN) {
    checker_->Error(ParsedAst::GetMeta(logical_not.argument).location)
        << "Expression should be of type "
        << util::Detail(types::Primitive::BOOLEAN)
        << ", but is actually of type " << util::Detail(type) << ".";
    return std::nullopt;
  }
  return AnnotatedAst::LogicalNot{{types::Primitive::BOOLEAN}, *argument};
}

std::optional<AnnotatedAst::Expression> FunctionChecker::CheckAnyExpression(
    const ParsedAst::Expression& expression) {
  return expression.visit(
      [&](const auto& value) -> std::optional<AnnotatedAst::Expression> {
        return CheckExpression(value);
      });
}

std::optional<AnnotatedAst::DefineVariable> FunctionChecker::CheckStatement(
    const ParsedAst::DefineVariable& definition) {
  auto value = CheckAnyExpression(definition.value);
  auto type = GetType(value);
  if (type.has_value() && !IsValueType(*type)) {
    checker_->Error(definition.location)
        << "Assignment expression in definition yields type "
        << util::Detail(*type)
        << ", which is not a suitable type for a variable.";
  }
  // A call to Define() will succeed if there is no variable with the same name
  // that was defined in the current scope. However, there may still be a name
  // conflict in a surrounding scope. This isn't strictly a bug, so it should
  // produce a warning.
  auto* previous_entry = scope_->Lookup(definition.variable.name);
  Scope::Entry entry{definition.variable.location, type};
  if (scope_->Define(definition.variable.name, entry)) {
    if (previous_entry) {
      checker_->Warning(definition.location)
          << "Definition of " << util::Detail(definition.variable.name)
          << " shadows an existing definition.";
      checker_->Note(previous_entry->location)
          << util::Detail(definition.variable.name)
          << " was previously declared here.";
    }
  } else {
    checker_->Error(definition.location)
        << "Redefinition of variable " << util::Detail(definition.variable.name)
        << ".";
    checker_->Note(previous_entry->location)
        << util::Detail(definition.variable.name)
        << " was previously declared here.";
  }
  if (value.has_value()) {
    return AnnotatedAst::DefineVariable{
        {},
        AnnotatedAst::Identifier{{*entry.type}, definition.variable.name},
        *value};
  } else {
    return std::nullopt;
  }
}

std::optional<AnnotatedAst::Assign> FunctionChecker::CheckStatement(
    const ParsedAst::Assign& assignment) {
  auto value = CheckAnyExpression(assignment.value);
  auto type = GetType(value);
  const auto* entry = scope_->Lookup(assignment.variable.name);
  if (entry == nullptr) {
    checker_->Error(assignment.location)
        << "Assignment to undefined variable "
        << util::Detail(assignment.variable.name) << ". Did you mean to write "
        << util::Detail("let") << "?";
    // Assume a definition was intended.
    scope_->Define(assignment.variable.name,
                  Scope::Entry{assignment.location, type});
    entry = scope_->Lookup(assignment.variable.name);
  }
  if (entry->type.has_value() && type.has_value()) {
    if (*entry->type == *type) {
      assert(value.has_value());
      return AnnotatedAst::Assign{
          {},
          AnnotatedAst::Identifier{{*type}, assignment.variable.name},
          std::move(*value)};
    } else {
      checker_->Error(assignment.location)
          << "Type mismatch in assignment: "
          << util::Detail(assignment.variable.name) << " has type "
          << util::Detail(*entry->type) << ", but expression yields type "
          << util::Detail(*type) << ".";
      checker_->Note(entry->location)
          << util::Detail(assignment.variable.name) << " is declared here.";
      return std::nullopt;
    }
  } else {
    return std::nullopt;
  }
}

std::optional<AnnotatedAst::DoFunction> FunctionChecker::CheckStatement(
    const ParsedAst::DoFunction& do_function) {
  auto call = CheckExpression(do_function.function_call);
  if (!call.has_value()) return std::nullopt;
  if (call->type != types::Void{}) {
    checker_->Warning(do_function.location)
        << "Discarding return value of type " << util::Detail(call->type)
        << " in call to " << util::Detail(do_function.function_call.function)
        << ".";
  }
  return AnnotatedAst::DoFunction{{}, std::move(*call)};
}

std::optional<AnnotatedAst::If> FunctionChecker::CheckStatement(
    const ParsedAst::If& if_statement) {
  auto condition = CheckAnyExpression(if_statement.condition);
  auto type = GetType(condition);
  if (type.has_value() && *type != types::Primitive::BOOLEAN) {
    checker_->Error(ParsedAst::GetMeta(if_statement.condition).location)
        << "Condition for if statement has type " << util::Detail(*type)
        << ", not " << util::Detail(types::Primitive::BOOLEAN) << ".";
  }
  Scope true_scope{scope_};
  FunctionChecker true_checker{type_, this_function_, checker_, &true_scope};
  auto if_true = true_checker.CheckStatement(if_statement.if_true);
  Scope false_scope{scope_};
  FunctionChecker false_checker{type_, this_function_, checker_, &false_scope};
  auto if_false = false_checker.CheckStatement(if_statement.if_false);
  if (condition.has_value() && if_true.has_value() && if_false.has_value()) {
    return AnnotatedAst::If{
        {}, std::move(*condition), std::move(*if_true), std::move(*if_false)};
  } else {
    return std::nullopt;
  }
}

std::optional<AnnotatedAst::While> FunctionChecker::CheckStatement(
    const ParsedAst::While& while_statement) {
  auto condition = CheckAnyExpression(while_statement.condition);
  auto type = GetType(condition);
  if (type.has_value() && *type != types::Primitive::BOOLEAN) {
    checker_->Error(ParsedAst::GetMeta(while_statement.condition).location)
        << "Condition for while statement has type " << util::Detail(*type)
        << ", not " << util::Detail(types::Primitive::BOOLEAN) << ".";
  }
  Scope body_scope{scope_};
  FunctionChecker body_checker{type_, this_function_, checker_, &body_scope};
  auto body = body_checker.CheckStatement(while_statement.body);
  if (condition.has_value() && body.has_value()) {
    return AnnotatedAst::While{{}, std::move(*condition), std::move(*body)};
  } else {
    return std::nullopt;
  }
}

std::optional<AnnotatedAst::ReturnVoid> FunctionChecker::CheckStatement(
    const ParsedAst::ReturnVoid& return_statement) {
  if (type_.return_type != types::Void{}) {
    checker_->Error(return_statement.location)
        << "Cannot return without a value: " << util::Detail(this_function_)
        << " has return type " << util::Detail(type_.return_type) << ".";
    return std::nullopt;
  }
  return AnnotatedAst::ReturnVoid{};
}

std::optional<AnnotatedAst::Return> FunctionChecker::CheckStatement(
    const ParsedAst::Return& return_statement) {
  auto value = CheckAnyExpression(return_statement.value);
  if (!value.has_value()) return std::nullopt;
  auto type = AnnotatedAst::GetMeta(*value).type;
  if (type != type_.return_type) {
    checker_->Error(return_statement.location)
        << "Type mismatch in return statement: " << util::Detail(this_function_)
        << " has return type " << util::Detail(type_.return_type)
        << " but expression has type " << util::Detail(type) << ".";
  }
  return AnnotatedAst::Return{{}, std::move(*value)};
}

std::optional<std::vector<AnnotatedAst::Statement>>
FunctionChecker::CheckStatement(
    const std::vector<ParsedAst::Statement>& statements) {
  std::vector<AnnotatedAst::Statement> output;
  bool error = false;
  for (const auto& statement : statements) {
    auto result = CheckAnyStatement(statement);
    if (result.has_value()) {
      output.push_back(std::move(*result));
    } else {
      error = true;
    }
  }
  if (error) {
    return std::nullopt;
  } else {
    return std::move(output);
  }
}

std::optional<AnnotatedAst::Statement> FunctionChecker::CheckAnyStatement(
    const ParsedAst::Statement& statement) {
  return statement.visit(
      [&](const auto& x) -> std::optional<AnnotatedAst::Statement> {
        return CheckStatement(x);
      });
}

std::optional<AnnotatedAst::DefineFunction> Checker::CheckTopLevel(
    const ParsedAst::DefineFunction& definition) {
  if (!scope_.Define(definition.name,
                     Scope::Entry{definition.location, definition.type})) {
    Error(definition.location)
        << "Redefinition of name " << util::Detail(definition.name)
        << ".";
    auto* previous_entry = scope_.Lookup(definition.name);
    Note(previous_entry->location)
        << util::Detail(definition.name) << " previously declared here.";
  }
  Scope function_scope{&scope_};
  assert(definition.parameters.size() == definition.type.parameters.size());
  std::size_t n = definition.parameters.size();
  std::vector<AnnotatedAst::Identifier> output_parameters;
  bool parameter_error = false;
  for (std::size_t i = 0; i < n; i++) {
    const auto& parameter = definition.parameters[i];
    const auto& type = definition.type.parameters[i];
    if (function_scope.Define(parameter.name,
                              Scope::Entry{parameter.location, type})) {
      output_parameters.push_back(
          AnnotatedAst::Identifier{{type}, parameter.name});
    } else {
      Error(parameter.location) << "Multiple parameters called "
                                << util::Detail(parameter.name) << ".";
      auto* previous_entry = function_scope.Lookup(parameter.name);
      Note(previous_entry->location) << "Previous definition is here.";
      parameter_error = true;
    }
  }
  FunctionChecker function_checker{definition.type, definition.name, this,
                                   &function_scope};
  auto body = function_checker.CheckStatement(definition.body);
  if (!parameter_error && body.has_value()) {
    return AnnotatedAst::DefineFunction{{},
                                        definition.type,
                                        definition.name,
                                        std::move(output_parameters),
                                        std::move(*body)};
  } else {
    return std::nullopt;
  }
}

std::optional<std::vector<AnnotatedAst::DefineFunction>> Checker::CheckTopLevel(
    const std::vector<ParsedAst::DefineFunction>& definitions) {
  bool error = false;
  std::vector<AnnotatedAst::DefineFunction> output;
  for (const auto& definition : definitions) {
    auto result = CheckTopLevel(definition);
    if (result.has_value()) {
      output.push_back(std::move(*result));
    } else {
      error = true;
    }
  }
  return output;
}

std::optional<AnnotatedAst::TopLevel> Checker::CheckAnyTopLevel(
    const ParsedAst::TopLevel& top_level) {
  return top_level.visit(
      [&](const auto& x) -> std::optional<AnnotatedAst::TopLevel> {
        return CheckTopLevel(x);
      });
}

Result Check(const ParsedAst::TopLevel& top_level) {
  Checker checker;
  Result result;
  result.annotated_ast = checker.CheckAnyTopLevel(top_level);
  result.required_types = checker.ConsumeTypes();
  result.diagnostics = checker.ConsumeDiagnostics();
  return result;
}

}  // namespace analysis
