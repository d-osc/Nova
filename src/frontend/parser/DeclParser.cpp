#include "nova/Frontend/Parser.h"

namespace nova {

// Type annotation parsing (simplified placeholders)
std::unique_ptr<TypeAnnotation> Parser::parseTypeAnnotation() {
    // For now, just consume tokens until we hit a delimiter
    // Full TypeScript type system would be implemented here
    
    std::string typeName;
    Type::Kind kind = Type::Kind::Any;
    
    if (check(TokenType::Identifier)) {
        Token typeToken = advance();
        typeName = typeToken.value;
        
        // Map simple type names to Type::Kind
        if (typeName == "void") kind = Type::Kind::Void;
        else if (typeName == "any") kind = Type::Kind::Any;
        else if (typeName == "unknown") kind = Type::Kind::Unknown;
        else if (typeName == "never") kind = Type::Kind::Never;
        else if (typeName == "number") kind = Type::Kind::Number;
        else if (typeName == "string") kind = Type::Kind::String;
        else if (typeName == "boolean") kind = Type::Kind::Boolean;
        else if (typeName == "null") kind = Type::Kind::Null;
        else if (typeName == "undefined") kind = Type::Kind::Undefined;
        else if (typeName == "object") kind = Type::Kind::Object;
        else kind = Type::Kind::Any;  // Default for unknown types
    }
    
    auto type = std::make_unique<TypeAnnotation>(kind, typeName);
    type->location = getCurrentLocation();
    
    return type;
}

std::unique_ptr<TypeAnnotation> Parser::parsePrimaryType() {
    return parseTypeAnnotation();
}

std::unique_ptr<TypeAnnotation> Parser::parseUnionType() {
    return parseTypeAnnotation();
}

std::unique_ptr<TypeAnnotation> Parser::parseIntersectionType() {
    return parseTypeAnnotation();
}

std::unique_ptr<TypeAnnotation> Parser::parseArrayType() {
    return parseTypeAnnotation();
}

std::unique_ptr<TypeAnnotation> Parser::parseTupleType() {
    return parseTypeAnnotation();
}

std::unique_ptr<TypeAnnotation> Parser::parseFunctionType() {
    return parseTypeAnnotation();
}

std::unique_ptr<TypeAnnotation> Parser::parseObjectType() {
    return parseTypeAnnotation();
}

} // namespace nova
