#include "nova/Frontend/TypeChecker.h"
#include <algorithm>
#include <sstream>

namespace nova {

TypePtr TypeChecker::makeType(Type::Kind kind, const std::string& name) {
    return std::make_shared<Type>(kind, name);
}

std::string TypeChecker::typeName(const TypePtr& type) {
    if (!type) return "any";
    if (!type->name.empty()) return type->name;
    switch (type->kind) {
        case Type::Kind::Void: return "void";
        case Type::Kind::Any: return "any";
        case Type::Kind::Unknown: return "unknown";
        case Type::Kind::Never: return "never";
        case Type::Kind::Number: return "number";
        case Type::Kind::String: return "string";
        case Type::Kind::Boolean: return "boolean";
        case Type::Kind::BigInt: return "bigint";
        case Type::Kind::Symbol: return "symbol";
        case Type::Kind::Null: return "null";
        case Type::Kind::Undefined: return "undefined";
        case Type::Kind::Object: return "object";
        case Type::Kind::Array:
            return type->elementType ? typeName(type->elementType) + "[]" : "array";
        case Type::Kind::Function: return "function";
        case Type::Kind::Union:
        case Type::Kind::Intersection:
        case Type::Kind::Tuple: {
            const char* separator = type->kind == Type::Kind::Union ? " | "
                : type->kind == Type::Kind::Intersection ? " & " : ", ";
            std::string result = type->kind == Type::Kind::Tuple ? "[" : "";
            for (size_t index = 0; index < type->types.size(); ++index) {
                if (index > 0) result += separator;
                result += typeName(type->types[index]);
            }
            if (type->kind == Type::Kind::Tuple) result += "]";
            return result;
        }
        case Type::Kind::Literal: return "literal";
        case Type::Kind::TypeParameter: return "type parameter";
        case Type::Kind::IndexedAccess: return "indexed access";
    }
    return "unknown";
}

void TypeChecker::pushScope() { scopes_.emplace_back(); }
void TypeChecker::popScope() { scopes_.pop_back(); }

void TypeChecker::bind(const std::string& name, TypePtr type) {
    if (scopes_.empty()) pushScope();
    scopes_.back()[name] = type ? std::move(type) : makeType(Type::Kind::Any);
}

TypePtr TypeChecker::lookup(const std::string& name) const {
    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
        auto found = scope->find(name);
        if (found != scope->end()) return found->second;
    }
    return makeType(Type::Kind::Any);
}

TypePtr TypeChecker::resolveType(const TypePtr& type) const {
    TypePtr current = type;
    std::unordered_set<std::string> visited;
    while (current && current->kind == Type::Kind::Object &&
           !current->name.empty() && current->properties.empty()) {
        if (!visited.insert(current->name).second) break;
        auto found = namedTypes_.find(current->name);
        if (found == namedTypes_.end() || !found->second ||
            found->second.get() == current.get()) break;
        current = found->second;
    }
    return current;
}

TypePtr TypeChecker::propertyType(const TypePtr& object,
                                  const std::string& property) const {
    TypePtr resolved = resolveType(object);
    if (!resolved) return nullptr;
    if ((resolved->kind == Type::Kind::Array || resolved->kind == Type::Kind::String) &&
        property == "length") {
        return makeType(Type::Kind::Number);
    }
    if (resolved->kind == Type::Kind::Union ||
        resolved->kind == Type::Kind::Intersection) {
        std::vector<TypePtr> members;
        for (const auto& type : resolved->types) {
            TypePtr found = propertyType(type, property);
            if (!found && resolved->kind == Type::Kind::Union) return nullptr;
            if (found) members.push_back(found);
        }
        if (members.empty()) return nullptr;
        if (members.size() == 1) return members.front();
        TypePtr combined = makeType(resolved->kind == Type::Kind::Union
            ? Type::Kind::Union : Type::Kind::Intersection);
        combined->types = std::move(members);
        return combined;
    }
    if (resolved->kind != Type::Kind::Object) return nullptr;
    auto found = resolved->properties.find(property);
    if (found == resolved->properties.end()) return nullptr;
    if (resolved->optionalProperties.count(property) == 0) return found->second;
    TypePtr optional = makeType(Type::Kind::Union);
    optional->types = {found->second, makeType(Type::Kind::Undefined)};
    return optional;
}

