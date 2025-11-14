#pragma once

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include "nova/Frontend/Token.h"

namespace nova {

// Forward declarations
class Type;
class Expr;
class Stmt;
class Decl;
class Pattern;

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;
using DeclPtr = std::shared_ptr<Decl>;
using TypePtr = std::shared_ptr<Type>;

// ==================== Base Classes ====================

class ASTNode {
public:
    SourceLocation location;
    virtual ~ASTNode() = default;
    virtual void accept(class ASTVisitor& visitor) = 0;
};

// ==================== Type System ====================

class Type : public ASTNode {
public:
    enum class Kind {
        Void, Any, Unknown, Never,
        Number, String, Boolean, Null, Undefined,
        Object, Array, Function, Union, Intersection,
        Tuple, Literal, TypeParameter, IndexedAccess
    };
    
    Kind kind;
    std::string name;  // For named types
    
    explicit Type(Kind k, const std::string& n = "") : kind(k), name(n) {}
    void accept(ASTVisitor& visitor) override { (void)visitor; }
};

// Type annotation is the same as Type for now
using TypeAnnotation = Type;// ==================== Expressions ====================

class Expr : public ASTNode {
public:
    TypePtr type;
};

class NumberLiteral : public Expr {
public:
    double value;
    explicit NumberLiteral(double v) : value(v) {}
    void accept(ASTVisitor& visitor) override;
};

class StringLiteral : public Expr {
public:
    std::string value;
    explicit StringLiteral(const std::string& v) : value(v) {}
    void accept(ASTVisitor& visitor) override;
};

class BooleanLiteral : public Expr {
public:
    bool value;
    explicit BooleanLiteral(bool v) : value(v) {}
    void accept(ASTVisitor& visitor) override;
};

class NullLiteral : public Expr {
public:
    NullLiteral() = default;
    void accept(ASTVisitor& visitor) override;
};

class UndefinedLiteral : public Expr {
public:
    UndefinedLiteral() = default;
    void accept(ASTVisitor& visitor) override;
};

class Identifier : public Expr {
public:
    std::string name;
    explicit Identifier(const std::string& n) : name(n) {}
    void accept(ASTVisitor& visitor) override;
};

class BinaryExpr : public Expr {
public:
    enum class Op {
        Add, Sub, Mul, Div, Mod, Pow,
        BitAnd, BitOr, BitXor, LeftShift, RightShift, UnsignedRightShift,
        Equal, NotEqual, StrictEqual, StrictNotEqual,
        Less, Greater, LessEqual, GreaterEqual,
        LogicalAnd, LogicalOr, NullishCoalescing,
        In, Instanceof
    };
    
    Op op;
    ExprPtr left;
    ExprPtr right;
    
    BinaryExpr(Op o, ExprPtr l, ExprPtr r) : op(o), left(l), right(r) {}
    void accept(ASTVisitor& visitor) override;
};

class UnaryExpr : public Expr {
public:
    enum class Op {
        Plus, Minus, Not, BitNot, Typeof, Void, Delete, Await
    };
    
    Op op;
    ExprPtr operand;
    bool isPrefix;
    
    UnaryExpr(Op o, ExprPtr operand, bool prefix = true)
        : op(o), operand(operand), isPrefix(prefix) {}
    void accept(ASTVisitor& visitor) override;
};

class UpdateExpr : public Expr {
public:
    enum class Op { Increment, Decrement };
    
    Op op;
    ExprPtr argument;
    bool isPrefix;
    
    UpdateExpr(Op o, ExprPtr arg, bool prefix)
        : op(o), argument(arg), isPrefix(prefix) {}
    void accept(ASTVisitor& visitor) override;
};

class CallExpr : public Expr {
public:
    ExprPtr callee;
    std::vector<ExprPtr> arguments;
    
    CallExpr(ExprPtr c, std::vector<ExprPtr> args)
        : callee(c), arguments(std::move(args)) {}
    void accept(ASTVisitor& visitor) override;
};

class MemberExpr : public Expr {
public:
    ExprPtr object;
    ExprPtr property;
    bool isComputed;
    bool isOptional;
    
