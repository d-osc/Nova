#include "nova/Frontend/Lexer.h"
#include <iostream>
#include "nova/Frontend/Token.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace nova {

std::unordered_map<std::string, TokenType> Lexer::keywords_;

Lexer::Lexer(const std::string& filename, const std::string& source)
    : filename_(filename), source_(source), position_(0), line_(1), column_(1) {
    if (keywords_.empty()) {
        initializeKeywords();
    }
}

Lexer::Lexer(const std::string& source)
    : Lexer("<input>", source) {
}

void Lexer::initializeKeywords() {
    keywords_ = {
        // JavaScript Keywords
        {"break", TokenType::KeywordBreak},
        {"case", TokenType::KeywordCase},
        {"catch", TokenType::KeywordCatch},
        {"class", TokenType::KeywordClass},
        {"const", TokenType::KeywordConst},
        {"continue", TokenType::KeywordContinue},
        {"debugger", TokenType::KeywordDebugger},
        {"default", TokenType::KeywordDefault},
        {"delete", TokenType::KeywordDelete},
        {"do", TokenType::KeywordDo},
        {"else", TokenType::KeywordElse},
        {"export", TokenType::KeywordExport},
        {"extends", TokenType::KeywordExtends},
        {"finally", TokenType::KeywordFinally},
        {"for", TokenType::KeywordFor},
        {"function", TokenType::KeywordFunction},
        {"if", TokenType::KeywordIf},
        {"import", TokenType::KeywordImport},
        {"in", TokenType::KeywordIn},
        {"instanceof", TokenType::KeywordInstanceof},
        {"let", TokenType::KeywordLet},
        {"new", TokenType::KeywordNew},
        {"return", TokenType::KeywordReturn},
        {"super", TokenType::KeywordSuper},
        {"switch", TokenType::KeywordSwitch},
        {"this", TokenType::KeywordThis},
        {"throw", TokenType::KeywordThrow},
        {"try", TokenType::KeywordTry},
        {"typeof", TokenType::KeywordTypeof},
        {"var", TokenType::KeywordVar},
        {"void", TokenType::KeywordVoid},
        {"while", TokenType::KeywordWhile},
        {"with", TokenType::KeywordWith},
        {"yield", TokenType::KeywordYield},
        {"await", TokenType::KeywordAwait},
        {"async", TokenType::KeywordAsync},
        {"from", TokenType::KeywordFrom},
        {"as", TokenType::KeywordAs},
        {"of", TokenType::KeywordOf},
        
        // TypeScript Keywords
        {"type", TokenType::KeywordType},
        {"interface", TokenType::KeywordInterface},
        {"namespace", TokenType::KeywordNamespace},
        {"declare", TokenType::KeywordDeclare},
        {"abstract", TokenType::KeywordAbstract},
        {"public", TokenType::KeywordPublic},
        {"private", TokenType::KeywordPrivate},
        {"protected", TokenType::KeywordProtected},
        {"readonly", TokenType::KeywordReadonly},
        {"static", TokenType::KeywordStatic},
        {"get", TokenType::KeywordGet},
        {"set", TokenType::KeywordSet},
        {"implements", TokenType::KeywordImplements},
        {"override", TokenType::KeywordOverride},
        {"satisfies", TokenType::KeywordSatisfies},
        {"keyof", TokenType::KeywordKeyof},
        {"infer", TokenType::KeywordInfer},
        {"is", TokenType::KeywordIs},
        {"asserts", TokenType::KeywordAsserts},
        {"enum", TokenType::KeywordEnum},
        {"unique", TokenType::KeywordUnique},
        {"using", TokenType::KeywordUsing},

        // Literals
        {"true", TokenType::TrueLiteral},
        {"false", TokenType::FalseLiteral},
        {"null", TokenType::NullLiteral},
        {"undefined", TokenType::UndefinedLiteral}
    };
}