TypePtr TypeChecker::substituteType(
    const TypePtr& type,
    const std::unordered_map<std::string, TypePtr>& substitutions) const {
    if (!type) return makeType(Type::Kind::Any);
    if (!type->name.empty()) {
        auto replacement = substitutions.find(type->name);
        if (replacement != substitutions.end()) return replacement->second;
    }

    TypePtr result = makeType(type->kind, type->name);
    if (type->elementType) {
        result->elementType = substituteType(type->elementType, substitutions);
    }
    for (const auto& member : type->types) {
        result->types.push_back(substituteType(member, substitutions));
    }
    for (const auto& [name, member] : type->properties) {
        result->properties[name] = substituteType(member, substitutions);
    }
    result->optionalProperties = type->optionalProperties;
    return result;
}

void TypeChecker::inferTypeArguments(
    const TypePtr& pattern, const TypePtr& actual,
    const std::vector<std::string>& typeParameters,
    std::unordered_map<std::string, TypePtr>& substitutions) const {
    if (!pattern || !actual) return;
    if (!pattern->name.empty() &&
        std::find(typeParameters.begin(), typeParameters.end(), pattern->name) !=
            typeParameters.end()) {
        if (substitutions.count(pattern->name) == 0) {
            substitutions[pattern->name] = actual;
        }
        return;
    }
    if (pattern->kind == Type::Kind::Array && actual->kind == Type::Kind::Array) {
        inferTypeArguments(pattern->elementType, actual->elementType,
                           typeParameters, substitutions);
    }
    const size_t memberCount = std::min(pattern->types.size(), actual->types.size());
    for (size_t index = 0; index < memberCount; ++index) {
        inferTypeArguments(pattern->types[index], actual->types[index],
                           typeParameters, substitutions);
    }
}

bool TypeChecker::isAssignable(const TypePtr& source, const TypePtr& target) const {
    TypePtr resolvedSource = resolveType(source);
    TypePtr resolvedTarget = resolveType(target);
    if (!resolvedSource || !resolvedTarget) return true;
    if (resolvedSource.get() == resolvedTarget.get() ||
        (!resolvedSource->name.empty() &&
         resolvedSource->name == resolvedTarget->name)) {
        return true;
    }
    if (resolvedSource->kind == Type::Kind::Any || resolvedTarget->kind == Type::Kind::Any ||
        resolvedTarget->kind == Type::Kind::Unknown ||
        resolvedSource->kind == Type::Kind::Never) {
        return true;
    }
    if (resolvedSource->kind == Type::Kind::Union) {
        for (const auto& member : resolvedSource->types) {
            if (!isAssignable(member, resolvedTarget)) return false;
        }
        return true;
    }
    if (resolvedTarget->kind == Type::Kind::Union) {
        for (const auto& member : resolvedTarget->types) {
            if (isAssignable(resolvedSource, member)) return true;
        }
        return false;
    }
    if (resolvedTarget->kind == Type::Kind::Intersection) {
        for (const auto& member : resolvedTarget->types) {
            if (!isAssignable(resolvedSource, member)) return false;
        }
        return true;
    }
    if (resolvedSource->kind == Type::Kind::Array &&
        resolvedTarget->kind == Type::Kind::Array) {
        return !resolvedSource->elementType || !resolvedTarget->elementType ||
            isAssignable(resolvedSource->elementType, resolvedTarget->elementType);
    }
    if (resolvedTarget->kind == Type::Kind::Object &&
        !resolvedTarget->properties.empty()) {
        if (resolvedSource->kind != Type::Kind::Object &&
            resolvedSource->kind != Type::Kind::Intersection) return false;
        for (const auto& [name, requiredType] : resolvedTarget->properties) {
            TypePtr actualType;
            if (resolvedSource->kind == Type::Kind::Object) {
                auto actual = resolvedSource->properties.find(name);
                if (actual != resolvedSource->properties.end()) {
                    actualType = actual->second;
                }
            } else {
                actualType = propertyType(resolvedSource, name);
            }
            if (!actualType) {
                if (resolvedTarget->optionalProperties.count(name) != 0) continue;
                return false;
            }
            if (!isAssignable(actualType, requiredType)) return false;
        }
        return true;
    }
    if (resolvedSource->kind == Type::Kind::Function &&
        resolvedTarget->kind == Type::Kind::Function) {
        if (!resolvedTarget->types.empty() &&
            resolvedSource->types.size() != resolvedTarget->types.size()) return false;
        const size_t count = std::min(resolvedSource->types.size(),
                                      resolvedTarget->types.size());
        for (size_t index = 0; index < count; ++index) {
            if (!isAssignable(resolvedTarget->types[index],
                              resolvedSource->types[index])) return false;
        }
        return !resolvedTarget->elementType || !resolvedSource->elementType ||
            isAssignable(resolvedSource->elementType, resolvedTarget->elementType);
    }
    return resolvedSource->kind == resolvedTarget->kind;
}

