#include "nova/Frontend/Parser.h"
#include <stdexcept>

namespace nova {

// Forward declarations of helper functions
BinaryExpr::Op tokenToBinaryOp(TokenType type);
UnaryExpr::Op tokenToUnaryOp(TokenType type);
UpdateExpr::Op tokenToUpdateOp(TokenType type);
AssignmentExpr::Op tokenToAssignmentOp(TokenType type);

// Expression parsing with precedence climbing
std::unique_ptr<Expr> Parser::parseExpression() {
    auto expr = parseAssignmentExpression();
    
    // Sequence expression (comma operator)
    if (match(TokenType::Comma)) {
        std::vector<ExprPtr> expressions;
        expressions.push_back(std::move(expr));
        
        do {
            expressions.push_back(parseAssignmentExpression());
        } while (match(TokenType::Comma));
        
        auto seqExpr = std::make_unique<SequenceExpr>(std::move(expressions));
        seqExpr->location = getCurrentLocation();
        return seqExpr;
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseAssignmentExpression() {
    // Check for async arrow function: async (params) => body
    bool isAsync = false;
    if (check(TokenType::KeywordAsync)) {
        size_t savedPos = current_;
        advance(); // consume 'async'
        
        // Check if it's followed by arrow function pattern
        if (check(TokenType::Identifier) || check(TokenType::LeftParen)) {
            // Look ahead for =>
            size_t checkPos = current_;
            if (check(TokenType::Identifier)) {
                advance();
                if (check(TokenType::Arrow)) {
                    isAsync = true;
                    current_ = checkPos; // restore to after 'async'
                } else {
                    current_ = savedPos; // restore to before 'async'
                }
            } else {
                current_ = savedPos; // will check parenthesized later
                isAsync = true;
            }
        } else {
            current_ = savedPos;
        }
    }
    
    // Check for arrow function: identifier => body
    if (check(TokenType::Identifier) && !isAsync) {
        size_t savedPos = current_;
        Token id = advance();

        if (match(TokenType::Arrow)) {
            auto arrow = std::make_unique<ArrowFunctionExpr>();
            arrow->location = id.location;
            arrow->params.push_back(id.value);
            arrow->paramTypes.push_back(nullptr);  // No type annotation for single param

            // Parse body
            if (check(TokenType::LeftBrace)) {
                arrow->body = parseBlockStatement();
            } else {
                auto expr = parseAssignmentExpression();
                auto exprStmt = std::make_unique<ExprStmt>(std::move(expr));
                arrow->body = std::move(exprStmt);
            }

            return arrow;
        } else {
            current_ = savedPos;
        }
    }

    // Check for async single param: async x => body
    if (isAsync && check(TokenType::Identifier)) {
        Token id = advance();
        if (match(TokenType::Arrow)) {
            auto arrow = std::make_unique<ArrowFunctionExpr>();
            arrow->location = id.location;
            arrow->isAsync = true;
            arrow->params.push_back(id.value);
            arrow->paramTypes.push_back(nullptr);  // No type annotation for single param

            if (check(TokenType::LeftBrace)) {
                arrow->body = parseBlockStatement();
            } else {
                auto expr = parseAssignmentExpression();
                auto exprStmt = std::make_unique<ExprStmt>(std::move(expr));
                arrow->body = std::move(exprStmt);
            }

            return arrow;
        }
    }
    
    auto expr = parseConditionalExpression();
    
    // Check for assignment operators
    TokenType type = peek().type;
    if (type == TokenType::Equal ||
        type == TokenType::PlusEqual ||
        type == TokenType::MinusEqual ||
        type == TokenType::StarEqual ||
        type == TokenType::SlashEqual ||
        type == TokenType::PercentEqual ||
        type == TokenType::StarStarEqual ||
        type == TokenType::AmpersandEqual ||
        type == TokenType::PipeEqual ||
        type == TokenType::CaretEqual ||
        type == TokenType::LessLessEqual ||
        type == TokenType::GreaterGreaterEqual ||
        type == TokenType::GreaterGreaterGreaterEqual ||
        type == TokenType::AmpersandAmpersandEqual ||
        type == TokenType::PipePipeEqual ||
        type == TokenType::QuestionQuestionEqual) {
        
        Token op = advance();
        auto right = parseAssignmentExpression();
        
        auto assign = std::make_unique<AssignmentExpr>(
            tokenToAssignmentOp(op.type),
            std::move(expr),
            std::move(right)
        );
        assign->location = op.location;
        
        return assign;
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseConditionalExpression() {
    auto expr = parseLogicalOrExpression();
    
    if (match(TokenType::Question)) {
        auto test = std::move(expr);
        auto consequent = parseAssignmentExpression();
        consume(TokenType::Colon, "Expected ':' in ternary expression");
        auto alternate = parseAssignmentExpression();
        
        auto conditional = std::make_unique<ConditionalExpr>(
            std::move(test),
            std::move(consequent),
            std::move(alternate)
        );
        conditional->location = getCurrentLocation();
        return conditional;
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parseLogicalOrExpression() {
    auto left = parseLogicalAndExpression();
    
    while (match(TokenType::PipePipe) || match(TokenType::QuestionQuestion)) {
        Token op = peek(-1);
        auto right = parseLogicalAndExpression();
        
        auto binary = std::make_unique<BinaryExpr>(
            tokenToBinaryOp(op.type),
            std::move(left),
            std::move(right)
        );
        binary->location = op.location;
        
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseLogicalAndExpression() {
    auto left = parseBitwiseOrExpression();
    
    while (match(TokenType::AmpersandAmpersand)) {
        Token op = peek(-1);
        auto right = parseBitwiseOrExpression();
        
        auto binary = std::make_unique<BinaryExpr>(
            tokenToBinaryOp(op.type),
            std::move(left),
            std::move(right)
        );
        binary->location = op.location;
        
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseBitwiseOrExpression() {
    auto left = parseBitwiseXorExpression();
    
    while (match(TokenType::Pipe)) {
        Token op = peek(-1);
        auto right = parseBitwiseXorExpression();
        
        auto binary = std::make_unique<BinaryExpr>(tokenToBinaryOp(op.type), std::move(left), std::move(right));
        
        binary->location = op.location;
        
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseBitwiseXorExpression() {
    auto left = parseBitwiseAndExpression();
    
    while (match(TokenType::Caret)) {
        Token op = peek(-1);
        auto right = parseBitwiseAndExpression();
        
        auto binary = std::make_unique<BinaryExpr>(tokenToBinaryOp(op.type), std::move(left), std::move(right));
        
        binary->location = op.location;
        
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseBitwiseAndExpression() {
    auto left = parseEqualityExpression();
    
    while (match(TokenType::Ampersand)) {
        Token op = peek(-1);
        auto right = parseEqualityExpression();
        
        auto binary = std::make_unique<BinaryExpr>(tokenToBinaryOp(op.type), std::move(left), std::move(right));
        
        binary->location = op.location;
        
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseEqualityExpression() {
    auto left = parseRelationalExpression();
    
    while (match(TokenType::EqualEqual) || 
           match(TokenType::ExclamationEqual) ||
           match(TokenType::EqualEqualEqual) ||
           match(TokenType::ExclamationEqualEqual)) {
        Token op = peek(-1);
        auto right = parseRelationalExpression();
        
        auto binary = std::make_unique<BinaryExpr>(tokenToBinaryOp(op.type), std::move(left), std::move(right));
        
        binary->location = op.location;
        
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseRelationalExpression() {
    auto left = parseShiftExpression();
    
    while (match(TokenType::Less) || 
           match(TokenType::Greater) ||
           match(TokenType::LessEqual) ||
           match(TokenType::GreaterEqual) ||
           match(TokenType::KeywordInstanceof) ||
           match(TokenType::KeywordIn)) {
        Token op = peek(-1);
        auto right = parseShiftExpression();
        
        auto binary = std::make_unique<BinaryExpr>(tokenToBinaryOp(op.type), std::move(left), std::move(right));
        
        binary->location = op.location;
        
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseShiftExpression() {
    auto left = parseAdditiveExpression();
    
    while (match(TokenType::LessLess) || 
           match(TokenType::GreaterGreater) ||
           match(TokenType::GreaterGreaterGreater)) {
        Token op = peek(-1);
        auto right = parseAdditiveExpression();
        
        auto binary = std::make_unique<BinaryExpr>(tokenToBinaryOp(op.type), std::move(left), std::move(right));
        
        binary->location = op.location;
        
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseAdditiveExpression() {
    auto left = parseMultiplicativeExpression();
    
    while (match(TokenType::Plus) || match(TokenType::Minus)) {
        Token op = peek(-1);
        auto right = parseMultiplicativeExpression();
        
        auto binary = std::make_unique<BinaryExpr>(tokenToBinaryOp(op.type), std::move(left), std::move(right));
        
        binary->location = op.location;
        
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseMultiplicativeExpression() {
    auto left = parseExponentiationExpression();
    
    while (match(TokenType::Star) || 
           match(TokenType::Slash) || 
           match(TokenType::Percent)) {
        Token op = peek(-1);
        auto right = parseExponentiationExpression();
        
        auto binary = std::make_unique<BinaryExpr>(tokenToBinaryOp(op.type), std::move(left), std::move(right));
        
        binary->location = op.location;
        
        left = std::move(binary);
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseExponentiationExpression() {
    auto left = parseUnaryExpression();
    
    if (match(TokenType::StarStar)) {
        Token op = peek(-1);
        // Right associative
        auto right = parseExponentiationExpression();
        
        auto binary = std::make_unique<BinaryExpr>(tokenToBinaryOp(op.type), std::move(left), std::move(right));
        
        binary->location = op.location;
        
        return binary;
    }
    
    return left;
}

std::unique_ptr<Expr> Parser::parseUnaryExpression() {
    // Yield expression
    if (match(TokenType::KeywordYield)) {
        bool isDelegate = false;
        ExprPtr argument = nullptr;
        
        // yield* (delegate)
        if (match(TokenType::Star)) {
            isDelegate = true;
        }
        
        // Optional value
        if (!check(TokenType::Semicolon) && !check(TokenType::RightBrace) && 
            !check(TokenType::RightParen) && !isAtEnd()) {
            argument = parseAssignmentExpression();
        }
        
        auto yieldExpr = std::make_unique<YieldExpr>(std::move(argument), isDelegate);
        yieldExpr->location = getCurrentLocation();
        return yieldExpr;
    }
    
    // Prefix operators
    if (match(TokenType::Plus) || 
        match(TokenType::Minus) ||
        match(TokenType::Exclamation) ||
        match(TokenType::Tilde) ||
        match(TokenType::KeywordTypeof) ||
        match(TokenType::KeywordVoid) ||
        match(TokenType::KeywordDelete) ||
        match(TokenType::KeywordAwait)) {
        Token op = peek(-1);
        auto argument = parseUnaryExpression();
        
        auto unary = std::make_unique<UnaryExpr>(
            tokenToUnaryOp(op.type),
            std::move(argument),
            true
        );
        unary->location = op.location;
        
        return unary;
    }
    
    // Prefix ++ / --
    if (match(TokenType::PlusPlus) || match(TokenType::MinusMinus)) {
        Token op = peek(-1);
        auto argument = parsePostfixExpression();
        
        auto update = std::make_unique<UpdateExpr>(
            tokenToUpdateOp(op.type),
            std::move(argument),
            true
        );
        update->location = op.location;
        
        return update;
    }
    
    return parsePostfixExpression();
}

std::unique_ptr<Expr> Parser::parsePostfixExpression() {
    auto expr = parsePrimaryExpression();
    
    while (true) {
        // Member access: obj.prop
        if (match(TokenType::Dot)) {
            // Allow identifiers and keywords as property names (JavaScript behavior)
            Token prop;
            if (check(TokenType::Identifier) || peek().isKeyword()) {
                prop = advance();
            } else {
                reportError("Expected property name");
                prop = Token(TokenType::Invalid, "", getCurrentLocation());
            }

            auto propExpr = std::make_unique<Identifier>(prop.value);
            propExpr->location = prop.location;
            
            auto member = std::make_unique<MemberExpr>(
                std::move(expr),
                std::move(propExpr),
                false, // isComputed
                false  // isOptional
            );
            member->location = prop.location;
            
            expr = std::move(member);
        }
        // Computed member: obj[prop]
        else if (match(TokenType::LeftBracket)) {
            auto property = parseExpression();
            consume(TokenType::RightBracket, "Expected ']'");
            
            auto member = std::make_unique<MemberExpr>(
                std::move(expr),
                std::move(property),
                true,  // isComputed
                false  // isOptional
            );
            member->location = getCurrentLocation();
            
            expr = std::move(member);
        }
        // Function call: func(args)
        else if (match(TokenType::LeftParen)) {
            std::vector<ExprPtr> arguments;
            
            while (!check(TokenType::RightParen) && !isAtEnd()) {
                arguments.push_back(parseAssignmentExpression());
                if (!check(TokenType::RightParen)) {
                    consume(TokenType::Comma, "Expected ',' between arguments");
                }
            }
            consume(TokenType::RightParen, "Expected ')' after arguments");
            
            auto call = std::make_unique<CallExpr>(std::move(expr), std::move(arguments));
            call->location = getCurrentLocation();
            
            expr = std::move(call);
        }
        // Optional chaining: obj?.prop
        else if (match(TokenType::QuestionDot)) {
            Token prop = consume(TokenType::Identifier, "Expected property name");
            
            auto propExpr = std::make_unique<Identifier>(prop.value);
            propExpr->location = prop.location;
            
            auto member = std::make_unique<MemberExpr>(
                std::move(expr),
                std::move(propExpr),
                false, // isComputed
                true   // isOptional
            );
            member->location = prop.location;
            
            expr = std::move(member);
        }
        // Postfix ++ / --
        else if (match(TokenType::PlusPlus) || match(TokenType::MinusMinus)) {
            Token op = peek(-1);
            
            auto update = std::make_unique<UpdateExpr>(
                tokenToUpdateOp(op.type),
                std::move(expr),
                false
            );
            update->location = op.location;
            
            expr = std::move(update);
        }
        // Non-null assertion: expr!
        else if (match(TokenType::Exclamation)) {
            auto nonNull = std::make_unique<NonNullExpr>(std::move(expr));
            nonNull->location = getCurrentLocation();
            expr = std::move(nonNull);
        }
        // Type assertion: expr as Type
        else if (match(TokenType::KeywordAs)) {
            auto asExpr = std::make_unique<AsExpr>(std::move(expr), parseTypeAnnotation());
            asExpr->location = getCurrentLocation();
            expr = std::move(asExpr);
        }
        // Satisfies: expr satisfies Type
        else if (match(TokenType::KeywordSatisfies)) {
            auto satisfies = std::make_unique<SatisfiesExpr>(std::move(expr), parseTypeAnnotation());
            satisfies->location = getCurrentLocation();
            expr = std::move(satisfies);
        }
        // Tagged template literal: tag`template`
        else if (check(TokenType::TemplateLiteral)) {
            auto templateLit = parseTemplateLiteral();
            
            // Extract quasis and expressions from template literal
            auto* tempLit = dynamic_cast<TemplateLiteralExpr*>(templateLit.get());
            if (tempLit) {
                auto tagged = std::make_unique<TaggedTemplateExpr>(
                    std::move(expr),
                    std::move(tempLit->quasis),
                    std::move(tempLit->expressions)
                );
                tagged->location = getCurrentLocation();
                expr = std::move(tagged);
            } else {
                reportError("Invalid template literal in tagged template");
            }
        }
        else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expr> Parser::parsePrimaryExpression() {
    // JSX Element or Fragment
    if (check(TokenType::LessThan)) {
        // Look ahead to distinguish from comparison
        size_t savedPos = current_;
        advance(); // consume '<'
        
        // JSX Fragment: <>...</>
        if (check(TokenType::Greater)) {
            current_ = savedPos;
            return parseJSXElement();
        }
        
        // JSX Element: <TagName or closing tag </
        if (check(TokenType::Identifier) || check(TokenType::Slash)) {
            current_ = savedPos;
            return parseJSXElement();
        }
        
        // Not JSX, restore and continue
        current_ = savedPos;
    }
    
    // Literals
    if (check(TokenType::NumberLiteral) || 
        check(TokenType::StringLiteral) ||
        check(TokenType::TrueLiteral) ||
        check(TokenType::FalseLiteral) ||
        check(TokenType::NullLiteral) ||
        check(TokenType::UndefinedLiteral)) {
        return parseLiteral();
    }
    
    // Identifier
    if (check(TokenType::Identifier)) {
        return parseIdentifier();
    }
    
    // Array literal
    if (check(TokenType::LeftBracket)) {
        return parseArrayLiteral();
    }
    
    // Object literal
    if (check(TokenType::LeftBrace)) {
        return parseObjectLiteral();
    }
    
    // Parenthesized expression or arrow function with params
    if (match(TokenType::LeftParen)) {
        // Try to detect arrow function: (a, b) => body
        size_t savedPos = current_;
        std::vector<std::string> params;
        bool couldBeArrow = true;
        
        // Empty params: () => body
        if (check(TokenType::RightParen)) {
            advance();
            if (check(TokenType::Arrow)) {
                advance();
                auto arrow = std::make_unique<ArrowFunctionExpr>();
                arrow->location = getCurrentLocation();
                // Empty params and paramTypes (no parameters)

                if (check(TokenType::LeftBrace)) {
                    arrow->body = parseBlockStatement();
                } else {
                    auto expr = parseAssignmentExpression();
                    auto exprStmt = std::make_unique<ExprStmt>(std::move(expr));
                    arrow->body = std::move(exprStmt);
                }
                return arrow;
            } else {
                // Empty parentheses but not arrow - treat as group
                current_ = savedPos;
                auto expr = parseExpression();
                consume(TokenType::RightParen, "Expected ')' after expression");
                return expr;
            }
        }
        
        // Try parsing as parameter list
        std::vector<TypePtr> paramTypes;
        while (!check(TokenType::RightParen) && !isAtEnd()) {
            if (check(TokenType::Identifier)) {
                params.push_back(advance().value);

                // Optional type annotation
                TypePtr paramType = nullptr;
                if (match(TokenType::Colon)) {
                    paramType = parseTypeAnnotation();
                }
                paramTypes.push_back(std::move(paramType));

                if (check(TokenType::RightParen)) break;
                if (!match(TokenType::Comma)) {
                    couldBeArrow = false;
                    break;
                }
            } else {
                couldBeArrow = false;
                break;
            }
        }

        if (couldBeArrow && match(TokenType::RightParen) && check(TokenType::Arrow)) {
            advance(); // consume '=>'
            auto arrow = std::make_unique<ArrowFunctionExpr>();
            arrow->location = getCurrentLocation();
            arrow->params = std::move(params);
            arrow->paramTypes = std::move(paramTypes);
            
            if (check(TokenType::LeftBrace)) {
                arrow->body = parseBlockStatement();
            } else {
                auto expr = parseAssignmentExpression();
                auto exprStmt = std::make_unique<ExprStmt>(std::move(expr));
                arrow->body = std::move(exprStmt);
            }
            return arrow;
        } else {
            // Not arrow function, parse as grouped expression
            current_ = savedPos;
            auto expr = parseExpression();
            consume(TokenType::RightParen, "Expected ')' after expression");
            return expr;
        }
    }
    
    // Arrow function or function expression
    if (check(TokenType::KeywordFunction)) {
        return parseFunctionExpression();
    }
    
    // Template literal
    if (check(TokenType::TemplateLiteral)) {
        return parseTemplateLiteral();
    }
    
    // This
    if (match(TokenType::KeywordThis)) {
        auto thisExpr = std::make_unique<ThisExpr>();
        thisExpr->location = getCurrentLocation();
        return thisExpr;
    }
    
    // Super
    if (match(TokenType::KeywordSuper)) {
        auto superExpr = std::make_unique<SuperExpr>();
        superExpr->location = getCurrentLocation();
        return superExpr;
    }
    
    // New expression or new.target
    if (match(TokenType::KeywordNew)) {
        // Check for new.target
        if (match(TokenType::Dot)) {
            Token target = consume(TokenType::Identifier, "Expected 'target' after 'new.'");
            if (target.value == "target") {
                auto meta = std::make_unique<MetaProperty>("new", "target");
                meta->location = getCurrentLocation();
                return meta;
            } else {
                reportError("Expected 'target' after 'new.'");
            }
        }
        
        auto callee = parsePrimaryExpression();
        
        std::vector<ExprPtr> arguments;
        
        // Arguments (optional for new)
        if (match(TokenType::LeftParen)) {
            while (!check(TokenType::RightParen) && !isAtEnd()) {
                arguments.push_back(parseAssignmentExpression());
                if (!check(TokenType::RightParen)) {
                    consume(TokenType::Comma, "Expected ',' between arguments");
                }
            }
            consume(TokenType::RightParen, "Expected ')' after arguments");
        }
        
        auto newExpr = std::make_unique<NewExpr>(std::move(callee), std::move(arguments));
        newExpr->location = getCurrentLocation();
        
        return newExpr;
    }
    
    // import() or import.meta
    if (match(TokenType::KeywordImport)) {
        // import()
        if (match(TokenType::LeftParen)) {
            auto source = parseAssignmentExpression();
            consume(TokenType::RightParen, "Expected ')' after import source");
            
            auto importExpr = std::make_unique<ImportExpr>(std::move(source));
            importExpr->location = getCurrentLocation();
            return importExpr;
        }
        // import.meta
        else if (match(TokenType::Dot)) {
            Token meta = consume(TokenType::Identifier, "Expected 'meta' after 'import.'");
            if (meta.value == "meta") {
                auto metaProp = std::make_unique<MetaProperty>("import", "meta");
                metaProp->location = getCurrentLocation();
                return metaProp;
            } else {
                reportError("Expected 'meta' after 'import.'");
            }
        }
    }
    
    reportError("Unexpected token in expression");
    throw std::runtime_error("Unexpected token");
}

std::unique_ptr<Expr> Parser::parseIdentifier() {
    Token id = consume(TokenType::Identifier, "Expected identifier");
    
    auto identifier = std::make_unique<Identifier>(id.value);
    identifier->location = id.location;
    
    return identifier;
}

std::unique_ptr<Expr> Parser::parseLiteral() {
    Token lit = advance();
    
    switch (lit.type) {
        case TokenType::NumberLiteral: {
            auto numLit = std::make_unique<NumberLiteral>(std::stod(lit.value));
            numLit->location = lit.location;
            return numLit;
        }
        case TokenType::StringLiteral: {
            auto strLit = std::make_unique<StringLiteral>(lit.value);
            strLit->location = lit.location;
            return strLit;
        }
        case TokenType::TrueLiteral: {
            auto boolLit = std::make_unique<BooleanLiteral>(true);
            boolLit->location = lit.location;
            return boolLit;
        }
        case TokenType::FalseLiteral: {
            auto boolLit = std::make_unique<BooleanLiteral>(false);
            boolLit->location = lit.location;
            return boolLit;
        }
        case TokenType::NullLiteral: {
            auto nullLit = std::make_unique<NullLiteral>();
            nullLit->location = lit.location;
            return nullLit;
        }
        case TokenType::UndefinedLiteral: {
            auto undefLit = std::make_unique<UndefinedLiteral>();
            undefLit->location = lit.location;
            return undefLit;
        }
        default:
            reportError("Invalid literal type");
            throw std::runtime_error("Invalid literal type");
    }
}

std::unique_ptr<Expr> Parser::parseArrayLiteral() {
    consume(TokenType::LeftBracket, "Expected '['");
    
    std::vector<ExprPtr> elements;
    
    while (!check(TokenType::RightBracket) && !isAtEnd()) {
        // Allow holes: [1, , 3]
        if (check(TokenType::Comma)) {
            elements.push_back(nullptr);
        } 
        // Spread element: [...arr]
        else if (match(TokenType::DotDotDot)) {
            auto spread = std::make_unique<SpreadExpr>(parseAssignmentExpression());
            spread->location = getCurrentLocation();
            elements.push_back(std::move(spread));
        } 
        else {
            elements.push_back(parseAssignmentExpression());
        }
        
        if (!check(TokenType::RightBracket)) {
            consume(TokenType::Comma, "Expected ',' between array elements");
        }
    }
    
    consume(TokenType::RightBracket, "Expected ']'");
    
    auto array = std::make_unique<ArrayExpr>(std::move(elements));
    array->location = getCurrentLocation();
    
    return array;
}

std::unique_ptr<Expr> Parser::parseObjectLiteral() {
    consume(TokenType::LeftBrace, "Expected '{'");
    
    std::vector<ObjectExpr::Property> properties;
    
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        // Spread property: { ...obj }
        if (match(TokenType::DotDotDot)) {
            auto spread = std::make_unique<SpreadExpr>(parseAssignmentExpression());
            spread->location = getCurrentLocation();
            
            // Create a special spread property
            ObjectExpr::Property prop;
            // Use a special marker for spread properties
            auto keyIdent = std::make_unique<StringLiteral>("...");
            prop.key = std::move(keyIdent);
            prop.value = std::move(spread);
            prop.isComputed = false;
            prop.isShorthand = false;
            prop.kind = ObjectExpr::Property::Kind::Init;
            
            properties.push_back(std::move(prop));
            
            if (!check(TokenType::RightBrace)) {
                consume(TokenType::Comma, "Expected ',' after spread property");
            }
            continue;
        }
        
        ObjectExpr::Property property;
        
        // Computed property: [key]: value
        if (match(TokenType::LeftBracket)) {
            property.isComputed = true;
            property.key = parseAssignmentExpression();
            consume(TokenType::RightBracket, "Expected ']' after computed property");
            consume(TokenType::Colon, "Expected ':' after computed property key");
            property.value = parseAssignmentExpression();
            property.isShorthand = false;
            property.kind = ObjectExpr::Property::Kind::Init;
        }
        // Key
        else if (check(TokenType::Identifier)) {
            Token key = advance();
            auto keyIdent = std::make_unique<Identifier>(key.value);
            keyIdent->location = key.location;
            property.key = std::move(keyIdent);
            
            // Shorthand: { x } instead of { x: x }
            if (check(TokenType::Comma) || check(TokenType::RightBrace)) {
                property.isShorthand = true;
                auto valueIdent = std::make_unique<Identifier>(key.value);
                valueIdent->location = key.location;
                property.value = std::move(valueIdent);
                property.isComputed = false;
                property.kind = ObjectExpr::Property::Kind::Init;
            } else {
                consume(TokenType::Colon, "Expected ':' after property key");
                property.value = parseAssignmentExpression();
                property.isShorthand = false;
                property.isComputed = false;
                property.kind = ObjectExpr::Property::Kind::Init;
            }
        } 
        else if (check(TokenType::StringLiteral)) {
            Token key = advance();
            auto keyStr = std::make_unique<StringLiteral>(key.value);
            keyStr->location = key.location;
            property.key = std::move(keyStr);
            consume(TokenType::Colon, "Expected ':' after property key");
            property.value = parseAssignmentExpression();
            property.isShorthand = false;
            property.isComputed = false;
            property.kind = ObjectExpr::Property::Kind::Init;
        } 
        else {
            reportError("Expected property name");
            break;
        }
        
        properties.push_back(std::move(property));
        
        if (!check(TokenType::RightBrace)) {
            consume(TokenType::Comma, "Expected ',' between properties");
        }
    }
    
    consume(TokenType::RightBrace, "Expected '}'");
    
    auto object = std::make_unique<ObjectExpr>(std::move(properties));
    object->location = getCurrentLocation();
    
    return object;
}

// Placeholder implementations
std::unique_ptr<Expr> Parser::parseFunctionExpression() {
    auto func = std::make_unique<FunctionExpr>();
    func->location = getCurrentLocation();
    
    // Generator? (function*)
    if (match(TokenType::Star)) {
        func->isGenerator = true;
    }
    
    // Optional name
    if (check(TokenType::Identifier)) {
        func->name = advance().value;
    }
    
    // Parse parameters
    consume(TokenType::LeftParen, "Expected '(' after function");
    
    while (!check(TokenType::RightParen) && !isAtEnd()) {
        Token param = consume(TokenType::Identifier, "Expected parameter name");
        func->params.push_back(param.value);
        
        // Optional type annotation
        if (match(TokenType::Colon)) {
            parseTypeAnnotation(); // Parse and discard for now
        }
        
        if (!check(TokenType::RightParen)) {
            consume(TokenType::Comma, "Expected ',' between parameters");
        }
    }
    
    consume(TokenType::RightParen, "Expected ')' after parameters");
    
    // Optional return type
    if (match(TokenType::Colon)) {
        func->returnType = parseTypeAnnotation();
    }
    
    // Function body
    func->body = parseBlockStatement();
    
    return func;
}

std::unique_ptr<Expr> Parser::parseArrowFunction() {
    auto arrow = std::make_unique<ArrowFunctionExpr>();
    arrow->location = getCurrentLocation();
    
    // Parameters - already parsed by caller
    // We need to handle this differently - arrow function parsing
    // should be triggered when we see => after identifiers or ()
    
    // This is a simplified version - real implementation needs
    // to be integrated with primary expression parsing
    
    reportError("Arrow functions need special handling in expression parsing");
    return nullptr;
}

std::unique_ptr<Expr> Parser::parseClassExpression() {
    auto classExpr = std::make_unique<ClassExpr>();
    classExpr->location = getCurrentLocation();
    
    // Optional name
    if (check(TokenType::Identifier)) {
        classExpr->name = advance().value;
    }
    
    // Extends clause
    if (match(TokenType::KeywordExtends)) {
        Token parent = consume(TokenType::Identifier, "Expected parent class name");
        classExpr->superclass = parent.value;
    }
    
    consume(TokenType::LeftBrace, "Expected '{' before class body");
    
    // Parse class members
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        // Static methods
        bool isStatic = match(TokenType::KeywordStatic);
        
        // Async methods
        bool isAsync = match(TokenType::KeywordAsync);
        
        // Getter/Setter
        bool isGetter = match(TokenType::KeywordGet);
        bool isSetter = match(TokenType::KeywordSet);
        
        // Method name
        Token name = consume(TokenType::Identifier, "Expected method name");
        
        ClassExpr::Method method;
        method.name = name.value;
        method.isStatic = isStatic;
        method.isAsync = isAsync;
        
        if (isGetter) {
            method.kind = ClassExpr::Method::Kind::Get;
        } else if (isSetter) {
            method.kind = ClassExpr::Method::Kind::Set;
        } else if (name.value == "constructor") {
            method.kind = ClassExpr::Method::Kind::Constructor;
        } else {
            method.kind = ClassExpr::Method::Kind::Method;
        }
        
        // Parse parameters
        consume(TokenType::LeftParen, "Expected '(' after method name");
        
        while (!check(TokenType::RightParen) && !isAtEnd()) {
            Token param = consume(TokenType::Identifier, "Expected parameter name");
            method.params.push_back(param.value);
            
            if (match(TokenType::Colon)) {
                parseTypeAnnotation();
            }
            
            if (!check(TokenType::RightParen)) {
                consume(TokenType::Comma, "Expected ',' between parameters");
            }
        }
        
        consume(TokenType::RightParen, "Expected ')' after parameters");
        
        // Return type
        if (match(TokenType::Colon)) {
            method.returnType = parseTypeAnnotation();
        }
        
        // Method body
        method.body = parseBlockStatement();
        
        classExpr->methods.push_back(std::move(method));
    }
    
    consume(TokenType::RightBrace, "Expected '}' after class body");
    
    return classExpr;
}

std::unique_ptr<Expr> Parser::parseTemplateLiteral() {
    Token lit = advance();
    std::string templateStr = lit.value;
    
    // Parse template literal with ${} expressions
    std::vector<std::string> quasis;
    std::vector<ExprPtr> expressions;
    
    size_t pos = 0;
    size_t start = 0;
    
    while ((pos = templateStr.find("${", start)) != std::string::npos) {
        // Add the string part before ${
        quasis.push_back(templateStr.substr(start, pos - start));
        
        // Find matching }
        size_t end = pos + 2;
        int braceDepth = 1;
        while (end < templateStr.length() && braceDepth > 0) {
            if (templateStr[end] == '{') braceDepth++;
            else if (templateStr[end] == '}') braceDepth--;
            end++;
        }
        
        if (braceDepth != 0) {
            reportError("Unterminated template expression");
            break;
        }
        
        // Extract and parse the expression
        std::string exprStr = templateStr.substr(pos + 2, end - pos - 3);
        
        // Create a mini-lexer for the expression
        Lexer exprLexer(lit.location.filename, exprStr);
        
        // Save current parser state
        size_t savedPos = current_;
        std::vector<Token> savedTokens = std::move(tokens_);
        
        // Parse expression with new tokens
        tokens_ = exprLexer.getAllTokens();
        current_ = 0;
        auto expr = parseAssignmentExpression();
        expressions.push_back(std::move(expr));
        
        // Restore parser state
        tokens_ = std::move(savedTokens);
        current_ = savedPos;
        
        start = end;
    }
    
    // Add the final string part
    quasis.push_back(templateStr.substr(start));
    
    auto templateLit = std::make_unique<TemplateLiteralExpr>(
        std::move(quasis),
        std::move(expressions)
    );
    templateLit->location = lit.location;
    return templateLit;
}

std::unique_ptr<Expr> Parser::parseParenthesizedExpression() {
    auto expr = parseExpression();
    consume(TokenType::RightParen, "Expected ')'");
    return expr;
}

// ==================== JSX/TSX Parsing ====================

std::unique_ptr<Expr> Parser::parseJSXElement() {
    consume(TokenType::LessThan, "Expected '<'");
    
    // JSX Fragment: <>...</>
    if (check(TokenType::Greater)) {
        advance(); // consume '>'
        auto fragment = std::make_unique<JSXFragment>();
        fragment->location = getCurrentLocation();
        
        // Parse children until </>
        while (!check(TokenType::LessThanSlash) && !isAtEnd()) {
            fragment->children.push_back(parseJSXChild());
        }
        
        consume(TokenType::LessThanSlash, "Expected '</'");
        consume(TokenType::Greater, "Expected '>' after fragment");
        
        return fragment;
    }
    
    // JSX Element: <TagName ...>
    if (!check(TokenType::Identifier)) {
        reportError("Expected JSX tag name");
        return std::make_unique<NullLiteral>();
    }
    
    std::string tagName = advance().value;
    auto element = std::make_unique<JSXElement>(tagName);
    element->location = getCurrentLocation();
    
    // Parse attributes
    while (!check(TokenType::Greater) && 
           !check(TokenType::SlashGreaterThan) && 
           !isAtEnd()) {
        
        // Spread attribute: {...expr}
        if (check(TokenType::LeftBrace)) {
            advance();
            if (match(TokenType::DotDotDot)) {
                auto expr = parseExpression();
                element->spreadAttributes.push_back(
                    std::make_unique<JSXSpreadAttribute>(std::move(expr)));
                consume(TokenType::RightBrace, "Expected '}' after spread");
                continue;
            }
            current_--; // not spread, restore
        }
        
        // Regular attribute: name={value} or name="value" or name
        if (check(TokenType::Identifier)) {
            std::string attrName = advance().value;
            ExprPtr attrValue = nullptr;
            
            if (match(TokenType::Equal)) {
                if (check(TokenType::StringLiteral)) {
                    attrValue = parseLiteral();
                } else if (match(TokenType::LeftBrace)) {
                    attrValue = parseExpression();
                    consume(TokenType::RightBrace, "Expected '}' after expression");
                }
            }
            
            element->attributes.push_back(
                std::make_unique<JSXAttribute>(attrName, std::move(attrValue)));
        } else {
            break;
        }
    }
    
    // Self-closing: <Tag />
    if (match(TokenType::SlashGreaterThan)) {
        element->selfClosing = true;
        return element;
    }
    
    consume(TokenType::Greater, "Expected '>' or '/>'");
    
    // Parse children until </TagName>
    while (!check(TokenType::LessThanSlash) && !isAtEnd()) {
        element->children.push_back(parseJSXChild());
    }
    
    // Closing tag: </TagName>
    consume(TokenType::LessThanSlash, "Expected closing tag");
    if (!check(TokenType::Identifier) || advance().value != tagName) {
        reportError("Mismatched JSX closing tag");
    }
    consume(TokenType::Greater, "Expected '>' after closing tag");
    
    return element;
}

std::unique_ptr<Expr> Parser::parseJSXChild() {
    // JSX Expression: {expr}
    if (check(TokenType::LeftBrace)) {
        advance();
        auto expr = parseExpression();
        consume(TokenType::RightBrace, "Expected '}'");
        return std::make_unique<JSXExpressionContainer>(std::move(expr));
    }
    
    // Nested JSX Element
    if (check(TokenType::LessThan)) {
        return parseJSXElement();
    }
    
    // JSX Text - consume until special character
    std::string text;
    while (!check(TokenType::LessThan) && 
           !check(TokenType::LessThanSlash) &&
           !check(TokenType::LeftBrace) &&
           !isAtEnd()) {
        text += advance().value;
    }
    
    if (!text.empty()) {
        return std::make_unique<JSXText>(text);
    }
    
    reportError("Unexpected JSX child");
    return std::make_unique<NullLiteral>();
}

// ==================== Destructuring Patterns ====================

std::unique_ptr<Pattern> Parser::parseBindingPattern() {
    if (check(TokenType::LeftBrace)) {
        return parseObjectPattern();
    } else if (check(TokenType::LeftBracket)) {
        return parseArrayPattern();
    } else if (check(TokenType::Identifier)) {
        auto name = advance().value;
        TypePtr type = nullptr;
        if (match(TokenType::Colon)) {
            type = parseTypeAnnotation();
        }
        return std::make_unique<IdentifierPattern>(name, std::move(type));
    }
    
    reportError("Expected binding pattern");
    return nullptr;
}

std::unique_ptr<Pattern> Parser::parseObjectPattern() {
    consume(TokenType::LeftBrace, "Expected '{'");
    
    auto pattern = std::make_unique<ObjectPattern>();
    
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        // Rest pattern: ...rest
        if (match(TokenType::DotDotDot)) {
            pattern->rest = parseBindingPattern();
            break;
        }
        
        // Property pattern: key or key: pattern
        if (!check(TokenType::Identifier)) {
            reportError("Expected property name");
            break;
        }
        
        std::string key = advance().value;
        ObjectPattern::Property prop;
        prop.key = key;
        prop.shorthand = true;
        
        // key: pattern
        if (match(TokenType::Colon)) {
            prop.shorthand = false;
            prop.value = parseBindingPattern();
        } else {
            // Shorthand: {x} means {x: x}
            prop.value = std::make_unique<IdentifierPattern>(key);
        }
        
        // Default value: = expr
        if (match(TokenType::Equal)) {
            prop.defaultValue = parseAssignmentExpression();
        }
        
        pattern->properties.push_back(std::move(prop));
        
        if (!check(TokenType::RightBrace)) {
            consume(TokenType::Comma, "Expected ',' or '}'");
        }
    }
    
    consume(TokenType::RightBrace, "Expected '}'");
    return pattern;
}

std::unique_ptr<Pattern> Parser::parseArrayPattern() {
    consume(TokenType::LeftBracket, "Expected '['");
    
    auto pattern = std::make_unique<ArrayPattern>();
    
    while (!check(TokenType::RightBracket) && !isAtEnd()) {
        // Rest pattern: ...rest
        if (match(TokenType::DotDotDot)) {
            pattern->rest = parseBindingPattern();
            break;
        }
        
        // Hole: [, , x]
        if (check(TokenType::Comma)) {
            pattern->elements.push_back(nullptr);
        } else {
            auto element = parseBindingPattern();
            
            // Default value: = expr
            if (match(TokenType::Equal)) {
                auto defaultValue = parseAssignmentExpression();
                element = std::make_unique<AssignmentPattern>(
                    std::move(element), std::move(defaultValue));
            }
            
            pattern->elements.push_back(std::move(element));
        }
        
        if (!check(TokenType::RightBracket)) {
            consume(TokenType::Comma, "Expected ',' or ']'");
        }
    }
    
    consume(TokenType::RightBracket, "Expected ']'");
    return pattern;
}

// Helper function to convert TokenType to BinaryExpr::Op
BinaryExpr::Op tokenToBinaryOp(TokenType type) {
    switch (type) {
        case TokenType::Plus: return BinaryExpr::Op::Add;
        case TokenType::Minus: return BinaryExpr::Op::Sub;
        case TokenType::Star: return BinaryExpr::Op::Mul;
        case TokenType::Slash: return BinaryExpr::Op::Div;
        case TokenType::Percent: return BinaryExpr::Op::Mod;
        case TokenType::StarStar: return BinaryExpr::Op::Pow;
        case TokenType::Ampersand: return BinaryExpr::Op::BitAnd;
        case TokenType::Pipe: return BinaryExpr::Op::BitOr;
        case TokenType::Caret: return BinaryExpr::Op::BitXor;
        case TokenType::LessLess: return BinaryExpr::Op::LeftShift;
        case TokenType::GreaterGreater: return BinaryExpr::Op::RightShift;
        case TokenType::GreaterGreaterGreater: return BinaryExpr::Op::UnsignedRightShift;
        case TokenType::EqualEqual: return BinaryExpr::Op::Equal;
        case TokenType::ExclamationEqual: return BinaryExpr::Op::NotEqual;
        case TokenType::EqualEqualEqual: return BinaryExpr::Op::StrictEqual;
        case TokenType::ExclamationEqualEqual: return BinaryExpr::Op::StrictNotEqual;
        case TokenType::Less: return BinaryExpr::Op::Less;
        case TokenType::Greater: return BinaryExpr::Op::Greater;
        case TokenType::LessEqual: return BinaryExpr::Op::LessEqual;
        case TokenType::GreaterEqual: return BinaryExpr::Op::GreaterEqual;
        case TokenType::AmpersandAmpersand: return BinaryExpr::Op::LogicalAnd;
        case TokenType::PipePipe: return BinaryExpr::Op::LogicalOr;
        case TokenType::QuestionQuestion: return BinaryExpr::Op::NullishCoalescing;
        case TokenType::KeywordIn: return BinaryExpr::Op::In;
        case TokenType::KeywordInstanceof: return BinaryExpr::Op::Instanceof;
        default: return BinaryExpr::Op::Add; // fallback
    }
}

// Helper function to convert TokenType to UnaryExpr::Op
UnaryExpr::Op tokenToUnaryOp(TokenType type) {
    switch (type) {
        case TokenType::Plus: return UnaryExpr::Op::Plus;
        case TokenType::Minus: return UnaryExpr::Op::Minus;
        case TokenType::Exclamation: return UnaryExpr::Op::Not;
        case TokenType::Tilde: return UnaryExpr::Op::BitNot;
        case TokenType::KeywordTypeof: return UnaryExpr::Op::Typeof;
        case TokenType::KeywordVoid: return UnaryExpr::Op::Void;
        case TokenType::KeywordDelete: return UnaryExpr::Op::Delete;
        case TokenType::KeywordAwait: return UnaryExpr::Op::Await;
        default: return UnaryExpr::Op::Plus; // fallback
    }
}

// Helper function to convert TokenType to UpdateExpr::Op
UpdateExpr::Op tokenToUpdateOp(TokenType type) {
    switch (type) {
        case TokenType::PlusPlus: return UpdateExpr::Op::Increment;
        case TokenType::MinusMinus: return UpdateExpr::Op::Decrement;
        default: return UpdateExpr::Op::Increment; // fallback
    }
}

// Helper function to convert TokenType to AssignmentExpr::Op
AssignmentExpr::Op tokenToAssignmentOp(TokenType type) {
    switch (type) {
        case TokenType::Equal: return AssignmentExpr::Op::Assign;
        case TokenType::PlusEqual: return AssignmentExpr::Op::AddAssign;
        case TokenType::MinusEqual: return AssignmentExpr::Op::SubAssign;
        case TokenType::StarEqual: return AssignmentExpr::Op::MulAssign;
        case TokenType::SlashEqual: return AssignmentExpr::Op::DivAssign;
        case TokenType::PercentEqual: return AssignmentExpr::Op::ModAssign;
        case TokenType::StarStarEqual: return AssignmentExpr::Op::PowAssign;
        case TokenType::LessLessEqual: return AssignmentExpr::Op::LeftShiftAssign;
        case TokenType::GreaterGreaterEqual: return AssignmentExpr::Op::RightShiftAssign;
        case TokenType::GreaterGreaterGreaterEqual: return AssignmentExpr::Op::UnsignedRightShiftAssign;
        case TokenType::AmpersandEqual: return AssignmentExpr::Op::BitAndAssign;
        case TokenType::PipeEqual: return AssignmentExpr::Op::BitOrAssign;
        case TokenType::CaretEqual: return AssignmentExpr::Op::BitXorAssign;
        case TokenType::AmpersandAmpersandEqual: return AssignmentExpr::Op::LogicalAndAssign;
        case TokenType::PipePipeEqual: return AssignmentExpr::Op::LogicalOrAssign;
        case TokenType::QuestionQuestionEqual: return AssignmentExpr::Op::NullishCoalescingAssign;
        default: return AssignmentExpr::Op::Assign; // fallback
    }
}

} // namespace nova