// Helper to check if last token type allows regex to follow
bool canPrecedeRegex(TokenType type) {
    switch (type) {
        case TokenType::Invalid:  // Start of file
        case TokenType::LeftParen:
        case TokenType::LeftBracket:
        case TokenType::LeftBrace:
        case TokenType::Comma:
        case TokenType::Semicolon:
        case TokenType::Colon:
        case TokenType::Question:
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:
        case TokenType::StarStar:
        case TokenType::Less:
        case TokenType::Greater:
        case TokenType::LessEqual:
        case TokenType::GreaterEqual:
        case TokenType::EqualEqual:
        case TokenType::ExclamationEqual:
        case TokenType::EqualEqualEqual:
        case TokenType::ExclamationEqualEqual:
        case TokenType::Ampersand:
        case TokenType::Pipe:
        case TokenType::Caret:
        case TokenType::AmpersandAmpersand:
        case TokenType::PipePipe:
        case TokenType::Equal:
        case TokenType::PlusEqual:
        case TokenType::MinusEqual:
        case TokenType::StarEqual:
        case TokenType::SlashEqual:
        case TokenType::PercentEqual:
        case TokenType::LessLess:
        case TokenType::GreaterGreater:
        case TokenType::GreaterGreaterGreater:
        case TokenType::Arrow:
        case TokenType::KeywordReturn:
        case TokenType::KeywordThrow:
        case TokenType::KeywordCase:
        case TokenType::KeywordNew:
        case TokenType::KeywordIn:
        case TokenType::KeywordOf:
        case TokenType::KeywordTypeof:
        case TokenType::KeywordDelete:
        case TokenType::KeywordVoid:
        case TokenType::KeywordYield:
        case TokenType::KeywordAwait:
        case TokenType::KeywordIf:
        case TokenType::KeywordElse:
        case TokenType::KeywordWhile:
        case TokenType::KeywordDo:
        case TokenType::KeywordFor:
        case TokenType::KeywordSwitch:
        case TokenType::KeywordWith:
        case TokenType::KeywordExport:
        case TokenType::KeywordDefault:
        case TokenType::Exclamation:
        case TokenType::Tilde:
        case TokenType::QuestionQuestion:
            return true;
        default:
            return false;
    }
}

Token Lexer::nextToken() {
    skipWhitespace();

    if (position_ >= source_.length()) {
        Token token(TokenType::EndOfFile, "", currentLocation());
        lastTokenType_ = token.type;
        return token;
    }

    char current = currentChar();

    // Debug: Print EVERY character and check for backtick
    static int tokenCount = 0;
    tokenCount++;
    if (tokenCount <= 50) {  // Only first 50 tokens to avoid spam
        std::cerr << "Token #" << tokenCount << " pos=" << position_ << " char='" << current << "' (ASCII " << (int)current << ")" << std::endl;
        if (current == '`') {
            std::cerr << "  ^^^ THIS IS A BACKTICK!" << std::endl;
        }
    }

    // Comments
    if (current == '/' && peekChar() == '/') {
        skipLineComment();
        return nextToken();
    }

    if (current == '/' && peekChar() == '*') {
        skipBlockComment();
        return nextToken();
    }

    // Check for regex literal (context-dependent)
    // A '/' starts a regex if the last token allows an expression to start
    if (current == '/' && peekChar() != '=' && canPrecedeRegex(lastTokenType_)) {
        Token token = lexRegex();
        lastTokenType_ = token.type;
        return token;
    }

    // Numbers
    if (isDigit(current)) {
        Token token = lexNumber();
        lastTokenType_ = token.type;
        return token;
    }

    // Strings
    if (current == '"' || current == '\'') {
        Token token = lexString(current);
        lastTokenType_ = token.type;
        return token;
    }

    // Template literals
    if (current == '`') {
        std::cerr << "*** LEXER: Found backtick, lexing template literal..." << std::endl;
        Token token = lexTemplateLiteral();
        std::cerr << "*** LEXER: Template literal token value = '" << token.value << "'" << std::endl;
        lastTokenType_ = token.type;
        return token;
    }

    // Identifiers and keywords
    if (isIdentifierStart(current)) {
        Token token = lexIdentifierOrKeyword();
        lastTokenType_ = token.type;
        return token;
    }

    // Operators and punctuation
    Token token = lexOperator();
    lastTokenType_ = token.type;
    return token;
}

Token Lexer::tryLexRegex() {
    // If current char is '/', try to lex as regex regardless of context
    if (position_ < source_.length() && currentChar() == '/') {
        return lexRegex();
    }
    return Token(TokenType::Invalid, "", currentLocation());
}

void Lexer::skipWhitespace() {
    while (position_ < source_.length() && isWhitespace(currentChar())) {
        if (currentChar() == '\n') {
            line_++;
            column_ = 0;
        }
        advance();
    }
}

void Lexer::skipLineComment() {
    while (position_ < source_.length() && currentChar() != '\n') {
        advance();
    }
}