void TypeChecker::report(const ASTNode& node, const std::string& code,
                         const std::string& message) {
    std::ostringstream output;
    if (!node.location.filename.empty()) output << node.location.filename;
    else output << "<input>";
    output << ':' << node.location.line << ':' << node.location.column
           << ": error " << code << ": " << message;
    diagnostics_.push_back(output.str());
}

TypePtr TypeChecker::inferExpression(Expr* expression) {
    if (!expression) return makeType(Type::Kind::Undefined);
    if (dynamic_cast<NumberLiteral*>(expression)) return makeType(Type::Kind::Number);
    if (dynamic_cast<BigIntLiteral*>(expression)) return makeType(Type::Kind::BigInt);
    if (dynamic_cast<StringLiteral*>(expression) ||
        dynamic_cast<TemplateLiteralExpr*>(expression)) return makeType(Type::Kind::String);
    if (dynamic_cast<BooleanLiteral*>(expression)) return makeType(Type::Kind::Boolean);
    if (dynamic_cast<NullLiteral*>(expression)) return makeType(Type::Kind::Null);
    if (dynamic_cast<UndefinedLiteral*>(expression)) return makeType(Type::Kind::Undefined);
    if (auto* array = dynamic_cast<ArrayExpr*>(expression)) {
        TypePtr result = makeType(Type::Kind::Array);
        for (auto& element : array->elements) {
            TypePtr elementType = inferExpression(element.get());
            if (!result->elementType) result->elementType = elementType;
            else if (result->elementType->kind != elementType->kind) {
                result->elementType = makeType(Type::Kind::Any);
            }
        }
        return result;
    }
    if (auto* object = dynamic_cast<ObjectExpr*>(expression)) {
        TypePtr result = makeType(Type::Kind::Object);
        for (auto& property : object->properties) {
            TypePtr valueType = inferExpression(property.value.get());
            if (auto* spread = dynamic_cast<SpreadExpr*>(property.value.get())) {
                TypePtr spreadType = resolveType(inferExpression(spread->argument.get()));
                if (spreadType && spreadType->kind == Type::Kind::Object) {
                    for (const auto& [name, type] : spreadType->properties) {
                        result->properties[name] = type;
                    }
                }
                continue;
            }
            std::string key;
            if (auto* identifier = dynamic_cast<Identifier*>(property.key.get())) {
                key = identifier->name;
            } else if (auto* string = dynamic_cast<StringLiteral*>(property.key.get())) {
                key = string->value;
            } else if (auto* number = dynamic_cast<NumberLiteral*>(property.key.get())) {
                key = number->raw.empty() ? std::to_string(number->value) : number->raw;
            }
            if (!key.empty()) result->properties[key] = valueType;
        }
        return result;
    }
    if (dynamic_cast<NewExpr*>(expression) ||
        dynamic_cast<RegexLiteralExpr*>(expression)) return makeType(Type::Kind::Object);
    if (auto* arrow = dynamic_cast<ArrowFunctionExpr*>(expression)) {
        TypePtr result = makeType(Type::Kind::Function);
        result->types = arrow->paramTypes;
        while (result->types.size() < arrow->params.size()) {
            result->types.push_back(makeType(Type::Kind::Any));
        }
        result->elementType = arrow->returnType ? arrow->returnType : makeType(Type::Kind::Any);
        return result;
    }
    if (auto* function = dynamic_cast<FunctionExpr*>(expression)) {
        TypePtr result = makeType(Type::Kind::Function);
        result->types = function->paramTypes;
        while (result->types.size() < function->params.size()) {
            result->types.push_back(makeType(Type::Kind::Any));
        }
        result->elementType = function->returnType
            ? function->returnType : makeType(Type::Kind::Any);
        return result;
    }
    if (auto* identifier = dynamic_cast<Identifier*>(expression)) {
        return lookup(identifier->name);
    }
    if (auto* parenthesized = dynamic_cast<ParenthesizedExpr*>(expression)) {
        return inferExpression(parenthesized->expression.get());
    }
    if (auto* assertion = dynamic_cast<AsExpr*>(expression)) {
        inferExpression(assertion->expression.get());
        return assertion->targetType ? assertion->targetType : makeType(Type::Kind::Any);
    }
    if (auto* satisfies = dynamic_cast<SatisfiesExpr*>(expression)) {
        TypePtr source = inferExpression(satisfies->expression.get());
        if (!isAssignable(source, satisfies->targetType)) {
            report(*satisfies, "TS1360", "Type '" + typeName(source) +
                "' does not satisfy the expected type '" +
                typeName(satisfies->targetType) + "'.");
        }
        return source;
    }
    if (auto* nonNull = dynamic_cast<NonNullExpr*>(expression)) {
        return inferExpression(nonNull->expression.get());
    }
    if (auto* unary = dynamic_cast<UnaryExpr*>(expression)) {
        TypePtr operand = inferExpression(unary->operand.get());
        if (unary->op == UnaryExpr::Op::Not) return makeType(Type::Kind::Boolean);
        if (unary->op == UnaryExpr::Op::Typeof) return makeType(Type::Kind::String);
        if (unary->op == UnaryExpr::Op::Void) return makeType(Type::Kind::Undefined);
        if (unary->op == UnaryExpr::Op::Delete) return makeType(Type::Kind::Boolean);
        if (unary->op == UnaryExpr::Op::Await) return makeType(Type::Kind::Any);
        if (operand->kind == Type::Kind::Symbol ||
            (unary->op == UnaryExpr::Op::Plus && operand->kind == Type::Kind::BigInt)) {
            report(*unary, "TS2469", "The operator cannot be applied to type '" +
                typeName(operand) + "'.");
            return makeType(Type::Kind::Any);
        }
        if (operand->kind == Type::Kind::BigInt) return makeType(Type::Kind::BigInt);
        return makeType(Type::Kind::Number);
    }
    if (auto* update = dynamic_cast<UpdateExpr*>(expression)) {
        TypePtr operand = inferExpression(update->argument.get());
        if (operand->kind == Type::Kind::Symbol) {
            report(*update, "TS2356", "An arithmetic operand must be of type 'number' or 'bigint'.");
            return makeType(Type::Kind::Any);
        }
        return operand->kind == Type::Kind::BigInt
            ? makeType(Type::Kind::BigInt) : makeType(Type::Kind::Number);
    }
    if (auto* binary = dynamic_cast<BinaryExpr*>(expression)) {
        TypePtr left = inferExpression(binary->left.get());
        TypePtr right = inferExpression(binary->right.get());
        using Op = BinaryExpr::Op;
        switch (binary->op) {
            case Op::Equal: case Op::NotEqual: case Op::StrictEqual:
            case Op::StrictNotEqual: case Op::In: case Op::Instanceof:
                return makeType(Type::Kind::Boolean);
            case Op::Less: case Op::Greater: case Op::LessEqual: case Op::GreaterEqual:
                if (left->kind == Type::Kind::Symbol || right->kind == Type::Kind::Symbol) {
                    report(*binary, "TS2469", "The relational operator cannot be applied to type 'symbol'.");
                }
                return makeType(Type::Kind::Boolean);
            case Op::LogicalAnd: case Op::LogicalOr: case Op::NullishCoalescing:
                return left->kind == right->kind ? left : makeType(Type::Kind::Any);
            default: break;
        }

        if (left->kind == Type::Kind::Symbol || right->kind == Type::Kind::Symbol) {
            report(*binary, "TS2469", "The operator cannot be applied to type 'symbol'.");
            return makeType(Type::Kind::Any);
        }
        const bool leftBigInt = left->kind == Type::Kind::BigInt;
        const bool rightBigInt = right->kind == Type::Kind::BigInt;
        if (binary->op == Op::UnsignedRightShift && (leftBigInt || rightBigInt)) {
            report(*binary, "TS2365", "Operator '>>>' cannot be applied to types '" +
                typeName(left) + "' and '" + typeName(right) + "'.");
            return makeType(Type::Kind::Any);
        }
        if (leftBigInt != rightBigInt) {
            report(*binary, "TS2365", "Operator cannot be applied to types '" +
                typeName(left) + "' and '" + typeName(right) + "'.");
            return makeType(Type::Kind::Any);
        }
        if (leftBigInt) return makeType(Type::Kind::BigInt);
        if (binary->op == Op::Add &&
            (left->kind == Type::Kind::String || right->kind == Type::Kind::String)) {
            return makeType(Type::Kind::String);
        }
        return makeType(Type::Kind::Number);
    }
    if (auto* conditional = dynamic_cast<ConditionalExpr*>(expression)) {
        inferExpression(conditional->test.get());
        TypePtr consequent = inferExpression(conditional->consequent.get());
        TypePtr alternate = inferExpression(conditional->alternate.get());
        return consequent->kind == alternate->kind
            ? consequent : makeType(Type::Kind::Any);
    }
    if (auto* assignment = dynamic_cast<AssignmentExpr*>(expression)) {
        TypePtr value = inferExpression(assignment->right.get());
        if (assignment->pattern) return value;
        TypePtr target = inferExpression(assignment->left.get());
        if (!isAssignable(value, target)) {
            report(*assignment, "TS2322", "Type '" + typeName(value) +
                "' is not assignable to type '" + typeName(target) + "'.");
        }
        return value;
    }
    if (auto* call = dynamic_cast<CallExpr*>(expression)) {
        if (auto* identifier = dynamic_cast<Identifier*>(call->callee.get())) {
            if (identifier->name == "BigInt") {
                for (auto& argument : call->arguments) inferExpression(argument.get());
                return makeType(Type::Kind::BigInt);
            }
            if (identifier->name == "Symbol") {
                for (auto& argument : call->arguments) inferExpression(argument.get());
                return makeType(Type::Kind::Symbol);
            }
            auto signature = functions_.find(identifier->name);
            if (signature != functions_.end()) {
                if (call->arguments.size() != signature->second.parameters.size()) {
                    report(*call, "TS2554", "Expected " +
                        std::to_string(signature->second.parameters.size()) +
                        " arguments, but got " + std::to_string(call->arguments.size()) + ".");
                }
                const size_t count = std::min(call->arguments.size(),
                                              signature->second.parameters.size());
                std::vector<TypePtr> argumentTypes;
                argumentTypes.reserve(count);
                std::unordered_map<std::string, TypePtr> substitutions;
                for (size_t index = 0; index < count; ++index) {
                    TypePtr argument = inferExpression(call->arguments[index].get());
                    argumentTypes.push_back(argument);
                    inferTypeArguments(signature->second.parameters[index], argument,
                                       signature->second.typeParameters,
                                       substitutions);
                }
                for (size_t index = 0; index < count; ++index) {
                    TypePtr argument = argumentTypes[index];
                    TypePtr parameter = substituteType(
                        signature->second.parameters[index], substitutions);
                    if (!isAssignable(argument, parameter)) {
                        report(*call->arguments[index], "TS2345", "Argument of type '" +
                            typeName(argument) + "' is not assignable to parameter of type '" +
                            typeName(parameter) + "'.");
                    }
                }
                const size_t constraintCount = std::min(
                    signature->second.typeParameters.size(),
                    signature->second.typeParameterConstraints.size());
                for (size_t index = 0; index < constraintCount; ++index) {
                    const TypePtr& constraint =
                        signature->second.typeParameterConstraints[index];
                    auto inferred = substitutions.find(
                        signature->second.typeParameters[index]);
                    if (!constraint || inferred == substitutions.end()) continue;
                    TypePtr resolvedConstraint = substituteType(
                        constraint, substitutions);
                    if (!isAssignable(inferred->second, resolvedConstraint)) {
                        report(*call, "TS2345", "Type '" +
                            typeName(inferred->second) +
                            "' does not satisfy the constraint '" +
                            typeName(resolvedConstraint) + "'.");
                    }
                }
                return signature->second.returnType
                    ? substituteType(signature->second.returnType, substitutions)
                    : makeType(Type::Kind::Any);
            }
        }
        inferExpression(call->callee.get());
        for (auto& argument : call->arguments) inferExpression(argument.get());
        return makeType(Type::Kind::Any);
    }
    if (auto* sequence = dynamic_cast<SequenceExpr*>(expression)) {
        TypePtr result = makeType(Type::Kind::Undefined);
        for (auto& item : sequence->expressions) result = inferExpression(item.get());
        return result;
    }
    if (auto* member = dynamic_cast<MemberExpr*>(expression)) {
        TypePtr object = inferExpression(member->object.get());
        std::string property;
        if (auto* identifier = dynamic_cast<Identifier*>(member->property.get())) {
            property = identifier->name;
        } else if (auto* string = dynamic_cast<StringLiteral*>(member->property.get())) {
            property = string->value;
        } else if (auto* number = dynamic_cast<NumberLiteral*>(member->property.get())) {
            property = number->raw.empty() ? std::to_string(number->value) : number->raw;
        } else if (member->isComputed) {
            inferExpression(member->property.get());
        }
        if (property.empty()) return makeType(Type::Kind::Any);
        TypePtr result = propertyType(object, property);
        if (result) return result;
        TypePtr resolved = resolveType(object);
        if (resolved && resolved->kind == Type::Kind::Object &&
            (!resolved->properties.empty() || !resolved->name.empty())) {
            report(*member, "TS2339", "Property '" + property +
                "' does not exist on type '" + typeName(object) + "'.");
        }
        return makeType(Type::Kind::Any);
    }
    return makeType(Type::Kind::Any);
}

