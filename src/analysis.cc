#include "analysis.h"

#include "util.h"

namespace analysis {

MessageBuilder::~MessageBuilder() {
  context_->diagnostics.push_back(Message{type_, location_, text_.str()});
}

MessageBuilder Context::Error(Reader::Location location) {
  return MessageBuilder{this, Message::Type::ERROR, location};
}

MessageBuilder Context::Warning(Reader::Location location) {
  return MessageBuilder{this, Message::Type::WARNING, location};
}

MessageBuilder Context::Note(Reader::Location location) {
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

ast::Identifier Check(const ast::Identifier& identifier, Context* context,
                      const Scope* scope) {
  auto* entry = scope->Lookup(identifier.name);
  if (entry == nullptr) {
    context->Error(identifier.location)
        << "Undefined identifier " << util::Detail(identifier.name) << ".";
    return identifier;
  }
  auto copy = identifier;
  copy.type = entry->type;
  return copy;
}

ast::Boolean Check(const ast::Boolean& boolean, Context*, const Scope*) {
  auto copy = boolean;
  copy.type = ast::Primitive::BOOLEAN;
  return copy;
}

ast::Integer Check(const ast::Integer& integer, Context*, const Scope*) {
  auto copy = integer;
  copy.type = ast::Primitive::INTEGER;
  return copy;
}

ast::Binary Check(const ast::Binary& binary, Context* context,
                  const Scope* scope) {
  auto left = Check(binary.left, context, scope);
  auto left_type = GetMeta(left).type;
  auto right = Check(binary.right, context, scope);
  auto right_type = GetMeta(right).type;

  switch (binary.operation) {
    case ast::Binary::ADD:
    case ast::Binary::DIVIDE:
    case ast::Binary::MULTIPLY:
    case ast::Binary::SUBTRACT: {
      bool left_acceptable =
          !left_type.has_value() || *left_type == ast::Primitive::INTEGER;
      // Both arguments should be integers.
      if (!left_acceptable) {
        context->Error(binary.location)
            << "Left argument of arithmetic operator has type "
            << util::Detail(*left_type) << ", which is not an arithmetic type.";
      }
      bool right_acceptable =
          !right_type.has_value() || *right_type == ast::Primitive::INTEGER;
      if (!right_acceptable) {
        context->Error(binary.location)
            << "Left argument of arithmetic operator has type "
            << util::Detail(*right_type)
            << ", which is not an arithmetic type.";
      }
      if (!left_acceptable || !right_acceptable) return binary;
      if (left_type.has_value() && right_type.has_value() &&
          !(*left_type == *right_type)) {
        context->Error(binary.location)
            << "Mismatched arguments to arithmetic operator. "
            << "Left argument has type " << util::Detail(*left_type)
            << ", but right argument has type " << util::Detail(*right_type)
            << ".";
        return binary;
      }
      auto copy = binary;
      copy.type = left_type.has_value()
                      ? left_type.value()
                      : right_type.has_value() ? right_type.value()
                                               : ast::Primitive::INTEGER;
      return copy;
    }
    case ast::Binary::LOGICAL_AND:
    case ast::Binary::LOGICAL_OR: {
      // Both arguments should be booleans.
      if (left_type.has_value() && !(*left_type == ast::Primitive::BOOLEAN)) {
        context->Error(binary.location)
            << "Left argument to logical operation has type "
            << util::Detail(*left_type) << ", which is not a boolean type.";
      }
      if (right_type.has_value() && !(*right_type == ast::Primitive::BOOLEAN)) {
        context->Error(binary.location)
            << "Right argument to logical operation has type "
            << util::Detail(*left_type) << ", which is not a boolean type.";
      }
      auto copy = binary;
      copy.type = ast::Primitive::BOOLEAN;
      return copy;
    }
    case ast::Binary::COMPARE_EQ:
    case ast::Binary::COMPARE_GE:
    case ast::Binary::COMPARE_GT:
    case ast::Binary::COMPARE_LE:
    case ast::Binary::COMPARE_LT:
    case ast::Binary::COMPARE_NE: {
      if (left_type.has_value() && right_type.has_value() &&
          !(left_type == right_type)) {
        context->Error(binary.location)
            << "Mismatched arguments to comparison operator. "
            << "Left argument has type " << util::Detail(*left_type)
            << ", but right argument has type " << util::Detail(*right_type)
            << ".";
      }
      auto copy = binary;
      copy.type = ast::Primitive::BOOLEAN;
      return copy;
    }
  }
}

ast::FunctionCall Check(const ast::FunctionCall& call, Context* context,
                        const Scope* scope) {
  auto* entry = scope->Lookup(call.function.name);
  if (entry == nullptr) {
    context->Error(call.function.location)
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
    context->Error(call.function.location)
        << util::Detail(call.function.name) << " is not of function type.";
    context->Note(entry->location)
        << util::Detail(call.function.name) << " is declared here.";
    auto copy = call;
    copy.type = ast::Primitive::INTEGER;
    // Visit each argument just for the diagnostics.
    for (const auto& argument : call.arguments) Check(argument, context, scope);
    return copy;
  }

  if (call.arguments.size() != type->parameters.size()) {
    context->Error(call.function.location)
        << util::Detail(call.function.name) << " expects "
        << util::Detail(type->parameters.size()) << " arguments but "
        << util::Detail(call.arguments.size()) << " were provided.";
    context->Note(entry->location)
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
      context->Error(GetMeta(call.arguments[i]).location)
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

ast::LogicalNot Check(const ast::LogicalNot& logical_not, Context* context,
                      const Scope* scope) {
  auto value_copy = Check(logical_not.argument, context, scope);
  const std::optional<ast::Type>& value_type = GetMeta(value_copy).type;
  if (value_type.has_value() && !(*value_type == ast::Primitive::BOOLEAN)) {
    context->Error(GetMeta(value_copy).location)
        << "Argument to logical negation is of type "
        << util::Detail(*value_type) << ", not boolean.";
  }
  auto copy = logical_not;
  copy.type = ast::Primitive::BOOLEAN;
  return copy;
}

ast::Expression Check(const ast::Expression& expression, Context* context,
                      const Scope* scope) {
  return expression.visit([&](const auto& value) -> ast::Expression {
    return Check(value, context, scope);
  });
}

Statement::Statement(Context* context, Scope* scope)
    : context_(context), scope_(scope) {}

ast::DefineVariable Statement::Check(
    const ast::DefineVariable& definition) const {
  auto value_copy = analysis::Check(definition.value, context_, scope_);
  auto value_type = GetMeta(value_copy).type;
  if (value_type.has_value() && !value_type->is<ast::Primitive>()) {
    context_->Error(definition.location)
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
  auto* previous_entry = scope_->Lookup(definition.variable.name);
  if (scope_->Define(definition.variable.name, entry)) {
    if (previous_entry) {
      context_->Warning(definition.location)
          << "Definition of " << util::Detail(definition.variable.name)
          << " shadows an existing definition.";
      context_->Note(previous_entry->location)
          << util::Detail(definition.variable.name)
          << " was previously declared here.";
    }
  } else {
    context_->Error(definition.location)
        << "Redefinition of variable " << util::Detail(definition.variable.name)
        << ".";
    context_->Note(previous_entry->location)
        << util::Detail(definition.variable.name)
        << " was previously declared here.";
  }
  return copy;
}

ast::Assign Statement::Check(const ast::Assign& assignment) const {
  auto value_copy = analysis::Check(assignment.value, context_, scope_);
  auto value_type = GetMeta(value_copy).type;
  auto* entry = scope_->Lookup(assignment.variable.name);
  if (entry == nullptr) {
    context_->Error(assignment.location)
        << "Assignment to undefined variable "
        << util::Detail(assignment.variable.name) << ". Did you mean to write "
        << util::Detail("let") << "?";
    // Assume a definition was intended.
    scope_->Define(assignment.variable.name,
                   Scope::Entry{assignment.location, value_type});
    entry = scope_->Lookup(assignment.variable.name);
  }
  if (entry->type.has_value() && value_type.has_value() &&
      !(*entry->type == *value_type)) {
    context_->Error(assignment.location)
        << "Type mismatch in assignment: "
        << util::Detail(assignment.variable.name) << " has type "
        << util::Detail(*entry->type) << ", but expression yields type "
        << util::Detail(*value_type) << ".";
    context_->Note(entry->location)
        << util::Detail(assignment.variable.name) << " is declared here.";
  }
  return assignment;
}

ast::DoFunction Statement::Check(const ast::DoFunction& do_function) const {
  auto copy = do_function;
  copy.function_call = analysis::Check(do_function.function_call, context_, scope_);
  auto type = copy.function_call.type;
  if (type.has_value() && !(*type == ast::Void{})) {
    context_->Warning(copy.location)
        << "Discarding return value of type " << util::Detail(*type)
        << " in call to " << util::Detail(copy.function_call.function.name)
        << ".";
  }
  return copy;
}

ast::If Statement::Check(const ast::If& if_statement) const {
  auto copy = if_statement;
  copy.condition = analysis::Check(if_statement.condition, context_, scope_);
  auto condition_type = GetMeta(copy.condition).type;
  if (condition_type.has_value() &&
      !(*condition_type == ast::Primitive::BOOLEAN)) {
    context_->Error(GetMeta(copy.condition).location)
        << "Condition for if statement has type "
        << util::Detail(*condition_type) << ", not "
        << util::Detail(ast::Primitive::BOOLEAN) << ".";
  }
  Scope true_scope{scope_};
  Statement true_checker{context_, &true_scope};
  copy.if_true = true_checker.Check(if_statement.if_true);
  Scope false_scope{scope_};
  Statement false_checker{context_, &false_scope};
  copy.if_false = false_checker.Check(if_statement.if_false);
  return copy;
}

ast::While Statement::Check(const ast::While& while_statement) const {
  auto copy = while_statement;
  copy.condition =
      analysis::Check(while_statement.condition, context_, scope_);
  auto condition_type = GetMeta(copy.condition).type;
  if (condition_type.has_value() &&
      !(*condition_type == ast::Primitive::BOOLEAN)) {
    context_->Error(GetMeta(copy.condition).location)
        << "Condition for while statement has type "
        << util::Detail(*condition_type) << ", not "
        << util::Detail(ast::Primitive::BOOLEAN) << ".";
  }
  Scope body_scope{scope_};
  Statement body_checker{context_, &body_scope};
  copy.body = body_checker.Check(while_statement.body);
  return copy;
}

ast::ReturnVoid Statement::Check(
    const ast::ReturnVoid& return_statement) const {
  return return_statement;
}

ast::Return Statement::Check(const ast::Return& return_statement) const {
  auto copy = return_statement;
  copy.value = analysis::Check(return_statement.value, context_, scope_);
  return copy;
}

std::vector<ast::Statement> Statement::Check(
    const std::vector<ast::Statement>& statements) const {
  std::vector<ast::Statement> result;
  for (const auto& statement : statements)
    result.push_back(analysis::Check(context_, scope_, statement));
  return result;
}

ast::Statement Check(Context* context, Scope* scope,
                     const ast::Statement& statement) {
  Statement checker{context, scope};
  statement.Visit(checker);
  return checker.result();
}

TopLevel::TopLevel(Context* context, Scope* scope)
    : context_(context), scope_(scope) {}

ast::DefineFunction TopLevel::Check(
    const ast::DefineFunction& definition) const {
  // For now, treat all input/outputs as integers.
  auto copy = definition;
  for (auto& parameter : copy.parameters) {
    parameter.type = ast::Primitive::INTEGER;
  }
  ast::Type type = ast::Function{ast::Primitive::INTEGER, copy.parameters};
  copy.function.type = type;
  if (!scope_->Define(definition.function.name,
                      Scope::Entry{definition.location, type})) {
    context_->Error(definition.location)
        << "Redefinition of name " << util::Detail(definition.function.name)
        << ".";
    auto* previous_entry = scope_->Lookup(definition.function.name);
    context_->Note(previous_entry->location)
        << util::Detail(definition.function.name)
        << " previously declared here.";
  }
  Scope function_scope{scope_};
  for (const auto& parameter : copy.parameters) {
    if (!function_scope.Define(
            parameter.name, Scope::Entry{parameter.location, parameter.type})) {
      context_->Error(parameter.location)
          << "Multiple parameters called " << util::Detail(parameter.name)
          << ".";
      auto* previous_entry = function_scope.Lookup(parameter.name);
      context_->Note(previous_entry->location)
          << "Previous definition is here.";
    }
  }
  Statement checker{context_, &function_scope};
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

ast::TopLevel Check(Context* context, Scope* scope,
                    const ast::TopLevel& top_level) {
  TopLevel checker{context, scope};
  top_level.Visit(checker);
  return checker.result();
}

}  // namespace analysis
