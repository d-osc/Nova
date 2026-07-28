#include "nova/Frontend/Parser.h"
#include <stdexcept>

namespace nova {

namespace {
Type::Kind primitiveKind(const std::string& name) {
    if (name == "void") return Type::Kind::Void;
    if (name == "any") return Type::Kind::Any;
    if (name == "unknown") return Type::Kind::Unknown;
    if (name == "never") return Type::Kind::Never;
    if (name == "number") return Type::Kind::Number;
    if (name == "string") return Type::Kind::String;
    if (name == "boolean") return Type::Kind::Boolean;
    if (name == "bigint") return Type::Kind::BigInt;
    if (name == "symbol") return Type::Kind::Symbol;
    if (name == "null") return Type::Kind::Null;
    if (name == "undefined") return Type::Kind::Undefined;
    if (name == "object") return Type::Kind::Object;
    if (name == "Function") return Type::Kind::Function;
    return Type::Kind::Object;
}

TypePtr share(std::unique_ptr<TypeAnnotation> type) {
    return TypePtr(std::move(type));
}
} // namespace

bool Parser::checkTypeArgumentClose() const {
    if (pendingTypeArgumentClosers_ > 0) return true;
    if (isAtEnd()) return false;
    return peek().type == TokenType::Greater ||
           peek().type == TokenType::GreaterGreater ||
           peek().type == TokenType::GreaterGreaterGreater;
}

void Parser::consumeTypeArgumentClose(const std::string& message) {
    if (pendingTypeArgumentClosers_ > 0) {
        --pendingTypeArgumentClosers_;
        return;
    }
    if (match(TokenType::Greater)) return;
    if (match(TokenType::GreaterGreater)) {
        pendingTypeArgumentClosers_ = 1;
        return;
    }
    if (match(TokenType::GreaterGreaterGreater)) {
        pendingTypeArgumentClosers_ = 2;
        return;
    }
    reportError(message);
    throw std::runtime_error(message);
}

std::vector<TypePtr> Parser::parseTypeArgumentList() {
    consume(TokenType::Less, "Expected '<' before type arguments");
    std::vector<TypePtr> arguments;
    if (!checkTypeArgumentClose()) {
        do {
            arguments.push_back(share(parseTypeAnnotation()));
        } while (match(TokenType::Comma));
    }
    consumeTypeArgumentClose("Expected '>' after type arguments");
    return arguments;
}

void Parser::parseTypeParameterList(std::vector<std::string>& names,
                                    std::vector<TypePtr>& constraints,
                                    std::vector<TypePtr>& defaults) {
    consume(TokenType::Less, "Expected '<' before type parameters");
    if (checkTypeArgumentClose()) {
        reportError("Type parameter list cannot be empty");
    }
    do {
        Token parameter = consumeBindingIdentifier(
            "Expected type parameter name");
        names.push_back(parameter.value);

        TypePtr constraint;
        if (match(TokenType::KeywordExtends)) {
            constraint = share(parseTypeAnnotation());
        }
        constraints.push_back(std::move(constraint));

        TypePtr defaultType;
        if (match(TokenType::Equal)) {
            defaultType = share(parseTypeAnnotation());
        }
        defaults.push_back(std::move(defaultType));
    } while (match(TokenType::Comma));
    consumeTypeArgumentClose("Expected '>' after type parameters");
}

std::unique_ptr<TypeAnnotation> Parser::parseTypeAnnotation() {
    const SourceLocation location = getCurrentLocation();
    if (match(TokenType::KeywordAsserts)) {
        Token parameter = consumeBindingIdentifier(
            "Expected parameter name after 'asserts'");
        auto predicate = std::make_unique<TypeAnnotation>(
            Type::Kind::TypePredicate, parameter.value);
        predicate->location = location;
        predicate->isAssertion = true;
        if (match(TokenType::KeywordIs)) {
            predicate->elementType = share(parseTypeAnnotation());
        }
        return predicate;
    }

    auto t = parseUnionType();
    if (match(TokenType::KeywordIs)) {
        auto predicate = std::make_unique<TypeAnnotation>(
            Type::Kind::TypePredicate, t->name);
        predicate->location = t->location;
        predicate->elementType = share(parseTypeAnnotation());
        return predicate;
    }

    // Conditional type: T extends U ? X : Y
    if (match(TokenType::KeywordExtends)) {
        auto extends = parseUnionType();
        consume(TokenType::Question, "Expected '?' in conditional type");
        auto trueT = parseTypeAnnotation();
        consume(TokenType::Colon, "Expected ':' in conditional type");
        auto falseT = parseTypeAnnotation();
        auto cond = std::make_unique<TypeAnnotation>(Type::Kind::Conditional);
        cond->location = t->location;
        cond->checkType = share(std::move(t));
        cond->extendsType = share(std::move(extends));
        cond->trueType = share(std::move(trueT));
        cond->falseType = share(std::move(falseT));
        return cond;
    }
    return t;
}

std::unique_ptr<TypeAnnotation> Parser::parsePrimaryType() {
    const SourceLocation location = getCurrentLocation();
    if (match(TokenType::KeywordReadonly)) {
        auto readonlyType = parsePrimaryType();
        readonlyType->isReadonly = true;
        return readonlyType;
    }
    if (match(TokenType::KeywordUnique)) {
        auto uniqueType = parsePrimaryType();
        uniqueType->name = uniqueType->name.empty()
            ? "unique" : "unique " + uniqueType->name;
        return uniqueType;
    }
    if (match(TokenType::KeywordTypeof)) {
        Token name = consumeBindingIdentifier(
            "Expected value name after 'typeof'");
        std::string qualifiedName = name.value;
        while (match(TokenType::Dot)) {
            qualifiedName += "." +
                consumeIdentifierName(
                    "Expected name after '.' in type query").value;
        }
        auto query = std::make_unique<TypeAnnotation>(
            Type::Kind::TypeQuery, qualifiedName);
        query->location = location;
        return query;
    }
    if (match(TokenType::KeywordKeyof)) {
        auto inner = parsePrimaryType();
        auto k = std::make_unique<TypeAnnotation>(Type::Kind::Keyof);
        k->location = location;
        k->elementType = share(std::move(inner));
        return k;
    }
    if (match(TokenType::KeywordInfer)) {
        // infer T at type level (only valid inside conditional extends)
        const Token nameTok = consumeBindingIdentifier(
            "Expected identifier after 'infer'");
        auto inf = std::make_unique<TypeAnnotation>(Type::Kind::Infer, nameTok.value);
        inf->location = location;
        return inf;
    }
    if (check(TokenType::LeftParen)) {
        const size_t saved = current_;
        advance();

        auto function = std::make_unique<TypeAnnotation>(
            Type::Kind::Function);
        function->location = location;
        bool functionSyntax = true;
        if (!check(TokenType::RightParen)) {
            do {
                if (!checkBindingIdentifier()) {
                    functionSyntax = false;
                    break;
                }
                advance();
                match(TokenType::Question);
                if (!match(TokenType::Colon)) {
                    functionSyntax = false;
                    break;
                }
                function->types.push_back(share(parseTypeAnnotation()));
            } while (match(TokenType::Comma));
        }
        if (functionSyntax && match(TokenType::RightParen) &&
            match(TokenType::Arrow)) {
            function->elementType = share(parseTypeAnnotation());
            return function;
        }

        current_ = saved;
        consume(TokenType::LeftParen, "Expected '('");
        auto type = parseTypeAnnotation();
        consume(TokenType::RightParen, "Expected ')' after type annotation");
        return type;
    }
    if (check(TokenType::LeftBracket)) return parseTupleType();
    if (check(TokenType::LeftBrace)) return parseObjectType();
    if (check(TokenType::TemplateLiteral)) {
        Token literal = advance();
        auto templateType = std::make_unique<TypeAnnotation>(
            Type::Kind::TemplateLiteral, literal.value);
        templateType->location = literal.location;
        return templateType;
    }

    std::string name;
    if (checkBindingIdentifier()) {
        name = advance().value;
    } else if (match(TokenType::KeywordVoid)) {
        name = "void";
    } else if (match(TokenType::NullLiteral)) {
        name = "null";
    } else if (match(TokenType::UndefinedLiteral)) {
        name = "undefined";
    } else if (check(TokenType::StringLiteral) || check(TokenType::NumberLiteral) ||
               check(TokenType::TrueLiteral) || check(TokenType::FalseLiteral)) {
        name = advance().value;
        auto literal = std::make_unique<TypeAnnotation>(Type::Kind::Literal, name);
        literal->location = location;
        return literal;
    } else {
        reportError("Expected a TypeScript type annotation");
        auto fallback = std::make_unique<TypeAnnotation>(Type::Kind::Any, "any");
        fallback->location = location;
        return fallback;
    }

    auto type = std::make_unique<TypeAnnotation>(primitiveKind(name), name);
    type->location = location;

    while (match(TokenType::Dot)) {
        type->name += "." +
            consumeIdentifierName(
                "Expected type name after '.'").value;
    }

    if (check(TokenType::Less)) {
        type->typeArguments = parseTypeArgumentList();
    }
    return type;
}

std::unique_ptr<TypeAnnotation> Parser::parseArrayType() {
    auto type = parsePrimaryType();
    while (check(TokenType::LeftBracket)) {
        const SourceLocation location = type->location;
        advance();
        if (match(TokenType::RightBracket)) {
            auto array = std::make_unique<TypeAnnotation>(Type::Kind::Array);
            array->location = location;
            array->elementType = share(std::move(type));
            type = std::move(array);
        } else {
            auto index = parseTypeAnnotation();
            consume(TokenType::RightBracket,
                    "Expected ']' after indexed access type");
            auto indexed = std::make_unique<TypeAnnotation>(
                Type::Kind::IndexedAccess);
            indexed->location = location;
            indexed->elementType = share(std::move(type));
            indexed->indexType = share(std::move(index));
            type = std::move(indexed);
        }
    }
    return type;
}

std::unique_ptr<TypeAnnotation> Parser::parseIntersectionType() {
    auto first = parseArrayType();
    if (!match(TokenType::Ampersand)) return first;
    auto intersection = std::make_unique<TypeAnnotation>(Type::Kind::Intersection);
    intersection->location = first->location;
    intersection->types.push_back(share(std::move(first)));
    do {
        intersection->types.push_back(share(parseArrayType()));
    } while (match(TokenType::Ampersand));
    return intersection;
}

std::unique_ptr<TypeAnnotation> Parser::parseUnionType() {
    // TypeScript permits a leading `|` for vertically formatted unions.
    match(TokenType::Pipe);
    auto first = parseIntersectionType();
    if (!match(TokenType::Pipe)) return first;
    auto unionType = std::make_unique<TypeAnnotation>(Type::Kind::Union);
    unionType->location = first->location;
    unionType->types.push_back(share(std::move(first)));
    do {
        unionType->types.push_back(share(parseIntersectionType()));
    } while (match(TokenType::Pipe));
    return unionType;
}

std::unique_ptr<TypeAnnotation> Parser::parseTupleType() {
    const SourceLocation location = getCurrentLocation();
    consume(TokenType::LeftBracket, "Expected '[' to start tuple type");
    auto tuple = std::make_unique<TypeAnnotation>(Type::Kind::Tuple);
    tuple->location = location;
    if (!check(TokenType::RightBracket)) {
        do {
            tuple->types.push_back(share(parseTypeAnnotation()));
        } while (match(TokenType::Comma));
    }
    consume(TokenType::RightBracket, "Expected ']' after tuple type");
    return tuple;
}

std::unique_ptr<TypeAnnotation> Parser::parseFunctionType() {
    return parsePrimaryType();
}

std::unique_ptr<TypeAnnotation> Parser::parseObjectType() {
    const SourceLocation location = getCurrentLocation();
    consume(TokenType::LeftBrace, "Expected '{' to start object type");
    auto object = std::make_unique<TypeAnnotation>(Type::Kind::Object);
    object->location = location;

    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        int readonlyModifier = 0;
        if (match(TokenType::Plus)) {
            consume(TokenType::KeywordReadonly,
                    "Expected 'readonly' after '+'");
            readonlyModifier = 1;
        } else if (match(TokenType::Minus)) {
            consume(TokenType::KeywordReadonly,
                    "Expected 'readonly' after '-'");
            readonlyModifier = -1;
        } else if (match(TokenType::KeywordReadonly)) {
            readonlyModifier = 1;
        }

        if (match(TokenType::LeftBracket)) {
            Token variable = consumeBindingIdentifier(
                "Expected mapped type parameter");
            consume(TokenType::KeywordIn,
                    "Expected 'in' in mapped type");
            auto source = parseTypeAnnotation();

            TypePtr nameType;
            if (match(TokenType::KeywordAs)) {
                nameType = share(parseTypeAnnotation());
            }
            consume(TokenType::RightBracket,
                    "Expected ']' after mapped type parameter");

            int optionalModifier = 0;
            if (match(TokenType::Plus)) {
                consume(TokenType::Question,
                        "Expected '?' after '+'");
                optionalModifier = 1;
            } else if (match(TokenType::Minus)) {
                consume(TokenType::Question,
                        "Expected '?' after '-'");
                optionalModifier = -1;
            } else if (match(TokenType::Question)) {
                optionalModifier = 1;
            }

            consume(TokenType::Colon,
                    "Expected ':' after mapped type key");
            auto value = parseTypeAnnotation();
            if (!match(TokenType::Semicolon)) match(TokenType::Comma);
            consume(TokenType::RightBrace,
                    "Expected '}' after mapped type");

            auto mapped = std::make_unique<TypeAnnotation>(
                Type::Kind::Mapped);
            mapped->location = location;
            mapped->mappedVar = variable.value;
            mapped->mappedSource = share(std::move(source));
            mapped->mappedNameType = std::move(nameType);
            mapped->mappedValue = share(std::move(value));
            mapped->mappedReadonlyModifier = readonlyModifier;
            mapped->mappedOptionalModifier = optionalModifier;
            return mapped;
        }

        Token member = consumeIdentifierName(
            "Expected property name in object type");
        const bool optional = match(TokenType::Question);

        TypePtr memberType;
        if (match(TokenType::LeftParen)) {
            auto functionType = std::make_shared<TypeAnnotation>(Type::Kind::Function);
            while (!check(TokenType::RightParen) && !isAtEnd()) {
                consumeBindingIdentifier("Expected parameter name");
                match(TokenType::Question);
                if (match(TokenType::Colon)) {
                    functionType->types.push_back(share(parseTypeAnnotation()));
                } else {
                    functionType->types.push_back(std::make_shared<TypeAnnotation>(Type::Kind::Any));
                }
                if (!check(TokenType::RightParen)) {
                    consume(TokenType::Comma, "Expected ',' between parameters");
                }
            }
            consume(TokenType::RightParen, "Expected ')' after parameters");
            functionType->elementType = match(TokenType::Colon)
                ? share(parseTypeAnnotation())
                : std::make_shared<TypeAnnotation>(Type::Kind::Any);
            memberType = functionType;
        } else {
            consume(TokenType::Colon, "Expected ':' after property name");
            memberType = share(parseTypeAnnotation());
        }

        object->properties[member.value] = memberType;
        if (optional) object->optionalProperties.insert(member.value);
        if (readonlyModifier > 0) {
            object->readonlyProperties.insert(member.value);
        }
        if (!match(TokenType::Semicolon)) match(TokenType::Comma);
    }

    consume(TokenType::RightBrace, "Expected '}' after object type");
    return object;
}

} // namespace nova
