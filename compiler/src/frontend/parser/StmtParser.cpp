#include "nova/Frontend/Parser.h"

namespace nova {

std::unique_ptr<Stmt> Parser::parseStatement() {
    // Decorators (for classes and methods)
    std::vector<std::unique_ptr<Decorator>> decorators;
    if (check(TokenType::At)) {
        decorators = parseDecorators();
    }
    
    // Declarations
    if (match(TokenType::KeywordVar) || match(TokenType::KeywordLet) || match(TokenType::KeywordConst)) {
        return parseVariableDeclaration();
    }
    if (match(TokenType::KeywordFunction)) {
        return parseFunctionDeclaration();
    }
    if (match(TokenType::KeywordClass)) {
        auto classDecl = parseClassDeclaration();
        // Attach decorators to class
        if (auto* cls = dynamic_cast<ClassDecl*>(classDecl.get())) {
            cls->decorators = std::move(decorators);
        }
        return classDecl;
    }
    if (match(TokenType::KeywordInterface)) {
        return parseInterfaceDeclaration();
    }
    if (match(TokenType::KeywordType)) {
        return parseTypeAliasDeclaration();
    }
    if (match(TokenType::KeywordEnum)) {
        return parseEnumDeclaration();
    }
    if (match(TokenType::KeywordImport)) {
        return parseImportDeclaration();
    }
    if (match(TokenType::KeywordExport)) {
        return parseExportDeclaration();
    }
    
    // Control flow
    if (match(TokenType::KeywordIf)) {
        return parseIfStatement();
    }
    if (match(TokenType::KeywordWhile)) {
        return parseWhileStatement();
    }
    if (match(TokenType::KeywordDo)) {
        return parseDoWhileStatement();
    }
    if (match(TokenType::KeywordFor)) {
        return parseForStatement();
    }
    if (match(TokenType::KeywordSwitch)) {
        return parseSwitchStatement();
    }
    if (match(TokenType::KeywordTry)) {
        return parseTryStatement();
    }
    if (match(TokenType::KeywordThrow)) {
        return parseThrowStatement();
    }
    if (match(TokenType::KeywordReturn)) {
        return parseReturnStatement();
    }
    if (match(TokenType::KeywordBreak)) {
        return parseBreakStatement();
    }
    if (match(TokenType::KeywordContinue)) {
        return parseContinueStatement();
    }
    if (match(TokenType::KeywordDebugger)) {
        return parseDebuggerStatement();
    }
    if (match(TokenType::KeywordWith)) {
        return parseWithStatement();
    }
    
    // Block
    if (check(TokenType::LeftBrace)) {
        return parseBlockStatement();
    }
    
    // Check for labeled statement (identifier followed by colon)
    if (check(TokenType::Identifier) && peek(1).type == TokenType::Colon) {
        Token label = advance();
        consume(TokenType::Colon, "Expected ':' after label");
        auto stmt = parseStatement();
        
        auto labeled = std::make_unique<LabeledStmt>(label.value, std::move(stmt));
        labeled->location = label.location;
        return labeled;
    }
    
    // Expression statement
    return parseExpressionStatement();
}

std::unique_ptr<Stmt> Parser::parseVariableDeclaration() {
    // Kind already consumed (var/let/const)
    Token kindToken = peek(-1);
    VarDeclStmt::Kind kind;
    if (kindToken.type == TokenType::KeywordVar) {
        kind = VarDeclStmt::Kind::Var;
    } else if (kindToken.type == TokenType::KeywordLet) {
        kind = VarDeclStmt::Kind::Let;
    } else {
        kind = VarDeclStmt::Kind::Const;
    }
    
    std::vector<VarDeclStmt::Declarator> declarators;
    
    // Parse declarators
    do {
        VarDeclStmt::Declarator declarator;
        
        // Identifier
        Token id = consume(TokenType::Identifier, "Expected variable name");
        declarator.name = id.value;
        
        // Optional type annotation
        if (match(TokenType::Colon)) {
            declarator.type = parseTypeAnnotation();
        }
        
        // Optional initializer
        if (match(TokenType::Equal)) {
            declarator.init = parseAssignmentExpression();
        }
        
        declarators.push_back(std::move(declarator));
        
    } while (match(TokenType::Comma));
    
    consume(TokenType::Semicolon, "Expected ';' after variable declaration");
    
    auto decl = std::make_unique<VarDeclStmt>(kind, std::move(declarators));
    decl->location = getCurrentLocation();
    
    return decl;
}

std::unique_ptr<Stmt> Parser::parseVariableDeclarationWithoutSemicolon() {
    // Kind already consumed (var/let/const)
    Token kindToken = peek(-1);
    VarDeclStmt::Kind kind;
    if (kindToken.type == TokenType::KeywordVar) {
        kind = VarDeclStmt::Kind::Var;
    } else if (kindToken.type == TokenType::KeywordLet) {
        kind = VarDeclStmt::Kind::Let;
    } else {
        kind = VarDeclStmt::Kind::Const;
    }
    
    std::vector<VarDeclStmt::Declarator> declarators;
    
    // Parse declarators
    do {
        VarDeclStmt::Declarator declarator;
        
        // Identifier
        Token id = consume(TokenType::Identifier, "Expected variable name");
        declarator.name = id.value;
        
        // Optional type annotation
        if (match(TokenType::Colon)) {
            declarator.type = parseTypeAnnotation();
        }
        
        // Optional initializer
        if (match(TokenType::Equal)) {
            declarator.init = parseAssignmentExpression();
        }
        
        declarators.push_back(std::move(declarator));
        
    } while (match(TokenType::Comma));
    
    // Note: Don't consume semicolon - this is for for loop initialization
    
    auto decl = std::make_unique<VarDeclStmt>(kind, std::move(declarators));
    decl->location = getCurrentLocation();
    
    return decl;
}

std::unique_ptr<Stmt> Parser::parseFunctionDeclaration() {
    auto func = std::make_unique<FunctionDecl>();
    func->location = getCurrentLocation();
    
    // Async?
    if (peek(-2).type == TokenType::KeywordAsync) {
        func->isAsync = true;
    }
    
    // Generator? (function*)
    if (match(TokenType::Star)) {
        func->isGenerator = true;
    }
    
    // Name
    Token name = consume(TokenType::Identifier, "Expected function name");
    func->name = name.value;
    
    // Type parameters (generics)
    if (match(TokenType::Less)) {
        // Parse type parameters - placeholder for now
        while (!check(TokenType::Greater) && !isAtEnd()) {
            advance();
        }
        consume(TokenType::Greater, "Expected '>' after type parameters");
    }
    
    // Parameters
    std::vector<std::string> params;
    std::vector<TypePtr> paramTypes;
    std::vector<ExprPtr> defaultValues;
    consume(TokenType::LeftParen, "Expected '(' after function name");
    while (!check(TokenType::RightParen) && !isAtEnd()) {
        Token paramName = consume(TokenType::Identifier, "Expected parameter name");
        params.push_back(paramName.value);

        // Optional type annotation
        TypePtr paramType = nullptr;
        if (match(TokenType::Colon)) {
            paramType = parseTypeAnnotation();
        }
        paramTypes.push_back(std::move(paramType));

        // Optional default value
        ExprPtr defaultValue = nullptr;
        if (match(TokenType::Equal)) {
            defaultValue = parseAssignmentExpression();
        }
        defaultValues.push_back(std::move(defaultValue));

        if (!check(TokenType::RightParen)) {
            consume(TokenType::Comma, "Expected ',' between parameters");
        }
    }
    consume(TokenType::RightParen, "Expected ')' after parameters");
    func->params = std::move(params);
    func->paramTypes = std::move(paramTypes);
    func->defaultValues = std::move(defaultValues);
    
    // Return type
    if (match(TokenType::Colon)) {
        func->returnType = parseTypeAnnotation();
    }
    
    // Body
    func->body = parseBlockStatement();
    
    // Wrap in DeclStmt
    auto declStmt = std::make_unique<DeclStmt>(std::move(func));
    declStmt->location = getCurrentLocation();
    
    return declStmt;
}

std::unique_ptr<Stmt> Parser::parseBlockStatement() {
    consume(TokenType::LeftBrace, "Expected '{'");
    
    std::vector<StmtPtr> statements;
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }
    
