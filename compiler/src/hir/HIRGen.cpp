#include "nova/HIR/HIRGen.h"
#include "nova/HIR/HIR.h"
#include "nova/Frontend/AST.h"
#include <memory>
#include <unordered_map>
#include <variant>
#include <functional>

namespace nova::hir {

class HIRGenerator : public ASTVisitor {
public:
    explicit HIRGenerator(HIRModule* module)
        : module_(module), builder_(nullptr), currentFunction_(nullptr) {}
    
    HIRModule* getModule() { return module_; }
    
    // Expressions
    void visit(NumberLiteral& node) override {
        // Create numeric constant
        // Check if the value is an integer
        if (node.value == static_cast<int64_t>(node.value)) {
            lastValue_ = builder_->createIntConstant(static_cast<int64_t>(node.value));
        } else {
            lastValue_ = builder_->createFloatConstant(node.value);
        }
    }
    
    void visit(StringLiteral& node) override {
        lastValue_ = builder_->createStringConstant(node.value);
    }
    
    void visit(BooleanLiteral& node) override {
        lastValue_ = builder_->createBoolConstant(node.value);
    }
    
    void visit(NullLiteral& node) override {
        (void)node;
        auto nullType = std::make_shared<HIRType>(HIRType::Kind::Any);
        lastValue_ = builder_->createNullConstant(nullType.get());
    }
    
    void visit(UndefinedLiteral& node) override {
        (void)node;
        auto undefType = std::make_shared<HIRType>(HIRType::Kind::Unknown);
        lastValue_ = builder_->createNullConstant(undefType.get());
    }
    
    void visit(Identifier& node) override {
        // Look up variable in symbol table
        auto it = symbolTable_.find(node.name);
        if (it != symbolTable_.end()) {
            auto value = it->second;
            // Check if this is an alloca (memory location)
            // Try to cast to HIRInstruction to check the opcode
            try {
                if (auto* inst = dynamic_cast<hir::HIRInstruction*>(value)) {
                    if (inst && inst->opcode == hir::HIRInstruction::Opcode::Alloca) {
                        // For allocas, we need to load the value
                        lastValue_ = builder_->createLoad(value, node.name);
                        return;
                    }
                }
            } catch (...) {
                // If cast fails, just use the value directly
            }
            // For other values (like function parameters), use directly
            lastValue_ = value;
        }
    }
    
    void visit(BinaryExpr& node) override {
        using Op = BinaryExpr::Op;

        // Handle logical operators
        // TODO: Implement proper short-circuit evaluation
        // For now, evaluate both operands (non-short-circuit)
        if (node.op == Op::LogicalAnd || node.op == Op::LogicalOr) {
            // Evaluate both operands
            node.left->accept(*this);
            auto lhs = lastValue_;

            node.right->accept(*this);
            auto rhs = lastValue_;

            // For AND: result is true if both are true
            // For OR: result is true if either is true
            // We can implement this using comparison and arithmetic
            auto zero = builder_->createIntConstant(0);
            auto lhsBool = builder_->createNe(lhs, zero);
            auto rhsBool = builder_->createNe(rhs, zero);

            if (node.op == Op::LogicalAnd) {
                // AND: both must be true
                // Multiply the booleans: true(1) * true(1) = true(1), otherwise false(0)
                // This will generate `and i1` in LLVM which is correct
                lastValue_ = builder_->createMul(lhsBool, rhsBool);
            } else {
                // OR: at least one must be true
                // Use the formula: a OR b = a + b - (a AND b)
                // This works for boolean values:
                //   0 OR 0 = 0 + 0 - 0 = 0
                //   0 OR 1 = 0 + 1 - 0 = 1
                //   1 OR 0 = 1 + 0 - 0 = 1
                //   1 OR 1 = 1 + 1 - 1 = 1
                auto product = builder_->createMul(lhsBool, rhsBool);  // a AND b
                auto sum = builder_->createAdd(lhsBool, rhsBool);       // a + b
                lastValue_ = builder_->createSub(sum, product);          // a + b - (a AND b)
            }

            return;
        }

        // For non-logical operators, evaluate both operands normally
        // Generate left operand
        node.left->accept(*this);
        auto lhs = lastValue_;

        // Generate right operand
        node.right->accept(*this);
        auto rhs = lastValue_;

        // Generate operation based on operator
        switch (node.op) {
            case Op::Add:
                lastValue_ = builder_->createAdd(lhs, rhs);
                break;
            case Op::Sub:
                lastValue_ = builder_->createSub(lhs, rhs);
                break;
            case Op::Mul:
                lastValue_ = builder_->createMul(lhs, rhs);
                break;
            case Op::Div:
                lastValue_ = builder_->createDiv(lhs, rhs);
                break;
            case Op::Equal:
            case Op::StrictEqual:  // === works same as == for primitive types
                lastValue_ = builder_->createEq(lhs, rhs);
                break;
            case Op::NotEqual:
            case Op::StrictNotEqual:  // !== works same as != for primitive types
                lastValue_ = builder_->createNe(lhs, rhs);
                break;
            case Op::Less:
                lastValue_ = builder_->createLt(lhs, rhs);
                break;
            case Op::LessEqual:
                lastValue_ = builder_->createLe(lhs, rhs);
                break;
            case Op::Greater:
                lastValue_ = builder_->createGt(lhs, rhs);
                break;
            case Op::GreaterEqual:
                lastValue_ = builder_->createGe(lhs, rhs);
                break;
            default:
                // Add more operators as needed
                break;
        }
    }
    
    void visit(UnaryExpr& node) override {
        node.operand->accept(*this);
        auto operand = lastValue_;
        
        using Op = UnaryExpr::Op;
        switch (node.op) {
            case Op::Minus:
                // Negate
                {
                    auto zero = builder_->createIntConstant(0);
                    lastValue_ = builder_->createSub(zero, operand);
                }
                break;
            case Op::Not:
                // Logical not - compare with false
                {
                    auto falsVal = builder_->createBoolConstant(false);
                    lastValue_ = builder_->createEq(operand, falsVal);
                }
                break;
            default:
                // Other operators
                break;
        }
    }
    
    void visit(UpdateExpr& node) override {
        // ++x or x++ implementation
        node.argument->accept(*this);
    }
    
