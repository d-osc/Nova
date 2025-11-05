#pragma once

#include "AST.h"
#include "Lexer.h"
#include "Token.h"
#include <memory>
#include <vector>
#include <string>

namespace nova {

class Parser {
public:
    explicit Parser(Lexer& lexer);
    
    // Parse the entire program
    std::unique_ptr<Program> parseProgram();
    
    // Error handling
    bool hasErrors() const { return !errors_.empty(); }
    const std::vector<std::string>& getErrors() const { return errors_; }
    
private:
    // Token management
    Token peek(int offset = 0) const;
    Token advance();
    bool match(TokenType type);
    bool check(TokenType type);
    Token consume(TokenType type, const std::string& message);
    
    // Statement parsing
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<Stmt> parseVariableDeclaration();
    std::unique_ptr<Stmt> parseFunctionDeclaration();
    std::unique_ptr<Stmt> parseClassDeclaration();
    std::unique_ptr<Stmt> parseInterfaceDeclaration();
    std::unique_ptr<Stmt> parseTypeAliasDeclaration();
    std::unique_ptr<Stmt> parseEnumDeclaration();
    std::unique_ptr<Stmt> parseImportDeclaration();
    std::unique_ptr<Stmt> parseExportDeclaration();
    std::unique_ptr<Stmt> parseBlockStatement();
    std::unique_ptr<Stmt> parseExpressionStatement();
    std::unique_ptr<Stmt> parseIfStatement();
    std::unique_ptr<Stmt> parseWhileStatement();
    std::unique_ptr<Stmt> parseDoWhileStatement();
    std::unique_ptr<Stmt> parseForStatement();
    std::unique_ptr<Stmt> parseForInStatement();
    std::unique_ptr<Stmt> parseForOfStatement();
    std::unique_ptr<Stmt> parseForInStatementBody(const std::string& variable, const std::string& kind);
    std::unique_ptr<Stmt> parseForOfStatementBody(const std::string& variable, const std::string& kind);
    std::unique_ptr<Stmt> parseSwitchStatement();
    std::unique_ptr<Stmt> parseTryStatement();
    std::unique_ptr<Stmt> parseThrowStatement();
    std::unique_ptr<Stmt> parseReturnStatement();
    std::unique_ptr<Stmt> parseBreakStatement();
    std::unique_ptr<Stmt> parseContinueStatement();
    std::unique_ptr<Stmt> parseDebuggerStatement();
    std::unique_ptr<Stmt> parseWithStatement();
    
    // Expression parsing (precedence climbing)
    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseAssignmentExpression();
    std::unique_ptr<Expr> parseConditionalExpression();
    std::unique_ptr<Expr> parseLogicalOrExpression();
    std::unique_ptr<Expr> parseLogicalAndExpression();
    std::unique_ptr<Expr> parseBitwiseOrExpression();
    std::unique_ptr<Expr> parseBitwiseXorExpression();
    std::unique_ptr<Expr> parseBitwiseAndExpression();
    std::unique_ptr<Expr> parseEqualityExpression();
    std::unique_ptr<Expr> parseRelationalExpression();
    std::unique_ptr<Expr> parseShiftExpression();
    std::unique_ptr<Expr> parseAdditiveExpression();
    std::unique_ptr<Expr> parseMultiplicativeExpression();
    std::unique_ptr<Expr> parseExponentiationExpression();
    std::unique_ptr<Expr> parseUnaryExpression();
    std::unique_ptr<Expr> parsePostfixExpression();
    std::unique_ptr<Expr> parsePrimaryExpression();
    
    // Primary expression helpers
    std::unique_ptr<Expr> parseIdentifier();
    std::unique_ptr<Expr> parseLiteral();
    std::unique_ptr<Expr> parseArrayLiteral();
    std::unique_ptr<Expr> parseObjectLiteral();
    std::unique_ptr<Expr> parseFunctionExpression();
    std::unique_ptr<Expr> parseArrowFunction();
    std::unique_ptr<Expr> parseClassExpression();
    std::unique_ptr<Expr> parseTemplateLiteral();
    std::unique_ptr<Expr> parseParenthesizedExpression();
    
    // Member/call expression helpers
    std::unique_ptr<Expr> parseMemberExpression(std::unique_ptr<Expr> object);
    std::unique_ptr<Expr> parseCallExpression(std::unique_ptr<Expr> callee);
    std::unique_ptr<Expr> parseComputedMemberExpression(std::unique_ptr<Expr> object);
    
    // JSX/TSX parsing
    std::unique_ptr<Expr> parseJSXElement();
    std::unique_ptr<Expr> parseJSXChild();
    
    // Destructuring pattern parsing
    std::unique_ptr<Pattern> parseBindingPattern();
    std::unique_ptr<Pattern> parseObjectPattern();
    std::unique_ptr<Pattern> parseArrayPattern();
    
    // Type annotation parsing
    std::unique_ptr<TypeAnnotation> parseTypeAnnotation();
    std::unique_ptr<TypeAnnotation> parsePrimaryType();
    std::unique_ptr<TypeAnnotation> parseUnionType();
    std::unique_ptr<TypeAnnotation> parseIntersectionType();
    std::unique_ptr<TypeAnnotation> parseArrayType();
    std::unique_ptr<TypeAnnotation> parseTupleType();
    std::unique_ptr<TypeAnnotation> parseFunctionType();
    std::unique_ptr<TypeAnnotation> parseObjectType();
    
    // Decorator parsing
    std::unique_ptr<Decorator> parseDecorator();
    std::vector<std::unique_ptr<Decorator>> parseDecorators();
    
    // Helper functions
    bool isAtEnd() const;
    void synchronize();
    void reportError(const std::string& message);
    SourceLocation getCurrentLocation() const;
    
    Lexer& lexer_;
    std::vector<Token> tokens_;
    size_t current_;
    std::vector<std::string> errors_;
};

} // namespace nova