    MemberExpr(ExprPtr obj, ExprPtr prop, bool computed, bool optional = false)
        : object(obj), property(prop), isComputed(computed), isOptional(optional) {}
    void accept(ASTVisitor& visitor) override;
};

class ConditionalExpr : public Expr {
public:
    ExprPtr test;
    ExprPtr consequent;
    ExprPtr alternate;
    
    ConditionalExpr(ExprPtr t, ExprPtr c, ExprPtr a)
        : test(t), consequent(c), alternate(a) {}
    void accept(ASTVisitor& visitor) override;
};

class ArrayExpr : public Expr {
public:
    std::vector<ExprPtr> elements;
    
    explicit ArrayExpr(std::vector<ExprPtr> elems)
        : elements(std::move(elems)) {}
    void accept(ASTVisitor& visitor) override;
};

class ObjectExpr : public Expr {
public:
    struct Property {
        ExprPtr key;
        ExprPtr value;
        bool isComputed;
        bool isShorthand;
        enum class Kind { Init, Get, Set, Method } kind;
    };
    
    std::vector<Property> properties;
    
    explicit ObjectExpr(std::vector<Property> props)
        : properties(std::move(props)) {}
    void accept(ASTVisitor& visitor) override;
};

class FunctionExpr : public Expr {
public:
    std::string name;  // optional for named function expressions
    std::vector<std::string> params;
    StmtPtr body;
    bool isAsync = false;
    bool isGenerator = false;
    TypePtr returnType;
    
    FunctionExpr() = default;
    void accept(ASTVisitor& visitor) override;
};

class ArrowFunctionExpr : public Expr {
public:
    std::vector<std::string> params;
    std::vector<TypePtr> paramTypes;  // Type annotations for parameters
    StmtPtr body;  // Always a block statement or expression statement
    bool isAsync = false;
    TypePtr returnType;

    ArrowFunctionExpr() = default;
    void accept(ASTVisitor& visitor) override;
};

class ClassExpr : public Expr {
public:
    struct Method {
        std::string name;
        std::vector<std::string> params;
        StmtPtr body;
        enum class Kind { Method, Constructor, Get, Set } kind;
        bool isStatic = false;
        bool isAsync = false;
        TypePtr returnType;
    };
    
    std::string name;  // optional
    std::string superclass;
    std::vector<Method> methods;
    
    ClassExpr() = default;
    void accept(ASTVisitor& visitor) override;
};

class NewExpr : public Expr {
public:
    ExprPtr callee;
    std::vector<ExprPtr> arguments;
    
    NewExpr(ExprPtr c, std::vector<ExprPtr> args)
        : callee(c), arguments(std::move(args)) {}
    void accept(ASTVisitor& visitor) override;
};

class ThisExpr : public Expr {
public:
    ThisExpr() = default;
    void accept(ASTVisitor& visitor) override;
};

class SuperExpr : public Expr {
public:
    SuperExpr() = default;
    void accept(ASTVisitor& visitor) override;
};

class SpreadExpr : public Expr {
public:
    ExprPtr argument;
    
    explicit SpreadExpr(ExprPtr arg) : argument(arg) {}
    void accept(ASTVisitor& visitor) override;
};

class TemplateLiteralExpr : public Expr {
public:
    std::vector<std::string> quasis;
    std::vector<ExprPtr> expressions;
    
    TemplateLiteralExpr(std::vector<std::string> q, std::vector<ExprPtr> e)
        : quasis(std::move(q)), expressions(std::move(e)) {}
    void accept(ASTVisitor& visitor) override;
};

class AwaitExpr : public Expr {
public:
    ExprPtr argument;
    
    explicit AwaitExpr(ExprPtr arg) : argument(arg) {}
    void accept(ASTVisitor& visitor) override;
};

class YieldExpr : public Expr {
public:
    ExprPtr argument;
    bool isDelegate;
    
    YieldExpr(ExprPtr arg, bool delegate)
        : argument(arg), isDelegate(delegate) {}
    void accept(ASTVisitor& visitor) override;
};

// TypeScript-specific expressions
class AsExpr : public Expr {
public:
    ExprPtr expression;
    TypePtr targetType;
    
    AsExpr(ExprPtr expr, TypePtr type)
        : expression(expr), targetType(type) {}
    void accept(ASTVisitor& visitor) override;
};

class SatisfiesExpr : public Expr {
public:
    ExprPtr expression;
    TypePtr targetType;
    