    consume(TokenType::RightBrace, "Expected '}'");
    
    auto block = std::make_unique<BlockStmt>(std::move(statements));
    block->location = getCurrentLocation();
    
    return block;
}

std::unique_ptr<Stmt> Parser::parseExpressionStatement() {
    auto expr = parseExpression();
    
    // Semicolon insertion
    if (!isAtEnd() && peek().type != TokenType::RightBrace) {
        match(TokenType::Semicolon);
    }
    
    auto stmt = std::make_unique<ExpressionStatement>(std::move(expr));
    stmt->location = getCurrentLocation();
    
    return stmt;
}

std::unique_ptr<Stmt> Parser::parseIfStatement() {
    consume(TokenType::LeftParen, "Expected '(' after 'if'");
    auto test = parseExpression();
    consume(TokenType::RightParen, "Expected ')' after if condition");
    
    auto consequent = parseStatement();
    
    StmtPtr alternate = nullptr;
    if (match(TokenType::KeywordElse)) {
        alternate = parseStatement();
    }
    
    auto ifStmt = std::make_unique<IfStatement>(std::move(test), std::move(consequent), std::move(alternate));
    ifStmt->location = getCurrentLocation();
    
    return ifStmt;
}

std::unique_ptr<Stmt> Parser::parseReturnStatement() {
    auto ret = std::make_unique<ReturnStatement>();
    ret->location = getCurrentLocation();
    
    if (!check(TokenType::Semicolon) && !isAtEnd()) {
        ret->argument = parseExpression();
    }
    
    match(TokenType::Semicolon);
    
    return ret;
}

