#include "analysis.h"

#include "util.h"

namespace analysis {

MessageBuilder::~MessageBuilder() {
  context_->diagnostics.push_back(Message{type_, location_, text_.str()});
}

MessageBuilder GlobalContext::Error(Reader::Location location) {
  return MessageBuilder{this, Message::Type::ERROR, location};
}

MessageBuilder GlobalContext::Warning(Reader::Location location) {
  return MessageBuilder{this, Message::Type::WARNING, location};
}

MessageBuilder GlobalContext::Note(Reader::Location location) {
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

ast::Identifier Check(const ast::Identifier& identifier,
                      FunctionContext* context, const Scope* scope) {
  auto* entry = scope->Lookup(identifier.name);
  if (entry == nullptr) {
    context->global_context->Error(identifier.location)
        << "Undefined identifier " << util::Detail(identifier.name) << ".";
    return identifier;
  }
  auto copy = identifier;
  copy.type = entry->type;
  return copy;
}

ast::Boolean Check(const ast::Boolean& boolean, FunctionContext*,
                   const Scope*) {
  auto copy = boolean;
  copy.type = ast::Primitive::BOOLEAN;
  return copy;
}

ast::Integer Check(const ast::Integer& integer, FunctionContext*,
                   const Scope*) {
  auto copy = integer;
  copy.type = ast::Primitive::INTEGER;
  return copy;
}

ast::Arithmetic Check(const ast::Arithmetic& binary, FunctionContext* context,
                      const Scope* scope) {
  auto left = Check(binary.left, context, scope);
  auto left_type = GetMeta(left).type;
  auto right = Check(binary.right, context, scope);
  auto right_type = GetMeta(right).type;

  bool left_acceptable =
      !left_type.has_value() || *left_type == ast::Primitive::INTEGER;
  // Both arguments should be integers.
  if (!left_acceptable) {
    context->global_context->Error(binary.location)
        << "Left argument of arithmetic operator has type "
        << util::Detail(*left_type) << ", which is not an arithmetic type.";
  }
  bool right_acceptable =
      !right_type.has_value() || *right_type == ast::Primitive::INTEGER;
  if (!right_acceptable) {
    context->global_context->Error(binary.location)
        << "Left argument of arithmetic operator has type "
        << util::Detail(*right_type) << ", which is not an arithmetic type.";
  }
  if (!left_acceptable || !right_acceptable) return binary;
  if (left_type.has_value() && right_type.has_value() &&
      !(*left_type == *right_type)) {
    context->global_context->Error(binary.location)
        << "Mismatched arguments to arithmetic operator. "
        << "Left argument has type " << util::Detail(*left_type)
        << ", but right argument has type " << util::Detail(*right_type) << ".";
    return binary;
  }
  auto copy = binary;
  copy.type = left_type.has_value()
                  ? left_type.value()
                  : right_type.has_value() ? right_type.value()
                                           : ast::Primitive::INTEGER;
  return copy;
}

ast::Compare Check(const ast::Compare& binary, FunctionContext* context,
                   const Scope* scope) {
  auto left = Check(binary.left, context, scope);
  auto left_type = GetMeta(left).type;
  auto right = Check(binary.right, context, scope);
  auto right_type = GetMeta(right).type;

  if (left_type == ast::Void{}) {
    context->global_context->Error(GetMeta(binary.left).location)
        << "Left argument to comparison operator has type "
        << util::Detail(ast::Void{}) << ".";
  }
  if (right_type == ast::Void{}) {
    context->global_context->Error(GetMeta(binary.right).location)
        << "Right argument to comparison operator has type "
        << util::Detail(ast::Void{}) << ".";
  }
  if (left_type.has_value() && right_type.has_value() &&
      left_type != right_type) {
    context->global_context->Error(binary.location)
        << "Mismatched arguments to comparison operator. "
        << "Left argument has type " << util::Detail(*left_type)
        << ", but right argument has type " << util::Detail(*right_type) << ".";
  }
  auto copy = binary;
  copy.type = ast::Primitive::BOOLEAN;
  return copy;
}

ast::Logical Check(const ast::Logical& binary, FunctionContext* context,
                   const Scope* scope) {
  auto left = Check(binary.left, context, scope);
  auto left_type = GetMeta(left).type;
  auto right = Check(binary.right, context, scope);
  auto right_type = GetMeta(right).type;

  // Both arguments should be booleans.
  if (left_type.has_value() && !(*left_type == ast::Primitive::BOOLEAN)) {
    context->global_context->Error(binary.location)
        << "Left argument to logical operation has type "
        << util::Detail(*left_type) << ", which is not a boolean type.";
  }
  if (right_type.has_value() && !(*right_type == ast::Primitive::BOOLEAN)) {
    context->global_context->Error(binary.location)
        << "Right argument to logical operation has type "
        << util::Detail(*left_type) << ", which is not a boolean type.";
  }
  auto copy = binary;
  copy.type = ast::Primitive::BOOLEAN;
  return copy;
}

ast::FunctionCall Check(const ast::FunctionCall& call, FunctionContext* context,
                        const Scope* scope) {
  auto* entry = scope->Lookup(call.function.name);
  if (entry == nullptr) {
    context->global_context->Error(call.function.location)
        << "Undefined identifier " << util::Detail(call.function.name) << ".";
    auto copy = call;
    copy.type = ast::Primitive::INTEGER;
    // Visit each argument just for the diagnostics.
    for (const auto& argument : call.arguments) Check(argument, context, scope);
    return copy;
  }

  const ast::Function* type =
      entry->type.has_value() ? entry->type->get_if<ast::Function>() : nullptr;
  if (type == nullptr) {
    context->global_context->Error(call.function.location)
        << util::Detail(call.function.name) << " is not of function type.";
    context->global_context->Note(entry->location)
        << util::Detail(call.function.name) << " is declared here.";
    auto copy = call;
    copy.type = ast::Primitive::INTEGER;
    // Visit each argument just for the diagnostics.
    for (const auto& argument : call.arguments) Check(argument, context, scope);
    return copy;
  }

  if (call.arguments.size() != type->parameters.size()) {
    context->global_context->Error(call.function.location)
        << util::Detail(call.function.name) << " expects "
        << util::Detail(type->parameters.size()) << " arguments but "
        << util::Detail(call.arguments.size()) << " were provided.";
    context->global_context->Note(entry->location)
        << util::Detail(call.function.name) << " is declared here.";
    auto copy = call;
    copy.type = ast::Primitive::INTEGER;
    // Visit each argument just for the diagnostics.
    for (const auto& argument : call.arguments) Check(argument, context, scope);
    return copy;
  }

  auto copy = call;
  copy.type = type->return_type;
  copy.arguments.clear();
  for (std::size_t i = 0; i < call.arguments.size(); i++) {
    auto arg_copy = Check(call.arguments[i], context, scope);
    auto arg_type = GetMeta(arg_copy).type;
    if (arg_type.has_value() && type->parameters[i].type.has_value() &&
        !(*arg_type == *type->parameters[i].type)) {
      context->global_context->Error(GetMeta(call.arguments[i]).location)
          << "Type mismatch for parameter "
          << util::Detail(type->parameters[i].name) << " of call to "
          << util::Detail(call.function.name) << ". Expected type is "
          << util::Detail(*type->parameters[i].type)
          << " but the actual type is " << util::Detail(*arg_type) << ".";
    }
    copy.arguments.push_back(std::move(arg_copy));
  }
  return copy;
}

ast::LogicalNot Check(const ast::LogicalNot& logical_not,
                      FunctionContext* context, const Scope* scope) {
  auto value_copy = Check(logical_not.argument, context, scope);
  const std::optional<ast::Type>& value_type = GetMeta(value_copy).type;
  if (value_type.has_value() && !(*value_type == ast::Primitive::BOOLEAN)) {
    context->global_context->Error(GetMeta(value_copy).location)
        << "Argument to logical negation is of type "
        << util::Detail(*value_type) << ", not boolean.";
  }
  auto copy = logical_not;
  copy.type = ast::Primitive::BOOLEAN;
  return copy;
}

ast::Expression Check(const ast::Expression& expression,
                      FunctionContext* context, const Scope* scope) {
  return expression.visit([&](const auto& value) -> ast::Expression {
    return Check(value, context, scope);
  });
}

ast::DefineVariable Check(const ast::DefineVariable& definition,
                          FunctionContext* context, Scope* scope) {
  auto value_copy = Check(definition.value, context, scope);
  auto value_type = GetMeta(value_copy).type;
  if (value_type.has_value() && !value_type->is<ast::Primitive>()) {
    context->global_context->Error(definition.location)
        << "Assignment expression in definition yields type "
        << util::Detail(*value_type)
        << ", which is not a suitable type for a variable.";
  }
  auto copy = definition;
  copy.variable.type = GetMeta(value_copy).type;
  Scope::Entry entry{definition.location, copy.variable.type};
  // A call to Define() will succeed if there is no variable with the same name
  // that was defined in the current scope. However, there may still be a name
  // conflict in a surrounding scope. This isn't strictly a bug, so it should
  // produce a warning.
  auto* previous_entry = scope->Lookup(definition.variable.name);
  if (scope->Define(definition.variable.name, entry)) {
    if (previous_entry) {
      context->global_context->Warning(definition.location)
          << "Definition of " << util::Detail(definition.variable.name)
          << " shadows an existing definition.";
      context->global_context->Note(previous_entry->location)
          << util::Detail(definition.variable.name)
          << " was previously declared here.";
    }
  } else {
    context->global_context->Error(definition.location)
        << "Redefinition of variable " << util::Detail(definition.variable.name)
        << ".";
    context->global_context->Note(previous_entry->location)
        << util::Detail(definition.variable.name)
        << " was previously declared here.";
  }
  return copy;
}

ast::Assign Check(const ast::Assign& assignment, FunctionContext* context,
                  Scope* scope) {
  auto value_copy = Check(assignment.value, context, scope);
  auto value_type = GetMeta(value_copy).type;
  auto* entry = scope->Lookup(assignment.variable.name);
  if (entry == nullptr) {
    context->global_context->Error(assignment.location)
        << "Assignment to undefined variable "
        << util::Detail(assignment.variable.name) << ". Did you mean to write "
        << util::Detail("let") << "?";
    // Assume a definition was intended.
    scope->Define(assignment.variable.name,
                  Scope::Entry{assignment.location, value_type});
    entry = scope->Lookup(assignment.variable.name);
  }
  if (entry->type.has_value() && value_type.has_value() &&
      !(*entry->type == *value_type)) {
    context->global_context->Error(assignment.location)
        << "Type mismatch in assignment: "
        << util::Detail(assignment.variable.name) << " has type "
        << util::Detail(*entry->type) << ", but expression yields type "
        << util::Detail(*value_type) << ".";
    context->global_context->Note(entry->location)
        << util::Detail(assignment.variable.name) << " is declared here.";
  }
  return assignment;
}

ast::DoFunction Check(const ast::DoFunction& do_function,
                      FunctionContext* context, Scope* scope) {
  auto copy = do_function;
  copy.function_call = Check(do_function.function_call, context, scope);
  auto type = copy.function_call.type;
  if (type.has_value() && !(*type == ast::Void{})) {
    context->global_context->Warning(copy.location)
        << "Discarding return value of type " << util::Detail(*type)
        << " in call to " << util::Detail(copy.function_call.function.name)
        << ".";
  }
  return copy;
}

ast::If Check(const ast::If& if_statement, FunctionContext* context,
              Scope* scope) {
  auto copy = if_statement;
  copy.condition = Check(if_statement.condition, context, scope);
  auto condition_type = GetMeta(copy.condition).type;
  if (condition_type.has_value() &&
      !(*condition_type == ast::Primitive::BOOLEAN)) {
    context->global_context->Error(GetMeta(copy.condition).location)
        << "Condition for if statement has type "
        << util::Detail(*condition_type) << ", not "
        << util::Detail(ast::Primitive::BOOLEAN) << ".";
  }
  Scope true_scope{scope};
  copy.if_true = Check(if_statement.if_true, context, &true_scope);
  Scope false_scope{scope};
  copy.if_false = Check(if_statement.if_false, context, &false_scope);
  return copy;
}

ast::While Check(const ast::While& while_statement, FunctionContext* context,
                 Scope* scope) {
  auto copy = while_statement;
  copy.condition = Check(while_statement.condition, context, scope);
  auto condition_type = GetMeta(copy.condition).type;
  if (condition_type.has_value() &&
      !(*condition_type == ast::Primitive::BOOLEAN)) {
    context->global_context->Error(GetMeta(copy.condition).location)
        << "Condition for while statement has type "
        << util::Detail(*condition_type) << ", not "
        << util::Detail(ast::Primitive::BOOLEAN) << ".";
  }
  Scope body_scope{scope};
  copy.body = Check(while_statement.body, context, &body_scope);
  return copy;
}

ast::ReturnVoid Check(const ast::ReturnVoid& return_statement,
                      FunctionContext* context, Scope*) {
  if (context->type.return_type != ast::Void{}) {
    context->global_context->Error(return_statement.location)
        << "Cannot return without a value: "
        << util::Detail(context->this_function.name) << " has return type "
        << util::Detail(context->type.return_type) << ".";
  }
  return return_statement;
}

ast::Return Check(const ast::Return& return_statement, FunctionContext* context,
                  Scope* scope) {
  auto copy = return_statement;
  copy.value = Check(return_statement.value, context, scope);
  auto type = GetMeta(copy.value).type;
  if (type.has_value() && *type != context->type.return_type) {
    context->global_context->Error(return_statement.location)
        << "Type mismatch in return statement: "
        << util::Detail(context->this_function.name) << " has return type "
        << util::Detail(context->type.return_type)
        << " but expression has type " << util::Detail(*type) << ".";
  }
  return copy;
}

std::vector<ast::Statement> Check(const std::vector<ast::Statement>& statements,
                                  FunctionContext* context, Scope* scope) {
  std::vector<ast::Statement> result;
  for (const auto& statement : statements)
    result.push_back(Check(statement, context, scope));
  return result;
}

ast::Statement Check(const ast::Statement& statement, FunctionContext* context,
                     Scope* scope) {
  return statement.visit([&](const auto& x) -> ast::Statement {
    return Check(x, context, scope);
  });
}

ast::DefineFunction Check(const ast::DefineFunction& definition,
                          GlobalContext* context, Scope* scope) {
  auto copy = definition;
  if (!scope->Define(definition.function.name,
                     Scope::Entry{definition.location, copy.function.type})) {
    context->Error(definition.location)
        << "Redefinition of name " << util::Detail(definition.function.name)
        << ".";
    auto* previous_entry = scope->Lookup(definition.function.name);
    context->Note(previous_entry->location)
        << util::Detail(definition.function.name)
        << " previously declared here.";
  }
  Scope function_scope{scope};
  for (const auto& parameter : copy.parameters) {
    if (!function_scope.Define(
            parameter.name, Scope::Entry{parameter.location, parameter.type})) {
      context->Error(parameter.location) << "Multiple parameters called "
                                         << util::Detail(parameter.name) << ".";
      auto* previous_entry = function_scope.Lookup(parameter.name);
      context->Note(previous_entry->location) << "Previous definition is here.";
    }
  }
  const auto* type = definition.function.type.has_value()
                         ? definition.function.type->get_if<ast::Function>()
                         : nullptr;
  if (type == nullptr)
    throw std::logic_error("No type information for function.");
  FunctionContext function_context{context, definition.function, *type};
  copy.body = Check(definition.body, &function_context, &function_scope);
  return copy;
}

std::vector<ast::DefineFunction> Check(
    const std::vector<ast::DefineFunction>& definitions, GlobalContext* context,
    Scope* scope) {
  std::vector<ast::DefineFunction> result;
  for (const auto& definition : definitions) {
    result.push_back(Check(definition, context, scope));
  }
  return result;
}

ast::TopLevel Check(const ast::TopLevel& top_level, GlobalContext* context,
                    Scope* scope) {
  return top_level.visit(
      [&](const auto& x) -> ast::TopLevel { return Check(x, context, scope); });
}

}  // namespace analysis
