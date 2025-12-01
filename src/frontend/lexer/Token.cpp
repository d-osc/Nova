#include "nova/Frontend/Token.h"
#include <sstream>

namespace nova {

bool Token::isKeyword() const {
    return (type >= TokenType::KeywordBreak && type <= TokenType::KeywordUnique);
}

bool Token::isOperator() const {
    return (type >= TokenType::Plus && type <= TokenType::DotDotDot);
}

bool Token::isLiteral() const {
    return (type >= TokenType::Identifier && type <= TokenType::UndefinedLiteral);
}

std::string Token::toString() const {
    std::stringstream ss;
    ss << "Token(";
    
    switch (type) {
        case TokenType::Identifier: ss << "Identifier"; break;
        case TokenType::NumberLiteral: ss << "Number"; break;
        case TokenType::StringLiteral: ss << "String"; break;
        case TokenType::TemplateLiteral: ss << "Template"; break;
        case TokenType::RegexLiteral: ss << "Regex"; break;
        case TokenType::TrueLiteral: ss << "true"; break;
        case TokenType::FalseLiteral: ss << "false"; break;
        case TokenType::NullLiteral: ss << "null"; break;
        case TokenType::UndefinedLiteral: ss << "undefined"; break;
        
        // Keywords
        case TokenType::KeywordBreak: ss << "break"; break;
        case TokenType::KeywordCase: ss << "case"; break;
        case TokenType::KeywordCatch: ss << "catch"; break;
        case TokenType::KeywordClass: ss << "class"; break;
        case TokenType::KeywordConst: ss << "const"; break;
        case TokenType::KeywordContinue: ss << "continue"; break;
        case TokenType::KeywordDebugger: ss << "debugger"; break;
        case TokenType::KeywordDefault: ss << "default"; break;
        case TokenType::KeywordDelete: ss << "delete"; break;
        case TokenType::KeywordDo: ss << "do"; break;
        case TokenType::KeywordElse: ss << "else"; break;
        case TokenType::KeywordEnum: ss << "enum"; break;
        case TokenType::KeywordExport: ss << "export"; break;
        case TokenType::KeywordExtends: ss << "extends"; break;
        case TokenType::KeywordFinally: ss << "finally"; break;
        case TokenType::KeywordFor: ss << "for"; break;
        case TokenType::KeywordFunction: ss << "function"; break;
        case TokenType::KeywordIf: ss << "if"; break;
        case TokenType::KeywordImport: ss << "import"; break;
        case TokenType::KeywordIn: ss << "in"; break;
        case TokenType::KeywordInstanceof: ss << "instanceof"; break;
        case TokenType::KeywordLet: ss << "let"; break;
        case TokenType::KeywordNew: ss << "new"; break;
        case TokenType::KeywordReturn: ss << "return"; break;
        case TokenType::KeywordSuper: ss << "super"; break;
        case TokenType::KeywordSwitch: ss << "switch"; break;
        case TokenType::KeywordThis: ss << "this"; break;
        case TokenType::KeywordThrow: ss << "throw"; break;
        case TokenType::KeywordTry: ss << "try"; break;
        case TokenType::KeywordTypeof: ss << "typeof"; break;
        case TokenType::KeywordVar: ss << "var"; break;
        case TokenType::KeywordVoid: ss << "void"; break;
        case TokenType::KeywordWhile: ss << "while"; break;
        case TokenType::KeywordWith: ss << "with"; break;
        case TokenType::KeywordYield: ss << "yield"; break;
        case TokenType::KeywordAwait: ss << "await"; break;
        case TokenType::KeywordAsync: ss << "async"; break;
        case TokenType::KeywordFrom: ss << "from"; break;
        case TokenType::KeywordAs: ss << "as"; break;
        case TokenType::KeywordOf: ss << "of"; break;
        
        // TypeScript Keywords
        case TokenType::KeywordType: ss << "type"; break;
        case TokenType::KeywordInterface: ss << "interface"; break;
        case TokenType::KeywordNamespace: ss << "namespace"; break;
        case TokenType::KeywordDeclare: ss << "declare"; break;
        case TokenType::KeywordAbstract: ss << "abstract"; break;
        case TokenType::KeywordPublic: ss << "public"; break;
        case TokenType::KeywordPrivate: ss << "private"; break;
        case TokenType::KeywordProtected: ss << "protected"; break;
        case TokenType::KeywordReadonly: ss << "readonly"; break;
        case TokenType::KeywordStatic: ss << "static"; break;
        case TokenType::KeywordGet: ss << "get"; break;
        case TokenType::KeywordSet: ss << "set"; break;
        case TokenType::KeywordImplements: ss << "implements"; break;
        case TokenType::KeywordOverride: ss << "override"; break;
        case TokenType::KeywordSatisfies: ss << "satisfies"; break;
        case TokenType::KeywordKeyof: ss << "keyof"; break;
        case TokenType::KeywordInfer: ss << "infer"; break;
        case TokenType::KeywordIs: ss << "is"; break;
        case TokenType::KeywordAsserts: ss << "asserts"; break;
        case TokenType::KeywordUnique: ss << "unique"; break;
        
        // Operators
        case TokenType::Plus: ss << "+"; break;
        case TokenType::Minus: ss << "-"; break;
        case TokenType::Star: ss << "*"; break;
        case TokenType::Slash: ss << "/"; break;
        case TokenType::Percent: ss << "%"; break;
        case TokenType::StarStar: ss << "**"; break;
        case TokenType::PlusPlus: ss << "++"; break;
        case TokenType::MinusMinus: ss << "--"; break;
        case TokenType::Equal: ss << "="; break;
        case TokenType::EqualEqual: ss << "=="; break;
        case TokenType::EqualEqualEqual: ss << "==="; break;
        case TokenType::ExclamationEqual: ss << "!="; break;
        case TokenType::ExclamationEqualEqual: ss << "!=="; break;
        case TokenType::Less: ss << "<"; break;
        case TokenType::Greater: ss << ">"; break;
        case TokenType::LessEqual: ss << "<="; break;
        case TokenType::GreaterEqual: ss << ">="; break;
        case TokenType::AmpersandAmpersand: ss << "&&"; break;
        case TokenType::PipePipe: ss << "||"; break;
        case TokenType::Exclamation: ss << "!"; break;
        case TokenType::Question: ss << "?"; break;
        case TokenType::QuestionQuestion: ss << "??"; break;
        case TokenType::QuestionDot: ss << "?."; break;
        case TokenType::Dot: ss << "."; break;
        case TokenType::DotDotDot: ss << "..."; break;
        case TokenType::Arrow: ss << "=>"; break;
        case TokenType::Colon: ss << ":"; break;
        case TokenType::Semicolon: ss << ";"; break;
        case TokenType::Comma: ss << ","; break;
        case TokenType::LeftParen: ss << "("; break;
        case TokenType::RightParen: ss << ")"; break;
        case TokenType::LeftBrace: ss << "{"; break;
        case TokenType::RightBrace: ss << "}"; break;
        case TokenType::LeftBracket: ss << "["; break;
        case TokenType::RightBracket: ss << "]"; break;
        
        case TokenType::EndOfFile: ss << "EOF"; break;
        case TokenType::Invalid: ss << "Invalid"; break;
        default: ss << "Unknown"; break;
    }
    
    if (!value.empty() && type != TokenType::EndOfFile) {
        ss << ", \"" << value << "\"";
    }
    
    ss << ", " << location.filename << ":" << location.line << ":" << location.column;
    ss << ")";
    
    return ss.str();
}

} // namespace nova