std::unique_ptr<Stmt> Parser::parseWhileStatement() {
    consume(TokenType::LeftParen, "Expected '(' after 'while'");
    auto test = parseExpression();
    consume(TokenType::RightParen, "Expected ')' after condition");
    
    auto body = parseStatement();
    
    auto whileStmt = std::make_unique<WhileStatement>(std::move(test), std::move(body));
    whileStmt->location = getCurrentLocation();
    
    return whileStmt;
}

std::unique_ptr<Stmt> Parser::parseBreakStatement() {
    auto brk = std::make_unique<BreakStatement>();
    brk->location = getCurrentLocation();
    match(TokenType::Semicolon);
    return brk;
}

std::unique_ptr<Stmt> Parser::parseContinueStatement() {
    auto cont = std::make_unique<ContinueStatement>();
    cont->location = getCurrentLocation();
    match(TokenType::Semicolon);
    return cont;
}

// Placeholder implementations for remaining statements
std::unique_ptr<Stmt> Parser::parseClassDeclaration() {
    auto classDecl = std::make_unique<ClassDecl>();
    classDecl->location = getCurrentLocation();
    
    // Parse class decorators (already consumed @tokens before calling this)
    // Decorators are passed from parseStatement
    
    // Class name
    Token name = consume(TokenType::Identifier, "Expected class name");
    classDecl->name = name.value;
    
    // Type parameters (simplified)
    if (match(TokenType::Less)) {
        while (!check(TokenType::Greater) && !isAtEnd()) {
            Token typeParam = consume(TokenType::Identifier, "Expected type parameter");
            classDecl->typeParams.push_back(typeParam.value);
            if (!check(TokenType::Greater)) {
                match(TokenType::Comma);
            }
        }
        consume(TokenType::Greater, "Expected '>' after type parameters");
    }
    
    // Extends clause
    if (match(TokenType::KeywordExtends)) {
        Token parent = consume(TokenType::Identifier, "Expected parent class name");
        classDecl->superclass = parent.value;
    }
    
    // Implements clause
    if (match(TokenType::KeywordImplements)) {
        do {
            Token iface = consume(TokenType::Identifier, "Expected interface name");
            classDecl->interfaces.push_back(iface.value);
        } while (match(TokenType::Comma));
    }
    
    consume(TokenType::LeftBrace, "Expected '{' before class body");
    
    // Parse class members
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        // Member decorators
        auto memberDecorators = parseDecorators();
        
        // Private field with # prefix
        bool isPrivateField = false;
        if (match(TokenType::Hash)) {
            isPrivateField = true;
        }
        
        // Visibility modifiers
        /*bool isPublic =*/ match(TokenType::KeywordPublic);
        /*bool isPrivate =*/ match(TokenType::KeywordPrivate) || isPrivateField;
        /*bool isProtected =*/ match(TokenType::KeywordProtected);
        
        // Static
        bool isStatic = match(TokenType::KeywordStatic);
        
        // Abstract/Readonly
        bool isAbstract = match(TokenType::KeywordAbstract);
        bool isReadonly = match(TokenType::KeywordReadonly);
        
        // Async
        bool isAsync = match(TokenType::KeywordAsync);
        
        // Getter/Setter
        bool isGetter = match(TokenType::KeywordGet);
        bool isSetter = match(TokenType::KeywordSet);
        
        // Member name
        if (!check(TokenType::Identifier)) {
            reportError("Expected class member name");
            synchronize();
            continue;
        }
        
        Token memberName = advance();
        
        // Add # prefix if it's a private field
        std::string finalName = isPrivateField ? "#" + memberName.value : memberName.value;
        
        // Check if it's a method (has parentheses) or property
        if (check(TokenType::LeftParen)) {
            // Method
            ClassDecl::Method method;
            method.name = finalName;
            method.isStatic = isStatic;
            method.isAsync = isAsync;
            method.isAbstract = isAbstract;
            
            if (isGetter) {
                method.kind = ClassDecl::Method::Kind::Get;
            } else if (isSetter) {
                method.kind = ClassDecl::Method::Kind::Set;
            } else if (memberName.value == "constructor") {
                method.kind = ClassDecl::Method::Kind::Constructor;
            } else {
                method.kind = ClassDecl::Method::Kind::Method;
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
            
            // Method body (unless abstract)
            if (!isAbstract) {
                method.body = parseBlockStatement();
            } else {
                match(TokenType::Semicolon);
            }
            
            // Attach decorators to method
            method.decorators = std::move(memberDecorators);
            
            classDecl->methods.push_back(std::move(method));
            
        } else {
            // Property
            ClassDecl::Property prop;
            prop.name = finalName;
            prop.isStatic = isStatic;
            prop.isReadonly = isReadonly;
            
            // Type annotation
            if (match(TokenType::Colon)) {
                prop.type = parseTypeAnnotation();
            }
            
            // Initializer
            if (match(TokenType::Equal)) {
                prop.initializer = parseExpression();
            }
            
            match(TokenType::Semicolon);
            
            // Attach decorators to property
            prop.decorators = std::move(memberDecorators);
            
            classDecl->properties.push_back(std::move(prop));
        }
    }
    
    consume(TokenType::RightBrace, "Expected '}' after class body");
    
    // Wrap in DeclStmt
    auto declStmt = std::make_unique<DeclStmt>(std::move(classDecl));
    declStmt->location = getCurrentLocation();
    
    return declStmt;
}