    SatisfiesExpr(ExprPtr expr, TypePtr type)
        : expression(expr), targetType(type) {}
    void accept(ASTVisitor& visitor) override;
};

class NonNullExpr : public Expr {
public:
    ExprPtr expression;
    
    explicit NonNullExpr(ExprPtr expr) : expression(expr) {}
    void accept(ASTVisitor& visitor) override;
};

class TaggedTemplateExpr : public Expr {
public:
    ExprPtr tag;
    std::vector<std::string> quasis;
    std::vector<ExprPtr> expressions;
    
    TaggedTemplateExpr(ExprPtr t, std::vector<std::string> q, std::vector<ExprPtr> e)
        : tag(t), quasis(std::move(q)), expressions(std::move(e)) {}
    void accept(ASTVisitor& visitor) override;
};

class SequenceExpr : public Expr {
public:
    std::vector<ExprPtr> expressions;
    
    explicit SequenceExpr(std::vector<ExprPtr> exprs)
        : expressions(std::move(exprs)) {}
    void accept(ASTVisitor& visitor) override;
};

class AssignmentExpr : public Expr {
public:
    enum class Op {
        Assign, AddAssign, SubAssign, MulAssign, DivAssign, ModAssign, PowAssign,
        LeftShiftAssign, RightShiftAssign, UnsignedRightShiftAssign,
        BitAndAssign, BitOrAssign, BitXorAssign,
        LogicalAndAssign, LogicalOrAssign, NullishCoalescingAssign
    };
    
    Op op;
    ExprPtr left;
    ExprPtr right;
    
    AssignmentExpr(Op o, ExprPtr l, ExprPtr r)
        : op(o), left(l), right(r) {}
    void accept(ASTVisitor& visitor) override;
};

class ParenthesizedExpr : public Expr {
public:
    ExprPtr expression;
    
    explicit ParenthesizedExpr(ExprPtr expr) : expression(expr) {}
    void accept(ASTVisitor& visitor) override;
};

class MetaProperty : public Expr {
public:
    std::string meta;  // "new" or "import"
    std::string property;  // "target" or "meta"
    
    MetaProperty(const std::string& m, const std::string& p)
        : meta(m), property(p) {}
    void accept(ASTVisitor& visitor) override;
};

class ImportExpr : public Expr {
public:
    ExprPtr source;
    
    explicit ImportExpr(ExprPtr src) : source(src) {}
    void accept(ASTVisitor& visitor) override;
};

// ==================== JSX/TSX ====================

class JSXAttribute : public ASTNode {
public:
    std::string name;
    ExprPtr value;  // can be null for boolean attributes
    
    JSXAttribute(const std::string& n, ExprPtr v = nullptr)
        : name(n), value(std::move(v)) {}
    void accept(ASTVisitor& visitor) override;
};

class JSXSpreadAttribute : public ASTNode {
public:
    ExprPtr expression;
    
    explicit JSXSpreadAttribute(ExprPtr expr) : expression(std::move(expr)) {}
    void accept(ASTVisitor& visitor) override;
};

class JSXElement : public Expr {
public:
    std::string tagName;
    std::vector<std::unique_ptr<JSXAttribute>> attributes;
    std::vector<std::unique_ptr<JSXSpreadAttribute>> spreadAttributes;
    std::vector<ExprPtr> children;
    bool selfClosing;
    
    JSXElement(const std::string& tag, bool selfClose = false)
        : tagName(tag), selfClosing(selfClose) {}
    void accept(ASTVisitor& visitor) override;
};

class JSXFragment : public Expr {
public:
    std::vector<ExprPtr> children;
    
    explicit JSXFragment(std::vector<ExprPtr> kids = {})
        : children(std::move(kids)) {}
    void accept(ASTVisitor& visitor) override;
};

class JSXText : public Expr {
public:
    std::string value;
    
    explicit JSXText(const std::string& val) : value(val) {}
    void accept(ASTVisitor& visitor) override;
};

class JSXExpressionContainer : public Expr {
public:
    ExprPtr expression;
    