void Lexer::skipBlockComment() {
    advance();  // skip /
    advance();  // skip *
    
    while (position_ < source_.length()) {
        if (currentChar() == '*' && peekChar() == '/') {
            advance();  // skip *
            advance();  // skip /
            break;
        }
        if (currentChar() == '\n') {
            line_++;
            column_ = 0;
        }
        advance();
    }
}

Token Lexer::lexNumber() {
    SourceLocation loc = currentLocation();
    std::string value;
    
    // Handle binary, octal, hex
    if (currentChar() == '0') {
        value += currentChar();
        advance();
        
        if (currentChar() == 'b' || currentChar() == 'B') {
            value += currentChar();
            advance();
            while (isBinaryDigit(currentChar())) {
                value += currentChar();
                advance();
            }
            return Token(TokenType::NumberLiteral, value, loc);
        }
        else if (currentChar() == 'o' || currentChar() == 'O') {
            value += currentChar();
            advance();
            while (isOctalDigit(currentChar())) {
                value += currentChar();
                advance();
            }
            return Token(TokenType::NumberLiteral, value, loc);
        }
        else if (currentChar() == 'x' || currentChar() == 'X') {
            value += currentChar();
            advance();
            while (isHexDigit(currentChar())) {
                value += currentChar();
                advance();
            }
            return Token(TokenType::NumberLiteral, value, loc);
        }
    }
    
    // Decimal number
    while (isDigit(currentChar()) || currentChar() == '_') {
        if (currentChar() != '_') {
            value += currentChar();
        }
        advance();
    }
    
    // Fractional part
    if (currentChar() == '.' && isDigit(peekChar())) {
        value += currentChar();
        advance();
        while (isDigit(currentChar()) || currentChar() == '_') {
            if (currentChar() != '_') {
                value += currentChar();
            }
            advance();
        }
    }
    
    // Exponent
    if (currentChar() == 'e' || currentChar() == 'E') {
        value += currentChar();
        advance();
        if (currentChar() == '+' || currentChar() == '-') {
            value += currentChar();
            advance();
        }
        while (isDigit(currentChar())) {
            value += currentChar();
            advance();
        }
    }
    
    // BigInt suffix
    if (currentChar() == 'n') {
        value += currentChar();
        advance();
    }
    
    return Token(TokenType::NumberLiteral, value, loc);
}

Token Lexer::lexString(char quote) {
    SourceLocation loc = currentLocation();
    std::string value;
    advance();  // skip opening quote

    while (position_ < source_.length() && currentChar() != quote) {
        if (currentChar() == '\\') {
            advance();
            if (position_ < source_.length()) {
                // Process escape sequences
                char escapeChar = currentChar();
                switch (escapeChar) {
                    case 'n':  value += '\n'; break;  // newline
                    case 't':  value += '\t'; break;  // tab
                    case 'r':  value += '\r'; break;  // carriage return
                    case 'b':  value += '\b'; break;  // backspace
                    case 'f':  value += '\f'; break;  // form feed
                    case 'v':  value += '\v'; break;  // vertical tab
                    case '0':  value += '\0'; break;  // null
                    case '\\': value += '\\'; break;  // backslash
                    case '\'': value += '\''; break;  // single quote
                    case '"':  value += '"';  break;  // double quote
                    default:
                        // For unknown escape sequences, keep the backslash and character
                        value += '\\';
                        value += escapeChar;
                        break;
                }
                advance();
            }
        } else {
            value += currentChar();
            advance();
        }
    }

    if (currentChar() == quote) {
        advance();  // skip closing quote
    } else {
        reportError("Unterminated string");
    }

    return Token(TokenType::StringLiteral, value, loc);
}

Token Lexer::lexTemplateLiteral() {
    SourceLocation loc = currentLocation();
    std::string value;
    advance();  // skip `
    
    while (position_ < source_.length() && currentChar() != '`') {
        value += currentChar();
        advance();
    }
    
    if (currentChar() == '`') {
        advance();
    }
    
    return Token(TokenType::TemplateLiteral, value, loc);
}

Token Lexer::lexIdentifierOrKeyword() {
    SourceLocation loc = currentLocation();
    std::string value;
    
    while (position_ < source_.length() && isIdentifierPart(currentChar())) {
        value += currentChar();
        advance();
    }
    
    auto it = keywords_.find(value);
    if (it != keywords_.end()) {
        return Token(it->second, value, loc);
    }
    
    return Token(TokenType::Identifier, value, loc);
}

