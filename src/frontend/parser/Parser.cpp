#include "nova/Frontend/Parser.h"
#include <sstream>

namespace nova {

Parser::Parser(Lexer& lexer) 
    : lexer_(lexer), current_(0) {
    // Pre-fetch all tokens for easier lookahead
    Token token;
    do {
        token = lexer_.nextToken();
        tokens_.push_back(token);
    } while (token.type != TokenType::EndOfFile);
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
       << ": error: " << message;
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