    explicit JSXExpressionContainer(ExprPtr expr) : expression(std::move(expr)) {}
    void accept(ASTVisitor& visitor) override;
};

// ==================== Destructuring Patterns ====================

class Pattern : public ASTNode {
};

class ObjectPattern : public Pattern {
public:
    struct Property {
        std::string key;
        std::unique_ptr<Pattern> value;
        ExprPtr defaultValue;
        bool shorthand;
    };
    
    std::vector<Property> properties;
    std::unique_ptr<Pattern> rest;  // rest pattern
    
    ObjectPattern() = default;
    void accept(ASTVisitor& visitor) override;
};

class ArrayPattern : public Pattern {
public:
    std::vector<std::unique_ptr<Pattern>> elements;
    std::unique_ptr<Pattern> rest;
    
    ArrayPattern() = default;
    void accept(ASTVisitor& visitor) override;
};

class AssignmentPattern : public Pattern {
public:
    std::unique_ptr<Pattern> left;
    ExprPtr right;  // default value
    
    AssignmentPattern(std::unique_ptr<Pattern> l, ExprPtr r)
        : left(std::move(l)), right(std::move(r)) {}
    void accept(ASTVisitor& visitor) override;
};

class RestElement : public Pattern {
public:
    std::unique_ptr<Pattern> argument;
    
    explicit RestElement(std::unique_ptr<Pattern> arg)
        : argument(std::move(arg)) {}
    void accept(ASTVisitor& visitor) override;
};

class IdentifierPattern : public Pattern {
public:
    std::string name;
    TypePtr type;
    
    explicit IdentifierPattern(const std::string& n, TypePtr t = nullptr)
        : name(n), type(std::move(t)) {}
    void accept(ASTVisitor& visitor) override;
};

// ==================== Decorators ====================

class Decorator : public ASTNode {
public:
    std::string name;
    std::vector<ExprPtr> arguments;
    
    Decorator(const std::string& n, std::vector<ExprPtr> args = {})
        : name(n), arguments(std::move(args)) {}
    void accept(ASTVisitor& visitor) override;
};

// ==================== Statements ====================

class Stmt : public ASTNode {
};

class BlockStmt : public Stmt {
public:
    std::vector<StmtPtr> statements;
    
    explicit BlockStmt(std::vector<StmtPtr> stmts)
        : statements(std::move(stmts)) {}
    void accept(ASTVisitor& visitor) override;
};

class ExprStmt : public Stmt {
public:
    ExprPtr expression;
    
    explicit ExprStmt(ExprPtr expr) : expression(expr) {}
    void accept(ASTVisitor& visitor) override;
};

class VarDeclStmt : public Stmt {
public:
    enum class Kind { Var, Let, Const };
    
    struct Declarator {
        std::string name;
        ExprPtr init;
        TypePtr type;
    };
    
    Kind kind;
    std::vector<Declarator> declarations;
    
    VarDeclStmt(Kind k, std::vector<Declarator> decls)
        : kind(k), declarations(std::move(decls)) {}
    void accept(ASTVisitor& visitor) override;
};

class DeclStmt : public Stmt {
public:
    DeclPtr declaration;
    
    explicit DeclStmt(DeclPtr decl) : declaration(decl) {}
    void accept(ASTVisitor& visitor) override;
};

class IfStmt : public Stmt {
public:
    ExprPtr test;
    StmtPtr consequent;
    StmtPtr alternate;  // nullptr if no else
    
    IfStmt(ExprPtr t, StmtPtr c, StmtPtr a = nullptr)
        : test(t), consequent(c), alternate(a) {}
    void accept(ASTVisitor& visitor) override;
};

class WhileStmt : public Stmt {
public:
    ExprPtr test;
    StmtPtr body;
    
    WhileStmt(ExprPtr t, StmtPtr b) : test(t), body(b) {}
    void accept(ASTVisitor& visitor) override;
};

class DoWhileStmt : public Stmt {
public:
    StmtPtr body;
    ExprPtr test;
    
    DoWhileStmt(StmtPtr b, ExprPtr t) : body(b), test(t) {}
    void accept(ASTVisitor& visitor) override;
};

class ForStmt : public Stmt {
public:
    StmtPtr init;  // can be null
    ExprPtr test;  // can be null
    ExprPtr update;  // can be null
    StmtPtr body;
    