Token Lexer::lexOperator() {
    SourceLocation loc = currentLocation();
    char current = currentChar();
    
    advance();
    
    switch (current) {
        case '+':
            if (currentChar() == '+') { advance(); return Token(TokenType::PlusPlus, "++", loc); }
            if (currentChar() == '=') { advance(); return Token(TokenType::PlusEqual, "+=", loc); }
            return Token(TokenType::Plus, "+", loc);
            
        case '-':
            if (currentChar() == '-') { advance(); return Token(TokenType::MinusMinus, "--", loc); }
            if (currentChar() == '=') { advance(); return Token(TokenType::MinusEqual, "-=", loc); }
            return Token(TokenType::Minus, "-", loc);
            
        case '*':
            if (currentChar() == '*') {
                advance();
                if (currentChar() == '=') { advance(); return Token(TokenType::StarStarEqual, "**=", loc); }
                return Token(TokenType::StarStar, "**", loc);
            }
            if (currentChar() == '=') { advance(); return Token(TokenType::StarEqual, "*=", loc); }
            return Token(TokenType::Star, "*", loc);
            
        case '/':
            if (currentChar() == '=') { advance(); return Token(TokenType::SlashEqual, "/=", loc); }
            return Token(TokenType::Slash, "/", loc);
            
        case '%':
            if (currentChar() == '=') { advance(); return Token(TokenType::PercentEqual, "%=", loc); }
            return Token(TokenType::Percent, "%", loc);
            
        case '=':
            if (currentChar() == '=') {
                advance();
                if (currentChar() == '=') { advance(); return Token(TokenType::EqualEqualEqual, "===", loc); }
                return Token(TokenType::EqualEqual, "==", loc);
            }
            if (currentChar() == '>') { advance(); return Token(TokenType::Arrow, "=>", loc); }
            return Token(TokenType::Equal, "=", loc);
            
        case '!':
            if (currentChar() == '=') {
                advance();
                if (currentChar() == '=') { advance(); return Token(TokenType::ExclamationEqualEqual, "!==", loc); }
                return Token(TokenType::ExclamationEqual, "!=", loc);
            }
            return Token(TokenType::Exclamation, "!", loc);
            
        case '<':
            if (currentChar() == '<') {
                advance();
                if (currentChar() == '=') { advance(); return Token(TokenType::LessLessEqual, "<<=", loc); }
                return Token(TokenType::LessLess, "<<", loc);
            }
            if (currentChar() == '=') { advance(); return Token(TokenType::LessEqual, "<=", loc); }
            return Token(TokenType::Less, "<", loc);
            
        case '>':
            if (currentChar() == '>') {
                advance();
                if (currentChar() == '>') {
                    advance();
                    if (currentChar() == '=') { advance(); return Token(TokenType::GreaterGreaterGreaterEqual, ">>>=", loc); }
                    return Token(TokenType::GreaterGreaterGreater, ">>>", loc);
                }
                if (currentChar() == '=') { advance(); return Token(TokenType::GreaterGreaterEqual, ">>=", loc); }
                return Token(TokenType::GreaterGreater, ">>", loc);
            }
            if (currentChar() == '=') { advance(); return Token(TokenType::GreaterEqual, ">=", loc); }
            return Token(TokenType::Greater, ">", loc);
            
        case '&':
            if (currentChar() == '&') {
                advance();
                if (currentChar() == '=') { advance(); return Token(TokenType::AmpersandAmpersandEqual, "&&=", loc); }
                return Token(TokenType::AmpersandAmpersand, "&&", loc);
            }
            if (currentChar() == '=') { advance(); return Token(TokenType::AmpersandEqual, "&=", loc); }
            return Token(TokenType::Ampersand, "&", loc);
            
        case '|':
            if (currentChar() == '|') {
                advance();
                if (currentChar() == '=') { advance(); return Token(TokenType::PipePipeEqual, "||=", loc); }
                return Token(TokenType::PipePipe, "||", loc);
            }
            if (currentChar() == '=') { advance(); return Token(TokenType::PipeEqual, "|=", loc); }
            return Token(TokenType::Pipe, "|", loc);
            
        case '?':
            if (currentChar() == '?') {
                advance();
                if (currentChar() == '=') { advance(); return Token(TokenType::QuestionQuestionEqual, "?\?=", loc); }
                return Token(TokenType::QuestionQuestion, "?\?", loc);
            }
            if (currentChar() == '.') { advance(); return Token(TokenType::QuestionDot, "?.", loc); }
            return Token(TokenType::Question, "?", loc);
            
        case '.':
            if (currentChar() == '.' && peekChar() == '.') {
                advance(); advance();
                return Token(TokenType::DotDotDot, "...", loc);
            }
            return Token(TokenType::Dot, ".", loc);
            
        case ':': return Token(TokenType::Colon, ":", loc);
        case ';': return Token(TokenType::Semicolon, ";", loc);
        case ',': return Token(TokenType::Comma, ",", loc);
        case '#': return Token(TokenType::Hash, "#", loc);
        case '@': return Token(TokenType::At, "@", loc);
        case '(': return Token(TokenType::LeftParen, "(", loc);
        case ')': return Token(TokenType::RightParen, ")", loc);
        case '{': return Token(TokenType::LeftBrace, "{", loc);
        case '}': return Token(TokenType::RightBrace, "}", loc);
        case '[': return Token(TokenType::LeftBracket, "[", loc);
        case ']': return Token(TokenType::RightBracket, "]", loc);
        case '^':
            if (currentChar() == '=') { advance(); return Token(TokenType::CaretEqual, "^=", loc); }
            return Token(TokenType::Caret, "^", loc);
        case '~': return Token(TokenType::Tilde, "~", loc);
        
        default:
            return Token(TokenType::Invalid, std::string(1, current), loc);
    }
}

