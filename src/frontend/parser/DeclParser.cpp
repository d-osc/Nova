#include "nova/Frontend/Parser.h"

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

std::unique_ptr<TypeAnnotation> Parser::parseTypeAnnotation() {
    auto t = parseUnionType();
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
    if (match(TokenType::KeywordKeyof)) {
        auto inner = parsePrimaryType();
        auto k = std::make_unique<TypeAnnotation>(Type::Kind::Keyof);
        k->location = location;
        k->elementType = share(std::move(inner));
        return k;
    }
    if (match(TokenType::KeywordInfer)) {
        // infer T at type level (only valid inside conditional extends)
        const Token nameTok = consume(TokenType::Identifier, "Expected identifier after 'infer'");
        auto inf = std::make_unique<TypeAnnotation>(Type::Kind::Infer, nameTok.value);
        inf->location = location;
        return inf;
    }
    if (match(TokenType::LeftParen)) {
        auto type = parseTypeAnnotation();
        consume(TokenType::RightParen, "Expected ')' after type annotation");
        return type;
    }
    if (check(TokenType::LeftBracket)) return parseTupleType();
    if (check(TokenType::LeftBrace)) return parseObjectType();

    std::string name;
    if (check(TokenType::Identifier)) {
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

    // Generic type reference: Foo<T1, T2, ...>
    if (check(TokenType::Less)) {
        if (tryParseTypeArguments()) {
            // Type arguments parsed successfully; for type-erasure purposes we
            // treat generic type refs as their named type. The TypeChecker can
            // still inspect the type arguments if needed.
            // (Type arguments themselves are consumed by tryParseTypeArguments.)
        }
    }
    return type;
}

std::unique_ptr<TypeAnnotation> Parser::parseArrayType() {
    auto type = parsePrimaryType();
    while (check(TokenType::LeftBracket) && peek(1).type == TokenType::RightBracket) {
        const SourceLocation location = type->location;
        advance();
        advance();
        auto array = std::make_unique<TypeAnnotation>(Type::Kind::Array);
        array->location = location;
        array->elementType = share(std::move(type));
        type = std::move(array);
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
        Token member = consume(TokenType::Identifier, "Expected property name in object type");
        const bool optional = match(TokenType::Question);

        TypePtr memberType;
        if (match(TokenType::LeftParen)) {
            auto functionType = std::make_shared<TypeAnnotation>(Type::Kind::Function);
            while (!check(TokenType::RightParen) && !isAtEnd()) {
                consume(TokenType::Identifier, "Expected parameter name");
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
        if (!match(TokenType::Semicolon)) match(TokenType::Comma);
    }

    consume(TokenType::RightBrace, "Expected '}' after object type");
    return object;
}

} // namespace nova