    ForStmt(StmtPtr i, ExprPtr t, ExprPtr u, StmtPtr b)
        : init(i), test(t), update(u), body(b) {}
    void accept(ASTVisitor& visitor) override;
};

class ForInStmt : public Stmt {
public:
    std::string left;      // variable name
    std::string kind;      // "var", "let", "const", or "" for existing variable
    ExprPtr right;
    StmtPtr body;
    
    ForInStmt(const std::string& l, const std::string& k, ExprPtr r, StmtPtr b)
        : left(l), kind(k), right(r), body(b) {}
    void accept(ASTVisitor& visitor) override;
};

class ForOfStmt : public Stmt {
public:
    std::string left;      // variable name
    std::string kind;      // "var", "let", "const", or "" for existing variable
    ExprPtr right;
    StmtPtr body;
    bool isAwait;
    
    ForOfStmt(const std::string& l, const std::string& k, ExprPtr r, StmtPtr b, bool await = false)
        : left(l), kind(k), right(r), body(b), isAwait(await) {}
    void accept(ASTVisitor& visitor) override;
};

class ReturnStmt : public Stmt {
public:
    ExprPtr argument;  // nullptr for bare return
    
    explicit ReturnStmt(ExprPtr arg = nullptr) : argument(arg) {}
    void accept(ASTVisitor& visitor) override;
};

class BreakStmt : public Stmt {
public:
    std::string label;
    
    explicit BreakStmt(const std::string& l = "") : label(l) {}
    void accept(ASTVisitor& visitor) override;
};

class ContinueStmt : public Stmt {
public:
    std::string label;
    
    explicit ContinueStmt(const std::string& l = "") : label(l) {}
    void accept(ASTVisitor& visitor) override;
};

class ThrowStmt : public Stmt {
public:
    ExprPtr argument;
    
    explicit ThrowStmt(ExprPtr arg) : argument(arg) {}
    void accept(ASTVisitor& visitor) override;
};

class TryStmt : public Stmt {
public:
    struct CatchClause {
        std::string param;
        StmtPtr body;
        SourceLocation location;
    };
    
    StmtPtr block;
    std::unique_ptr<CatchClause> handler;  // nullptr if no catch
    StmtPtr finalizer;     // nullptr if no finally
    
    TryStmt(StmtPtr b, std::unique_ptr<CatchClause> h, StmtPtr f)
        : block(std::move(b)), handler(std::move(h)), finalizer(std::move(f)) {}
    void accept(ASTVisitor& visitor) override;
};

class SwitchStmt : public Stmt {
public:
    struct Case {
        ExprPtr test;  // nullptr for default case
        std::vector<StmtPtr> consequent;
        SourceLocation location;
    };
    
    ExprPtr discriminant;
    std::vector<std::unique_ptr<Case>> cases;
    
    SwitchStmt(ExprPtr d, std::vector<std::unique_ptr<Case>> c)
        : discriminant(std::move(d)), cases(std::move(c)) {}
    void accept(ASTVisitor& visitor) override;
};

class LabeledStmt : public Stmt {
public:
    std::string label;
    StmtPtr statement;
    
    LabeledStmt(const std::string& l, StmtPtr stmt)
        : label(l), statement(stmt) {}
    void accept(ASTVisitor& visitor) override;
};

class WithStmt : public Stmt {
public:
    ExprPtr object;
    StmtPtr body;
    
    WithStmt(ExprPtr obj, StmtPtr b) : object(obj), body(b) {}
    void accept(ASTVisitor& visitor) override;
};

class DebuggerStmt : public Stmt {
public:
    DebuggerStmt() = default;
    void accept(ASTVisitor& visitor) override;
};

class EmptyStmt : public Stmt {
public:
    EmptyStmt() = default;
    void accept(ASTVisitor& visitor) override;
};

// Type aliases for compatibility
using BlockStatement = BlockStmt;
using ExpressionStatement = ExprStmt;
using VariableDeclaration = VarDeclStmt;
using IfStatement = IfStmt;
using WhileStatement = WhileStmt;
using DoWhileStatement = DoWhileStmt;
using ForStatement = ForStmt;
using ForInStatement = ForInStmt;
using ForOfStatement = ForOfStmt;
using ReturnStatement = ReturnStmt;
using BreakStatement = BreakStmt;
using ContinueStatement = ContinueStmt;
using ThrowStatement = ThrowStmt;
using TryStatement = TryStmt;
using SwitchStatement = SwitchStmt;
using SwitchCase = SwitchStmt::Case;
using CatchClause = TryStmt::CatchClause;

// ==================== Declarations ====================

class Decl : public ASTNode {
};

class FunctionDecl : public Decl {
public:
    std::string name;
    std::vector<std::string> params;
    std::vector<TypePtr> paramTypes;  // Type annotations for parameters
    StmtPtr body;
    bool isAsync = false;
    bool isGenerator = false;
    TypePtr returnType;