std::unique_ptr<Stmt> Parser::parseInterfaceDeclaration() {
    auto iface = std::make_unique<InterfaceDecl>();
    iface->location = getCurrentLocation();
    
    // Interface name
    Token name = consume(TokenType::Identifier, "Expected interface name");
    iface->name = name.value;
    
    // Type parameters
    if (match(TokenType::Less)) {
        while (!check(TokenType::Greater) && !isAtEnd()) {
            Token typeParam = consume(TokenType::Identifier, "Expected type parameter");
            iface->typeParams.push_back(typeParam.value);
            if (!check(TokenType::Greater)) {
                match(TokenType::Comma);
            }
        }
        consume(TokenType::Greater, "Expected '>' after type parameters");
    }
    
    // Extends clause
    if (match(TokenType::KeywordExtends)) {
        do {
            Token parent = consume(TokenType::Identifier, "Expected interface name");
            iface->extends.push_back(parent.value);
        } while (match(TokenType::Comma));
    }
    
    consume(TokenType::LeftBrace, "Expected '{' before interface body");
    
    // Parse interface members
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        Token memberName = consume(TokenType::Identifier, "Expected member name");
        
        if (check(TokenType::LeftParen)) {
            // Method signature
            InterfaceDecl::MethodSignature method;
            method.name = memberName.value;
            
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
            
            if (match(TokenType::Colon)) {
                method.returnType = parseTypeAnnotation();
            }
            
            match(TokenType::Semicolon);
            
            iface->methods.push_back(std::move(method));
        } else {
            // Property signature
            InterfaceDecl::PropertySignature prop;
            prop.name = memberName.value;
            
            if (match(TokenType::Question)) {
                prop.isOptional = true;
            }
            
            if (match(TokenType::Colon)) {
                prop.type = parseTypeAnnotation();
            }
            
            match(TokenType::Semicolon);
            
            iface->properties.push_back(std::move(prop));
        }
    }
    
    consume(TokenType::RightBrace, "Expected '}' after interface body");
    
    // Wrap in DeclStmt
    auto declStmt = std::make_unique<DeclStmt>(std::move(iface));
    declStmt->location = getCurrentLocation();
    
    return declStmt;
}