    void visit(CallExpr& node) override {
        // Check if this is a string method call: str.substring(...)
        if (auto* memberExpr = dynamic_cast<MemberExpr*>(node.callee.get())) {
            // Get the object and method name
            memberExpr->object->accept(*this);
            HIRValue* object = lastValue_;

            if (auto* propExpr = dynamic_cast<Identifier*>(memberExpr->property.get())) {
                std::string methodName = propExpr->name;

                // Check if object is a string type
                bool isStringMethod = object && object->type &&
                                     object->type->kind == hir::HIRType::Kind::String;

                if (isStringMethod) {
                    std::cerr << "DEBUG HIRGen: Detected string method call: " << methodName << std::endl;

                    // Generate arguments
                    std::vector<HIRValue*> args;
                    args.push_back(object);  // First argument is the string itself
                    for (auto& arg : node.arguments) {
                        arg->accept(*this);
                        args.push_back(lastValue_);
                    }

                    // Create or get runtime function based on method name
                    std::string runtimeFuncName;
                    std::vector<HIRTypePtr> paramTypes;
                    HIRTypePtr returnType;

                    if (methodName == "substring") {
                        runtimeFuncName = "nova_string_substring";
                        paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::String));
                        paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::I64));
                        paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::I64));
                        returnType = std::make_shared<HIRType>(HIRType::Kind::String);
                    } else if (methodName == "indexOf") {
                        runtimeFuncName = "nova_string_indexOf";
                        paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::String));
                        paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::String));
                        returnType = std::make_shared<HIRType>(HIRType::Kind::I64);
                    } else if (methodName == "charAt") {
                        runtimeFuncName = "nova_string_charAt";
                        paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::String));
                        paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::I64));
                        returnType = std::make_shared<HIRType>(HIRType::Kind::String);
                    } else {
                        std::cerr << "DEBUG HIRGen: Unknown string method: " << methodName << std::endl;
                        lastValue_ = nullptr;
                        return;
                    }

                    // Check if function already exists
                    HIRFunction* runtimeFunc = nullptr;
                    auto& functions = module_->functions;
                    for (auto& func : functions) {
                        if (func->name == runtimeFuncName) {
                            runtimeFunc = func.get();
                            break;
                        }
                    }

                    // Create function if it doesn't exist
                    if (!runtimeFunc) {
                        HIRFunctionType* funcType = new HIRFunctionType(paramTypes, returnType);
                        HIRFunctionPtr funcPtr = module_->createFunction(runtimeFuncName, funcType);
                        funcPtr->linkage = HIRFunction::Linkage::External;
                        runtimeFunc = funcPtr.get();
                        std::cerr << "DEBUG HIRGen: Created external function: " << runtimeFuncName << std::endl;
                    }

                    // Create call to runtime function
                    lastValue_ = builder_->createCall(runtimeFunc, args, "str_method");
                    return;
                }
            }
        }

        // Generate callee
        node.callee->accept(*this);

        // Generate arguments
        std::vector<HIRValue*> args;
        for (auto& arg : node.arguments) {
            arg->accept(*this);
            args.push_back(lastValue_);
        }

        // Lookup function
        if (auto* id = dynamic_cast<Identifier*>(node.callee.get())) {
            auto func = module_->getFunction(id->name);
            if (func) {
                lastValue_ = builder_->createCall(func.get(), args);
            }
        }
    }
    
    void visit(MemberExpr& node) override {
        // Evaluate the object/array
        node.object->accept(*this);
        auto object = lastValue_;

        if (node.isComputed) {
            // Computed member: obj[property] e.g., arr[index]
            node.property->accept(*this);
            auto index = lastValue_;

            // Create GetElement instruction for array indexing
            lastValue_ = builder_->createGetElement(object, index, "elem");
        } else {
            // Regular member: obj.property (struct field access)
            if (auto propExpr = dynamic_cast<Identifier*>(node.property.get())) {
                std::string propertyName = propExpr->name;

                // Try to get the struct type from the object
                uint32_t fieldIndex = 0;
                bool found = false;

                std::cerr << "DEBUG HIRGen: Accessing property '" << propertyName << "' on object" << std::endl;
                if (object && object->type) {
                    std::cerr << "DEBUG HIRGen: Object type kind=" << static_cast<int>(object->type->kind) << std::endl;
                    std::cerr << "DEBUG HIRGen: Object type ptr=" << object->type.get() << std::endl;

                    // Try to cast to HIRPointerType
                    hir::HIRPointerType* ptrTypeCast = dynamic_cast<hir::HIRPointerType*>(object->type.get());
                    std::cerr << "DEBUG HIRGen: dynamic_cast result=" << ptrTypeCast << std::endl;

                    // Check if it's a pointer to struct
                    if (auto ptrType = ptrTypeCast) {
                        std::cerr << "DEBUG HIRGen: Object is a pointer type" << std::endl;
                        if (ptrType->pointeeType) {
                            std::cerr << "DEBUG HIRGen: Pointee type kind=" << static_cast<int>(ptrType->pointeeType->kind) << std::endl;
                        }
                        if (auto structType = dynamic_cast<hir::HIRStructType*>(ptrType->pointeeType.get())) {
                            std::cerr << "DEBUG HIRGen: Pointee is a struct with " << structType->fields.size() << " fields" << std::endl;
                            // Find the field index by name
                            for (size_t i = 0; i < structType->fields.size(); ++i) {
                                if (structType->fields[i].name == propertyName) {
                                    fieldIndex = static_cast<uint32_t>(i);
                                    found = true;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (found) {
                    // Create GetField instruction with the correct field index
                    std::cerr << "DEBUG HIRGen: Found property '" << propertyName << "' at index " << fieldIndex << std::endl;
                    lastValue_ = builder_->createGetField(object, fieldIndex, propertyName);
                } else {
                    // Check for built-in string properties
                    if (object && object->type && object->type->kind == hir::HIRType::Kind::String && propertyName == "length") {
                        std::cerr << "DEBUG HIRGen: Accessing built-in string.length property" << std::endl;

                        // Try to find if this is a string literal constant
                        hir::HIRConstant* strConst = dynamic_cast<hir::HIRConstant*>(object);

                        // Check if we found a string literal constant
                        if (strConst && strConst->kind == hir::HIRConstant::Kind::String) {
                            // For string literals, we can compute length at compile time
                            const std::string& strVal = std::get<std::string>(strConst->value);
                            int64_t length = static_cast<int64_t>(strVal.length());
                            std::cerr << "DEBUG HIRGen: String literal '" << strVal << "' length = " << length << std::endl;
                            lastValue_ = builder_->createIntConstant(length);
                        } else {
                            // For dynamic strings (from concat, variables, etc.), call strlen runtime function
                            std::cerr << "DEBUG HIRGen: Creating strlen call for dynamic string" << std::endl;

                            // Create or get strlen intrinsic function
                            // We'll create a temporary HIRFunction for strlen
                            // The actual implementation will be provided at link time
                            hir::HIRFunction* strlenFunc = nullptr;

                            // Check if strlen function already exists in module
                            auto& functions = module_->functions;
                            for (auto& func : functions) {
                                if (func->name == "strlen") {
                                    strlenFunc = func.get();
                                    break;
                                }
                            }

                            // If not found, create it
                            if (!strlenFunc) {
                                std::cerr << "DEBUG HIRGen: Creating strlen intrinsic function declaration" << std::endl;

                                // Create function type: i64 strlen(i8*)
                                std::vector<HIRTypePtr> paramTypes;
                                paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::String));

                                HIRTypePtr retType = std::make_shared<HIRType>(HIRType::Kind::I64);
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, retType);

                                // Create function using module's createFunction
                                HIRFunctionPtr strlenFuncPtr = module_->createFunction("strlen", funcType);

                                // Set linkage to external (will be provided at link time)
                                strlenFuncPtr->linkage = HIRFunction::Linkage::External;

                                strlenFunc = strlenFuncPtr.get();
                                std::cerr << "DEBUG HIRGen: Created strlen function with external linkage" << std::endl;
                            }

                            // Create call to strlen
                            std::vector<HIRValue*> args = { object };
                            lastValue_ = builder_->createCall(strlenFunc, args, "str_len");
                        }
                    } else {
                        // Property not found, return 0 as placeholder
                        std::cerr << "Warning: Property '" << propertyName << "' not found in struct" << std::endl;
                        if (object && object->type) {
                            std::cerr << "  Object type: kind=" << static_cast<int>(object->type->kind) << std::endl;
                        }
                        lastValue_ = builder_->createIntConstant(0);
                    }
                }
            }
        }
    }
    
    void visit(ConditionalExpr& node) override {
        // Ternary operator: test ? consequent : alternate
        node.test->accept(*this);
    }
    
    void visit(ArrayExpr& node) override {
        // Array literal construction
        std::vector<HIRValue*> elementValues;

        // Evaluate all elements
        for (const auto& elem : node.elements) {
            elem->accept(*this);
            if (lastValue_) {
                elementValues.push_back(lastValue_);
            }
        }

        // Create array construction instruction
        lastValue_ = builder_->createArrayConstruct(elementValues, "arr");
    }
    
    void visit(ObjectExpr& node) override {
        // Object literal construction
        // Create struct type with fields for each property
        std::vector<hir::HIRStructType::Field> fields;
        std::vector<hir::HIRValue*> fieldValues;
        std::string structName = "anon_obj";

        // Evaluate all property values and build field list
        for (size_t i = 0; i < node.properties.size(); ++i) {
            auto& prop = node.properties[i];

            // Get the property name from the key
            std::string fieldName = "field" + std::to_string(i);  // Default name
            if (auto identifier = dynamic_cast<Identifier*>(prop.key.get())) {
                fieldName = identifier->name;
            }

            // Evaluate the property value
            prop.value->accept(*this);
            fieldValues.push_back(lastValue_);

            // Create field descriptor
            hir::HIRStructType::Field field;
            field.name = fieldName;
            field.type = lastValue_->type;  // Use the value's type
            field.isPublic = true;
            fields.push_back(field);
        }

        // Create the struct type
        auto structType = new hir::HIRStructType(structName, fields);

        // Create struct construction instruction
        lastValue_ = builder_->createStructConstruct(structType, fieldValues, "obj");
    }
    
    void visit(FunctionExpr& node) override {
        (void)node;
        // Anonymous function expression
    }
    
    void visit(ArrowFunctionExpr& node) override {
        // Arrow function: (a, b) => a + b
        // For now, treat as anonymous function with auto-generated name

        // Helper to convert AST Type::Kind to HIR HIRType::Kind
        auto convertTypeKind = [](Type::Kind astKind) -> HIRType::Kind {
            switch (astKind) {
                case Type::Kind::Void: return HIRType::Kind::Void;
                case Type::Kind::Number: return HIRType::Kind::I64;
                case Type::Kind::String: return HIRType::Kind::String;
                case Type::Kind::Boolean: return HIRType::Kind::Bool;
                case Type::Kind::Any: return HIRType::Kind::Any;
                default: return HIRType::Kind::Any;
            }
        };

        // Create function type with parameter types
        std::vector<HIRTypePtr> paramTypes;
        for (size_t i = 0; i < node.params.size(); ++i) {
            HIRType::Kind typeKind = HIRType::Kind::Any;
            if (i < node.paramTypes.size() && node.paramTypes[i]) {
                typeKind = convertTypeKind(node.paramTypes[i]->kind);
            }
            paramTypes.push_back(std::make_shared<HIRType>(typeKind));
        }

        // Return type
        HIRType::Kind retTypeKind = HIRType::Kind::Any;
        if (node.returnType) {
            retTypeKind = convertTypeKind(node.returnType->kind);
        }
        auto retType = std::make_shared<HIRType>(retTypeKind);

        auto funcType = new HIRFunctionType(paramTypes, retType);

        // Generate unique name for arrow function
        static int arrowFuncCounter = 0;
        std::string funcName = "__arrow_" + std::to_string(arrowFuncCounter++);

        // Create function
        auto func = module_->createFunction(funcName, funcType);
        func->isAsync = node.isAsync;

        // Save current function context
        HIRFunction* savedFunction = currentFunction_;
        currentFunction_ = func.get();

        // Create entry block
        auto entryBlock = func->createBasicBlock("entry");

        // Save current builder
        auto savedBuilder = std::move(builder_);
        builder_ = std::make_unique<HIRBuilder>(module_, func.get());
        builder_->setInsertPoint(entryBlock.get());

        // Save current symbol table
        auto savedSymbolTable = symbolTable_;

        // Add parameters to symbol table
        for (size_t i = 0; i < node.params.size(); ++i) {
            symbolTable_[node.params[i]] = func->parameters[i];
        }

        // Generate function body
        if (node.body) {
            // Check if body is an expression statement (implicit return)
            if (auto* exprStmt = dynamic_cast<ExprStmt*>(node.body.get())) {
                // Arrow function with expression body: x => x + 1
                // This should return the expression value
                exprStmt->expression->accept(*this);
                builder_->createReturn(lastValue_);
            } else {
                // Arrow function with block body: x => { return x + 1; }
                node.body->accept(*this);

                // Add implicit return if needed
                if (!entryBlock->hasTerminator()) {
                    builder_->createReturn(nullptr);
                }
            }
        }

        // Restore context
        symbolTable_ = savedSymbolTable;
        builder_ = std::move(savedBuilder);
        currentFunction_ = savedFunction;

        // For now, arrow functions as values are not fully supported
        // We just create the function and leave lastValue_ as nullptr
        // In the future, this should return a function pointer
        lastValue_ = nullptr;

        std::cerr << "DEBUG HIRGen: Created arrow function '" << funcName << "' with "
                  << node.params.size() << " parameters" << std::endl;
    }
    
    void visit(ClassExpr& node) override {
        (void)node;
        // Class expression
    }
    
    void visit(NewExpr& node) override {
        std::cerr << "DEBUG HIRGen: Processing 'new' expression" << std::endl;

        // Get class name from callee (should be an Identifier)
        std::string className;
        if (auto* id = dynamic_cast<Identifier*>(node.callee.get())) {
            className = id->name;
            std::cerr << "  DEBUG: Class name: " << className << std::endl;
        } else {
            std::cerr << "  ERROR: 'new' expression with non-identifier callee" << std::endl;
            lastValue_ = builder_->createIntConstant(0);
            return;
        }

        // Constructor function name: ClassName_constructor
        std::string constructorName = className + "_constructor";

        // Evaluate arguments
        std::vector<HIRValue*> args;
        for (auto& arg : node.arguments) {
            arg->accept(*this);
            args.push_back(lastValue_);
        }

        // Call constructor function
        auto constructorFunc = module_->getFunction(constructorName);
        if (!constructorFunc) {
            std::cerr << "  ERROR: Constructor function not found: " << constructorName << std::endl;
            lastValue_ = builder_->createIntConstant(0);
            return;
        }

        lastValue_ = builder_->createCall(constructorFunc.get(), args, "new_instance");
        std::cerr << "  DEBUG: Created call to constructor: " << constructorName << std::endl;
    }
    
    void visit(ThisExpr& node) override {
        (void)node;
        std::cerr << "DEBUG HIRGen: Processing 'this' expression" << std::endl;
        if (currentThis_) {
            lastValue_ = currentThis_;
            std::cerr << "  DEBUG: Using current 'this' context" << std::endl;
        } else {
            std::cerr << "  ERROR: 'this' used outside of method context!" << std::endl;
            // Create placeholder to avoid crash
            lastValue_ = builder_->createIntConstant(0);
        }
    }
    
    void visit(SuperExpr& node) override {
        (void)node;
        // super reference
    }
    
    void visit(SpreadExpr& node) override {
        (void)node;
        // Spread operator
    }
    
    void visit(TemplateLiteralExpr& node) override {
        // Template literal: `Hello ${name}!` becomes "Hello " + name + "!"
        // quasis: ["Hello ", "!"]
        // expressions: [name]

        std::cerr << "DEBUG HIRGen: Processing template literal with " << node.quasis.size()
                  << " quasis and " << node.expressions.size() << " expressions" << std::endl;

        // If there are no expressions, this is just a simple string
        if (node.expressions.empty()) {
            if (!node.quasis.empty()) {
                lastValue_ = builder_->createStringConstant(node.quasis[0]);
            } else {
                lastValue_ = builder_->createStringConstant("");
            }
            return;
        }

        // Start with the first quasi (string before first ${})
        HIRValue* result = builder_->createStringConstant(node.quasis[0]);

        // For each expression, concatenate: result + expression + next_quasi
        for (size_t i = 0; i < node.expressions.size(); ++i) {
            // Evaluate the expression
            node.expressions[i]->accept(*this);
            HIRValue* exprValue = lastValue_;

            // TODO: Convert non-string values to strings
            // For now, assume all expressions are already strings or numbers

            // Concatenate result with the expression
            result = builder_->createAdd(result, exprValue);

            // Concatenate with the next quasi (string after this ${})
            if (i + 1 < node.quasis.size() && !node.quasis[i + 1].empty()) {
                HIRValue* nextQuasi = builder_->createStringConstant(node.quasis[i + 1]);
                result = builder_->createAdd(result, nextQuasi);
            }
        }

        lastValue_ = result;
    }
    
    void visit(AwaitExpr& node) override {
        // await expression
        node.argument->accept(*this);
    }
    
    void visit(YieldExpr& node) override {
        // yield expression
        if (node.argument) {
            node.argument->accept(*this);
        }
    }
    
    void visit(AsExpr& node) override {
        // Type assertion - just evaluate expression
        node.expression->accept(*this);
    }
    
    void visit(SatisfiesExpr& node) override {
        // Satisfies operator - just evaluate expression
        node.expression->accept(*this);
    }
    
    void visit(NonNullExpr& node) override {
        // Non-null assertion - just evaluate expression
        node.expression->accept(*this);
    }
    
    void visit(TaggedTemplateExpr& node) override {
        (void)node;
        // Tagged template
    }
    
    void visit(SequenceExpr& node) override {
        // Comma operator - evaluate all, return last
        for (auto& expr : node.expressions) {
            expr->accept(*this);
        }
    }
    
    void visit(AssignmentExpr& node) override {
        // Generate right side
        node.right->accept(*this);
        auto value = lastValue_;

        // Store to left side
        if (auto* id = dynamic_cast<Identifier*>(node.left.get())) {
            // Simple variable assignment
            auto it = symbolTable_.find(id->name);
            if (it != symbolTable_.end()) {
                builder_->createStore(value, it->second);
            }
        } else if (auto* memberExpr = dynamic_cast<MemberExpr*>(node.left.get())) {
            // Get the object/array
            memberExpr->object->accept(*this);
            auto object = lastValue_;

            if (memberExpr->isComputed) {
                // Array element assignment: arr[index] = value
                memberExpr->property->accept(*this);
                auto index = lastValue_;

                // Use SetElement to store value directly to the array element
                builder_->createSetElement(object, index, value);
            } else if (auto propExpr = dynamic_cast<Identifier*>(memberExpr->property.get())) {
                // Object property assignment: obj.x = value
                std::string propertyName = propExpr->name;

                // Find field index
                uint32_t fieldIndex = 0;
                bool found = false;

                if (object && object->type) {
                    if (auto ptrType = dynamic_cast<hir::HIRPointerType*>(object->type.get())) {
                        if (auto structType = dynamic_cast<hir::HIRStructType*>(ptrType->pointeeType.get())) {
                            for (size_t i = 0; i < structType->fields.size(); ++i) {
                                if (structType->fields[i].name == propertyName) {
                                    fieldIndex = static_cast<uint32_t>(i);
                                    found = true;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (found) {
                    // Use SetField to store value directly to the field
                    builder_->createSetField(object, fieldIndex, value, propertyName);
                } else {
                    std::cerr << "Warning: Property '" << propertyName << "' not found for assignment" << std::endl;
                }
            }
        }
    }
    
    void visit(ParenthesizedExpr& node) override {
        node.expression->accept(*this);
    }
    
    void visit(MetaProperty& node) override {
        (void)node;
        // new.target or import.meta
    }
    
    void visit(ImportExpr& node) override {
        (void)node;
        // import() expression
    }
    
    void visit(Decorator& node) override {
        (void)node;
        // Decorator - metadata only
    }
    
    // JSX/TSX Expressions
    void visit(JSXElement& node) override {
        (void)node;
        // JSX element - translate to runtime createElement call
        // For now, treat as opaque object creation
        // TODO: Implement JSX transformation to React.createElement or similar
        lastValue_ = builder_->createNullConstant(
            std::make_shared<HIRType>(HIRType::Kind::Any).get()
        );
    }
    
    void visit(JSXFragment& node) override {
        (void)node;
        // JSX fragment - translate to Fragment component
        lastValue_ = builder_->createNullConstant(
            std::make_shared<HIRType>(HIRType::Kind::Any).get()
        );
    }
    
    void visit(JSXText& node) override {
        // JSX text node - convert to string constant
        lastValue_ = builder_->createStringConstant(node.value);
    }
    
    void visit(JSXExpressionContainer& node) override {
        // JSX expression container - just evaluate inner expression
        node.expression->accept(*this);
    }
    
    void visit(JSXAttribute& node) override {
        // JSX attribute - not yet implemented
        (void)node;
    }
    
    void visit(JSXSpreadAttribute& node) override {
        // JSX spread attribute - not yet implemented
        (void)node;
    }
    
    // Patterns (for destructuring)
    void visit(ObjectPattern& node) override {
        (void)node;
        // Object destructuring pattern
        // This is used in variable declarations and function parameters
        // TODO: Implement proper destructuring logic
    }
    
    void visit(ArrayPattern& node) override {
        (void)node;
        // Array destructuring pattern
        // TODO: Implement proper destructuring logic
    }
    
    void visit(AssignmentPattern& node) override {
        (void)node;
        // Pattern with default value
        // TODO: Implement default value assignment
    }
    
    void visit(RestElement& node) override {
        (void)node;
        // Rest element in destructuring (...rest)
        // TODO: Implement rest element collection
    }
    
    void visit(IdentifierPattern& node) override {
        // Simple identifier pattern - look up in symbol table
        auto it = symbolTable_.find(node.name);
        if (it != symbolTable_.end()) {
            lastValue_ = it->second;
        }
    }
    
    // Statements
    void visit(BlockStmt& node) override {
        for (auto& stmt : node.statements) {
            stmt->accept(*this);
        }
    }
    
    void visit(ExprStmt& node) override {
        node.expression->accept(*this);
    }
    
    void visit(VarDeclStmt& node) override {
        for (auto& decl : node.declarations) {
            // Evaluate the initializer first to get its type
            HIRValue* initValue = nullptr;
            if (decl.init) {
                decl.init->accept(*this);
                initValue = lastValue_;
            }

            // Use the initializer's type for the alloca, or default to i64
            HIRType* allocaType = nullptr;
            if (initValue && initValue->type) {
                allocaType = initValue->type.get();
            } else {
                auto i64Type = std::make_shared<HIRType>(HIRType::Kind::I64);
                allocaType = i64Type.get();
            }

            // Allocate storage with the correct type
            auto alloca = builder_->createAlloca(allocaType, decl.name);
            symbolTable_[decl.name] = alloca;

            // Store the initializer value if present
            if (initValue) {
                builder_->createStore(initValue, alloca);
            }
        }
    }
    
    void visit(DeclStmt& node) override {
        // Process the declaration within this statement
        if (node.declaration) {
            node.declaration->accept(*this);
        }
    }
    
    void visit(IfStmt& node) override {
        // Generate condition
        node.test->accept(*this);
        auto cond = lastValue_;
        
        // Create blocks
        auto* thenBlock = currentFunction_->createBasicBlock("if.then").get();
        auto* elseBlock = node.alternate ? 
            currentFunction_->createBasicBlock("if.else").get() : nullptr;
        auto* endBlock = currentFunction_->createBasicBlock("if.end").get();
        
        // Branch on condition
        if (elseBlock) {
            builder_->createCondBr(cond, thenBlock, elseBlock);
        } else {
            builder_->createCondBr(cond, thenBlock, endBlock);
        }
        
        // Generate then block
        builder_->setInsertPoint(thenBlock);
        node.consequent->accept(*this);
        
        // Only add branch to end block if the then block doesn't end with a return, break, or continue
        if (thenBlock->instructions.empty() || 
            (thenBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Return &&
             thenBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Break &&
             thenBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Continue)) {
            builder_->createBr(endBlock);
        }
        
        // Generate else block
        if (elseBlock) {
            builder_->setInsertPoint(elseBlock);
            node.alternate->accept(*this);
            
            // Only add branch to end block if the else block doesn't end with a return, break, or continue
            if (elseBlock->instructions.empty() || 
                (elseBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Return &&
                 elseBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Break &&
                 elseBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Continue)) {
                builder_->createBr(endBlock);
            }
        }
        
        // Continue at end block
        builder_->setInsertPoint(endBlock);
        
        // If end block is empty (both branches had returns), add unreachable
        if (builder_->getInsertBlock()->instructions.empty()) {
            // Create a dummy return instruction
            auto dummyConst = builder_->createIntConstant(0);
            builder_->createReturn(dummyConst);
        }
    }
    
    void visit(WhileStmt& node) override {
        std::cerr << "DEBUG: Entering WhileStmt generation" << std::endl;
        
        auto* condBlock = currentFunction_->createBasicBlock("while.cond").get();
        auto* bodyBlock = currentFunction_->createBasicBlock("while.body").get();
        auto* endBlock = currentFunction_->createBasicBlock("while.end").get();
        
        std::cerr << "DEBUG: Created while loop blocks: cond=" << condBlock << ", body=" << bodyBlock << ", end=" << endBlock << std::endl;
        
        // Jump to condition
        builder_->createBr(condBlock);
        
        // Condition block
        builder_->setInsertPoint(condBlock);
        std::cerr << "DEBUG: Evaluating while condition" << std::endl;
        node.test->accept(*this);
        std::cerr << "DEBUG: While condition evaluated, lastValue_=" << lastValue_ << std::endl;
        builder_->createCondBr(lastValue_, bodyBlock, endBlock);
        
        // Body block
        builder_->setInsertPoint(bodyBlock);
        std::cerr << "DEBUG: Executing while body" << std::endl;
        node.body->accept(*this);
        std::cerr << "DEBUG: While body executed" << std::endl;
        
        // Check if the body or any of its successors contain any break or continue instructions
        bool hasBreakOrContinue = bodyBlock->hasBreakOrContinue;
        
        // Check all successors of the body block
        std::function<void(hir::HIRBasicBlock*, bool&)> checkSuccessors = [&](hir::HIRBasicBlock* block, bool& found) {
            if (found) return;
            if (block->hasBreakOrContinue) {
                found = true;
                return;
            }
            for (const auto& succ : block->successors) {
                checkSuccessors(succ.get(), found);
            }
        };
        
        checkSuccessors(bodyBlock, hasBreakOrContinue);
        
        // Only add branch back to condition if the body doesn't contain break/continue and doesn't end with a return
        if (!hasBreakOrContinue && (bodyBlock->instructions.empty() || 
            bodyBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Return)) {
            std::cerr << "DEBUG: Creating branch back to condition" << std::endl;
            builder_->createBr(condBlock);
        } else {
            std::cerr << "DEBUG: Not creating branch back to condition because body or its successors contain break/continue or body ends with return" << std::endl;
        }
        
        // End block
        builder_->setInsertPoint(endBlock);
        std::cerr << "DEBUG: While loop generation completed" << std::endl;
    }
    
    void visit(DoWhileStmt& node) override {
        // Create basic blocks for the do-while loop
        auto* bodyBlock = currentFunction_->createBasicBlock("do-while.body").get();
        auto* condBlock = currentFunction_->createBasicBlock("do-while.cond").get();
        auto* endBlock = currentFunction_->createBasicBlock("do-while.end").get();
        
        // Jump to body block (do-while always executes at least once)
        builder_->createBr(bodyBlock);
        
        // Body block
        builder_->setInsertPoint(bodyBlock);
        node.body->accept(*this);
        
        // Check if the body or any of its successors contain any break or continue instructions
        bool hasBreakOrContinue = bodyBlock->hasBreakOrContinue;
        
        // Check all successors of the body block
        std::function<void(hir::HIRBasicBlock*, bool&)> checkSuccessors = [&](hir::HIRBasicBlock* block, bool& found) {
            if (found) return;
            if (block->hasBreakOrContinue) {
                found = true;
                return;
            }
            for (const auto& succ : block->successors) {
                checkSuccessors(succ.get(), found);
            }
        };
        
        checkSuccessors(bodyBlock, hasBreakOrContinue);
        
        // Only add branch to condition if the body doesn't contain break/continue and doesn't end with a return
        if (!hasBreakOrContinue && (bodyBlock->instructions.empty() || 
            bodyBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Return)) {
            // Branch to condition after body
            builder_->createBr(condBlock);
        }
        
        // Condition block
        builder_->setInsertPoint(condBlock);
        node.test->accept(*this);
        auto* condition = lastValue_;
        // If condition is true, branch back to body, otherwise go to end
        builder_->createCondBr(condition, bodyBlock, endBlock);
        
        // End block
        builder_->setInsertPoint(endBlock);
    }
    
    void visit(ForStmt& node) override {
        std::cerr << "DEBUG: Entering ForStmt generation" << std::endl;
        
        // Create basic blocks for the for loop
        auto* initBlock = currentFunction_->createBasicBlock("for.init").get();
        auto* condBlock = currentFunction_->createBasicBlock("for.cond").get();
        auto* bodyBlock = currentFunction_->createBasicBlock("for.body").get();
        auto* updateBlock = currentFunction_->createBasicBlock("for.update").get();
        auto* endBlock = currentFunction_->createBasicBlock("for.end").get();
        
        std::cerr << "DEBUG: Created for loop blocks: init=" << initBlock << ", cond=" << condBlock 
                  << ", body=" << bodyBlock << ", update=" << updateBlock << ", end=" << endBlock << std::endl;
        
        // Branch to init block
        builder_->createBr(initBlock);
        
        // Init block - execute initializer
        builder_->setInsertPoint(initBlock);
        std::cerr << "DEBUG: Executing for init" << std::endl;
        if (node.init) {
            if (auto* varDeclStmt = dynamic_cast<VarDeclStmt*>(node.init.get())) {
                varDeclStmt->accept(*this);
            } else if (auto* exprStmt = dynamic_cast<ExprStmt*>(node.init.get())) {
                exprStmt->accept(*this);
            } else {
                // For expression initializers, wrap in an expression statement
                node.init->accept(*this);
            }
        }
        std::cerr << "DEBUG: For init executed" << std::endl;
        // Branch to condition
        builder_->createBr(condBlock);
        
        // Condition block
        builder_->setInsertPoint(condBlock);
        std::cerr << "DEBUG: Evaluating for condition" << std::endl;
        if (node.test) {
            node.test->accept(*this);
            auto* condition = lastValue_;
            std::cerr << "DEBUG: For condition evaluated, condition=" << condition << std::endl;
            builder_->createCondBr(condition, bodyBlock, endBlock);
        } else {
            // No condition means infinite loop
            std::cerr << "DEBUG: No for condition, creating infinite loop" << std::endl;
            builder_->createBr(bodyBlock);
        }
        
        // Body block
        builder_->setInsertPoint(bodyBlock);
        std::cerr << "DEBUG: Executing for body" << std::endl;
        node.body->accept(*this);
        std::cerr << "DEBUG: For body executed" << std::endl;
        
        // Check if the body or any of its successors contain any break or continue instructions
        bool hasBreakOrContinue = bodyBlock->hasBreakOrContinue;
        
        // Check all successors of the body block
        std::function<void(hir::HIRBasicBlock*, bool&)> checkSuccessors = [&](hir::HIRBasicBlock* block, bool& found) {
            if (found) return;
            if (block->hasBreakOrContinue) {
                found = true;
                return;
            }
            for (const auto& succ : block->successors) {
                checkSuccessors(succ.get(), found);
            }
        };
        
        checkSuccessors(bodyBlock, hasBreakOrContinue);
        
        // Only add branch to update block if the body doesn't contain break/continue and doesn't end with a return
        if (!hasBreakOrContinue && (bodyBlock->instructions.empty() || 
            bodyBlock->instructions.back()->opcode != hir::HIRInstruction::Opcode::Return)) {
            // Branch to update block
            builder_->createBr(updateBlock);
        }
        
        // Update block
        builder_->setInsertPoint(updateBlock);
        std::cerr << "DEBUG: Executing for update" << std::endl;
        if (node.update) {
            node.update->accept(*this);
            // Result of update expression is ignored
        }
        std::cerr << "DEBUG: For update executed" << std::endl;
        // Branch back to condition
        builder_->createBr(condBlock);
        
        // End block
        builder_->setInsertPoint(endBlock);
        std::cerr << "DEBUG: For loop generation completed" << std::endl;
    }
    
    void visit(ForInStmt& node) override {
        (void)node;
        // for-in loop
    }
    
    void visit(ForOfStmt& node) override {
        (void)node;
        // for-of loop
    }
    
    void visit(ReturnStmt& node) override {
        if (node.argument) {
            node.argument->accept(*this);
            builder_->createReturn(lastValue_);
        } else {
            builder_->createReturn(nullptr);
        }
    }
    
    void visit(BreakStmt& node) override {
        (void)node;
        // Create break instruction
        auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
        auto breakInst = std::make_unique<HIRInstruction>(
            HIRInstruction::Opcode::Break,
            voidType,
            ""
        );
        auto* currentBlock = builder_->getInsertBlock();
        currentBlock->addInstruction(std::move(breakInst));
        currentBlock->hasBreakOrContinue = true;
    }
    
    void visit(ContinueStmt& node) override {
        (void)node;
        // Create continue instruction
        auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
        auto continueInst = std::make_unique<HIRInstruction>(
            HIRInstruction::Opcode::Continue,
            voidType,
            ""
        );
        auto* currentBlock = builder_->getInsertBlock();
        currentBlock->addInstruction(std::move(continueInst));
        currentBlock->hasBreakOrContinue = true;
    }
    
    void visit(ThrowStmt& node) override {
        // throw statement
        node.argument->accept(*this);
    }
    
    void visit(TryStmt& node) override {
        (void)node;
        // try-catch-finally
    }
    
    void visit(SwitchStmt& node) override {
        (void)node;
        // switch statement
    }
    
    void visit(LabeledStmt& node) override {
        // labeled statement
        node.statement->accept(*this);
    }
    
    void visit(WithStmt& node) override {
        (void)node;
        // with statement
    }
    
    void visit(DebuggerStmt& node) override {
        (void)node;
        // debugger statement - no-op in HIR
    }
    
    void visit(EmptyStmt& node) override {
        (void)node;
        // empty statement - no-op
    }
    
    // Declarations
    void visit(FunctionDecl& node) override {
        // Helper to convert AST Type::Kind to HIR HIRType::Kind
        auto convertTypeKind = [](Type::Kind astKind) -> HIRType::Kind {
            switch (astKind) {
                case Type::Kind::Void: return HIRType::Kind::Void;
                case Type::Kind::Number: return HIRType::Kind::I64;  // Default to i64 for numbers
                case Type::Kind::String: return HIRType::Kind::String;
                case Type::Kind::Boolean: return HIRType::Kind::Bool;
                case Type::Kind::Any: return HIRType::Kind::Any;
                case Type::Kind::Unknown: return HIRType::Kind::Unknown;
                case Type::Kind::Never: return HIRType::Kind::Never;
                case Type::Kind::Null: return HIRType::Kind::Any;  // Map to Any for now
                case Type::Kind::Undefined: return HIRType::Kind::Any;  // Map to Any for now
                default: return HIRType::Kind::Any;
            }
        };

        // Create function type with actual parameter types
        std::vector<HIRTypePtr> paramTypes;
        for (size_t i = 0; i < node.params.size(); ++i) {
            HIRType::Kind typeKind = HIRType::Kind::Any;  // Default to Any

            // Use type annotation if available
            if (i < node.paramTypes.size() && node.paramTypes[i]) {
                typeKind = convertTypeKind(node.paramTypes[i]->kind);
            }

            paramTypes.push_back(std::make_shared<HIRType>(typeKind));
        }

        // Use return type annotation if available
        HIRType::Kind retTypeKind = HIRType::Kind::Any;  // Default to Any
        if (node.returnType) {
            retTypeKind = convertTypeKind(node.returnType->kind);
        }
        auto retType = std::make_shared<HIRType>(retTypeKind);
        
        auto funcType = new HIRFunctionType(paramTypes, retType);
        
        // Create function
        auto func = module_->createFunction(node.name, funcType);
        func->isAsync = node.isAsync;
        func->isGenerator = node.isGenerator;
        
        currentFunction_ = func.get();
        
        // Create entry block
        auto entryBlock = func->createBasicBlock("entry");
        
        // Create builder
        builder_ = std::make_unique<HIRBuilder>(module_, func.get());
        builder_->setInsertPoint(entryBlock.get());
        
        // Add parameters to symbol table
        for (size_t i = 0; i < node.params.size(); ++i) {
            symbolTable_[node.params[i]] = func->parameters[i];
        }
        
        // Generate function body
        if (node.body) {
            node.body->accept(*this);
        }
        
        // Add implicit return if needed
        if (!entryBlock->hasTerminator()) {
            builder_->createReturn(nullptr);
        }
        
        currentFunction_ = nullptr;
    }
    
    void visit(ClassDecl& node) override {
        std::cerr << "DEBUG HIRGen: Processing class declaration: " << node.name << std::endl;

        // Helper to convert AST Type::Kind to HIR HIRType::Kind (same as in FunctionDecl)
        auto convertTypeKind = [](Type::Kind astKind) -> HIRType::Kind {
            switch (astKind) {
                case Type::Kind::Void: return HIRType::Kind::Void;
                case Type::Kind::Number: return HIRType::Kind::I64;
                case Type::Kind::String: return HIRType::Kind::String;
                case Type::Kind::Boolean: return HIRType::Kind::Bool;
                case Type::Kind::Any: return HIRType::Kind::Any;
                case Type::Kind::Unknown: return HIRType::Kind::Unknown;
                case Type::Kind::Never: return HIRType::Kind::Never;
                case Type::Kind::Null: return HIRType::Kind::Any;
                case Type::Kind::Undefined: return HIRType::Kind::Any;
                default: return HIRType::Kind::Any;
            }
        };

        // 1. Create struct type for class data
        std::vector<hir::HIRStructType::Field> fields;
        for (const auto& prop : node.properties) {
            // Convert property type to HIR type
            hir::HIRType::Kind typeKind = hir::HIRType::Kind::I64;  // Default to i64
            if (prop.type) {
                typeKind = convertTypeKind(prop.type->kind);
            }
            auto fieldType = std::make_shared<hir::HIRType>(typeKind);
            fields.push_back({prop.name, fieldType, true});
            std::cerr << "  DEBUG: Added field: " << prop.name << std::endl;
        }

        auto structType = module_->createStructType(node.name);
        structType->fields = fields;
        std::cerr << "  DEBUG: Created struct type with " << fields.size() << " fields" << std::endl;

        // 2. Find constructor and generate constructor function
        ClassDecl::Method* constructor = nullptr;
        for (auto& method : node.methods) {
            if (method.kind == ClassDecl::Method::Kind::Constructor) {
                constructor = &method;
                break;
            }
        }

        if (constructor) {
            std::cerr << "  DEBUG: Generating constructor function" << std::endl;
            generateConstructorFunction(node.name, *constructor, structType, convertTypeKind);
        }

        // 3. Generate method functions
        for (const auto& method : node.methods) {
            if (method.kind == ClassDecl::Method::Kind::Method) {
                std::cerr << "  DEBUG: Generating method: " << method.name << std::endl;
                generateMethodFunction(node.name, method, structType, convertTypeKind);
            }
        }

        std::cerr << "DEBUG HIRGen: Completed class declaration: " << node.name << std::endl;
    }

    void generateConstructorFunction(const std::string& className,
                                     const ClassDecl::Method& constructor,
                                     hir::HIRStructType* structType,
                                     std::function<HIRType::Kind(Type::Kind)> convertTypeKind) {
        // Constructor function name: ClassName_constructor
        std::string funcName = className + "_constructor";

        // Parameter types (from constructor parameters)
        std::vector<hir::HIRTypePtr> paramTypes;
        for (size_t i = 0; i < constructor.params.size(); ++i) {
            paramTypes.push_back(std::make_shared<hir::HIRType>(hir::HIRType::Kind::I64));
        }

        // Return type is pointer to struct (use Any for now)
        auto returnType = std::make_shared<hir::HIRType>(hir::HIRType::Kind::Any);

        // Create function type
        auto funcType = new HIRFunctionType(paramTypes, returnType);

        // Create HIR function
        auto func = module_->createFunction(funcName, funcType);

        // Save current function
        HIRFunction* savedFunction = currentFunction_;
        currentFunction_ = func.get();

        // Create entry block
        auto entryBlock = func->createBasicBlock("entry");

        // Create builder
        builder_ = std::make_unique<HIRBuilder>(module_, func.get());
        builder_->setInsertPoint(entryBlock.get());

        // Add parameters to symbol table
        for (size_t i = 0; i < constructor.params.size(); ++i) {
            symbolTable_[constructor.params[i]] = func->parameters[i];
        }

        // Allocate memory for class instance (this will be handled by runtime)
        // For now, just create a placeholder integer to represent the allocated instance
        // TODO: Implement malloc call for class instance
        auto instancePtr = builder_->createIntConstant(0);  // Placeholder

        // Save current 'this' context (for nested classes)
        hir::HIRValue* savedThis = currentThis_;
        // Set 'this' to the allocated instance
        currentThis_ = instancePtr;

        // Process constructor body
        if (constructor.body) {
            constructor.body->accept(*this);
        }

        // Add implicit return of instance if needed
        if (!entryBlock->hasTerminator()) {
            builder_->createReturn(instancePtr);
        }

        // Restore 'this' context
        currentThis_ = savedThis;
        currentFunction_ = savedFunction;

        std::cerr << "    DEBUG: Created constructor function: " << funcName << std::endl;
    }

    void generateMethodFunction(const std::string& className,
                                const ClassDecl::Method& method,
                                hir::HIRStructType* structType,
                                std::function<HIRType::Kind(Type::Kind)> convertTypeKind) {
        // Method function name: ClassName_methodName
        std::string funcName = className + "_" + method.name;

        // Parameter types: 'this' pointer + method parameters
        std::vector<hir::HIRTypePtr> paramTypes;
        // First parameter is 'this' (pointer to struct)
        paramTypes.push_back(std::make_shared<hir::HIRType>(hir::HIRType::Kind::Any));

        // Add method parameters
        for (size_t i = 0; i < method.params.size(); ++i) {
            paramTypes.push_back(std::make_shared<hir::HIRType>(hir::HIRType::Kind::I64));
        }

        // Return type
        hir::HIRTypePtr returnType;
        if (method.returnType) {
            returnType = std::make_shared<hir::HIRType>(convertTypeKind(method.returnType->kind));
        } else {
            returnType = std::make_shared<hir::HIRType>(hir::HIRType::Kind::I64);
        }

        // Create function type
        auto funcType = new HIRFunctionType(paramTypes, returnType);

        // Create HIR function
        auto func = module_->createFunction(funcName, funcType);

        // Save current function
        HIRFunction* savedFunction = currentFunction_;
        currentFunction_ = func.get();

        // Create entry block
        auto entryBlock = func->createBasicBlock("entry");

        // Create builder
        builder_ = std::make_unique<HIRBuilder>(module_, func.get());
        builder_->setInsertPoint(entryBlock.get());

        // First parameter is 'this'
        symbolTable_["this"] = func->parameters[0];

        // Add method parameters to symbol table (starting from index 1)
        for (size_t i = 0; i < method.params.size(); ++i) {
            symbolTable_[method.params[i]] = func->parameters[i + 1];
        }

        // Save current 'this' context
        hir::HIRValue* savedThis = currentThis_;
        // Set 'this' to the first parameter
        currentThis_ = func->parameters[0];

        // Process method body
        if (method.body) {
            method.body->accept(*this);
        }

        // Add implicit return if needed
        if (!entryBlock->hasTerminator()) {
            builder_->createReturn(nullptr);
        }

        // Restore 'this' context
        currentThis_ = savedThis;
        currentFunction_ = savedFunction;

        std::cerr << "    DEBUG: Created method function: " << funcName << std::endl;
    }
    
    void visit(InterfaceDecl& node) override {
        (void)node;
        // Interface - type information only
    }
    
    void visit(TypeAliasDecl& node) override {
        (void)node;
        // Type alias - type information only
    }
    
    void visit(EnumDecl& node) override {
        (void)node;
        // Enum declaration
    }
    
    void visit(ImportDecl& node) override {
        (void)node;
        // Import declaration - module system
    }
    
    void visit(ExportDecl& node) override {
        (void)node;
        // Export declaration
    }
    
    void visit(Program& node) override {
        for (auto& stmt : node.body) {
            stmt->accept(*this);
        }
    }
    
private:
    HIRModule* module_;
    std::unique_ptr<HIRBuilder> builder_;
    HIRFunction* currentFunction_;
    HIRValue* currentThis_ = nullptr;  // Current 'this' context for methods
    HIRValue* lastValue_ = nullptr;
    std::unordered_map<std::string, HIRValue*> symbolTable_;
};

// Public API to generate HIR from AST
HIRModule* generateHIR(Program& program, const std::string& moduleName) {
    auto* module = new HIRModule(moduleName);
    HIRGenerator generator(module);
    program.accept(generator);
    return module;
}

} // namespace nova::hir