    FunctionDecl() = default;
    FunctionDecl(const std::string& n, std::vector<std::string> p,
                 StmtPtr b, bool async, bool generator, TypePtr ret)
        : name(n), params(std::move(p)), body(b),
          isAsync(async), isGenerator(generator), returnType(ret) {}
    void accept(ASTVisitor& visitor) override;
};

class ClassDecl : public Decl {
public:
    struct Method {
        std::string name;
        std::vector<std::string> params;
        StmtPtr body;
        enum class Kind { Method, Constructor, Get, Set } kind;
        bool isStatic = false;
        bool isAsync = false;
        bool isAbstract = false;
        TypePtr returnType;
        std::vector<std::unique_ptr<Decorator>> decorators;
    };
    
    struct Property {
        std::string name;
        ExprPtr initializer;
        TypePtr type;
        bool isStatic = false;
        bool isReadonly = false;
        std::vector<std::unique_ptr<Decorator>> decorators;
    };
    
    std::string name;
    std::string superclass;
    std::vector<std::string> interfaces;
    std::vector<std::string> typeParams;
    std::vector<Method> methods;
    std::vector<Property> properties;
    std::vector<std::unique_ptr<Decorator>> decorators;
    
    ClassDecl() = default;
    void accept(ASTVisitor& visitor) override;
};

class InterfaceDecl : public Decl {
public:
    struct MethodSignature {
        std::string name;
        std::vector<std::string> params;
        TypePtr returnType;
    };
    
    struct PropertySignature {
        std::string name;
        TypePtr type;
        bool isOptional = false;
    };
    
    std::string name;
    std::vector<std::string> typeParams;
    std::vector<std::string> extends;
    std::vector<MethodSignature> methods;
    std::vector<PropertySignature> properties;
    
    InterfaceDecl() = default;
    void accept(ASTVisitor& visitor) override;
};

class TypeAliasDecl : public Decl {
public:
    std::string name;
    std::vector<std::string> typeParams;
    TypePtr type;
    
    TypeAliasDecl() = default;
    void accept(ASTVisitor& visitor) override;
};

class EnumDecl : public Decl {
public:
    struct Member {
        std::string name;
        ExprPtr initializer;
    };
    
    std::string name;
    std::vector<Member> members;
    
    EnumDecl() = default;
    void accept(ASTVisitor& visitor) override;
};

class ImportDecl : public Decl {
public:
    struct Specifier {
        std::string imported;
        std::string local;
    };
    
    std::string source;
    std::string defaultImport;
    std::string namespaceImport;
    std::vector<Specifier> specifiers;
    
    ImportDecl() = default;
    void accept(ASTVisitor& visitor) override;
};

class ExportDecl : public Decl {
public:
    struct Specifier {
        std::string local;
        std::string exported;
    };
    
    bool isDefault = false;
    std::string source;
    std::string namespaceExport;
    ExprPtr declaration;  // For export default <expr>
    DeclPtr exportedDecl; // For export <decl>
    std::vector<Specifier> specifiers;
    
    ExportDecl() = default;
    void accept(ASTVisitor& visitor) override;
};

class Program : public ASTNode {
public:
    std::vector<StmtPtr> body;
    