void TypeChecker::checkStatement(Stmt* statement) {
    if (!statement) return;
    if (auto* block = dynamic_cast<BlockStmt*>(statement)) {
        pushScope();
        for (auto& item : block->statements) checkStatement(item.get());
        popScope();
    } else if (auto* expression = dynamic_cast<ExprStmt*>(statement)) {
        inferExpression(expression->expression.get());
    } else if (auto* variables = dynamic_cast<VarDeclStmt*>(statement)) {
        for (auto& declarator : variables->declarations) {
            TypePtr initializer = declarator.init
                ? inferExpression(declarator.init.get()) : makeType(Type::Kind::Undefined);
            if (declarator.type && !isAssignable(initializer, declarator.type)) {
                report(*variables, "TS2322", "Type '" + typeName(initializer) +
                    "' is not assignable to type '" + typeName(declarator.type) + "'.");
            }
            if (!declarator.name.empty()) {
                bind(declarator.name, declarator.type ? declarator.type : initializer);
            }
        }
    } else if (auto* declaration = dynamic_cast<DeclStmt*>(statement)) {
        checkDeclaration(declaration->declaration.get());
    } else if (auto* returnStatement = dynamic_cast<ReturnStmt*>(statement)) {
        TypePtr actual = returnStatement->argument
            ? inferExpression(returnStatement->argument.get()) : makeType(Type::Kind::Void);
        if (expectedReturnType_ && !isAssignable(actual, expectedReturnType_)) {
            report(*returnStatement, "TS2322", "Type '" + typeName(actual) +
                "' is not assignable to return type '" + typeName(expectedReturnType_) + "'.");
        }
    } else if (auto* conditional = dynamic_cast<IfStmt*>(statement)) {
        inferExpression(conditional->test.get());
        checkStatement(conditional->consequent.get());
        checkStatement(conditional->alternate.get());
    } else if (auto* whileLoop = dynamic_cast<WhileStmt*>(statement)) {
        inferExpression(whileLoop->test.get()); checkStatement(whileLoop->body.get());
    } else if (auto* doWhileLoop = dynamic_cast<DoWhileStmt*>(statement)) {
        checkStatement(doWhileLoop->body.get()); inferExpression(doWhileLoop->test.get());
    } else if (auto* forLoop = dynamic_cast<ForStmt*>(statement)) {
        pushScope(); checkStatement(forLoop->init.get()); inferExpression(forLoop->test.get());
        inferExpression(forLoop->update.get()); checkStatement(forLoop->body.get()); popScope();
    } else if (auto* thrown = dynamic_cast<ThrowStmt*>(statement)) {
        inferExpression(thrown->argument.get());
    } else if (auto* labeled = dynamic_cast<LabeledStmt*>(statement)) {
        checkStatement(labeled->statement.get());
    }
}

