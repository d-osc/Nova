#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace nova {

enum class TokenType {
    // Literals
    Identifier,
    NumberLiteral,
    StringLiteral,
    TemplateLiteral,
    RegexLiteral,
    TrueLiteral,
    FalseLiteral,
    NullLiteral,
    UndefinedLiteral,
    
    // Keywords
    KeywordBreak,
    KeywordCase,
    KeywordCatch,
    KeywordClass,
    KeywordConst,
    KeywordContinue,
    KeywordDebugger,
    KeywordDefault,
    KeywordDelete,
    KeywordDo,
    KeywordElse,
    KeywordEnum,
    KeywordExport,
    KeywordExtends,
    KeywordFinally,
    KeywordFor,
    KeywordFunction,
    KeywordIf,
    KeywordImport,
    KeywordIn,
    KeywordInstanceof,
    KeywordLet,
    KeywordNew,
    KeywordReturn,
    KeywordSuper,
    KeywordSwitch,
    KeywordThis,
    KeywordThrow,
    KeywordTry,
    KeywordTypeof,
    KeywordVar,
    KeywordVoid,
    KeywordWhile,
    KeywordWith,
    KeywordYield,
    KeywordAwait,
    KeywordAsync,
    KeywordFrom,
    KeywordAs,
    KeywordOf,
    
    // TypeScript Keywords
    KeywordType,
    KeywordInterface,
    KeywordNamespace,
    KeywordDeclare,
    KeywordAbstract,
    KeywordPublic,
    KeywordPrivate,
    KeywordProtected,
    KeywordReadonly,
    KeywordStatic,
    KeywordGet,
    KeywordSet,
    KeywordOverride,
    KeywordSatisfies,
    KeywordKeyof,
    KeywordInfer,
    KeywordIs,
    KeywordAsserts,
    KeywordUnique,
    KeywordImplements,
    
    // Operators
    Plus,              // +
    Minus,             // -
    Star,              // *
    Slash,             // /
    Percent,           // %
    StarStar,          // **
    PlusPlus,          // ++
    MinusMinus,        // --
    
    Ampersand,         // &
    Pipe,              // |
    Caret,             // ^
    Tilde,             // ~
    LessLess,          // <<
    GreaterGreater,    // >>
    GreaterGreaterGreater, // >>>
    
    Equal,             // =
    PlusEqual,         // +=
    MinusEqual,        // -=
    StarEqual,         // *=
    SlashEqual,        // /=
    PercentEqual,      // %=
    StarStarEqual,     // **=
    LessLessEqual,     // <<=
    GreaterGreaterEqual, // >>=
    GreaterGreaterGreaterEqual, // >>>=
    AmpersandEqual,    // &=
    PipeEqual,         // |=
    CaretEqual,        // ^=
    AmpersandAmpersandEqual, // &&=
    PipePipeEqual,     // ||=
    QuestionQuestionEqual, // ??=
    
    EqualEqual,        // ==
    ExclamationEqual,  // !=
    EqualEqualEqual,   // ===
    ExclamationEqualEqual, // !==
    Less,              // <
    Greater,           // >
    LessEqual,         // <=
    GreaterEqual,      // >=
    
    AmpersandAmpersand, // &&
    PipePipe,          // ||
    Exclamation,       // !
    Question,          // ?
    QuestionQuestion,  // ??
    QuestionDot,       // ?.
    
    Dot,               // .
    DotDotDot,         // ...
    Arrow,             // =>
    Colon,             // :
    Semicolon,         // ;
    Comma,             // ,
    Hash,              // # (for private fields)
    At,                // @ (for decorators)
    
    // Brackets
    LeftParen,         // (
    RightParen,        // )
    LeftBrace,         // {
    RightBrace,        // }
    LeftBracket,       // [
    RightBracket,      // ]
    
    // JSX/TSX
    LessThan,          // < (JSX opening)
    SlashGreaterThan,  // />
    LessThanSlash,     // </
    
    // Special
    EndOfFile,
    Invalid
};

struct SourceLocation {
    std::string filename;
    uint32_t line;
    uint32_t column;
    uint32_t offset;
    
    SourceLocation() : line(0), column(0), offset(0) {}
    SourceLocation(const std::string& file, uint32_t l, uint32_t c, uint32_t o)
        : filename(file), line(l), column(c), offset(o) {}
};

class Token {
public:
    TokenType type;
    std::string value;
    SourceLocation location;
    
    Token() : type(TokenType::Invalid) {}
    Token(TokenType t, const std::string& v, const SourceLocation& loc)
        : type(t), value(v), location(loc) {}
    
    bool is(TokenType t) const { return type == t; }
    bool isNot(TokenType t) const { return type != t; }
    bool isOneOf(TokenType t1, TokenType t2) const {
        return is(t1) || is(t2);
    }
    
    template<typename... Args>
    bool isOneOf(TokenType t1, TokenType t2, Args... args) const {
        return is(t1) || isOneOf(t2, args...);
    }
    
    bool isKeyword() const;
    bool isOperator() const;
    bool isLiteral() const;
    bool isIdentifier() const { return type == TokenType::Identifier; }
    
    std::string toString() const;
};

} // namespace nova