std::unique_ptr<Stmt> Parser::parseTypeAliasDeclaration() {
    auto typeAlias = std::make_unique<TypeAliasDecl>();
    typeAlias->location = getCurrentLocation();
    
    // Type name
    Token name = consume(TokenType::Identifier, "Expected type name");
    typeAlias->name = name.value;
    
    // Type parameters
    if (match(TokenType::Less)) {
        while (!check(TokenType::Greater) && !isAtEnd()) {
            Token typeParam = consume(TokenType::Identifier, "Expected type parameter");
            typeAlias->typeParams.push_back(typeParam.value);
            if (!check(TokenType::Greater)) {
                match(TokenType::Comma);
            }
        }
        consume(TokenType::Greater, "Expected '>' after type parameters");
    }
    
    consume(TokenType::Equal, "Expected '=' in type alias");
    
    // Type definition
    typeAlias->type = parseTypeAnnotation();
    
    match(TokenType::Semicolon);
    
    // Wrap in DeclStmt
    auto declStmt = std::make_unique<DeclStmt>(std::move(typeAlias));
    declStmt->location = getCurrentLocation();
    
    return declStmt;
}

std::unique_ptr<Stmt> Parser::parseEnumDeclaration() {
    auto enumDecl = std::make_unique<EnumDecl>();
    enumDecl->location = getCurrentLocation();
    
    // Enum name
    Token name = consume(TokenType::Identifier, "Expected enum name");
    enumDecl->name = name.value;
    
    consume(TokenType::LeftBrace, "Expected '{' before enum body");
    
    // Parse enum members
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        EnumDecl::Member member;
        
        Token memberName = consume(TokenType::Identifier, "Expected enum member name");
        member.name = memberName.value;
        
        // Initializer - use parseAssignmentExpression to not consume comma
        if (match(TokenType::Equal)) {
            member.initializer = parseAssignmentExpression();
        }
        
        enumDecl->members.push_back(std::move(member));
        
        if (!check(TokenType::RightBrace)) {
            consume(TokenType::Comma, "Expected ',' between enum members");
        }
    }
    
    consume(TokenType::RightBrace, "Expected '}' after enum body");
    
    // Wrap in DeclStmt
    auto declStmt = std::make_unique<DeclStmt>(std::move(enumDecl));
    declStmt->location = getCurrentLocation();
    
    return declStmt;
}

std::unique_ptr<Stmt> Parser::parseImportDeclaration() {
    auto import = std::make_unique<ImportDecl>();
    import->location = getCurrentLocation();
    
    // import * as name from "module"
    if (match(TokenType::Star)) {
        consume(TokenType::KeywordAs, "Expected 'as' after '*'");
        Token name = consume(TokenType::Identifier, "Expected namespace name");
        import->namespaceImport = name.value;
        consume(TokenType::KeywordFrom, "Expected 'from' after import");
        Token source = consume(TokenType::StringLiteral, "Expected module path");
        import->source = source.value;
        match(TokenType::Semicolon);
        
        auto declStmt = std::make_unique<DeclStmt>(std::move(import));
        declStmt->location = getCurrentLocation();
        return declStmt;
    }
    
    // import { ... } from "module" or import name from "module"
    if (match(TokenType::LeftBrace)) {
        // Named imports
        while (!check(TokenType::RightBrace) && !isAtEnd()) {
            Token imported = consume(TokenType::Identifier, "Expected import name");
            std::string local = imported.value;
            
            if (match(TokenType::KeywordAs)) {
                Token localName = consume(TokenType::Identifier, "Expected local name");
                local = localName.value;
            }
            
            ImportDecl::Specifier spec;
            spec.imported = imported.value;
            spec.local = local;
            import->specifiers.push_back(std::move(spec));
            
            if (!check(TokenType::RightBrace)) {
                consume(TokenType::Comma, "Expected ',' between imports");
            }
        }
        consume(TokenType::RightBrace, "Expected '}' after imports");
    } else if (check(TokenType::Identifier)) {
        // Default import
        Token name = advance();
        import->defaultImport = name.value;
    }
    
    consume(TokenType::KeywordFrom, "Expected 'from' after import");
    Token source = consume(TokenType::StringLiteral, "Expected module path");
    import->source = source.value;
    
    match(TokenType::Semicolon);
    
    // Wrap in DeclStmt
    auto declStmt = std::make_unique<DeclStmt>(std::move(import));
    declStmt->location = getCurrentLocation();
    
    return declStmt;
}

