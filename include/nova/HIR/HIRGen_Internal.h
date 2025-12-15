// HIRGen_Internal.h - Internal header for HIRGenerator class
// This header exposes the HIRGenerator class structure to allow
// splitting the implementation across multiple .cpp files

#pragma once

#include "nova/HIR/HIR.h"
#include "nova/Frontend/AST.h"
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <variant>
#include <functional>
#include <limits>

namespace nova::hir {

class HIRGenerator : public ASTVisitor {
public:
    explicit HIRGenerator(HIRModule* module)
        : module_(module), builder_(nullptr), currentFunction_(nullptr) {}

    HIRModule* getModule() { return module_; }

    // Visitor method declarations
    // These are implemented in separate files for better code organization

    // Literals (HIRGen_Literals.cpp)
    void visit(NumberLiteral& node) override;
    void visit(BigIntLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(RegexLiteralExpr& node) override;
    void visit(BooleanLiteral& node) override;
    void visit(NullLiteral& node) override;
    void visit(UndefinedLiteral& node) override;
    void visit(TemplateLiteralExpr& node) override;

    // Operators (will be in HIRGen_Operators.cpp)
    void visit(BinaryExpr& node) override;
    void visit(UnaryExpr& node) override;
    void visit(UpdateExpr& node) override;
    void visit(AssignmentExpr& node) override;
    void visit(ConditionalExpr& node) override;

    // Functions (will be in HIRGen_Functions.cpp)
    void visit(FunctionExpr& node) override;
    void visit(ArrowFunctionExpr& node) override;
    void visit(FunctionDecl& node) override;

    // Classes (will be in HIRGen_Classes.cpp)
    void visit(ClassExpr& node) override;
    void visit(ClassDecl& node) override;
    void visit(NewExpr& node) override;
    void visit(ThisExpr& node) override;
    void visit(SuperExpr& node) override;

    // Arrays (will be in HIRGen_Arrays.cpp)
    void visit(ArrayExpr& node) override;

    // Objects (will be in HIRGen_Objects.cpp)
    void visit(ObjectExpr& node) override;
    void visit(MemberExpr& node) override;

    // Control Flow (will be in HIRGen_ControlFlow.cpp)
    void visit(IfStmt& node) override;
    void visit(SwitchStmt& node) override;
    void visit(ForStmt& node) override;
    void visit(WhileStmt& node) override;
    void visit(DoWhileStmt& node) override;
    void visit(ForInStmt& node) override;
    void visit(ForOfStmt& node) override;
    void visit(BreakStmt& node) override;
    void visit(ContinueStmt& node) override;
    void visit(ReturnStmt& node) override;
    void visit(ThrowStmt& node) override;
    void visit(TryStmt& node) override;

    // Statements (will be in HIRGen_Statements.cpp)
    void visit(VarDeclStmt& node) override;
    void visit(DeclStmt& node) override;
    void visit(UsingStmt& node) override;
    void visit(BlockStmt& node) override;
    void visit(ExprStmt& node) override;
    void visit(EmptyStmt& node) override;
    void visit(DebuggerStmt& node) override;
    void visit(WithStmt& node) override;
    void visit(LabeledStmt& node) override;

    // Calls (will be in HIRGen_Calls.cpp)
    void visit(CallExpr& node) override;

    // Advanced features (will be in HIRGen_Advanced.cpp)
    void visit(AwaitExpr& node) override;
    void visit(YieldExpr& node) override;
    void visit(SpreadExpr& node) override;
    void visit(ImportExpr& node) override;

    // Identifiers and other expressions
    void visit(Identifier& node) override;
    void visit(Program& node) override;
    void visit(SequenceExpr& node) override;
    void visit(ParenthesizedExpr& node) override;
    void visit(MetaProperty& node) override;
    void visit(TaggedTemplateExpr& node) override;

    // TypeScript specific expressions
    void visit(AsExpr& node) override;
    void visit(SatisfiesExpr& node) override;
    void visit(NonNullExpr& node) override;

    // JSX/TSX
    void visit(JSXElement& node) override;
    void visit(JSXFragment& node) override;
    void visit(JSXText& node) override;
    void visit(JSXExpressionContainer& node) override;
    void visit(JSXAttribute& node) override;
    void visit(JSXSpreadAttribute& node) override;

    // Patterns
    void visit(ObjectPattern& node) override;
    void visit(ArrayPattern& node) override;
    void visit(AssignmentPattern& node) override;
    void visit(RestElement& node) override;
    void visit(IdentifierPattern& node) override;

    // Decorators
    void visit(Decorator& node) override;

    // Declarations
    void visit(InterfaceDecl& node) override;
    void visit(TypeAliasDecl& node) override;
    void visit(EnumDecl& node) override;
    void visit(ImportDecl& node) override;
    void visit(ExportDecl& node) override;

    // Helper methods
    HIRValue* lookupVariable(const std::string& name);
    void generateYieldDelegate(YieldExpr& node);
    std::string getBuiltinFunctionName(const std::string& module, const std::string& funcName);
    bool isBuiltinFunctionCall(const std::string& name);
    std::string getBuiltinRuntimeFunction(const std::string& name);

    // Closure helper methods
    hir::HIRStructType* createClosureEnvironment(const std::string& funcName);

    // Class helper methods
    void generateConstructorFunction(const std::string& className, const ClassDecl::Method& constructor, hir::HIRStructType* structType, std::function<HIRType::Kind(Type::Kind)> convertTypeKind);
    void generateDefaultConstructor(const std::string& className, hir::HIRStructType* structType);
    void generateMethodFunction(const std::string& className, const ClassDecl::Method& method, hir::HIRStructType* structType, std::function<HIRType::Kind(Type::Kind)> convertTypeKind);
    void generateStaticMethodFunction(const std::string& className, const ClassDecl::Method& method, std::function<HIRType::Kind(Type::Kind)> convertTypeKind);
    void generateGetterFunction(const std::string& className, const ClassDecl::Method& method, hir::HIRStructType* structType, std::function<HIRType::Kind(Type::Kind)> convertTypeKind);
    void generateSetterFunction(const std::string& className, const ClassDecl::Method& method, hir::HIRStructType* structType, std::function<HIRType::Kind(Type::Kind)> convertTypeKind);
    std::string resolveMethodToClass(const std::string& className, const std::string& methodName);

private:
    // Core module and builder
    HIRModule* module_;
    std::unique_ptr<HIRBuilder> builder_;
    HIRFunction* currentFunction_;

    // Context tracking
    HIRValue* currentThis_ = nullptr;  // Current 'this' context for methods
    hir::HIRStructType* currentClassStructType_ = nullptr;  // Current class struct type
    HIRValue* lastValue_ = nullptr;

    // Symbol tables and scopes
    std::unordered_map<std::string, HIRValue*> symbolTable_;
    std::vector<std::unordered_map<std::string, HIRValue*>> scopeStack_;  // Parent scopes for closures

    // Function tracking
    std::unordered_map<std::string, std::string> functionReferences_;  // Maps variable names to function names
    std::string lastFunctionName_;  // Tracks the last created arrow function name
    std::unordered_map<std::string, const std::vector<ExprPtr>*> functionDefaultValues_;  // Maps function names to default values
    std::unordered_set<std::string> functionVars_;  // Set of variable names that are functions
    std::unordered_map<std::string, int64_t> functionParamCounts_;  // Function name -> param count

    // Closure tracking
    std::unordered_map<std::string, std::unordered_set<std::string>> capturedVariables_;  // Maps function name -> captured variable names
    std::unordered_map<std::string, hir::HIRStructType*> closureEnvironments_;  // Maps function name -> environment struct type
    std::unordered_map<std::string, std::vector<std::string>> environmentFieldNames_;  // Maps function name -> ordered field names
    std::unordered_map<std::string, std::vector<hir::HIRValue*>> environmentFieldValues_;  // Maps function name -> ordered field HIRValues

    // Class tracking
    std::string lastClassName_;      // Tracks the last created class name for class expressions
    std::unordered_map<std::string, std::string> classReferences_;  // Maps variable names to class names
    std::unordered_set<std::string> classNames_;  // Track class names for static method calls
    std::unordered_set<std::string> staticMethods_;  // Track static method names (ClassName_methodName)
    std::unordered_map<std::string, std::unordered_set<std::string>> classGetters_;  // Maps className -> getter property names
    std::unordered_map<std::string, std::unordered_set<std::string>> classSetters_;  // Maps className -> setter property names
    std::unordered_map<std::string, int64_t> staticPropertyValues_;  // Maps ClassName_propName -> value
    std::unordered_map<std::string, std::unordered_set<std::string>> classStaticProps_;  // Maps className -> static property names
    std::unordered_map<std::string, std::string> classInheritance_;  // Maps className -> parent className
    std::unordered_map<std::string, hir::HIRStructType*> classStructTypes_;  // Maps className -> struct type
    std::unordered_map<std::string, std::unordered_set<std::string>> classOwnMethods_;  // Maps className -> method names defined in that class (for inheritance resolution)

    // Store field initial literal values for inheritance
    struct FieldInitValue {
        enum class Kind { String, Number } kind;
        std::string stringValue;
        double numberValue;
    };
    std::unordered_map<std::string, std::unordered_map<std::string, FieldInitValue>> classFieldInitialValues_;

    // Enum tracking
    std::unordered_map<std::string, std::unordered_map<std::string, int64_t>> enumTable_;  // Maps enum name -> member name -> value

    // TypedArray type tracking
    std::unordered_map<std::string, std::string> typedArrayTypes_;  // Maps variable name -> TypedArray type
    std::string lastTypedArrayType_;  // Temporarily stores TypedArray type created by NewExpr

    // ArrayBuffer type tracking
    std::unordered_set<std::string> arrayBufferVars_;  // Set of variable names that are ArrayBuffers
    bool lastWasArrayBuffer_ = false;

    // SharedArrayBuffer type tracking (ES2017)
    std::unordered_set<std::string> sharedArrayBufferVars_;
    bool lastWasSharedArrayBuffer_ = false;

    // BigInt type tracking (ES2020)
    std::unordered_set<std::string> bigIntVars_;
    bool lastWasBigInt_ = false;

    // DataView type tracking
    std::unordered_set<std::string> dataViewVars_;
    bool lastWasDataView_ = false;

    // Date type tracking (ES1)
    std::unordered_set<std::string> dateVars_;
    bool lastWasDate_ = false;

    // Error type tracking (ES1)
    std::unordered_set<std::string> errorVars_;
    bool lastWasError_ = false;

    // SuppressedError tracking (ES2024)
    std::unordered_set<std::string> suppressedErrorVars_;
    bool lastWasSuppressedError_ = false;

    // Symbol tracking (ES2015)
    std::unordered_set<std::string> symbolVars_;
    bool lastWasSymbol_ = false;

    // DisposableStack tracking (ES2024)
    std::unordered_set<std::string> disposableStackVars_;
    bool lastWasDisposableStack_ = false;

    // AsyncDisposableStack tracking (ES2024)
    std::unordered_set<std::string> asyncDisposableStackVars_;
    bool lastWasAsyncDisposableStack_ = false;

    // FinalizationRegistry tracking (ES2021)
    std::unordered_set<std::string> finalizationRegistryVars_;
    bool lastWasFinalizationRegistry_ = false;

    // Promise tracking (ES2015)
    std::unordered_set<std::string> promiseVars_;
    bool lastWasPromise_ = false;

    // Generator tracking (ES2015)
    std::unordered_set<std::string> generatorVars_;
    std::unordered_set<std::string> generatorFuncs_;
    std::unordered_set<std::string> asyncGeneratorFuncs_;  // ES2018
    std::unordered_set<std::string> asyncGeneratorVars_;
    bool lastWasAsyncGenerator_ = false;
    bool lastWasGenerator_ = false;
    HIRValue* currentGeneratorPtr_ = nullptr;

    // Generator state machine
    int yieldStateCounter_ = 0;
    std::vector<HIRBasicBlock*> yieldResumeBlocks_;
    HIRBasicBlock* generatorBodyBlock_ = nullptr;
    HIRBasicBlock* generatorDispatchBlock_ = nullptr;
    HIRValue* generatorStateValue_ = nullptr;
    HIRFunction* currentSetStateFunc_ = nullptr;

    // Generator local variable storage
    std::unordered_map<std::string, int> generatorVarSlots_;
    int generatorNextLocalSlot_ = 0;
    HIRFunction* generatorStoreLocalFunc_ = nullptr;
    HIRFunction* generatorLoadLocalFunc_ = nullptr;

    // GeneratorFunction tracking (ES2015)
    std::unordered_set<std::string> generatorFunctionVars_;
    bool lastWasGeneratorFunction_ = false;

    // AsyncGeneratorFunction tracking (ES2018)
    std::unordered_set<std::string> asyncGeneratorFunctionVars_;
    bool lastWasAsyncGeneratorFunction_ = false;

    // IteratorResult tracking
    std::unordered_set<std::string> iteratorResultVars_;
    bool lastWasIteratorResult_ = false;

    // Runtime array tracking
    std::unordered_set<std::string> runtimeArrayVars_;
    bool lastWasRuntimeArray_ = false;

    // Label support
    std::string currentLabel_;

    // Exception handling
    HIRBasicBlock* currentCatchBlock_ = nullptr;
    [[maybe_unused]] HIRBasicBlock* currentFinallyBlock_ = nullptr;
    [[maybe_unused]] HIRBasicBlock* currentTryEndBlock_ = nullptr;

    // Break/continue target stacks for loops and switches
    std::vector<HIRBasicBlock*> breakTargetStack_;
    std::vector<HIRBasicBlock*> continueTargetStack_;

    // globalThis tracking (ES2020)
    bool lastWasGlobalThis_ = false;

    // Object method tracking
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> objectMethodFunctions_;
    // Maps object variable name -> property name -> generated function name
    std::unordered_map<std::string, std::unordered_set<std::string>> objectMethodProperties_;
    // Maps object variable name -> set of property names that are methods
    std::string currentObjectName_;  // Temporarily stores object variable name during assignment

    // Intl tracking (Internationalization API)
    std::unordered_set<std::string> numberFormatVars_;
    std::unordered_set<std::string> dateTimeFormatVars_;
    std::unordered_set<std::string> collatorVars_;
    std::unordered_set<std::string> pluralRulesVars_;
    std::unordered_set<std::string> relativeTimeFormatVars_;
    std::unordered_set<std::string> listFormatVars_;
    std::unordered_set<std::string> displayNamesVars_;
    std::unordered_set<std::string> localeVars_;
    std::unordered_set<std::string> segmenterVars_;
    bool lastWasNumberFormat_ = false;
    bool lastWasDateTimeFormat_ = false;
    bool lastWasCollator_ = false;
    bool lastWasPluralRules_ = false;
    bool lastWasRelativeTimeFormat_ = false;
    bool lastWasListFormat_ = false;
    bool lastWasDisplayNames_ = false;
    bool lastWasLocale_ = false;
    bool lastWasSegmenter_ = false;

    // Iterator tracking (ES2025 Iterator Helpers)
    std::unordered_set<std::string> iteratorVars_;
    bool lastWasIterator_ = false;

    // Map tracking (ES2015)
    std::unordered_set<std::string> mapVars_;
    bool lastWasMap_ = false;

    // Set tracking (ES2015)
    std::unordered_set<std::string> setVars_;
    bool lastWasSet_ = false;

    // WeakMap tracking (ES2015)
    std::unordered_set<std::string> weakMapVars_;
    bool lastWasWeakMap_ = false;

    // WeakRef tracking (ES2021)
    std::unordered_set<std::string> weakRefVars_;
    bool lastWasWeakRef_ = false;

    // WeakSet tracking (ES2015)
    std::unordered_set<std::string> weakSetVars_;
    bool lastWasWeakSet_ = false;

    // URL tracking (Web API)
    std::unordered_set<std::string> urlVars_;
    bool lastWasURL_ = false;

    // URLSearchParams tracking (Web API)
    std::unordered_set<std::string> urlSearchParamsVars_;
    bool lastWasURLSearchParams_ = false;

    // TextEncoder tracking (Web API)
    std::unordered_set<std::string> textEncoderVars_;
    bool lastWasTextEncoder_ = false;

    // TextDecoder tracking (Web API)
    std::unordered_set<std::string> textDecoderVars_;
    bool lastWasTextDecoder_ = false;

    // Headers tracking (Web API)
    std::unordered_set<std::string> headersVars_;
    bool lastWasHeaders_ = false;

    // Request tracking (Web API)
    std::unordered_set<std::string> requestVars_;
    bool lastWasRequest_ = false;

    // Response tracking (Web API)
    std::unordered_set<std::string> responseVars_;
    bool lastWasResponse_ = false;

    // Builtin object type tracking
    std::unordered_map<std::string, std::string> variableObjectTypes_;
    std::string lastBuiltinObjectType_;

    // Built-in module imports
    std::unordered_map<std::string, std::string> builtinModuleImports_;
    std::unordered_map<std::string, std::string> builtinFunctionImports_;
};

} // namespace nova::hir
