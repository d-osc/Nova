#include "nova/Frontend/TypeChecker.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>
#include <sstream>

namespace nova {

TypeChecker::TypeChecker(TypeCheckerOptions opts) : options_(std::move(opts)) {}

// Parse TypeScript test-file compiler-option directives (`// @name` or
// `// @name: value`) from the leading comment lines of a source file, and
// return a TypeCheckerOptions with the corresponding flags set. Only comment
// lines at the top of the file (before the first non-comment, non-blank line)
// are examined, matching the upstream harness behaviour. `// @strict` is an
// umbrella that enables the full strict family (mirroring tsconfig `strict`).
TypeCheckerOptions parseCompilerDirectives(const std::string& source) {
    TypeCheckerOptions opts;
    auto setFlag = [&](const std::string& name, bool value) {
        if (name == "strict") {
            if (value) {
                opts.strict = true;
                opts.noImplicitAny = true;
                opts.strictNullChecks = true;
                opts.strictFunctionTypes = true;
                opts.strictBindCallApply = true;
                opts.strictPropertyInitialization = true;
                opts.noImplicitThis = true;
                opts.useUnknownInCatchVariables = true;
                opts.alwaysStrict = true;
            }
        } else if (name == "noImplicitAny") { opts.noImplicitAny = value; }
        else if (name == "strictNullChecks") { opts.strictNullChecks = value; }
        else if (name == "strictFunctionTypes") { opts.strictFunctionTypes = value; }
        else if (name == "strictBindCallApply") { opts.strictBindCallApply = value; }
        else if (name == "strictPropertyInitialization") { opts.strictPropertyInitialization = value; }
        else if (name == "noImplicitThis") { opts.noImplicitThis = value; }
        else if (name == "useUnknownInCatchVariables") { opts.useUnknownInCatchVariables = value; }
        else if (name == "alwaysStrict") { opts.alwaysStrict = value; }
        else if (name == "noUnusedLocals") { opts.noUnusedLocals = value; }
        else if (name == "noUnusedParameters") { opts.noUnusedParameters = value; }
        else if (name == "exactOptionalPropertyTypes") { opts.exactOptionalPropertyTypes = value; }
        else if (name == "noImplicitReturns") { opts.noImplicitReturns = value; }
        else if (name == "noFallthroughCasesInSwitch") { opts.noFallthroughCasesInSwitch = value; }
        else if (name == "noUncheckedIndexedAccess") { opts.noUncheckedIndexedAccess = value; }
    };

    std::istringstream stream(source);
    std::string line;
    while (std::getline(stream, line)) {
        // Trim leading whitespace.
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;  // blank line — keep scanning
        // Must be a line comment.
        if (line.compare(start, 2, "//") != 0) break;
        // Extract content after "//".
        size_t content = start + 2;
        size_t at = line.find('@', content);
        if (at == std::string::npos) continue;  // non-directive comment — keep scanning
        size_t nameStart = at + 1;
        size_t nameEnd = nameStart;
        while (nameEnd < line.size() &&
               (std::isalnum(static_cast<unsigned char>(line[nameEnd])) ||
                line[nameEnd] == '_')) {
            ++nameEnd;
        }
        if (nameEnd == nameStart) continue;
        std::string name = line.substr(nameStart, nameEnd - nameStart);
        // Value: optional ": value" or just the bare flag (true).
        std::string valuePart =
            line.substr(nameEnd);
        // Trim whitespace and a leading colon.
        size_t colon = valuePart.find(':');
        std::string valueStr;
        if (colon != std::string::npos) {
            valueStr = valuePart.substr(colon + 1);
        }
        // Trim whitespace from valueStr.
        size_t vs = valueStr.find_first_not_of(" \t");
        size_t ve = valueStr.find_last_not_of(" \t\r\n");
        if (vs != std::string::npos && ve != std::string::npos) {
            valueStr = valueStr.substr(vs, ve - vs + 1);
        } else {
            valueStr.clear();
        }
        // Most directives are boolean (presence = true). A value of "false"
        // explicitly disables.
        bool value = true;
        if (valueStr == "false") value = false;
        setFlag(name, value);
    }
    return opts;
}

TypePtr TypeChecker::makeType(Type::Kind kind, const std::string& name) {
    return std::make_shared<Type>(kind, name);
}

std::string TypeChecker::typeName(const TypePtr& type) {
    if (!type) return "any";
    if (type->kind == Type::Kind::Literal && type->elementType) {
        return typeName(type->elementType);
    }
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
        case Type::Kind::Keyof: return "keyof";
        case Type::Kind::Conditional: return "conditional type";
        case Type::Kind::Mapped: return "mapped type";
        case Type::Kind::Infer: return "infer " + type->name;
        case Type::Kind::TypeQuery: return "typeof " + type->name;
        case Type::Kind::TypePredicate:
            return (type->isAssertion ? "asserts " : "") +
                type->name + (type->elementType
                    ? " is " + typeName(type->elementType) : "");
        case Type::Kind::TemplateLiteral: return "`" + type->name + "`";
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

bool TypeChecker::sameType(const TypePtr& left, const TypePtr& right) {
    if (!left || !right) return left == right;
    if (left->kind != right->kind || left->name != right->name) return false;
    if (!sameType(left->elementType, right->elementType) ||
        left->types.size() != right->types.size() ||
        left->typeArguments.size() != right->typeArguments.size()) {
        return false;
    }
    for (size_t index = 0; index < left->types.size(); ++index) {
        if (!sameType(left->types[index], right->types[index])) return false;
    }
    for (size_t index = 0; index < left->typeArguments.size(); ++index) {
        if (!sameType(left->typeArguments[index], right->typeArguments[index])) {
            return false;
        }
    }
    return true;
}

TypePtr TypeChecker::unionOf(std::vector<TypePtr> types) {
    std::vector<TypePtr> flattened;
    for (auto& type : types) {
        if (!type) continue;
        if (type->kind == Type::Kind::Never) continue;
        if (type->kind == Type::Kind::Union) {
            flattened.insert(
                flattened.end(), type->types.begin(), type->types.end());
        } else {
            flattened.push_back(type);
        }
    }
    std::vector<TypePtr> unique;
    for (auto& type : flattened) {
        if (std::none_of(unique.begin(), unique.end(),
                         [&](const TypePtr& existing) {
                             return sameType(existing, type);
                         })) {
            unique.push_back(type);
        }
    }
    if (unique.empty()) return makeType(Type::Kind::Never);
    if (unique.size() == 1) return unique.front();
    TypePtr result = makeType(Type::Kind::Union);
    result->types = std::move(unique);
    return result;
}

TypePtr TypeChecker::evaluateType(
    const TypePtr& type,
    const std::unordered_map<std::string, TypePtr>& substitutions) const {
    if (!type) return makeType(Type::Kind::Any);
    if (!type->name.empty()) {
        auto replacement = substitutions.find(type->name);
        if (replacement != substitutions.end()) {
            return evaluateType(replacement->second, substitutions);
        }
    }

    if (type->kind == Type::Kind::Object && !type->name.empty()) {
        const std::string& name = type->name;
        auto argument = [&](size_t index) -> TypePtr {
            return index < type->typeArguments.size()
                ? evaluateType(type->typeArguments[index], substitutions)
                : nullptr;
        };

        if ((name == "Array" || name == "ReadonlyArray") && argument(0)) {
            TypePtr array = makeType(Type::Kind::Array);
            array->elementType = argument(0);
            array->isReadonly = name == "ReadonlyArray" || type->isReadonly;
            return array;
        }
        if (name == "Promise" && argument(0)) {
            TypePtr promise = makeType(Type::Kind::Object, "Promise");
            promise->typeArguments = {argument(0)};
            return promise;
        }
        if ((name == "Partial" || name == "Required" ||
             name == "Readonly") && argument(0)) {
            TypePtr source = evaluateType(argument(0), substitutions);
            TypePtr result = makeType(Type::Kind::Object);
            if (source && source->kind == Type::Kind::Object) {
                result->properties = source->properties;
                result->optionalProperties = source->optionalProperties;
                result->readonlyProperties = source->readonlyProperties;
                if (name == "Partial") {
                    for (const auto& [property, ignored] : result->properties) {
                        (void)ignored;
                        result->optionalProperties.insert(property);
                    }
                } else if (name == "Required") {
                    result->optionalProperties.clear();
                } else {
                    for (const auto& [property, ignored] : result->properties) {
                        (void)ignored;
                        result->readonlyProperties.insert(property);
                    }
                }
            }
            return result;
        }
        if ((name == "Pick" || name == "Omit") && argument(0) && argument(1)) {
            TypePtr source = evaluateType(argument(0), substitutions);
            TypePtr keys = evaluateType(argument(1), substitutions);
            std::unordered_set<std::string> selected;
            const auto collect = [&](const TypePtr& candidate,
                                     auto&& collectRef) -> void {
                if (!candidate) return;
                if (candidate->kind == Type::Kind::Union) {
                    for (const auto& member : candidate->types) {
                        collectRef(member, collectRef);
                    }
                } else if (candidate->kind == Type::Kind::Literal) {
                    selected.insert(candidate->name);
                }
            };
            collect(keys, collect);
            TypePtr result = makeType(Type::Kind::Object);
            if (source && source->kind == Type::Kind::Object) {
                for (const auto& [property, member] : source->properties) {
                    const bool included = selected.count(property) != 0;
                    if ((name == "Pick" && !included) ||
                        (name == "Omit" && included)) {
                        continue;
                    }
                    result->properties[property] = member;
                    if (source->optionalProperties.count(property)) {
                        result->optionalProperties.insert(property);
                    }
                    if (source->readonlyProperties.count(property)) {
                        result->readonlyProperties.insert(property);
                    }
                }
            }
            return result;
        }
        if (name == "Record" && argument(0) && argument(1)) {
            TypePtr keys = evaluateType(argument(0), substitutions);
            TypePtr value = argument(1);
            TypePtr result = makeType(Type::Kind::Object);
            std::vector<TypePtr> keyTypes =
                keys->kind == Type::Kind::Union
                ? keys->types : std::vector<TypePtr>{keys};
            for (const auto& key : keyTypes) {
                if (key && key->kind == Type::Kind::Literal) {
                    result->properties[key->name] = value;
                }
            }
            return result;
        }
        if (name == "Extract" && argument(0) && argument(1)) {
            TypePtr source = argument(0);
            TypePtr filter = argument(1);
            std::vector<TypePtr> retained;
            std::vector<TypePtr> members = source->kind == Type::Kind::Union
                ? source->types : std::vector<TypePtr>{source};
            for (const auto& member : members) {
                if (isAssignable(member, filter)) retained.push_back(member);
            }
            return unionOf(std::move(retained));
        }
        if (name == "Capitalize" && argument(0)) {
            TypePtr source = argument(0);
            std::vector<TypePtr> members = source->kind == Type::Kind::Union
                ? source->types : std::vector<TypePtr>{source};
            std::vector<TypePtr> capitalized;
            for (const auto& member : members) {
                if (!member || member->kind != Type::Kind::Literal) continue;
                std::string text = member->name;
                if (!text.empty()) {
                    text[0] = static_cast<char>(
                        std::toupper(static_cast<unsigned char>(text[0])));
                }
                TypePtr literal = makeType(Type::Kind::Literal, text);
                literal->elementType = makeType(Type::Kind::String);
                capitalized.push_back(literal);
            }
            return unionOf(std::move(capitalized));
        }
        if (name == "ReturnType" && argument(0)) {
            TypePtr function = argument(0);
            return function && function->kind == Type::Kind::Function &&
                function->elementType
                ? evaluateType(function->elementType, substitutions)
                : makeType(Type::Kind::Unknown);
        }

        auto generic = genericTypes_.find(name);
        if (generic != genericTypes_.end()) {
            std::unordered_map<std::string, TypePtr> instantiated =
                substitutions;
            for (size_t index = 0;
                 index < generic->second.typeParameters.size(); ++index) {
                TypePtr value = index < type->typeArguments.size()
                    ? evaluateType(type->typeArguments[index], substitutions)
                    : (index < generic->second.defaults.size()
                        ? evaluateType(
                              generic->second.defaults[index], instantiated)
                        : makeType(Type::Kind::Unknown));
                instantiated[
                    generic->second.typeParameters[index]] = value;
            }
            return evaluateType(generic->second.body, instantiated);
        }
        auto named = namedTypes_.find(name);
        if (named != namedTypes_.end() && named->second.get() != type.get()) {
            return evaluateType(named->second, substitutions);
        }
    }

    if (type->kind == Type::Kind::Keyof) {
        TypePtr object = evaluateType(type->elementType, substitutions);
        std::vector<TypePtr> keys;
        if (object && object->kind == Type::Kind::Object) {
            for (const auto& [property, ignored] : object->properties) {
                (void)ignored;
                TypePtr key = makeType(Type::Kind::Literal, property);
                key->elementType = makeType(Type::Kind::String);
                keys.push_back(key);
            }
        }
        return unionOf(std::move(keys));
    }

    if (type->kind == Type::Kind::IndexedAccess) {
        TypePtr object = evaluateType(type->elementType, substitutions);
        TypePtr index = evaluateType(type->indexType, substitutions);
        std::vector<TypePtr> indexes = index && index->kind == Type::Kind::Union
            ? index->types : std::vector<TypePtr>{index};
        std::vector<TypePtr> values;
        for (const auto& key : indexes) {
            if (!key) continue;
            if (key->kind == Type::Kind::Literal) {
                TypePtr value = propertyType(object, key->name);
                if (value) values.push_back(value);
            }
        }
        return unionOf(std::move(values));
    }

    if (type->kind == Type::Kind::Conditional) {
        TypePtr check = evaluateType(type->checkType, substitutions);
        std::vector<TypePtr> checks = check && check->kind == Type::Kind::Union
            ? check->types : std::vector<TypePtr>{check};
        std::vector<TypePtr> results;
        for (const auto& member : checks) {
            std::unordered_map<std::string, TypePtr> inferred = substitutions;
            std::function<void(const TypePtr&, const TypePtr&)> matchInfer =
                [&](const TypePtr& pattern, const TypePtr& actual) {
                    if (!pattern || !actual) return;
                    if (pattern->kind == Type::Kind::Infer) {
                        inferred[pattern->name] = actual;
                        return;
                    }
                    if (pattern->kind == Type::Kind::Array &&
                        actual->kind == Type::Kind::Array) {
                        matchInfer(pattern->elementType, actual->elementType);
                    }
                    if (pattern->kind == Type::Kind::Object &&
                        actual->kind == Type::Kind::Object &&
                        pattern->name == actual->name) {
                        const size_t count = std::min(
                            pattern->typeArguments.size(),
                            actual->typeArguments.size());
                        for (size_t index = 0; index < count; ++index) {
                            matchInfer(pattern->typeArguments[index],
                                       actual->typeArguments[index]);
                        }
                    }
                };
            TypePtr constraint =
                evaluateType(type->extendsType, substitutions);
            matchInfer(type->extendsType, member);
            const bool matches = isAssignable(member, constraint) ||
                inferred.size() > substitutions.size();
            results.push_back(evaluateType(
                matches ? type->trueType : type->falseType, inferred));
        }
        return unionOf(std::move(results));
    }

    if (type->kind == Type::Kind::Mapped) {
        TypePtr keys = evaluateType(type->mappedSource, substitutions);
        std::vector<TypePtr> members = keys && keys->kind == Type::Kind::Union
            ? keys->types : std::vector<TypePtr>{keys};
        TypePtr result = makeType(Type::Kind::Object);
        for (const auto& key : members) {
            if (!key || key->kind != Type::Kind::Literal) continue;
            std::unordered_map<std::string, TypePtr> mapped = substitutions;
            mapped[type->mappedVar] = key;
            std::string property = key->name;
            if (type->mappedNameType) {
                TypePtr remapped =
                    evaluateType(type->mappedNameType, mapped);
                if (remapped && remapped->kind == Type::Kind::Literal) {
                    property = remapped->name;
                }
            }
            result->properties[property] =
                evaluateType(type->mappedValue, mapped);
            if (type->mappedOptionalModifier > 0) {
                result->optionalProperties.insert(property);
            }
            if (type->mappedReadonlyModifier > 0) {
                result->readonlyProperties.insert(property);
            }
        }
        return result;
    }

    if (type->kind == Type::Kind::TemplateLiteral) {
        const std::string& pattern = type->name;
        const auto open = pattern.find("${");
        const auto close = open == std::string::npos
            ? std::string::npos : pattern.rfind('}');
        if (open == std::string::npos || close == std::string::npos ||
            close < open + 2) {
            TypePtr literal = makeType(Type::Kind::Literal, pattern);
            literal->elementType = makeType(Type::Kind::String);
            return literal;
        }
        std::string expression =
            pattern.substr(open + 2, close - open - 2);
        bool capitalize = expression.rfind("Capitalize<", 0) == 0 &&
            expression.back() == '>';
        if (capitalize) {
            expression = expression.substr(
                std::strlen("Capitalize<"),
                expression.size() - std::strlen("Capitalize<") - 1);
        }
        const auto ampersand = expression.rfind('&');
        if (ampersand != std::string::npos) {
            expression = expression.substr(ampersand + 1);
        }
        expression.erase(
            std::remove_if(expression.begin(), expression.end(),
                           [](unsigned char character) {
                               return std::isspace(character) != 0;
                           }),
            expression.end());
        TypePtr expressionType;
        auto replacement = substitutions.find(expression);
        if (replacement != substitutions.end()) {
            expressionType = evaluateType(replacement->second, substitutions);
        } else {
            auto named = namedTypes_.find(expression);
            expressionType = named == namedTypes_.end()
                ? makeType(Type::Kind::Never)
                : evaluateType(named->second, substitutions);
        }
        std::vector<TypePtr> members =
            expressionType && expressionType->kind == Type::Kind::Union
            ? expressionType->types : std::vector<TypePtr>{expressionType};
        std::vector<TypePtr> expanded;
        for (const auto& member : members) {
            if (!member || member->kind != Type::Kind::Literal) continue;
            std::string text = member->name;
            if (capitalize && !text.empty()) {
                text[0] = static_cast<char>(
                    std::toupper(static_cast<unsigned char>(text[0])));
            }
            TypePtr literal = makeType(
                Type::Kind::Literal,
                pattern.substr(0, open) + text + pattern.substr(close + 1));
            literal->elementType = makeType(Type::Kind::String);
            expanded.push_back(literal);
        }
        return unionOf(std::move(expanded));
    }

    if (type->kind == Type::Kind::TypeQuery && !type->name.empty()) {
        auto function = functions_.find(type->name);
        if (function != functions_.end()) {
            TypePtr result = makeType(Type::Kind::Function);
            result->types = function->second.parameters;
            result->elementType = function->second.returnType;
            return result;
        }
        TypePtr value = lookup(type->name);
        return value && value->kind != Type::Kind::Any
            ? value : makeType(Type::Kind::Unknown);
    }

    TypePtr result = makeType(type->kind, type->name);
    result->elementType =
        type->elementType
        ? evaluateType(type->elementType, substitutions) : nullptr;
    result->indexType =
        type->indexType
        ? evaluateType(type->indexType, substitutions) : nullptr;
    for (const auto& member : type->types) {
        result->types.push_back(evaluateType(member, substitutions));
    }
    for (const auto& argument : type->typeArguments) {
        result->typeArguments.push_back(
            evaluateType(argument, substitutions));
    }
    for (const auto& [property, member] : type->properties) {
        result->properties[property] =
            evaluateType(member, substitutions);
    }
    result->optionalProperties = type->optionalProperties;
    result->readonlyProperties = type->readonlyProperties;
    result->isReadonly = type->isReadonly;
    return result;
}

TypePtr TypeChecker::resolveType(const TypePtr& type) const {
    return evaluateType(type);
}

TypePtr TypeChecker::propertyType(const TypePtr& object,
                                  const std::string& property) const {
    TypePtr resolved = resolveType(object);
    if (!resolved) return nullptr;
    if ((resolved->kind == Type::Kind::Array || resolved->kind == Type::Kind::String) &&
        property == "length") {
        return makeType(Type::Kind::Number);
    }
    if ((resolved->kind == Type::Kind::Number && property == "toFixed") ||
        (resolved->kind == Type::Kind::String &&
         (property == "toUpperCase" || property == "toLowerCase"))) {
        TypePtr method = makeType(Type::Kind::Function);
        method->elementType = makeType(Type::Kind::String);
        return method;
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
    for (const auto& argument : type->typeArguments) {
        result->typeArguments.push_back(
            substituteType(argument, substitutions));
    }
    for (const auto& [name, member] : type->properties) {
        result->properties[name] = substituteType(member, substitutions);
    }
    result->optionalProperties = type->optionalProperties;
    result->readonlyProperties = type->readonlyProperties;
    result->indexType = substituteType(type->indexType, substitutions);
    result->checkType = substituteType(type->checkType, substitutions);
    result->extendsType = substituteType(type->extendsType, substitutions);
    result->trueType = substituteType(type->trueType, substitutions);
    result->falseType = substituteType(type->falseType, substitutions);
    result->mappedSource =
        substituteType(type->mappedSource, substitutions);
    result->mappedValue =
        substituteType(type->mappedValue, substitutions);
    result->mappedNameType =
        substituteType(type->mappedNameType, substitutions);
    result->mappedVar = type->mappedVar;
    result->mappedReadonlyModifier = type->mappedReadonlyModifier;
    result->mappedOptionalModifier = type->mappedOptionalModifier;
    result->isReadonly = type->isReadonly;
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
    // strictNullChecks: `null` and `undefined` are only assignable to types
    // that explicitly include them (Any/Unknown/Never above, or a union member
    // via the target-union branch below). Without this flag they are loosely
    // assignable to everything (the historical behaviour). Defer the rejection
    // until AFTER the union branches so a union target containing null/undefined
    // is handled correctly first.
    if (resolvedTarget->kind == Type::Kind::Never) {
        return resolvedSource->kind == Type::Kind::Never;
    }
    if (resolvedSource->kind == Type::Kind::Unknown) {
        return resolvedTarget->kind == Type::Kind::Unknown;
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
    // strictNullChecks (reached after union handling): by this point the target
    // is not Any/Unknown/Never and not a union, so a null/undefined source is
    // only assignable if the target is literally null/undefined. Without the
    // flag, null/undefined are loosely assignable (historical behaviour).
    if (options_.strictNullChecks &&
        (resolvedSource->kind == Type::Kind::Null ||
         resolvedSource->kind == Type::Kind::Undefined) &&
        resolvedTarget->kind != resolvedSource->kind) {
        return false;
    }
    if (resolvedSource->kind == Type::Kind::Literal) {
        if (resolvedTarget->kind == Type::Kind::Literal) {
            return resolvedSource->name == resolvedTarget->name;
        }
        if (resolvedSource->elementType) {
            return isAssignable(resolvedSource->elementType, resolvedTarget);
        }
    }
    if (resolvedTarget->kind == Type::Kind::Literal) {
        return resolvedSource->kind == Type::Kind::Literal &&
            resolvedSource->name == resolvedTarget->name;
    }
    if (resolvedSource->kind == Type::Kind::TypeParameter ||
        resolvedTarget->kind == Type::Kind::TypeParameter) {
        return resolvedSource->kind == resolvedTarget->kind &&
            resolvedSource->name == resolvedTarget->name;
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

TypePtr TypeChecker::narrowTypeForCondition(
    const TypePtr& original, const Expr* condition,
    const std::string& variable, bool whenTrue) const {
    TypePtr resolved = resolveType(original);
    if (!resolved || !condition) return resolved;
    std::vector<TypePtr> candidates = resolved->kind == Type::Kind::Union
        ? resolved->types : std::vector<TypePtr>{resolved};
    std::function<bool(const TypePtr&)> matches;

    if (auto* binary = dynamic_cast<const BinaryExpr*>(condition)) {
        using Op = BinaryExpr::Op;
        const bool equality =
            binary->op == Op::Equal || binary->op == Op::StrictEqual;
        const bool inequality =
            binary->op == Op::NotEqual || binary->op == Op::StrictNotEqual;
        if (binary->op == Op::LogicalAnd && whenTrue) {
            return narrowTypeForCondition(
                narrowTypeForCondition(original, binary->left.get(),
                                       variable, true),
                binary->right.get(), variable, true);
        }
        if (equality || inequality) {
            const bool positive = equality ? whenTrue : !whenTrue;
            const auto* member =
                dynamic_cast<const MemberExpr*>(binary->left.get());
            const auto* literal =
                dynamic_cast<const StringLiteral*>(binary->right.get());
            if (member && literal) {
                const auto* base =
                    dynamic_cast<const Identifier*>(member->object.get());
                const auto* property =
                    dynamic_cast<const Identifier*>(member->property.get());
                if (base && property && base->name == variable) {
                    const std::string expected = literal->value;
                    const std::string propertyName = property->name;
                    matches = [=, this](const TypePtr& candidate) {
                        TypePtr value =
                            propertyType(candidate, propertyName);
                        return value && value->kind == Type::Kind::Literal &&
                            value->name == expected;
                    };
                    whenTrue = positive;
                }
            }
            const auto* identifier =
                dynamic_cast<const Identifier*>(binary->left.get());
            if (!matches && identifier && identifier->name == variable &&
                dynamic_cast<const NullLiteral*>(binary->right.get())) {
                matches = [](const TypePtr& candidate) {
                    return candidate &&
                        candidate->kind == Type::Kind::Null;
                };
                whenTrue = positive;
            }
            const auto* unary =
                dynamic_cast<const UnaryExpr*>(binary->left.get());
            if (!matches && unary && literal &&
                unary->op == UnaryExpr::Op::Typeof) {
                const auto* operand =
                    dynamic_cast<const Identifier*>(unary->operand.get());
                if (operand && operand->name == variable) {
                    Type::Kind expectedKind = Type::Kind::Unknown;
                    if (literal->value == "string") {
                        expectedKind = Type::Kind::String;
                    } else if (literal->value == "number") {
                        expectedKind = Type::Kind::Number;
                    } else if (literal->value == "boolean") {
                        expectedKind = Type::Kind::Boolean;
                    }
                    matches = [expectedKind](const TypePtr& candidate) {
                        if (!candidate) return false;
                        if (candidate->kind == expectedKind) return true;
                        return candidate->kind == Type::Kind::Literal &&
                            candidate->elementType &&
                            candidate->elementType->kind == expectedKind;
                    };
                    whenTrue = positive;
                }
            }
        } else if (binary->op == Op::In) {
            const auto* property =
                dynamic_cast<const StringLiteral*>(binary->left.get());
            const auto* object =
                dynamic_cast<const Identifier*>(binary->right.get());
            if (property && object && object->name == variable) {
                const std::string name = property->value;
                matches = [=, this](const TypePtr& candidate) {
                    return propertyType(candidate, name) != nullptr;
                };
            }
        }
    } else if (auto* call = dynamic_cast<const CallExpr*>(condition)) {
        const auto* callee =
            dynamic_cast<const Identifier*>(call->callee.get());
        if (callee && !call->arguments.empty()) {
            const auto* argument =
                dynamic_cast<const Identifier*>(call->arguments[0].get());
            auto signature = functions_.find(callee->name);
            if (argument && argument->name == variable &&
                signature != functions_.end() &&
                signature->second.returnType &&
                signature->second.returnType->kind ==
                    Type::Kind::TypePredicate) {
                TypePtr predicate =
                    evaluateType(signature->second.returnType->elementType);
                matches = [=, this](const TypePtr& candidate) {
                    return isAssignable(candidate, predicate);
                };
            }
        }
    } else if (auto* identifier =
                   dynamic_cast<const Identifier*>(condition)) {
        // Truthiness narrowing: `if (x)` removes falsy types (null, undefined,
        // false) from the union in the true branch; `else` / negation keeps
        // only them. This is the primary enabler for strictNullChecks guard
        // patterns like `if (maybeNull) { maybeNull.length }`.
        if (identifier->name == variable) {
            matches = [](const TypePtr& candidate) {
                if (!candidate) return false;
                return candidate->kind != Type::Kind::Null &&
                    candidate->kind != Type::Kind::Undefined &&
                    candidate->kind != Type::Kind::Boolean &&
                    !(candidate->kind == Type::Kind::Literal &&
                      (candidate->name == "false" ||
                       candidate->name == "0" ||
                       candidate->name == "" ));
            };
        }
    } else if (auto* unary =
                   dynamic_cast<const UnaryExpr*>(condition)) {
        // `if (!x)` narrows the same way as truthiness but inverted.
        if (unary->op == UnaryExpr::Op::Not) {
            return narrowTypeForCondition(original, unary->operand.get(),
                                          variable, !whenTrue);
        }
    }

    if (!matches) return resolved;
    std::vector<TypePtr> retained;
    for (const TypePtr& candidate : candidates) {
        if (matches(candidate) == whenTrue) retained.push_back(candidate);
    }
    return unionOf(std::move(retained));
}

void TypeChecker::validateTypeArguments(const TypePtr& type,
                                        const ASTNode& node) {
    if (!type) return;
    if (type->kind == Type::Kind::Object && !type->name.empty()) {
        auto generic = genericTypes_.find(type->name);
        if (generic != genericTypes_.end()) {
            std::unordered_map<std::string, TypePtr> substitutions;
            const size_t count = std::min(
                type->typeArguments.size(),
                generic->second.typeParameters.size());
            for (size_t index = 0; index < count; ++index) {
                TypePtr actual =
                    evaluateType(type->typeArguments[index], substitutions);
                if (index < generic->second.constraints.size() &&
                    generic->second.constraints[index]) {
                    TypePtr constraint = evaluateType(
                        generic->second.constraints[index], substitutions);
                    if (!isAssignable(actual, constraint)) {
                        report(node, "TS2344", "Type '" + typeName(actual) +
                            "' does not satisfy the constraint '" +
                            typeName(constraint) + "'.");
                    }
                }
                substitutions[generic->second.typeParameters[index]] = actual;
            }
        }
    }
    for (const auto& argument : type->typeArguments) {
        validateTypeArguments(argument, node);
    }
    for (const auto& member : type->types) {
        validateTypeArguments(member, node);
    }
    validateTypeArguments(type->elementType, node);
    validateTypeArguments(type->indexType, node);
    validateTypeArguments(type->checkType, node);
    validateTypeArguments(type->extendsType, node);
    validateTypeArguments(type->trueType, node);
    validateTypeArguments(type->falseType, node);
    validateTypeArguments(type->mappedSource, node);
    validateTypeArguments(type->mappedValue, node);
    validateTypeArguments(type->mappedNameType, node);
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
    if (auto* number = dynamic_cast<NumberLiteral*>(expression)) {
        TypePtr literal = makeType(
            Type::Kind::Literal,
            number->raw.empty() ? std::to_string(number->value) : number->raw);
        literal->elementType = makeType(Type::Kind::Number);
        return literal;
    }
    if (dynamic_cast<BigIntLiteral*>(expression)) return makeType(Type::Kind::BigInt);
    if (auto* string = dynamic_cast<StringLiteral*>(expression)) {
        TypePtr literal = makeType(Type::Kind::Literal, string->value);
        literal->elementType = makeType(Type::Kind::String);
        return literal;
    }
    if (dynamic_cast<TemplateLiteralExpr*>(expression)) return makeType(Type::Kind::String);
    if (auto* boolean = dynamic_cast<BooleanLiteral*>(expression)) {
        TypePtr literal = makeType(
            Type::Kind::Literal, boolean->value ? "true" : "false");
        literal->elementType = makeType(Type::Kind::Boolean);
        return literal;
    }
    if (dynamic_cast<NullLiteral*>(expression)) return makeType(Type::Kind::Null);
    if (dynamic_cast<UndefinedLiteral*>(expression)) return makeType(Type::Kind::Undefined);
    if (auto* array = dynamic_cast<ArrayExpr*>(expression)) {
        TypePtr result = makeType(Type::Kind::Array);
        for (auto& element : array->elements) {
            TypePtr elementType = inferExpression(element.get());
            if (!result->elementType) result->elementType = elementType;
            else result->elementType =
                unionOf({result->elementType, elementType});
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
    if (auto* created = dynamic_cast<NewExpr*>(expression)) {
        if (auto* identifier = dynamic_cast<Identifier*>(created->callee.get())) {
            auto constructor = constructors_.find(identifier->name);
            if (constructor != constructors_.end()) {
                const size_t count = std::min(
                    created->arguments.size(), constructor->second.size());
                for (size_t index = 0; index < created->arguments.size(); ++index) {
                    TypePtr actual = inferExpression(created->arguments[index].get());
                    if (index < count &&
                        !isAssignable(actual, constructor->second[index])) {
                        report(*created->arguments[index], "TS2345",
                               "Argument of type '" + typeName(actual) +
                               "' is not assignable to parameter of type '" +
                               typeName(constructor->second[index]) + "'.");
                    }
                }
                auto instance = namedTypes_.find(identifier->name);
                return instance == namedTypes_.end()
                    ? makeType(Type::Kind::Object, identifier->name)
                    : instance->second;
            }
            auto instance = namedTypes_.find(identifier->name);
            if (instance != namedTypes_.end()) {
                for (auto& argument : created->arguments) {
                    inferExpression(argument.get());
                }
                return instance->second;
            }
        }
        for (auto& argument : created->arguments) {
            inferExpression(argument.get());
        }
        return makeType(Type::Kind::Object);
    }
    if (dynamic_cast<RegexLiteralExpr*>(expression)) {
        return makeType(Type::Kind::Object);
    }
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
        if (auto* member = dynamic_cast<MemberExpr*>(assignment->left.get())) {
            std::string property;
            if (auto* identifier =
                    dynamic_cast<Identifier*>(member->property.get())) {
                property = identifier->name;
            } else if (auto* string =
                           dynamic_cast<StringLiteral*>(member->property.get())) {
                property = string->value;
            }
            TypePtr object = resolveType(inferExpression(member->object.get()));
            if (object && !property.empty() &&
                object->readonlyProperties.count(property) != 0) {
                report(*assignment, "TS2540", "Cannot assign to '" + property +
                    "' because it is a read-only property.");
            }
        }
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
            auto overloadSet = overloads_.find(identifier->name);
            if (overloadSet != overloads_.end() &&
                !overloadSet->second.empty()) {
                std::vector<TypePtr> argumentTypes;
                for (auto& argument : call->arguments) {
                    argumentTypes.push_back(inferExpression(argument.get()));
                }
                for (const FunctionSignature& candidate :
                     overloadSet->second) {
                    if (candidate.parameters.size() != argumentTypes.size()) {
                        continue;
                    }
                    bool matches = true;
                    for (size_t index = 0; index < argumentTypes.size();
                         ++index) {
                        if (!isAssignable(argumentTypes[index],
                                          candidate.parameters[index])) {
                            matches = false;
                            break;
                        }
                    }
                    if (matches) {
                        return evaluateType(candidate.returnType);
                    }
                }
                report(*call, "TS2769",
                       "No overload matches this call.");
                return makeType(Type::Kind::Unknown);
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
                    resolvedConstraint = evaluateType(resolvedConstraint);
                    if (!isAssignable(inferred->second, resolvedConstraint)) {
                        report(*call, "TS2345", "Type '" +
                            typeName(inferred->second) +
                            "' does not satisfy the constraint '" +
                            typeName(resolvedConstraint) + "'.");
                    }
                }
                return signature->second.returnType
                    ? evaluateType(substituteType(
                          signature->second.returnType, substitutions))
                    : makeType(Type::Kind::Any);
            }
        }
        TypePtr callee = resolveType(inferExpression(call->callee.get()));
        for (auto& argument : call->arguments) inferExpression(argument.get());
        if (callee && callee->kind == Type::Kind::Function &&
            callee->elementType) {
            return evaluateType(callee->elementType);
        }
        return makeType(Type::Kind::Any);
    }
    if (auto* sequence = dynamic_cast<SequenceExpr*>(expression)) {
        TypePtr result = makeType(Type::Kind::Undefined);
        for (auto& item : sequence->expressions) result = inferExpression(item.get());
        return result;
    }
    if (auto* member = dynamic_cast<MemberExpr*>(expression)) {
        TypePtr object = inferExpression(member->object.get());
        if (member->isComputed) {
            TypePtr index = inferExpression(member->property.get());
            TypePtr resolvedObject = resolveType(object);
            if (resolvedObject &&
                (resolvedObject->kind == Type::Kind::Array ||
                 resolvedObject->kind == Type::Kind::Tuple) &&
                index && (index->kind == Type::Kind::Number ||
                    (index->kind == Type::Kind::Literal &&
                     index->elementType &&
                     index->elementType->kind == Type::Kind::Number))) {
                if (resolvedObject->kind == Type::Kind::Array) {
                    return resolvedObject->elementType
                        ? resolvedObject->elementType
                        : makeType(Type::Kind::Unknown);
                }
            }
            if (dynamic_cast<Identifier*>(member->property.get())) {
                TypePtr indexed = makeType(Type::Kind::IndexedAccess);
                indexed->elementType = object;
                indexed->indexType = index;
                return evaluateType(indexed);
            }
        }
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
            TypePtr initializer = variables->isDeclare && declarator.type
                ? evaluateType(declarator.type)
                : declarator.init
                ? inferExpression(declarator.init.get()) : makeType(Type::Kind::Undefined);
            if (declarator.type) {
                validateTypeArguments(declarator.type, *variables);
            }
            TypePtr declared = declarator.type
                ? evaluateType(declarator.type) : initializer;
            if (!variables->isDeclare && declarator.type &&
                !isAssignable(initializer, declared)) {
                report(*variables, "TS2322", "Type '" + typeName(initializer) +
                    "' is not assignable to type '" + typeName(declared) + "'.");
            }
            if (!declarator.name.empty()) {
                bind(declarator.name, declared);
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
        std::unordered_map<std::string, TypePtr> visible;
        for (const auto& scope : scopes_) {
            for (const auto& [name, type] : scope) visible[name] = type;
        }
        pushScope();
        for (const auto& [name, type] : visible) {
            TypePtr narrowed = narrowTypeForCondition(
                type, conditional->test.get(), name, true);
            if (!sameType(narrowed, resolveType(type))) bind(name, narrowed);
        }
        checkStatement(conditional->consequent.get());
        popScope();
        pushScope();
        for (const auto& [name, type] : visible) {
            TypePtr narrowed = narrowTypeForCondition(
                type, conditional->test.get(), name, false);
            if (!sameType(narrowed, resolveType(type))) bind(name, narrowed);
        }
        checkStatement(conditional->alternate.get());
        popScope();
        const auto definitelyReturns = [](const Stmt* branch) {
            if (dynamic_cast<const ReturnStmt*>(branch)) return true;
            const auto* block = dynamic_cast<const BlockStmt*>(branch);
            return block && !block->statements.empty() &&
                dynamic_cast<const ReturnStmt*>(
                    block->statements.back().get()) != nullptr;
        };
        if (!conditional->alternate &&
            definitelyReturns(conditional->consequent.get())) {
            for (const auto& [name, type] : visible) {
                TypePtr narrowed = narrowTypeForCondition(
                    type, conditional->test.get(), name, false);
                if (!sameType(narrowed, resolveType(type))) {
                    bind(name, narrowed);
                }
            }
        }
    } else if (auto* whileLoop = dynamic_cast<WhileStmt*>(statement)) {
        inferExpression(whileLoop->test.get()); checkStatement(whileLoop->body.get());
    } else if (auto* doWhileLoop = dynamic_cast<DoWhileStmt*>(statement)) {
        checkStatement(doWhileLoop->body.get()); inferExpression(doWhileLoop->test.get());
    } else if (auto* forLoop = dynamic_cast<ForStmt*>(statement)) {
        pushScope(); checkStatement(forLoop->init.get()); inferExpression(forLoop->test.get());
        inferExpression(forLoop->update.get()); checkStatement(forLoop->body.get()); popScope();
    } else if (auto* thrown = dynamic_cast<ThrowStmt*>(statement)) {
        inferExpression(thrown->argument.get());
    } else if (auto* switchStatement =
                   dynamic_cast<SwitchStmt*>(statement)) {
        TypePtr discriminant =
            inferExpression(switchStatement->discriminant.get());
        const auto* member = dynamic_cast<MemberExpr*>(
            switchStatement->discriminant.get());
        const auto* base = member
            ? dynamic_cast<Identifier*>(member->object.get()) : nullptr;
        const auto* property = member
            ? dynamic_cast<Identifier*>(member->property.get()) : nullptr;
        TypePtr original = base ? lookup(base->name) : nullptr;
        std::vector<std::string> covered;
        for (const auto& switchCase : switchStatement->cases) {
            pushScope();
            if (base && property && original) {
                std::vector<TypePtr> candidates =
                    resolveType(original)->kind == Type::Kind::Union
                    ? resolveType(original)->types
                    : std::vector<TypePtr>{resolveType(original)};
                std::string expected;
                if (auto* string =
                        dynamic_cast<StringLiteral*>(switchCase->test.get())) {
                    expected = string->value;
                    covered.push_back(expected);
                }
                std::vector<TypePtr> retained;
                for (const TypePtr& candidate : candidates) {
                    TypePtr tag = propertyType(candidate, property->name);
                    const bool selected = switchCase->test
                        ? (tag && tag->kind == Type::Kind::Literal &&
                           tag->name == expected)
                        : (tag && std::find(
                               covered.begin(), covered.end(), tag->name) ==
                               covered.end());
                    if (selected) retained.push_back(candidate);
                }
                bind(base->name, unionOf(std::move(retained)));
            }
            for (const auto& consequent : switchCase->consequent) {
                checkStatement(consequent.get());
            }
            popScope();
        }
        (void)discriminant;
    } else if (auto* labeled = dynamic_cast<LabeledStmt*>(statement)) {
        checkStatement(labeled->statement.get());
    }
}

void TypeChecker::checkFunction(FunctionDecl& function) {
    if (!function.body || function.isOverload) return;
    TypePtr savedReturn = expectedReturnType_;
    expectedReturnType_ =
        function.returnType &&
        function.returnType->kind == Type::Kind::TypePredicate
            ? makeType(Type::Kind::Boolean)
            : function.returnType;
    std::unordered_map<std::string, TypePtr> savedTypeParameters;
    for (size_t index = 0; index < function.typeParams.size(); ++index) {
        const std::string& name = function.typeParams[index];
        auto existing = namedTypes_.find(name);
        if (existing != namedTypes_.end()) savedTypeParameters[name] = existing->second;
        TypePtr constraint = index < function.typeParamConstraints.size()
            ? function.typeParamConstraints[index] : nullptr;
        namedTypes_[name] = constraint
            ? constraint : makeType(Type::Kind::TypeParameter, name);
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
    diagnostics_.clear(); scopes_.clear(); functions_.clear();
    overloads_.clear(); namedTypes_.clear(); genericTypes_.clear();
    constructors_.clear();
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

    // Binder pass: create type-space identities before resolving members.
    for (Decl* declaration : declarations) {
        if (auto* interface = dynamic_cast<InterfaceDecl*>(declaration)) {
            if (interface->typeParams.empty()) {
                auto existing = namedTypes_.find(interface->name);
                if (existing == namedTypes_.end()) {
                    namedTypes_[interface->name] =
                        makeType(Type::Kind::Object, interface->name);
                }
            } else if (genericTypes_.count(interface->name) == 0) {
                GenericTypeDeclaration generic;
                generic.typeParameters = interface->typeParams;
                generic.constraints = interface->typeParamConstraints;
                generic.defaults = interface->typeParamDefaults;
                generic.body = makeType(Type::Kind::Object);
                genericTypes_[interface->name] = std::move(generic);
            }
        } else if (auto* klass = dynamic_cast<ClassDecl*>(declaration)) {
            namedTypes_[klass->name] =
                makeType(Type::Kind::Object, klass->name);
        }
    }
    for (Decl* declaration : declarations) {
        if (auto* alias = dynamic_cast<TypeAliasDecl*>(declaration)) {
            if (alias->typeParams.empty()) {
                namedTypes_[alias->name] = alias->type;
            } else {
                GenericTypeDeclaration generic;
                generic.typeParameters = alias->typeParams;
                generic.constraints = alias->typeParamConstraints;
                generic.defaults = alias->typeParamDefaults;
                generic.body = alias->type;
                genericTypes_[alias->name] = std::move(generic);
            }
        }
    }
    for (Decl* declaration : declarations) {
        if (auto* alias = dynamic_cast<TypeAliasDecl*>(declaration)) {
            validateTypeArguments(alias->type, *alias);
        }
    }
    for (Decl* declaration : declarations) {
        auto* interface = dynamic_cast<InterfaceDecl*>(declaration);
        if (!interface) continue;
        TypePtr type = interface->typeParams.empty()
            ? namedTypes_[interface->name]
            : genericTypes_[interface->name].body;
        for (const auto& property : interface->properties) {
            type->properties[property.name] = property.type
                ? property.type : makeType(Type::Kind::Any);
            if (property.isOptional) type->optionalProperties.insert(property.name);
            if (property.isReadonly) type->readonlyProperties.insert(property.name);
        }
        for (const auto& method : interface->methods) {
            TypePtr methodType = makeType(Type::Kind::Function);
            methodType->types.resize(method.params.size(), makeType(Type::Kind::Any));
            methodType->elementType = method.returnType
                ? method.returnType : makeType(Type::Kind::Any);
            type->properties[method.name] = methodType;
        }
    }
    // Class instance shapes, inheritance and constructor signatures.
    for (size_t pass = 0; pass <= declarations.size(); ++pass) {
        for (Decl* declaration : declarations) {
            auto* klass = dynamic_cast<ClassDecl*>(declaration);
            if (!klass) continue;
            TypePtr type = namedTypes_[klass->name];
            if (!klass->superclass.empty()) {
                auto parent = namedTypes_.find(klass->superclass);
                if (parent != namedTypes_.end()) {
                    TypePtr parentType = resolveType(parent->second);
                    if (parentType) {
                        for (const auto& [name, member] :
                             parentType->properties) {
                            if (type->properties.count(name) == 0) {
                                type->properties[name] = member;
                            }
                        }
                        type->readonlyProperties.insert(
                            parentType->readonlyProperties.begin(),
                            parentType->readonlyProperties.end());
                    }
                }
            }
            for (const auto& property : klass->properties) {
                if (property.isStatic) continue;
                type->properties[property.name] = property.type
                    ? property.type : makeType(Type::Kind::Unknown);
                if (property.isReadonly) {
                    type->readonlyProperties.insert(property.name);
                }
                if (property.isOptional) {
                    type->optionalProperties.insert(property.name);
                }
            }
            for (const auto& method : klass->methods) {
                if (method.kind == ClassDecl::Method::Kind::Constructor) {
                    constructors_[klass->name] = method.paramTypes;
                    while (constructors_[klass->name].size() <
                           method.params.size()) {
                        constructors_[klass->name].push_back(
                            makeType(Type::Kind::Unknown));
                    }
                    continue;
                }
                if (method.isStatic) continue;
                TypePtr methodType = makeType(Type::Kind::Function);
                methodType->types = method.paramTypes;
                while (methodType->types.size() < method.params.size()) {
                    methodType->types.push_back(
                        makeType(Type::Kind::Unknown));
                }
                methodType->elementType = method.returnType
                    ? method.returnType : makeType(Type::Kind::Void);
                type->properties[method.name] = methodType;
            }
        }
    }
    for (Decl* declaration : declarations) {
        auto* klass = dynamic_cast<ClassDecl*>(declaration);
        if (!klass) continue;
        for (const std::string& interfaceName : klass->interfaces) {
            auto implemented = namedTypes_.find(interfaceName);
            if (implemented != namedTypes_.end() &&
                !isAssignable(namedTypes_[klass->name],
                              implemented->second)) {
                report(*klass, "TS2420", "Class '" + klass->name +
                    "' incorrectly implements interface '" +
                    interfaceName + "'.");
            }
        }
    }

    // Merge inherited members after every interface has its own members.
    for (size_t pass = 0; pass < declarations.size(); ++pass) {
        for (Decl* declaration : declarations) {
            auto* interface = dynamic_cast<InterfaceDecl*>(declaration);
            if (!interface) continue;
            TypePtr type = interface->typeParams.empty()
                ? namedTypes_[interface->name]
                : genericTypes_[interface->name].body;
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

    // Bind ambient/runtime namespace members into qualified type names and a
    // structural namespace value. Repeated namespace declarations merge.
    std::function<void(NamespaceDecl&, const std::string&)> bindNamespace =
        [&](NamespaceDecl& nameSpace, const std::string& parent) {
            const std::string qualified = parent.empty()
                ? nameSpace.name : parent + "." + nameSpace.name;
            TypePtr namespaceValue;
            auto existingValue = namedTypes_.find("$namespace:" + qualified);
            if (existingValue == namedTypes_.end()) {
                namespaceValue = makeType(Type::Kind::Object);
                namedTypes_["$namespace:" + qualified] = namespaceValue;
            } else {
                namespaceValue = existingValue->second;
            }

            for (const auto& statement : nameSpace.body) {
                auto* declarationStatement =
                    dynamic_cast<DeclStmt*>(statement.get());
                Decl* member = declarationStatement
                    ? declarationStatement->declaration.get() : nullptr;
                if (auto* exported = dynamic_cast<ExportDecl*>(member)) {
                    member = exported->exportedDecl.get();
                }
                if (auto* interface = dynamic_cast<InterfaceDecl*>(member)) {
                    const std::string memberName = qualified == "global"
                        ? interface->name
                        : qualified + "." + interface->name;
                    TypePtr type;
                    auto existing = namedTypes_.find(memberName);
                    if (existing == namedTypes_.end()) {
                        type = makeType(Type::Kind::Object, memberName);
                        namedTypes_[memberName] = type;
                    } else {
                        type = existing->second;
                    }
                    for (const auto& property : interface->properties) {
                        type->properties[property.name] = property.type
                            ? property.type
                            : makeType(Type::Kind::Unknown);
                        if (property.isOptional) {
                            type->optionalProperties.insert(property.name);
                        }
                        if (property.isReadonly) {
                            type->readonlyProperties.insert(property.name);
                        }
                    }
                    for (const auto& method : interface->methods) {
                        TypePtr methodType =
                            makeType(Type::Kind::Function);
                        methodType->types.resize(
                            method.params.size(),
                            makeType(Type::Kind::Unknown));
                        methodType->elementType = method.returnType
                            ? method.returnType
                            : makeType(Type::Kind::Void);
                        type->properties[method.name] = methodType;
                    }
                } else if (auto* alias =
                               dynamic_cast<TypeAliasDecl*>(member)) {
                    namedTypes_[qualified + "." + alias->name] =
                        alias->type;
                } else if (auto* function =
                               dynamic_cast<FunctionDecl*>(member)) {
                    TypePtr method = makeType(Type::Kind::Function);
                    method->types = function->paramTypes;
                    while (method->types.size() < function->params.size()) {
                        method->types.push_back(
                            makeType(Type::Kind::Unknown));
                    }
                    method->elementType = function->returnType
                        ? function->returnType
                        : makeType(Type::Kind::Void);
                    namespaceValue->properties[function->name] = method;
                } else if (auto* nested =
                               dynamic_cast<NamespaceDecl*>(member)) {
                    bindNamespace(*nested, qualified);
                    namespaceValue->properties[nested->name] =
                        namedTypes_["$namespace:" + qualified + "." +
                                    nested->name];
                }
            }
            if (parent.empty() && nameSpace.name != "global") {
                bind(nameSpace.name, namespaceValue);
            }
        };
    for (Decl* declaration : declarations) {
        if (auto* nameSpace = dynamic_cast<NamespaceDecl*>(declaration)) {
            bindNamespace(*nameSpace, "");
        }
    }

    for (Decl* declaration : declarations) {
        auto* function = dynamic_cast<FunctionDecl*>(declaration);
        if (!function) continue;
        FunctionSignature signature;
        signature.typeParameters = function->typeParams;
        signature.typeParameterConstraints = function->typeParamConstraints;
        signature.typeParameterDefaults = function->typeParamDefaults;
        signature.parameters = function->paramTypes;
        while (signature.parameters.size() < function->params.size()) {
            signature.parameters.push_back(makeType(Type::Kind::Any));
        }
        signature.returnType = function->returnType
            ? function->returnType : makeType(Type::Kind::Any);
        signature.isOverload = function->isOverload || !function->body;
        if (signature.isOverload) {
            overloads_[function->name].push_back(signature);
        } else {
            functions_[function->name] = signature;
        }
        bind(function->name, makeType(Type::Kind::Function));
    }
    for (auto& statement : program.body) checkStatement(statement.get());
    popScope();
    return diagnostics_.empty();
}

} // namespace nova