void TypeChecker::checkFunction(FunctionDecl& function) {
    TypePtr savedReturn = expectedReturnType_;
    expectedReturnType_ = function.returnType;
    std::unordered_map<std::string, TypePtr> savedTypeParameters;
    for (size_t index = 0; index < function.typeParams.size(); ++index) {
        const std::string& name = function.typeParams[index];
        auto existing = namedTypes_.find(name);
        if (existing != namedTypes_.end()) savedTypeParameters[name] = existing->second;
        TypePtr constraint = index < function.typeParamConstraints.size()
            ? function.typeParamConstraints[index] : nullptr;
        namedTypes_[name] = constraint ? constraint : makeType(Type::Kind::Any);
    }
    pushScope();
    for (size_t index = 0; index < function.params.size(); ++index) {
        TypePtr parameter = index < function.paramTypes.size() && function.paramTypes[index]
            ? function.paramTypes[index] : makeType(Type::Kind::Any);
        bind(function.params[index], parameter);
    }
    checkStatement(function.body.get());
    popScope();
    for (const std::string& name : function.typeParams) {
        auto saved = savedTypeParameters.find(name);
        if (saved != savedTypeParameters.end()) namedTypes_[name] = saved->second;
        else namedTypes_.erase(name);
    }
    expectedReturnType_ = savedReturn;
}

