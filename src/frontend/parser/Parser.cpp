#include "nova/Frontend/Parser.h"
#include <sstream>
#include <unordered_map>

namespace nova {

Parser::Parser(Lexer& lexer) 
    : lexer_(lexer), current_(0), jsxMode_(lexer.isJSXMode()) {
    // Pre-fetch all tokens for easier lookahead
    Token token;
    do {
        token = lexer_.nextToken();
        tokens_.push_back(token);
    } while (token.type != TokenType::EndOfFile);
    // Track a top-level strict directive for grammar productions whose
    // Annex-B behavior differs between strict and non-strict scripts.
    if (!tokens_.empty() &&
        tokens_.front().type == TokenType::StringLiteral &&
        tokens_.front().value == "use strict") {
        strictMode_ = true;
    }
}

std::unique_ptr<Program> Parser::parseProgram() {
    std::vector<StmtPtr> statements;
    
    while (!isAtEnd()) {
        try {
            auto stmt = parseStatement();
            if (stmt) {
                statements.push_back(std::move(stmt));
            }
        } catch (const std::exception& e) {
            reportError(e.what());
            synchronize();
        }
    }
    
    // Connect overload signatures to their implementation without changing
    // source order or ownership in the AST.
    std::unordered_map<std::string, std::vector<FunctionDecl*>> overloads;
    std::unordered_map<std::string, FunctionDecl*> implementations;
    for (auto& statement : statements) {
        auto* declarationStatement =
            dynamic_cast<DeclStmt*>(statement.get());
        auto* function = declarationStatement
            ? dynamic_cast<FunctionDecl*>(
                  declarationStatement->declaration.get())
            : nullptr;
        if (!function || function->isDeclare) continue;
        if (function->isOverload) {
            overloads[function->name].push_back(function);
        } else {
            implementations[function->name] = function;
        }
    }
    for (auto& [name, signatures] : overloads) {
        auto found = implementations.find(name);
        if (found == implementations.end()) continue;
        found->second->overloads = signatures;
        for (FunctionDecl* signature : signatures) {
            signature->implementation = found->second;
        }
    }

    auto program = std::make_unique<Program>(std::move(statements));
    program->location = getCurrentLocation();
    
    return program;
}

// Token management
Token Parser::peek(int offset) const {
    size_t index = current_ + offset;
    if (index >= tokens_.size()) {
        return tokens_.back(); // Return EOF
    }
    return tokens_[index];
}

Token Parser::advance() {
    if (!isAtEnd()) {
        current_++;
    }
    return tokens_[current_ - 1];
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) {
    if (isAtEnd()) return false;
    return peek().type == type;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }
    reportError(message);
    throw std::runtime_error(message);
}

bool Parser::checkIdentifierName() const {
    if (isAtEnd()) return false;
    const Token token = peek();
    return token.type == TokenType::Identifier || token.isKeyword() ||
           token.isLiteral();
}

bool Parser::checkBindingIdentifier() const {
    if (isAtEnd()) return false;
    switch (peek().type) {
        case TokenType::Identifier:
        case TokenType::KeywordAsync:
        case TokenType::KeywordFrom:
        case TokenType::KeywordAs:
        case TokenType::KeywordOf:
        case TokenType::KeywordType:
        case TokenType::KeywordInterface:
        case TokenType::KeywordNamespace:
        case TokenType::KeywordDeclare:
        case TokenType::KeywordAbstract:
        case TokenType::KeywordPublic:
        case TokenType::KeywordPrivate:
        case TokenType::KeywordProtected:
        case TokenType::KeywordReadonly:
        case TokenType::KeywordStatic:
        case TokenType::KeywordGet:
        case TokenType::KeywordSet:
        case TokenType::KeywordOverride:
        case TokenType::KeywordSatisfies:
        case TokenType::KeywordKeyof:
        case TokenType::KeywordInfer:
        case TokenType::KeywordIs:
        case TokenType::KeywordAsserts:
        case TokenType::KeywordUnique:
        case TokenType::KeywordImplements:
        case TokenType::KeywordUsing:
            return true;
        default:
            return false;
    }
}

Token Parser::consumeIdentifierName(const std::string& message) {
    if (checkIdentifierName()) return advance();
    reportError(message);
    throw std::runtime_error(message);
}

Token Parser::consumeBindingIdentifier(const std::string& message) {
    if (checkBindingIdentifier()) return advance();
    reportError(message);
    throw std::runtime_error(message);
}

bool Parser::isAtEnd() const {
    return current_ >= tokens_.size() || peek().type == TokenType::EndOfFile;
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        // Stop at statement boundaries
        if (peek(-1).type == TokenType::Semicolon) return;
        
        switch (peek().type) {
            case TokenType::KeywordClass:
            case TokenType::KeywordFunction:
            case TokenType::KeywordVar:
            case TokenType::KeywordLet:
            case TokenType::KeywordConst:
            case TokenType::KeywordFor:
            case TokenType::KeywordIf:
            case TokenType::KeywordWhile:
            case TokenType::KeywordReturn:
                return;
            default:
                advance();
        }
    }
}

void Parser::reportError(const std::string& message) {
    auto loc = getCurrentLocation();
    std::stringstream ss;
    ss << loc.filename << ":" << loc.line << ":" << loc.column
       << ": error SyntaxError: " << message;
    errors_.push_back(ss.str());
}

SourceLocation Parser::getCurrentLocation() const {
    if (current_ < tokens_.size()) {
        return tokens_[current_].location;
    }
    if (!tokens_.empty()) {
        return tokens_.back().location;
    }
    return SourceLocation("<unknown>", 1, 1, 0);
}

} // namespace nova
