#pragma once

#include "Token.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace nova {

class Lexer {
public:
    // Main constructor
    explicit Lexer(const std::string& filename, const std::string& source);
    
    // Convenience constructor for testing (filename = "<input>")
    explicit Lexer(const std::string& source);
    
    Token nextToken();
    Token peekToken();

    // Try to lex a regex literal when parser expects one (context-dependent)
    // Call this when parser sees a Slash token and expects a regex
    Token tryLexRegex();

    const std::vector<Token>& getAllTokens();
    
    bool hasErrors() const { return !errors_.empty(); }
    const std::vector<std::string>& getErrors() const { return errors_; }
    
private:
    std::string filename_;
    std::string source_;
    size_t position_;
    uint32_t line_;
    uint32_t column_;

    std::vector<Token> tokens_;
    std::vector<std::string> errors_;

    TokenType lastTokenType_ = TokenType::Invalid;  // Track last token for regex context
    
    static std::unordered_map<std::string, TokenType> keywords_;
    
    void initializeKeywords();
    
    char currentChar() const;
    char peekChar(size_t offset = 1) const;
    void advance();
    void skipWhitespace();
    void skipLineComment();
    void skipBlockComment();
    
    Token lexNumber();
    Token lexString(char quote);
    Token lexTemplateLiteral();
    Token lexIdentifierOrKeyword();
    Token lexRegex();
    Token lexOperator();
    
    SourceLocation currentLocation() const;
    void reportError(const std::string& message);
    
    bool isDigit(char c) const;
    bool isHexDigit(char c) const;
    bool isBinaryDigit(char c) const;
    bool isOctalDigit(char c) const;
    bool isIdentifierStart(char c) const;
    bool isIdentifierPart(char c) const;
    bool isWhitespace(char c) const;
};

} // namespace nova