void TypeChecker::checkDeclaration(Decl* declaration) {
    if (!declaration) return;
    if (auto* function = dynamic_cast<FunctionDecl*>(declaration)) {
        checkFunction(*function);
    } else if (auto* exported = dynamic_cast<ExportDecl*>(declaration)) {
        checkDeclaration(exported->exportedDecl.get());
        checkStatement(exported->exportedStmt.get());
        inferExpression(exported->declaration.get());
    }
}

bool TypeChecker::check(Program& program) {
    diagnostics_.clear(); scopes_.clear(); functions_.clear(); namedTypes_.clear();
    expectedReturnType_.reset();
    pushScope();

    std::vector<Decl*> declarations;
    for (auto& statement : program.body) {
        auto* declarationStatement = dynamic_cast<DeclStmt*>(statement.get());
        Decl* declaration = declarationStatement
            ? declarationStatement->declaration.get() : nullptr;
        if (auto* exported = dynamic_cast<ExportDecl*>(declaration)) {
            if (exported->exportedDecl) declarations.push_back(exported->exportedDecl.get());
        } else if (declaration) {
            declarations.push_back(declaration);
        }
    }

    // Create interface identities first so recursive and forward references resolve.
    for (Decl* declaration : declarations) {
        if (auto* interface = dynamic_cast<InterfaceDecl*>(declaration)) {
            namedTypes_[interface->name] = makeType(Type::Kind::Object, interface->name);
        }
    }
    for (Decl* declaration : declarations) {
        if (auto* alias = dynamic_cast<TypeAliasDecl*>(declaration)) {
            namedTypes_[alias->name] = alias->type;
        }
    }
    for (Decl* declaration : declarations) {
        auto* interface = dynamic_cast<InterfaceDecl*>(declaration);
        if (!interface) continue;
        TypePtr type = namedTypes_[interface->name];
        for (const auto& property : interface->properties) {
            type->properties[property.name] = property.type
                ? property.type : makeType(Type::Kind::Any);
            if (property.isOptional) type->optionalProperties.insert(property.name);
        }
        for (const auto& method : interface->methods) {
            TypePtr methodType = makeType(Type::Kind::Function);
            methodType->types.resize(method.params.size(), makeType(Type::Kind::Any));
            methodType->elementType = method.returnType
                ? method.returnType : makeType(Type::Kind::Any);
            type->properties[method.name] = methodType;
        }
    }
    // Merge inherited members after every interface has its own members.
    for (size_t pass = 0; pass < declarations.size(); ++pass) {
        for (Decl* declaration : declarations) {
            auto* interface = dynamic_cast<InterfaceDecl*>(declaration);
            if (!interface) continue;
            TypePtr type = namedTypes_[interface->name];
            for (const std::string& parentName : interface->extends) {
                auto parent = namedTypes_.find(parentName);
                if (parent == namedTypes_.end()) continue;
                TypePtr parentType = resolveType(parent->second);
                if (!parentType || parentType->kind != Type::Kind::Object) continue;
                for (const auto& [name, memberType] : parentType->properties) {
                    if (type->properties.count(name) == 0) type->properties[name] = memberType;
                }
                type->optionalProperties.insert(parentType->optionalProperties.begin(),
                                                parentType->optionalProperties.end());
            }
        }
    }

    for (Decl* declaration : declarations) {
        auto* function = dynamic_cast<FunctionDecl*>(declaration);
        if (!function) continue;
        FunctionSignature signature;
        signature.typeParameters = function->typeParams;
        signature.typeParameterConstraints = function->typeParamConstraints;
        signature.parameters = function->paramTypes;
        while (signature.parameters.size() < function->params.size()) {
            signature.parameters.push_back(makeType(Type::Kind::Any));
        }
        signature.returnType = function->returnType
            ? function->returnType : makeType(Type::Kind::Any);
        functions_[function->name] = signature;
        bind(function->name, makeType(Type::Kind::Function));
    }
    for (auto& statement : program.body) checkStatement(statement.get());
    popScope();
    return diagnostics_.empty();
}

} // namespace nova
