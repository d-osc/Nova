#pragma once

#include "nova/Frontend/AST.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace nova {

class TypeChecker {
public:
    bool check(Program& program);
    const std::vector<std::string>& diagnostics() const { return diagnostics_; }

private:
    struct FunctionSignature {
        std::vector<std::string> typeParameters;
        std::vector<TypePtr> typeParameterConstraints;
        std::vector<TypePtr> parameters;
        TypePtr returnType;
    };

    std::vector<std::unordered_map<std::string, TypePtr>> scopes_;
    std::unordered_map<std::string, FunctionSignature> functions_;
    std::unordered_map<std::string, TypePtr> namedTypes_;
    std::vector<std::string> diagnostics_;
    TypePtr expectedReturnType_;

    void pushScope();
    void popScope();
    void bind(const std::string& name, TypePtr type);
    TypePtr lookup(const std::string& name) const;
    void checkStatement(Stmt* statement);
    void checkDeclaration(Decl* declaration);
    void checkFunction(FunctionDecl& function);
    TypePtr inferExpression(Expr* expression);
    TypePtr resolveType(const TypePtr& type) const;
    TypePtr propertyType(const TypePtr& object, const std::string& property) const;
    TypePtr substituteType(
        const TypePtr& type,
        const std::unordered_map<std::string, TypePtr>& substitutions) const;
    void inferTypeArguments(
        const TypePtr& pattern, const TypePtr& actual,
        const std::vector<std::string>& typeParameters,
        std::unordered_map<std::string, TypePtr>& substitutions) const;
    bool isAssignable(const TypePtr& source, const TypePtr& target) const;
    void report(const ASTNode& node, const std::string& code,
                const std::string& message);
    static TypePtr makeType(Type::Kind kind, const std::string& name = "");
    static std::string typeName(const TypePtr& type);
};

} // namespace nova