std::unique_ptr<Stmt> Parser::parseExportDeclaration() {
    auto exportDecl = std::make_unique<ExportDecl>();
    exportDecl->location = getCurrentLocation();
    
    // export default
    if (match(TokenType::KeywordDefault)) {
        exportDecl->isDefault = true;
        exportDecl->declaration = parseExpression();
        match(TokenType::Semicolon);
        
        auto declStmt = std::make_unique<DeclStmt>(std::move(exportDecl));
        declStmt->location = getCurrentLocation();
        return declStmt;
    }
    
    // export { ... }
    if (match(TokenType::LeftBrace)) {
        while (!check(TokenType::RightBrace) && !isAtEnd()) {
            Token local = consume(TokenType::Identifier, "Expected export name");
            std::string exported = local.value;
            
            if (match(TokenType::KeywordAs)) {
                Token exportedName = consume(TokenType::Identifier, "Expected exported name");
                exported = exportedName.value;
            }
            
            ExportDecl::Specifier spec;
            spec.local = local.value;
            spec.exported = exported;
            exportDecl->specifiers.push_back(std::move(spec));
            
            if (!check(TokenType::RightBrace)) {
                consume(TokenType::Comma, "Expected ',' between exports");
            }
        }
        consume(TokenType::RightBrace, "Expected '}' after exports");
        
        if (match(TokenType::KeywordFrom)) {
            Token source = consume(TokenType::StringLiteral, "Expected module path");
            exportDecl->source = source.value;
        }
        
        match(TokenType::Semicolon);
        
        auto declStmt = std::make_unique<DeclStmt>(std::move(exportDecl));
        declStmt->location = getCurrentLocation();
        return declStmt;
    }
    
    // export * from "module"
    if (match(TokenType::Star)) {
        if (match(TokenType::KeywordAs)) {
            Token name = consume(TokenType::Identifier, "Expected namespace name");
            exportDecl->namespaceExport = name.value;
        }
        consume(TokenType::KeywordFrom, "Expected 'from' after export *");
        Token source = consume(TokenType::StringLiteral, "Expected module path");
        exportDecl->source = source.value;
        match(TokenType::Semicolon);
        
        auto declStmt = std::make_unique<DeclStmt>(std::move(exportDecl));
        declStmt->location = getCurrentLocation();
        return declStmt;
    }
    
    // export declaration
    auto stmt = parseStatement();
    // Extract the declaration from the statement
    if (auto* declStmt = dynamic_cast<DeclStmt*>(stmt.get())) {
        exportDecl->exportedDecl = std::move(declStmt->declaration);
    }
    
    auto resultDeclStmt = std::make_unique<DeclStmt>(std::move(exportDecl));
    resultDeclStmt->location = getCurrentLocation();
    return resultDeclStmt;
}

std::unique_ptr<Stmt> Parser::parseDoWhileStatement() {
    auto body = parseStatement();
    
    consume(TokenType::KeywordWhile, "Expected 'while' after do-while body");
    consume(TokenType::LeftParen, "Expected '(' after 'while'");
    auto test = parseExpression();
    consume(TokenType::RightParen, "Expected ')' after condition");
    match(TokenType::Semicolon);
    
    auto doWhile = std::make_unique<DoWhileStatement>(std::move(body), std::move(test));
    doWhile->location = getCurrentLocation();
    
    return doWhile;
}