char Lexer::currentChar() const {
    return position_ < source_.length() ? source_[position_] : '\0';
}

char Lexer::peekChar(size_t offset) const {
    size_t pos = position_ + offset;
    return pos < source_.length() ? source_[pos] : '\0';
}

void Lexer::advance() {
    if (position_ < source_.length()) {
        position_++;
        column_++;
    }
}

SourceLocation Lexer::currentLocation() const {
    return SourceLocation(filename_, line_, column_, static_cast<uint32_t>(position_));
}

void Lexer::reportError(const std::string& message) {
    std::stringstream ss;
    ss << filename_ << ":" << line_ << ":" << column_ << ": error: " << message;
    errors_.push_back(ss.str());
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isHexDigit(char c) const {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool Lexer::isBinaryDigit(char c) const {
    return c == '0' || c == '1';
}

bool Lexer::isOctalDigit(char c) const {
    return c >= '0' && c <= '7';
}

bool Lexer::isIdentifierStart(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
}

bool Lexer::isIdentifierPart(char c) const {
    return isIdentifierStart(c) || isDigit(c);
}

bool Lexer::isWhitespace(char c) const {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

Token Lexer::lexRegex() {
    SourceLocation loc = currentLocation();
    std::string pattern;
    
    advance();  // skip initial '/'
    
    // Lex pattern
    while (position_ < source_.length() && currentChar() != '/') {
        if (currentChar() == '\\') {
            pattern += currentChar();
            advance();
            if (position_ < source_.length()) {
                pattern += currentChar();
                advance();
            }
        } else if (currentChar() == '\n') {
            reportError("Unterminated regular expression");
            return Token(TokenType::Invalid, pattern, loc);
        } else if (currentChar() == '[') {
            // Character class - don't treat / inside [] as end
            pattern += currentChar();
            advance();
            while (position_ < source_.length() && currentChar() != ']') {
                if (currentChar() == '\\') {
                    pattern += currentChar();
                    advance();
                    if (position_ < source_.length()) {
                        pattern += currentChar();
                        advance();
                    }
                } else {
                    pattern += currentChar();
                    advance();
                }
            }
            if (currentChar() == ']') {
                pattern += currentChar();
                advance();
            }
        } else {
            pattern += currentChar();
            advance();
        }
    }
    
    if (currentChar() != '/') {
        reportError("Unterminated regular expression");
        return Token(TokenType::Invalid, pattern, loc);
    }
    
    advance();  // skip closing '/'
    
    // Lex flags (g, i, m, s, u, y, d)
    std::string flags;
    while (position_ < source_.length() && 
           (currentChar() == 'g' || currentChar() == 'i' || currentChar() == 'm' || 
            currentChar() == 's' || currentChar() == 'u' || currentChar() == 'y' || 
            currentChar() == 'd')) {
        flags += currentChar();
        advance();
    }
    
    std::string value = "/" + pattern + "/" + flags;
    return Token(TokenType::RegexLiteral, value, loc);
}

const std::vector<Token>& Lexer::getAllTokens() {
    if (tokens_.empty()) {
        Token token;
        do {
            token = nextToken();
            tokens_.push_back(token);
        } while (token.type != TokenType::EndOfFile);
    }
    return tokens_;
}

} // namespace nova