    explicit Program(std::vector<StmtPtr> b) : body(std::move(b)) {}
    void accept(ASTVisitor& visitor) override;
};

// ==================== Visitor ====================

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    // Expressions
    virtual void visit(NumberLiteral& node) = 0;
    virtual void visit(StringLiteral& node) = 0;
    virtual void visit(BooleanLiteral& node) = 0;
    virtual void visit(NullLiteral& node) = 0;
    virtual void visit(UndefinedLiteral& node) = 0;
    virtual void visit(Identifier& node) = 0;
    virtual void visit(BinaryExpr& node) = 0;
    virtual void visit(UnaryExpr& node) = 0;
    virtual void visit(UpdateExpr& node) = 0;
    virtual void visit(CallExpr& node) = 0;
    virtual void visit(MemberExpr& node) = 0;
    virtual void visit(ConditionalExpr& node) = 0;
    virtual void visit(ArrayExpr& node) = 0;
    virtual void visit(ObjectExpr& node) = 0;
    virtual void visit(FunctionExpr& node) = 0;
    virtual void visit(ArrowFunctionExpr& node) = 0;
    virtual void visit(ClassExpr& node) = 0;
    virtual void visit(NewExpr& node) = 0;
    virtual void visit(ThisExpr& node) = 0;
    virtual void visit(SuperExpr& node) = 0;
    virtual void visit(SpreadExpr& node) = 0;
    virtual void visit(TemplateLiteralExpr& node) = 0;
    virtual void visit(AwaitExpr& node) = 0;
    virtual void visit(YieldExpr& node) = 0;
    virtual void visit(AsExpr& node) = 0;
    virtual void visit(SatisfiesExpr& node) = 0;
    virtual void visit(NonNullExpr& node) = 0;
    virtual void visit(TaggedTemplateExpr& node) = 0;
    virtual void visit(SequenceExpr& node) = 0;
    virtual void visit(AssignmentExpr& node) = 0;
    virtual void visit(ParenthesizedExpr& node) = 0;
    virtual void visit(MetaProperty& node) = 0;
    virtual void visit(ImportExpr& node) = 0;
    
    // JSX/TSX
    virtual void visit(JSXElement& node) = 0;
    virtual void visit(JSXFragment& node) = 0;
    virtual void visit(JSXText& node) = 0;
    virtual void visit(JSXExpressionContainer& node) = 0;
    virtual void visit(JSXAttribute& node) = 0;
    virtual void visit(JSXSpreadAttribute& node) = 0;
    
    // Patterns
    virtual void visit(ObjectPattern& node) = 0;
    virtual void visit(ArrayPattern& node) = 0;
    virtual void visit(AssignmentPattern& node) = 0;
    virtual void visit(RestElement& node) = 0;
    virtual void visit(IdentifierPattern& node) = 0;
    
    virtual void visit(Decorator& node) = 0;
    
    // Statements
    virtual void visit(BlockStmt& node) = 0;
    virtual void visit(ExprStmt& node) = 0;
    virtual void visit(VarDeclStmt& node) = 0;
    virtual void visit(DeclStmt& node) = 0;
    virtual void visit(IfStmt& node) = 0;
    virtual void visit(WhileStmt& node) = 0;
    virtual void visit(DoWhileStmt& node) = 0;
    virtual void visit(ForStmt& node) = 0;
    virtual void visit(ForInStmt& node) = 0;
    virtual void visit(ForOfStmt& node) = 0;
    virtual void visit(ReturnStmt& node) = 0;
    virtual void visit(BreakStmt& node) = 0;
    virtual void visit(ContinueStmt& node) = 0;
    virtual void visit(ThrowStmt& node) = 0;
    virtual void visit(TryStmt& node) = 0;
    virtual void visit(SwitchStmt& node) = 0;
    virtual void visit(LabeledStmt& node) = 0;
    virtual void visit(WithStmt& node) = 0;
    virtual void visit(DebuggerStmt& node) = 0;
    virtual void visit(EmptyStmt& node) = 0;
    
    // Declarations
    virtual void visit(FunctionDecl& node) = 0;
    virtual void visit(ClassDecl& node) = 0;
    virtual void visit(InterfaceDecl& node) = 0;
    virtual void visit(TypeAliasDecl& node) = 0;
    virtual void visit(EnumDecl& node) = 0;
    virtual void visit(ImportDecl& node) = 0;
    virtual void visit(ExportDecl& node) = 0;
    
    // Program
    virtual void visit(Program& node) = 0;
};

} // namespace nova