std::unique_ptr<Stmt> Parser::parseForStatement() {
    consume(TokenType::LeftParen, "Expected '(' after 'for'");
    
    // Check for for-in or for-of by looking ahead
    size_t savedPos = current_;
    bool isForInOf = false;
    
    // Try to detect for-in/for-of pattern
    if (check(TokenType::KeywordVar) || check(TokenType::KeywordLet) || check(TokenType::KeywordConst)) {
        advance();
        if (check(TokenType::Identifier)) {
            advance();
            if (check(TokenType::KeywordIn) || check(TokenType::KeywordOf)) {
                isForInOf = true;
            }
        }
    } else if (check(TokenType::Identifier)) {
        advance();
        if (check(TokenType::KeywordIn) || check(TokenType::KeywordOf)) {
            isForInOf = true;
        }
    }
    
    // Restore position
    current_ = savedPos;
    
    if (isForInOf) {
        // Delegate to for-in or for-of parser
        if (check(TokenType::KeywordVar) || check(TokenType::KeywordLet) || check(TokenType::KeywordConst)) {
            bool isConst = check(TokenType::KeywordConst);
            bool isLet = check(TokenType::KeywordLet);
            advance();
            
            Token id = consume(TokenType::Identifier, "Expected variable name");
            
            if (match(TokenType::KeywordIn)) {
                return parseForInStatementBody(id.value, isConst ? "const" : (isLet ? "let" : "var"));
            } else if (match(TokenType::KeywordOf)) {
                return parseForOfStatementBody(id.value, isConst ? "const" : (isLet ? "let" : "var"));
            }
        } else {
            Token id = consume(TokenType::Identifier, "Expected variable name");
            
            if (match(TokenType::KeywordIn)) {
                return parseForInStatementBody(id.value, "");
            } else if (match(TokenType::KeywordOf)) {
                return parseForOfStatementBody(id.value, "");
            }
        }
    }
    
    // Regular for loop: for (init; test; update) body
    
    StmtPtr init = nullptr;
    ExprPtr test = nullptr;
    ExprPtr update = nullptr;
    
    // Init (optional)
    if (!check(TokenType::Semicolon)) {
        if (check(TokenType::KeywordVar) || check(TokenType::KeywordLet) || check(TokenType::KeywordConst)) {
            // Consume the keyword before calling parseVariableDeclarationWithoutSemicolon
            Token kindToken = advance();
            init = parseVariableDeclarationWithoutSemicolon();
        } else {
            auto expr = parseExpression();
            init = std::make_unique<ExpressionStatement>(std::move(expr));
        }
    }
    consume(TokenType::Semicolon, "Expected ';' after for loop initializer");
    
    // Test (optional)
    if (!check(TokenType::Semicolon)) {
        test = parseExpression();
    }
    consume(TokenType::Semicolon, "Expected ';' after for loop condition");
    
    // Update (optional)
    if (!check(TokenType::RightParen)) {
        update = parseExpression();
    }
    
    consume(TokenType::RightParen, "Expected ')' after for clauses");
    
    auto body = parseStatement();
    
    auto forStmt = std::make_unique<ForStatement>(std::move(init), std::move(test), std::move(update), std::move(body));
    forStmt->location = getCurrentLocation();
    
    return forStmt;
}

std::unique_ptr<Stmt> Parser::parseForInStatement() {
    // This is called from parseForStatement
    reportError("Internal error: parseForInStatement called directly");
    return nullptr;
}

std::unique_ptr<Stmt> Parser::parseForOfStatement() {
    // This is called from parseForStatement
    reportError("Internal error: parseForOfStatement called directly");
    return nullptr;
}

std::unique_ptr<Stmt> Parser::parseForInStatementBody(const std::string& variable, const std::string& kind) {
    auto right = parseExpression();
    
    consume(TokenType::RightParen, "Expected ')' after for-in");
    auto body = parseStatement();
    
    auto forIn = std::make_unique<ForInStatement>(variable, kind, std::move(right), std::move(body));
    forIn->location = getCurrentLocation();
    
    return forIn;
}

std::unique_ptr<Stmt> Parser::parseForOfStatementBody(const std::string& variable, const std::string& kind) {
    auto right = parseExpression();
    
    consume(TokenType::RightParen, "Expected ')' after for-of");
    auto body = parseStatement();
    
    auto forOf = std::make_unique<ForOfStatement>(variable, kind, std::move(right), std::move(body), false);
    forOf->location = getCurrentLocation();
    
    return forOf;
}

std::unique_ptr<Stmt> Parser::parseSwitchStatement() {
    consume(TokenType::LeftParen, "Expected '(' after 'switch'");
    auto discriminant = parseExpression();
    consume(TokenType::RightParen, "Expected ')' after switch expression");
    
    consume(TokenType::LeftBrace, "Expected '{' to start switch body");
    
    std::vector<std::unique_ptr<SwitchCase>> cases;
    
    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        if (match(TokenType::KeywordCase)) {
            auto caseClause = std::make_unique<SwitchCase>();
            caseClause->location = getCurrentLocation();
            caseClause->test = parseExpression();
            consume(TokenType::Colon, "Expected ':' after case expression");
            
            // Parse statements until next case/default/closing brace
            while (!check(TokenType::KeywordCase) && 
                   !check(TokenType::KeywordDefault) && 
                   !check(TokenType::RightBrace) && 
                   !isAtEnd()) {
                auto stmt = parseStatement();
                if (stmt) {
                    caseClause->consequent.push_back(std::move(stmt));
                }
            }
            
            cases.push_back(std::move(caseClause));
            
        } else if (match(TokenType::KeywordDefault)) {
            auto defaultClause = std::make_unique<SwitchCase>();
            defaultClause->location = getCurrentLocation();
            // test is null for default case
            consume(TokenType::Colon, "Expected ':' after 'default'");
            
            while (!check(TokenType::KeywordCase) && 
                   !check(TokenType::KeywordDefault) && 
                   !check(TokenType::RightBrace) && 
                   !isAtEnd()) {
                auto stmt = parseStatement();
                if (stmt) {
                    defaultClause->consequent.push_back(std::move(stmt));
                }
            }
            
            cases.push_back(std::move(defaultClause));
            
        } else {
            reportError("Expected 'case' or 'default' in switch statement");
            synchronize();
            break;
        }
    }
    
    consume(TokenType::RightBrace, "Expected '}' after switch body");
    
    auto switchStmt = std::make_unique<SwitchStatement>(std::move(discriminant), std::move(cases));
    switchStmt->location = getCurrentLocation();
    
    return switchStmt;
}

std::unique_ptr<Stmt> Parser::parseTryStatement() {
    // Try block
    auto block = parseBlockStatement();
    
    // Catch clause (optional)
    std::unique_ptr<CatchClause> handler = nullptr;
    if (match(TokenType::KeywordCatch)) {
        auto catchClause = std::make_unique<CatchClause>();
        catchClause->location = getCurrentLocation();
        
        // Parameter (optional in ES2019+)
        if (match(TokenType::LeftParen)) {
            Token param = consume(TokenType::Identifier, "Expected catch parameter");
            catchClause->param = param.value;
            consume(TokenType::RightParen, "Expected ')' after catch parameter");
        }
        
        catchClause->body = parseBlockStatement();
        handler = std::move(catchClause);
    }
    
    // Finally block (optional)
    StmtPtr finalizer = nullptr;
    if (match(TokenType::KeywordFinally)) {
        finalizer = parseBlockStatement();
    }
    
    // Must have either catch or finally
    if (!handler && !finalizer) {
        reportError("Missing catch or finally after try");
    }
    
    auto tryStmt = std::make_unique<TryStatement>(std::move(block), std::move(handler), std::move(finalizer));
    tryStmt->location = getCurrentLocation();
    
    return tryStmt;
}

std::unique_ptr<Stmt> Parser::parseThrowStatement() {
    auto argument = parseExpression();
    match(TokenType::Semicolon);
    
    auto throwStmt = std::make_unique<ThrowStatement>(std::move(argument));
    throwStmt->location = getCurrentLocation();
    
    return throwStmt;
}

std::unique_ptr<Stmt> Parser::parseDebuggerStatement() {
    auto debugger = std::make_unique<DebuggerStmt>();
    debugger->location = getCurrentLocation();
    match(TokenType::Semicolon);
    return debugger;
}

std::unique_ptr<Stmt> Parser::parseWithStatement() {
    auto withStmt = std::make_unique<WithStmt>(nullptr, nullptr);
    withStmt->location = getCurrentLocation();
    
    consume(TokenType::LeftParen, "Expected '(' after 'with'");
    withStmt->object = parseExpression();
    consume(TokenType::RightParen, "Expected ')' after with object");
    
    withStmt->body = parseStatement();
    
    return withStmt;
}

std::unique_ptr<Decorator> Parser::parseDecorator() {
    // Expect @ to be already consumed
    Token name = consume(TokenType::Identifier, "Expected decorator name");
    
    std::vector<ExprPtr> args;
    
    // If there's a call expression with arguments
    if (match(TokenType::LeftParen)) {
        while (!check(TokenType::RightParen) && !isAtEnd()) {
            args.push_back(parseAssignmentExpression());
            if (!check(TokenType::RightParen)) {
                consume(TokenType::Comma, "Expected ',' between decorator arguments");
            }
        }
        consume(TokenType::RightParen, "Expected ')' after decorator arguments");
    }
    
    auto decorator = std::make_unique<Decorator>(name.value, std::move(args));
    decorator->location = name.location;
    return decorator;
}

std::vector<std::unique_ptr<Decorator>> Parser::parseDecorators() {
    std::vector<std::unique_ptr<Decorator>> decorators;
    
    while (match(TokenType::At)) {
        decorators.push_back(parseDecorator());
    }
    
    return decorators;
}

} // namespace nova
