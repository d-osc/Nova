// HIRGen_Classes.cpp - Class expression and declaration visitors
// Extracted from HIRGen.cpp for better code organization

#include "nova/HIR/HIRGen_Internal.h"
#define NOVA_DEBUG 1

namespace nova::hir {

void HIRGenerator::visit(ClassExpr& node) {
        // Class expression: let C = class { value: number; constructor(v) { this.value = v; } }

        // Generate unique class name if not provided
        static int classExprCounter = 0;
        std::string className = node.name.empty() ?
            "__class_" + std::to_string(classExprCounter++) : node.name;

        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing class expression: " << className << std::endl;

        // Register class name for static method call detection
        classNames_.insert(className);

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

        // 1. Create struct type for class data (instance properties)
        std::vector<hir::HIRStructType::Field> fields;
        std::set<std::string> fieldNames;  // Track field names to avoid duplicates

        // Add explicitly declared properties
        for (const auto& prop : node.properties) {
            if (!prop.isStatic) {
                hir::HIRType::Kind typeKind = hir::HIRType::Kind::I64;
                if (prop.type) {
                    typeKind = convertTypeKind(prop.type->kind);
                }
                auto fieldType = std::make_shared<hir::HIRType>(typeKind);
                fields.push_back({prop.name, fieldType, true});
                fieldNames.insert(prop.name);
            }
        }

        // Also scan constructor for this.property assignments to auto-add fields
        for (auto& method : node.methods) {
            if (method.kind == ClassExpr::Method::Kind::Constructor && method.body) {
                BlockStmt* bodyBlock = dynamic_cast<BlockStmt*>(method.body.get());
                if (bodyBlock) {
                    for (auto& stmt : bodyBlock->statements) {
                        ExprStmt* exprStmt = dynamic_cast<ExprStmt*>(stmt.get());
                        if (exprStmt) {
                            AssignmentExpr* assignExpr = dynamic_cast<AssignmentExpr*>(exprStmt->expression.get());
                            if (assignExpr) {
                                MemberExpr* memberExpr = dynamic_cast<MemberExpr*>(assignExpr->left.get());
                                if (memberExpr) {
                                    ThisExpr* thisExpr = dynamic_cast<ThisExpr*>(memberExpr->object.get());
                                    if (thisExpr) {
                                        Identifier* propIdent = dynamic_cast<Identifier*>(memberExpr->property.get());
                                        if (propIdent) {
                                            std::string propName = propIdent->name;
                                            if (fieldNames.find(propName) == fieldNames.end()) {
                                                // Infer field type from RHS of assignment
                                                hir::HIRType::Kind typeKind = hir::HIRType::Kind::I64; // default

                                                // Check what's being assigned
                                                if (auto* stringLit = dynamic_cast<StringLiteral*>(assignExpr->right.get())) {
                                                    typeKind = hir::HIRType::Kind::String;
                                                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Field '" << propName << "' inferred as String from string literal" << std::endl;
                                                } else if (auto* numLit = dynamic_cast<NumberLiteral*>(assignExpr->right.get())) {
                                                    typeKind = hir::HIRType::Kind::I64;
                                                } else if (auto* ident = dynamic_cast<Identifier*>(assignExpr->right.get())) {
                                                    // Constructor parameter - use Any for dynamic typing (supports all types)
                                                    typeKind = hir::HIRType::Kind::Any;
                                                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Field '" << propName << "' inferred as Any from parameter '" << ident->name << "'" << std::endl;
                                                }

                                                auto fieldType = std::make_shared<hir::HIRType>(typeKind);
                                                fields.push_back({propName, fieldType, true});
                                                fieldNames.insert(propName);
                                                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Auto-added field '" << propName << "' from constructor" << std::endl;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        auto structType = module_->createStructType(className);
        structType->fields = fields;

        // 2. Find constructor and generate constructor function
        ClassExpr::Method* constructor = nullptr;
        for (auto& method : node.methods) {
            if (method.kind == ClassExpr::Method::Kind::Constructor) {
                constructor = &method;
                break;
            }
        }

        if (constructor) {
            // Generate constructor function similar to ClassDecl
            std::string funcName = className + "_constructor";

            std::vector<hir::HIRTypePtr> paramTypes;
            for (size_t i = 0; i < constructor->params.size(); ++i) {
                paramTypes.push_back(std::make_shared<hir::HIRType>(hir::HIRType::Kind::Any));
            }

            auto returnType = std::make_shared<hir::HIRType>(hir::HIRType::Kind::Any);
            auto funcType = new HIRFunctionType(paramTypes, returnType);
            auto func = module_->createFunction(funcName, funcType);

            // Save context
            HIRFunction* savedFunction = currentFunction_;
            hir::HIRStructType* savedClassStructType = currentClassStructType_;
            currentFunction_ = func.get();
            currentClassStructType_ = structType;

            auto entryBlock = func->createBasicBlock("entry");

            auto savedBuilder = std::move(builder_);
            builder_ = std::make_unique<HIRBuilder>(module_, func.get());
            builder_->setInsertPoint(entryBlock.get());

            // Add parameters to symbol table
            auto savedSymbolTable = symbolTable_;
            symbolTable_.clear();
            for (size_t i = 0; i < constructor->params.size(); ++i) {
                symbolTable_[constructor->params[i]] = func->parameters[i];
            }

            // Allocate memory for class instance
            size_t instanceSize = fields.size() * 8;
            auto sizeValue = builder_->createIntConstant(instanceSize);

            // Get or create malloc
            HIRFunction* mallocFunc = nullptr;
            for (auto& f : module_->functions) {
                if (f->name == "malloc") {
                    mallocFunc = f.get();
                    break;
                }
            }
            if (!mallocFunc) {
                std::vector<hir::HIRTypePtr> mallocParams;
                mallocParams.push_back(std::make_shared<hir::HIRType>(hir::HIRType::Kind::I64));
                auto mallocRetType = std::make_shared<hir::HIRType>(hir::HIRType::Kind::Any);
                auto mallocType = new HIRFunctionType(mallocParams, mallocRetType);
                auto mallocFuncPtr = module_->createFunction("malloc", mallocType);
                mallocFuncPtr->linkage = HIRFunction::Linkage::External;
                mallocFunc = mallocFuncPtr.get();
            }

            std::vector<HIRValue*> mallocArgs = {sizeValue};
            auto instancePtr = builder_->createCall(mallocFunc, mallocArgs, "instance");
            instancePtr->type = std::make_shared<hir::HIRPointerType>(
                std::shared_ptr<hir::HIRStructType>(structType, [](hir::HIRStructType*){}), true);

            symbolTable_["this"] = instancePtr;

            // Set currentThis_ for ThisExpr visitor
            HIRValue* savedThis = currentThis_;
            currentThis_ = instancePtr;

            // Generate constructor body
            if (constructor->body) {
                constructor->body->accept(*this);
            }

            builder_->createReturn(instancePtr);

            // Restore currentThis_
            currentThis_ = savedThis;

            // Restore context
            symbolTable_ = savedSymbolTable;
            builder_ = std::move(savedBuilder);
            currentFunction_ = savedFunction;
            currentClassStructType_ = savedClassStructType;
        } else {
            // Generate default constructor
            generateDefaultConstructor(className, structType);
        }

        // 3. Generate method functions
        for (const auto& method : node.methods) {
            if (method.kind == ClassExpr::Method::Kind::Method) {
                // Generate method function
                std::string methodFuncName = className + "_" + method.name;

                std::vector<hir::HIRTypePtr> paramTypes;
                // Create proper pointer-to-struct type for 'this' parameter
                paramTypes.push_back(std::make_shared<hir::HIRPointerType>(
                    std::shared_ptr<hir::HIRStructType>(structType, [](hir::HIRStructType*){}), true));
                for (size_t i = 0; i < method.params.size(); ++i) {
                    paramTypes.push_back(std::make_shared<hir::HIRType>(hir::HIRType::Kind::Any));
                }

                hir::HIRType::Kind retTypeKind = hir::HIRType::Kind::Any;
                if (method.returnType) {
                    retTypeKind = convertTypeKind(method.returnType->kind);
                }
                auto returnType = std::make_shared<hir::HIRType>(retTypeKind);

                auto funcType = new HIRFunctionType(paramTypes, returnType);
                auto func = module_->createFunction(methodFuncName, funcType);

                // DEBUG: Verify parameter types after function creation
                std::cerr << "DEBUG METHOD: Created function " << methodFuncName << " with " << func->parameters.size() << " parameters" << std::endl;
                if (!func->parameters.empty() && func->parameters[0]->type) {
                    std::cerr << "  parameter[0] type->kind = " << static_cast<int>(func->parameters[0]->type->kind) << std::endl;
                } else {
                    std::cerr << "  ERROR: parameter[0] type is NULL!" << std::endl;
                }

                HIRFunction* savedFunction = currentFunction_;
                hir::HIRStructType* savedClassStructType = currentClassStructType_;
                currentFunction_ = func.get();
                currentClassStructType_ = structType;

                auto entryBlock = func->createBasicBlock("entry");

                auto savedBuilder = std::move(builder_);
                builder_ = std::make_unique<HIRBuilder>(module_, func.get());
                builder_->setInsertPoint(entryBlock.get());

                auto savedSymbolTable = symbolTable_;
                symbolTable_.clear();

                symbolTable_["this"] = func->parameters[0];
                for (size_t i = 0; i < method.params.size(); ++i) {
                    symbolTable_[method.params[i]] = func->parameters[i + 1];
                }

                // Set currentThis_ for ThisExpr visitor
                HIRValue* savedThis = currentThis_;
                currentThis_ = func->parameters[0];

                // DEBUG: Verify parameter type
                std::cerr << "DEBUG METHOD: After setting currentThis_ from parameter[0]" << std::endl;
                std::cerr << "  currentThis_ pointer: " << currentThis_ << std::endl;
                if (currentThis_ && currentThis_->type) {
                    std::cerr << "  currentThis_->type->kind = " << static_cast<int>(currentThis_->type->kind) << std::endl;
                } else {
                    std::cerr << "  ERROR: currentThis_->type is NULL!" << std::endl;
                }

                if (method.body) {
                    method.body->accept(*this);
                }

                // Infer return type for method if not explicitly annotated
                if (func->functionType && func->functionType->returnType &&
                    func->functionType->returnType->kind == hir::HIRType::Kind::Any) {
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Method " << method.name << " has Any return type, inferring..." << std::endl;
                    // Scan blocks for return statements to infer type from actual return values
                    for (auto& block : func->basicBlocks) {
                        if(NOVA_DEBUG) std::cerr << "  Scanning block with " << block->instructions.size() << " instructions" << std::endl;
                        for (auto& inst : block->instructions) {
                            if (inst->opcode == HIRInstruction::Opcode::Return && !inst->operands.empty()) {
                                if(NOVA_DEBUG) std::cerr << "  Found Return instruction with operands" << std::endl;
                                auto retVal = inst->operands[0].get();
                                if (retVal && retVal->type && retVal->type->kind != HIRType::Kind::Void) {
                                    func->functionType->returnType = retVal->type;
                                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Inferred return type for method " << method.name
                                                              << " from return statement: kind " << static_cast<int>(retVal->type->kind) << std::endl;
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Method " << method.name << " return type already set or not Any" << std::endl;
                }

                // Restore currentThis_
                currentThis_ = savedThis;

                // Add implicit return if the current block doesn't have a terminator
                auto* currentBlock = builder_->getInsertBlock();
                if (currentBlock && !currentBlock->hasTerminator()) {
                    builder_->createReturn(nullptr);
                }

                symbolTable_ = savedSymbolTable;
                builder_ = std::move(savedBuilder);
                currentFunction_ = savedFunction;
                currentClassStructType_ = savedClassStructType;

                // Track method for inheritance resolution
                classOwnMethods_[className].insert(method.name);
            }
        }

        // Store class name for variable assignment tracking
        lastClassName_ = className;

        // Return placeholder value (class is registered by name)
        lastValue_ = builder_->createIntConstant(0);

        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Completed class expression: " << className << std::endl;
    }
    

void HIRGenerator::visit(NewExpr& node) {
        std::cerr << "=== DEBUG HIRGen: Processing 'new' expression ===" << std::endl;

        // Get class name from callee (should be an Identifier or MemberExpr for Intl.*)
        std::string className;
        std::string objectName;  // For MemberExpr like Intl.NumberFormat
        if (auto* id = dynamic_cast<Identifier*>(node.callee.get())) {
            className = id->name;
            std::cerr << "  DEBUG NEW: Class name: " << className << std::endl;
        } else if (auto* memberExpr = dynamic_cast<MemberExpr*>(node.callee.get())) {
            // Handle Intl.NumberFormat, Intl.DateTimeFormat, etc.
            if (auto* objId = dynamic_cast<Identifier*>(memberExpr->object.get())) {
                objectName = objId->name;
                if (auto* propId = dynamic_cast<Identifier*>(memberExpr->property.get())) {
                    className = propId->name;
                    std::cerr << "  DEBUG: MemberExpr class: " << objectName << "." << className << std::endl;
                }
            }
            if (objectName.empty() || className.empty()) {
                std::cerr << "  ERROR: 'new' expression with complex MemberExpr callee" << std::endl;
                lastValue_ = builder_->createIntConstant(0);
                return;
            }
        } else {
            std::cerr << "  ERROR: 'new' expression with non-identifier callee" << std::endl;
            lastValue_ = builder_->createIntConstant(0);
            return;
        }

        // Handle Intl.* constructors
        if (objectName == "Intl") {
            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

            std::string runtimeFunc;
            HIRValue* localeArg = nullptr;
            HIRValue* optionsArg = nullptr;

            // Get locale argument (first arg)
            if (node.arguments.size() >= 1) {
                node.arguments[0]->accept(*this);
                localeArg = lastValue_;
            } else {
                // Default locale: empty string means use system locale
                localeArg = builder_->createStringConstant("");
            }

            // Get options argument (second arg)
            if (node.arguments.size() >= 2) {
                node.arguments[1]->accept(*this);
                optionsArg = lastValue_;
            } else {
                optionsArg = builder_->createIntConstant(0);  // null options
            }

            if (className == "NumberFormat") {
                runtimeFunc = "nova_intl_numberformat_create";
            } else if (className == "DateTimeFormat") {
                runtimeFunc = "nova_intl_datetimeformat_create";
            } else if (className == "Collator") {
                runtimeFunc = "nova_intl_collator_create";
            } else if (className == "PluralRules") {
                runtimeFunc = "nova_intl_pluralrules_create";
            } else if (className == "RelativeTimeFormat") {
                runtimeFunc = "nova_intl_relativetimeformat_create";
            } else if (className == "ListFormat") {
                runtimeFunc = "nova_intl_listformat_create";
            } else if (className == "DisplayNames") {
                runtimeFunc = "nova_intl_displaynames_create";
            } else if (className == "Locale") {
                runtimeFunc = "nova_intl_locale_create";
            } else if (className == "Segmenter") {
                runtimeFunc = "nova_intl_segmenter_create";
            } else {
                std::cerr << "  ERROR: Unknown Intl constructor: " << className << std::endl;
                lastValue_ = builder_->createIntConstant(0);
                return;
            }

            std::vector<HIRTypePtr> paramTypes = {ptrType, ptrType};
            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            std::vector<HIRValue*> args = {localeArg, optionsArg};
            lastValue_ = builder_->createCall(func, args);
            
            // Set tracking flag for VarDecl
            if (className == "NumberFormat") lastWasNumberFormat_ = true;
            else if (className == "DateTimeFormat") lastWasDateTimeFormat_ = true;
            else if (className == "Collator") lastWasCollator_ = true;
            else if (className == "PluralRules") lastWasPluralRules_ = true;
            else if (className == "RelativeTimeFormat") lastWasRelativeTimeFormat_ = true;
            else if (className == "ListFormat") lastWasListFormat_ = true;
            else if (className == "DisplayNames") lastWasDisplayNames_ = true;
            else if (className == "Locale") lastWasLocale_ = true;
            else if (className == "Segmenter") lastWasSegmenter_ = true;
            return;
        }

        // Handle AggregateError separately - it has different signature: (errors, message)
        if (className == "AggregateError") {
            std::cerr << "  DEBUG: Handling AggregateError" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

            // JavaScript API: new AggregateError(errors, message)
            // Runtime API: nova_aggregate_error_create(message, errors, count)
            HIRValue* errorsArg = nullptr;
            HIRValue* messageArg = nullptr;
            int64_t errorCount = 0;

            // First argument is errors array
            if (node.arguments.size() >= 1) {
                node.arguments[0]->accept(*this);
                errorsArg = lastValue_;

                // Try to get array length if it's an array literal
                if (auto* arrLit = dynamic_cast<ArrayExpr*>(node.arguments[0].get())) {
                    errorCount = static_cast<int64_t>(arrLit->elements.size());
                }
            }

            // Second argument is message
            if (node.arguments.size() >= 2) {
                node.arguments[1]->accept(*this);
                messageArg = lastValue_;
            }

            // Create function type: ptr @nova_aggregate_error_create(ptr, ptr, i64)
            std::vector<HIRTypePtr> paramTypes = {ptrType, ptrType, intType};

            auto existingFunc = module_->getFunction("nova_aggregate_error_create");
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction("nova_aggregate_error_create", funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
                std::cerr << "  DEBUG: Created external function: nova_aggregate_error_create" << std::endl;
            }

            // Prepare arguments in runtime order: (message, errors, count)
            std::vector<HIRValue*> args;
            args.push_back(messageArg ? messageArg : builder_->createStringConstant(""));
            args.push_back(errorsArg ? errorsArg : builder_->createIntConstant(0));
            args.push_back(builder_->createIntConstant(errorCount));

            lastValue_ = builder_->createCall(func, args, "aggregate_error");
            lastValue_->type = ptrType;
            std::cerr << "  DEBUG: Created AggregateError with " << errorCount << " errors" << std::endl;
            return;
        }

        // Handle ArrayBuffer constructor
        if (className == "ArrayBuffer") {
            std::cerr << "  DEBUG: Handling ArrayBuffer constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

            // Get length argument
            HIRValue* lengthArg = nullptr;
            if (!node.arguments.empty()) {
                node.arguments[0]->accept(*this);
                lengthArg = lastValue_;
            } else {
                lengthArg = builder_->createIntConstant(0);
            }

            std::vector<HIRTypePtr> paramTypes = {intType};
            auto existingFunc = module_->getFunction("nova_arraybuffer_create");
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction("nova_arraybuffer_create", funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            std::vector<HIRValue*> args = {lengthArg};
            lastValue_ = builder_->createCall(func, args, "arraybuffer");
            lastValue_->type = ptrType;
            lastWasArrayBuffer_ = true;  // Track for variable declaration
            return;
        }

        // Handle SharedArrayBuffer constructor (ES2017)
        if (className == "SharedArrayBuffer") {
            std::cerr << "  DEBUG: Handling SharedArrayBuffer constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

            // Get length argument
            HIRValue* lengthArg = nullptr;
            if (!node.arguments.empty()) {
                node.arguments[0]->accept(*this);
                lengthArg = lastValue_;
            } else {
                lengthArg = builder_->createIntConstant(0);
            }

            std::vector<HIRTypePtr> paramTypes = {intType};
            auto existingFunc = module_->getFunction("nova_sharedarraybuffer_create");
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction("nova_sharedarraybuffer_create", funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            std::vector<HIRValue*> args = {lengthArg};
            lastValue_ = builder_->createCall(func, args, "sharedarraybuffer");
            lastValue_->type = ptrType;
            lastWasSharedArrayBuffer_ = true;  // Track for variable declaration
            return;
        }

        // Handle Map constructor (ES2015)
        if (className == "Map") {
            std::cerr << "  DEBUG: Handling Map constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

            std::string runtimeFunc = "nova_map_create";
            std::vector<HIRTypePtr> paramTypes;  // No parameters for new Map()
            std::vector<HIRValue*> args;

            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            lastValue_ = builder_->createCall(func, args, "map");
            lastValue_->type = ptrType;
            lastWasMap_ = true;  // Track for variable declaration
            return;
        }

        // Handle Set constructor (ES2015)
        if (className == "Set") {
            std::cerr << "  DEBUG: Handling Set constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

            std::string runtimeFunc = "nova_set_create";
            std::vector<HIRTypePtr> paramTypes;  // No parameters for new Set()
            std::vector<HIRValue*> args;

            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            lastValue_ = builder_->createCall(func, args, "set");
            lastValue_->type = ptrType;
            lastWasSet_ = true;  // Track for variable declaration
            return;
        }

        // Handle WeakMap constructor (ES2015)
        if (className == "WeakMap") {
            std::cerr << "  DEBUG: Handling WeakMap constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

            std::string runtimeFunc = "nova_weakmap_create";
            std::vector<HIRTypePtr> paramTypes;  // No parameters for new WeakMap()
            std::vector<HIRValue*> args;

            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            lastValue_ = builder_->createCall(func, args, "weakmap");
            lastValue_->type = ptrType;
            lastWasWeakMap_ = true;  // Track for variable declaration
            return;
        }

        // Handle WeakRef constructor (ES2021)
        if (className == "WeakRef") {
            std::cerr << "  DEBUG: Handling WeakRef constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

            std::string runtimeFunc = "nova_weakref_create";
            std::vector<HIRTypePtr> paramTypes = {ptrType};  // target object
            std::vector<HIRValue*> args;

            // Get target argument
            if (node.arguments.size() > 0) {
                node.arguments[0]->accept(*this);
                args.push_back(lastValue_);
            } else {
                args.push_back(builder_->createNullConstant(ptrType.get()));
            }

            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            lastValue_ = builder_->createCall(func, args, "weakref");
            lastValue_->type = ptrType;
            lastWasWeakRef_ = true;  // Track for variable declaration
            return;
        }

        // Handle WeakSet constructor (ES2015)
        if (className == "WeakSet") {
            std::cerr << "  DEBUG: Handling WeakSet constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

            std::string runtimeFunc = "nova_weakset_create";
            std::vector<HIRTypePtr> paramTypes;  // No parameters for new WeakSet()
            std::vector<HIRValue*> args;

            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            lastValue_ = builder_->createCall(func, args, "weakset");
            lastValue_->type = ptrType;
            lastWasWeakSet_ = true;  // Track for variable declaration
            return;
        }

        // Handle URL constructor (Web API)
        if (className == "URL") {
            std::cerr << "  DEBUG: Handling URL constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto strType = std::make_shared<HIRType>(HIRType::Kind::String);

            if (node.arguments.size() >= 2) {
                // new URL(url, base)
                std::string runtimeFunc = "nova_url_create_with_base";
                std::vector<HIRTypePtr> paramTypes = {strType, strType};
                std::vector<HIRValue*> args;

                node.arguments[0]->accept(*this);
                args.push_back(lastValue_);
                node.arguments[1]->accept(*this);
                args.push_back(lastValue_);

                auto existingFunc = module_->getFunction(runtimeFunc);
                HIRFunction* func = nullptr;
                if (existingFunc) {
                    func = existingFunc.get();
                } else {
                    HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                    HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                    funcPtr->linkage = HIRFunction::Linkage::External;
                    func = funcPtr.get();
                }

                lastValue_ = builder_->createCall(func, args, "url");
            } else if (node.arguments.size() == 1) {
                // new URL(url)
                std::string runtimeFunc = "nova_url_create";
                std::vector<HIRTypePtr> paramTypes = {strType};
                std::vector<HIRValue*> args;

                node.arguments[0]->accept(*this);
                args.push_back(lastValue_);

                auto existingFunc = module_->getFunction(runtimeFunc);
                HIRFunction* func = nullptr;
                if (existingFunc) {
                    func = existingFunc.get();
                } else {
                    HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                    HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                    funcPtr->linkage = HIRFunction::Linkage::External;
                    func = funcPtr.get();
                }

                lastValue_ = builder_->createCall(func, args, "url");
            } else {
                lastValue_ = builder_->createNullConstant(ptrType.get());
            }
            lastValue_->type = ptrType;
            lastWasURL_ = true;
            return;
        }

        // Handle URLSearchParams constructor (Web API)
        if (className == "URLSearchParams") {
            std::cerr << "  DEBUG: Handling URLSearchParams constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto strType = std::make_shared<HIRType>(HIRType::Kind::String);

            std::string runtimeFunc = "nova_urlsearchparams_create";
            std::vector<HIRTypePtr> paramTypes = {strType};
            std::vector<HIRValue*> args;

            if (node.arguments.size() >= 1) {
                node.arguments[0]->accept(*this);
                args.push_back(lastValue_);
            } else {
                args.push_back(builder_->createStringConstant(""));
            }

            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            lastValue_ = builder_->createCall(func, args, "urlsearchparams");
            lastValue_->type = ptrType;
            lastWasURLSearchParams_ = true;
            return;
        }

        // Handle TextEncoder constructor (Web API)
        if (className == "TextEncoder") {
            std::cerr << "  DEBUG: Handling TextEncoder constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

            std::string runtimeFunc = "nova_textencoder_create";
            std::vector<HIRTypePtr> paramTypes;
            std::vector<HIRValue*> args;

            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            lastValue_ = builder_->createCall(func, args, "textencoder");
            lastValue_->type = ptrType;
            lastWasTextEncoder_ = true;
            return;
        }

        // Handle TextDecoder constructor (Web API)
        if (className == "TextDecoder") {
            std::cerr << "  DEBUG: Handling TextDecoder constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto strType = std::make_shared<HIRType>(HIRType::Kind::String);

            std::string runtimeFunc;
            std::vector<HIRTypePtr> paramTypes;
            std::vector<HIRValue*> args;

            if (node.arguments.size() == 0) {
                runtimeFunc = "nova_textdecoder_create";
            } else {
                runtimeFunc = "nova_textdecoder_create_with_encoding";
                paramTypes.push_back(strType);
                node.arguments[0]->accept(*this);
                args.push_back(lastValue_);
            }

            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            lastValue_ = builder_->createCall(func, args, "textdecoder");
            lastValue_->type = ptrType;
            lastWasTextDecoder_ = true;
            return;
        }

        // Handle Headers constructor (Web API)
        if (className == "Headers") {
            std::cerr << "  DEBUG: Handling Headers constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

            std::string runtimeFunc = "nova_headers_create";
            std::vector<HIRTypePtr> paramTypes;
            std::vector<HIRValue*> args;

            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            lastValue_ = builder_->createCall(func, args, "headers");
            lastValue_->type = ptrType;
            lastWasHeaders_ = true;
            return;
        }

        // Handle Request constructor (Web API)
        if (className == "Request") {
            std::cerr << "  DEBUG: Handling Request constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto strType = std::make_shared<HIRType>(HIRType::Kind::String);

            std::string runtimeFunc = "nova_request_create";
            std::vector<HIRTypePtr> paramTypes = {strType};
            std::vector<HIRValue*> args;

            if (node.arguments.size() >= 1) {
                node.arguments[0]->accept(*this);
                args.push_back(lastValue_);
            } else {
                args.push_back(builder_->createStringConstant(""));
            }

            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            lastValue_ = builder_->createCall(func, args, "request");
            lastValue_->type = ptrType;
            lastWasRequest_ = true;
            return;
        }

        // Handle Response constructor (Web API)
        if (className == "Response") {
            std::cerr << "  DEBUG: Handling Response constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto strType = std::make_shared<HIRType>(HIRType::Kind::String);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

            std::string runtimeFunc = "nova_response_create";
            std::vector<HIRTypePtr> paramTypes = {strType, intType, strType};
            std::vector<HIRValue*> args;

            // body (optional)
            if (node.arguments.size() >= 1) {
                node.arguments[0]->accept(*this);
                args.push_back(lastValue_);
            } else {
                args.push_back(builder_->createNullConstant(strType.get()));
            }

            // status (default 200)
            args.push_back(builder_->createIntConstant(200));
            // statusText (default "OK")
            args.push_back(builder_->createStringConstant("OK"));

            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            lastValue_ = builder_->createCall(func, args, "response");
            lastValue_->type = ptrType;
            lastWasResponse_ = true;
            return;
        }

        // Handle Proxy constructor (ES2015)
        if (className == "Proxy") {
            std::cerr << "  DEBUG: Handling Proxy constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

            std::string runtimeFunc = "nova_proxy_create";
            std::vector<HIRTypePtr> paramTypes = {ptrType, ptrType};  // target, handler
            std::vector<HIRValue*> args;

            // Get target argument
            if (node.arguments.size() > 0) {
                node.arguments[0]->accept(*this);
                args.push_back(lastValue_);
            } else {
                args.push_back(builder_->createNullConstant(ptrType.get()));
            }

            // Get handler argument
            if (node.arguments.size() > 1) {
                node.arguments[1]->accept(*this);
                args.push_back(lastValue_);
            } else {
                args.push_back(builder_->createNullConstant(ptrType.get()));
            }

            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            lastValue_ = builder_->createCall(func, args, "proxy");
            lastValue_->type = ptrType;
            return;
        }

        // Handle Date constructor (ES1)
        if (className == "Date") {
            std::cerr << "  DEBUG: Handling Date constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

            std::string runtimeFunc;
            std::vector<HIRTypePtr> paramTypes;
            std::vector<HIRValue*> args;

            if (node.arguments.empty()) {
                // new Date() - current time
                runtimeFunc = "nova_date_create";
                // No parameters
            } else if (node.arguments.size() == 1) {
                // new Date(timestamp) or new Date(dateString)
                // For now, assume timestamp (number)
                runtimeFunc = "nova_date_create_timestamp";
                paramTypes.push_back(intType);
                node.arguments[0]->accept(*this);
                args.push_back(lastValue_);
            } else {
                // new Date(year, month, day?, hour?, minute?, second?, ms?)
                runtimeFunc = "nova_date_create_parts";
                for (int i = 0; i < 7; i++) {
                    paramTypes.push_back(intType);
                }

                // Evaluate provided arguments
                for (size_t i = 0; i < node.arguments.size() && i < 7; i++) {
                    node.arguments[i]->accept(*this);
                    args.push_back(lastValue_);
                }
                // Fill remaining with defaults (0, except day which defaults to 1)
                while (args.size() < 7) {
                    if (args.size() == 2) {
                        args.push_back(builder_->createIntConstant(1));  // day defaults to 1
                    } else {
                        args.push_back(builder_->createIntConstant(0));
                    }
                }
            }

            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            lastValue_ = builder_->createCall(func, args, "date");
            lastValue_->type = ptrType;
            lastWasDate_ = true;  // Track for variable declaration
            return;
        }

        // Handle TypedArray constructors
        if (className == "Int8Array" || className == "Uint8Array" || className == "Uint8ClampedArray" ||
            className == "Int16Array" || className == "Uint16Array" ||
            className == "Int32Array" || className == "Uint32Array" ||
            className == "Float32Array" || className == "Float64Array" ||
            className == "BigInt64Array" || className == "BigUint64Array") {

            std::cerr << "  DEBUG: Handling TypedArray constructor: " << className << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

            // Check if first argument is an ArrayBuffer
            bool isFromBuffer = false;
            if (!node.arguments.empty()) {
                if (auto* argIdent = dynamic_cast<Identifier*>(node.arguments[0].get())) {
                    if (arrayBufferVars_.count(argIdent->name) > 0) {
                        isFromBuffer = true;
                        std::cerr << "    DEBUG: Creating TypedArray from ArrayBuffer: " << argIdent->name << std::endl;
                    }
                }
            }

            if (isFromBuffer) {
                // Create TypedArray from ArrayBuffer
                std::string runtimeFunc;
                if (className == "Int8Array") runtimeFunc = "nova_int8array_from_buffer";
                else if (className == "Uint8Array") runtimeFunc = "nova_uint8array_from_buffer";
                else if (className == "Uint8ClampedArray") runtimeFunc = "nova_uint8clampedarray_from_buffer";
                else if (className == "Int16Array") runtimeFunc = "nova_int16array_from_buffer";
                else if (className == "Uint16Array") runtimeFunc = "nova_uint16array_from_buffer";
                else if (className == "Int32Array") runtimeFunc = "nova_int32array_from_buffer";
                else if (className == "Uint32Array") runtimeFunc = "nova_uint32array_from_buffer";
                else if (className == "Float32Array") runtimeFunc = "nova_float32array_from_buffer";
                else if (className == "Float64Array") runtimeFunc = "nova_float64array_from_buffer";
                else if (className == "BigInt64Array") runtimeFunc = "nova_bigint64array_from_buffer";
                else if (className == "BigUint64Array") runtimeFunc = "nova_biguint64array_from_buffer";

                // Get arguments: buffer, byteOffset (optional), length (optional)
                HIRValue* bufferArg = nullptr;
                HIRValue* offsetArg = nullptr;
                HIRValue* lengthArg = nullptr;

                node.arguments[0]->accept(*this);
                bufferArg = lastValue_;

                if (node.arguments.size() >= 2) {
                    node.arguments[1]->accept(*this);
                    offsetArg = lastValue_;
                } else {
                    offsetArg = builder_->createIntConstant(0);
                }

                if (node.arguments.size() >= 3) {
                    node.arguments[2]->accept(*this);
                    lengthArg = lastValue_;
                } else {
                    lengthArg = builder_->createIntConstant(-1);  // -1 means use remaining buffer
                }

                std::vector<HIRTypePtr> paramTypes = {ptrType, intType, intType};
                auto existingFunc = module_->getFunction(runtimeFunc);
                HIRFunction* func = nullptr;
                if (existingFunc) {
                    func = existingFunc.get();
                } else {
                    HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                    HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                    funcPtr->linkage = HIRFunction::Linkage::External;
                    func = funcPtr.get();
                }

                std::vector<HIRValue*> args = {bufferArg, offsetArg, lengthArg};
                lastValue_ = builder_->createCall(func, args, "typedarray");
                lastValue_->type = ptrType;
                lastTypedArrayType_ = className;  // Track for element access
                return;
            }

            // Create TypedArray with length
            std::string runtimeFunc;
            if (className == "Int8Array") runtimeFunc = "nova_int8array_create";
            else if (className == "Uint8Array") runtimeFunc = "nova_uint8array_create";
            else if (className == "Uint8ClampedArray") runtimeFunc = "nova_uint8clampedarray_create";
            else if (className == "Int16Array") runtimeFunc = "nova_int16array_create";
            else if (className == "Uint16Array") runtimeFunc = "nova_uint16array_create";
            else if (className == "Int32Array") runtimeFunc = "nova_int32array_create";
            else if (className == "Uint32Array") runtimeFunc = "nova_uint32array_create";
            else if (className == "Float32Array") runtimeFunc = "nova_float32array_create";
            else if (className == "Float64Array") runtimeFunc = "nova_float64array_create";
            else if (className == "BigInt64Array") runtimeFunc = "nova_bigint64array_create";
            else if (className == "BigUint64Array") runtimeFunc = "nova_biguint64array_create";

            // Get length argument
            HIRValue* lengthArg = nullptr;
            if (!node.arguments.empty()) {
                node.arguments[0]->accept(*this);
                lengthArg = lastValue_;
            } else {
                lengthArg = builder_->createIntConstant(0);
            }

            std::vector<HIRTypePtr> paramTypes = {intType};
            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            std::vector<HIRValue*> args = {lengthArg};
            lastValue_ = builder_->createCall(func, args, "typedarray");
            lastValue_->type = ptrType;
            lastTypedArrayType_ = className;  // Track for element access
            return;
        }

        // Handle DataView constructor
        if (className == "DataView") {
            std::cerr << "  DEBUG: Handling DataView constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

            // Get arguments: buffer, byteOffset (optional), byteLength (optional)
            HIRValue* bufferArg = nullptr;
            HIRValue* offsetArg = nullptr;
            HIRValue* lengthArg = nullptr;

            if (node.arguments.size() >= 1) {
                node.arguments[0]->accept(*this);
                bufferArg = lastValue_;
            }
            if (node.arguments.size() >= 2) {
                node.arguments[1]->accept(*this);
                offsetArg = lastValue_;
            } else {
                offsetArg = builder_->createIntConstant(0);
            }
            if (node.arguments.size() >= 3) {
                node.arguments[2]->accept(*this);
                lengthArg = lastValue_;
            } else {
                lengthArg = builder_->createIntConstant(-1);  // -1 means use remaining buffer
            }

            std::vector<HIRTypePtr> paramTypes = {ptrType, intType, intType};
            auto existingFunc = module_->getFunction("nova_dataview_create");
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction("nova_dataview_create", funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            std::vector<HIRValue*> args = {bufferArg, offsetArg, lengthArg};
            lastValue_ = builder_->createCall(func, args, "dataview");
            lastValue_->type = ptrType;
            lastWasDataView_ = true;  // Track for method calls
            return;
        }

        // Handle DisposableStack constructor (ES2024)
        if (className == "DisposableStack") {
            std::cerr << "  DEBUG: Handling DisposableStack constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

            // DisposableStack takes no arguments
            std::vector<HIRTypePtr> paramTypes = {};
            auto existingFunc = module_->getFunction("nova_disposablestack_create");
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction("nova_disposablestack_create", funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            std::vector<HIRValue*> args = {};
            lastValue_ = builder_->createCall(func, args, "disposablestack");
            lastValue_->type = ptrType;
            lastWasDisposableStack_ = true;  // Track for variable declaration
            return;
        }

        // Handle AsyncDisposableStack constructor (ES2024)
        if (className == "AsyncDisposableStack") {
            std::cerr << "  DEBUG: Handling AsyncDisposableStack constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

            // AsyncDisposableStack takes no arguments
            std::vector<HIRTypePtr> paramTypes = {};
            auto existingFunc = module_->getFunction("nova_asyncdisposablestack_create");
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction("nova_asyncdisposablestack_create", funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            std::vector<HIRValue*> args = {};
            lastValue_ = builder_->createCall(func, args, "asyncdisposablestack");
            lastValue_->type = ptrType;
            lastWasAsyncDisposableStack_ = true;  // Track for variable declaration
            return;
        }

        // Handle FinalizationRegistry constructor (ES2021)
        if (className == "FinalizationRegistry") {
            std::cerr << "  DEBUG: Handling FinalizationRegistry constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

            // FinalizationRegistry takes a callback function
            HIRValue* callbackArg = nullptr;
            if (!node.arguments.empty()) {
                node.arguments[0]->accept(*this);
                callbackArg = lastValue_;
            } else {
                // If no callback provided, use null
                callbackArg = builder_->createIntConstant(0);
            }

            std::vector<HIRTypePtr> paramTypes = {ptrType};  // callback
            auto existingFunc = module_->getFunction("nova_finalization_registry_create");
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction("nova_finalization_registry_create", funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            std::vector<HIRValue*> args = {callbackArg};
            lastValue_ = builder_->createCall(func, args, "finalization_registry");
            lastValue_->type = ptrType;
            lastWasFinalizationRegistry_ = true;  // Track for variable declaration
            return;
        }

        // Handle GeneratorFunction constructor (ES2015)
        // new GeneratorFunction([arg1, arg2, ...argN], functionBody)
        if (className == "GeneratorFunction") {
            std::cerr << "  DEBUG: Handling GeneratorFunction constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

            // Last argument is the function body, all others are parameter names
            std::string body;
            std::vector<std::string> paramNames;

            if (!node.arguments.empty()) {
                // Last argument is body
                size_t bodyIndex = node.arguments.size() - 1;

                // Get body string (must be a constant string for AOT)
                if (auto* strLit = dynamic_cast<StringLiteral*>(node.arguments[bodyIndex].get())) {
                    body = strLit->value;
                }

                // Get parameter names (all arguments except last)
                for (size_t i = 0; i < bodyIndex; i++) {
                    if (auto* paramLit = dynamic_cast<StringLiteral*>(node.arguments[i].get())) {
                        paramNames.push_back(paramLit->value);
                    }
                }
            }

            std::cerr << "  DEBUG: GeneratorFunction body: " << body << std::endl;
            std::cerr << "  DEBUG: GeneratorFunction params: " << paramNames.size() << std::endl;

            // Create runtime call (will log warning about AOT limitation)
            std::vector<HIRTypePtr> paramTypes = {ptrType, ptrType, intType};  // body, paramNames, paramCount
            auto existingFunc = module_->getFunction("nova_generator_function_create");
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction("nova_generator_function_create", funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            HIRValue* bodyArg = builder_->createStringConstant(body);
            HIRValue* paramsArg = builder_->createIntConstant(0);  // Simplified: null for param names
            HIRValue* paramCountArg = builder_->createIntConstant(static_cast<int64_t>(paramNames.size()));

            std::vector<HIRValue*> args = {bodyArg, paramsArg, paramCountArg};
            lastValue_ = builder_->createCall(func, args, "generator_function");
            lastValue_->type = ptrType;
            lastWasGeneratorFunction_ = true;  // Track for method calls
            return;
        }

        // Handle AsyncGeneratorFunction constructor (ES2018)
        // new AsyncGeneratorFunction([arg1, arg2, ...argN], functionBody)
        if (className == "AsyncGeneratorFunction") {
            std::cerr << "  DEBUG: Handling AsyncGeneratorFunction constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

            // Last argument is the function body, all others are parameter names
            std::string body;
            std::vector<std::string> paramNames;

            if (!node.arguments.empty()) {
                size_t bodyIndex = node.arguments.size() - 1;

                if (auto* strLit = dynamic_cast<StringLiteral*>(node.arguments[bodyIndex].get())) {
                    body = strLit->value;
                }

                for (size_t i = 0; i < bodyIndex; i++) {
                    if (auto* paramLit = dynamic_cast<StringLiteral*>(node.arguments[i].get())) {
                        paramNames.push_back(paramLit->value);
                    }
                }
            }

            std::cerr << "  DEBUG: AsyncGeneratorFunction body: " << body << std::endl;
            std::cerr << "  DEBUG: AsyncGeneratorFunction params: " << paramNames.size() << std::endl;

            std::vector<HIRTypePtr> paramTypes = {ptrType, ptrType, intType};
            auto existingFunc = module_->getFunction("nova_async_generator_function_create");
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction("nova_async_generator_function_create", funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            HIRValue* bodyArg = builder_->createStringConstant(body);
            HIRValue* paramsArg = builder_->createIntConstant(0);
            HIRValue* paramCountArg = builder_->createIntConstant(static_cast<int64_t>(paramNames.size()));

            std::vector<HIRValue*> args = {bodyArg, paramsArg, paramCountArg};
            lastValue_ = builder_->createCall(func, args, "async_generator_function");
            lastValue_->type = ptrType;
            lastWasAsyncGeneratorFunction_ = true;  // Track for method calls
            return;
        }

        // Handle Promise constructor (ES2015)
        if (className == "Promise") {
            std::cerr << "  DEBUG: Handling Promise constructor" << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

            // Promise takes an executor function: (resolve, reject) => void
            // For now, create a pending promise
            std::vector<HIRTypePtr> paramTypes = {};
            auto existingFunc = module_->getFunction("nova_promise_create");
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction("nova_promise_create", funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            std::vector<HIRValue*> args = {};
            lastValue_ = builder_->createCall(func, args, "promise");
            lastValue_->type = ptrType;
            lastWasPromise_ = true;  // Track for method calls
            return;
        }

        // Handle SuppressedError (ES2024) - takes 3 arguments: error, suppressed, message
        if (className == "SuppressedError") {
            std::cerr << "  DEBUG: Handling SuppressedError constructor" << std::endl;

            // Get arguments: error, suppressed, message
            HIRValue* errorArg = nullptr;
            HIRValue* suppressedArg = nullptr;
            HIRValue* messageArg = nullptr;

            if (node.arguments.size() >= 1) {
                node.arguments[0]->accept(*this);
                errorArg = lastValue_;
            }
            if (node.arguments.size() >= 2) {
                node.arguments[1]->accept(*this);
                suppressedArg = lastValue_;
            }
            if (node.arguments.size() >= 3) {
                node.arguments[2]->accept(*this);
                messageArg = lastValue_;
            }

            // Create external function reference
            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            std::vector<HIRTypePtr> paramTypes;
            paramTypes.push_back(ptrType);  // void* error
            paramTypes.push_back(ptrType);  // void* suppressed
            paramTypes.push_back(ptrType);  // const char* message

            std::string runtimeFunc = "nova_suppressederror_create";
            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
                std::cerr << "  DEBUG: Created external function: " << runtimeFunc << std::endl;
            }

            // Prepare arguments with defaults
            std::vector<HIRValue*> args;
            args.push_back(errorArg ? errorArg : builder_->createIntConstant(0));
            args.push_back(suppressedArg ? suppressedArg : builder_->createIntConstant(0));
            args.push_back(messageArg ? messageArg : builder_->createStringConstant(""));

            lastValue_ = builder_->createCall(func, args, "suppressed_error");
            lastValue_->type = ptrType;
            lastWasSuppressedError_ = true;
            std::cerr << "  DEBUG: Created SuppressedError" << std::endl;
            return;
        }

        // Handle other builtin Error types
        if (className == "Error" || className == "TypeError" || className == "RangeError" ||
            className == "ReferenceError" || className == "SyntaxError" || className == "URIError" ||
            className == "InternalError" || className == "EvalError") {

            std::cerr << "  DEBUG: Handling builtin error type: " << className << std::endl;

            // Get message argument if provided
            HIRValue* messageArg = nullptr;
            if (!node.arguments.empty()) {
                node.arguments[0]->accept(*this);
                messageArg = lastValue_;
            }

            // Determine runtime function name
            std::string runtimeFunc;
            if (className == "Error") runtimeFunc = "nova_error_create";
            else if (className == "TypeError") runtimeFunc = "nova_type_error_create";
            else if (className == "RangeError") runtimeFunc = "nova_range_error_create";
            else if (className == "ReferenceError") runtimeFunc = "nova_reference_error_create";
            else if (className == "SyntaxError") runtimeFunc = "nova_syntax_error_create";
            else if (className == "URIError") runtimeFunc = "nova_uri_error_create";
            else if (className == "InternalError") runtimeFunc = "nova_internal_error_create";
            else if (className == "EvalError") runtimeFunc = "nova_eval_error_create";

            // Create external function reference
            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            std::vector<HIRTypePtr> paramTypes;
            paramTypes.push_back(ptrType);  // const char* message

            // Check if function already exists
            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
                std::cerr << "  DEBUG: Created external function: " << runtimeFunc << std::endl;
            }

            // Prepare arguments
            std::vector<HIRValue*> args;
            if (messageArg) {
                args.push_back(messageArg);
            } else {
                // Pass empty string if no message
                args.push_back(builder_->createStringConstant(""));
            }

            lastValue_ = builder_->createCall(func, args, "error_obj");
            lastValue_->type = ptrType;
            std::cerr << "  DEBUG: Created " << className << " via " << runtimeFunc << std::endl;
            lastWasError_ = true;  // Track for variable declaration
            return;
        }

        // Check if this is a class expression reference
        std::string actualClassName = className;
        auto classRefIt = classReferences_.find(className);
        if (classRefIt != classReferences_.end()) {
            actualClassName = classRefIt->second;
            std::cerr << "  DEBUG: Resolved class reference: " << className
                      << " -> " << actualClassName << std::endl;
        }

        // Constructor function name: ClassName_constructor
        std::string constructorName = actualClassName + "_constructor";

        // Evaluate arguments
        std::vector<HIRValue*> args;
        std::cerr << "  DEBUG NEW: Evaluating " << node.arguments.size() << " constructor arguments" << std::endl;
        for (size_t i = 0; i < node.arguments.size(); ++i) {
            node.arguments[i]->accept(*this);
            if (lastValue_ && lastValue_->type) {
                std::cerr << "    arg[" << i << "] type->kind = " << static_cast<int>(lastValue_->type->kind) << std::endl;
            }
            args.push_back(lastValue_);
        }

        // Check if this is a builtin module constructor (e.g., EventEmitter, Readable)
        auto builtinIt = builtinFunctionImports_.find(className);
        if (builtinIt != builtinFunctionImports_.end()) {
            std::string runtimeFuncName = builtinIt->second + "_new";
            if(NOVA_DEBUG) std::cerr << "  DEBUG: Handling builtin constructor: " << className << " -> " << runtimeFuncName << std::endl;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            std::vector<HIRTypePtr> paramTypes;  // Most constructors take no args
            std::vector<HIRValue*> callArgs;

            // Check if function exists or create it
            auto existingFunc = module_->getFunction(runtimeFuncName);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFuncName, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
                if(NOVA_DEBUG) std::cerr << "  DEBUG: Created external function: " << runtimeFuncName << std::endl;
            }

            lastValue_ = builder_->createCall(func, callArgs, "builtin_obj");
            lastValue_->type = ptrType;

            // Determine the object type from the runtime function name
            // e.g., "nova_events_EventEmitter" -> "events:EventEmitter"
            std::string runtimeFunc = builtinIt->second;  // e.g., "nova_events_EventEmitter"
            if (runtimeFunc.substr(0, 5) == "nova_") {
                size_t firstUnderscore = runtimeFunc.find('_', 5);
                if (firstUnderscore != std::string::npos) {
                    std::string moduleName = runtimeFunc.substr(5, firstUnderscore - 5);  // "events"
                    std::string typeName = runtimeFunc.substr(firstUnderscore + 1);  // "EventEmitter"
                    lastBuiltinObjectType_ = moduleName + ":" + typeName;  // "events:EventEmitter"
                    if(NOVA_DEBUG) std::cerr << "  DEBUG: Set builtin object type: " << lastBuiltinObjectType_ << std::endl;
                }
            }

            return;
        }

        // Call constructor function
        auto constructorFunc = module_->getFunction(constructorName);
        if (!constructorFunc) {
            std::cerr << "  ERROR: Constructor function not found: " << constructorName << std::endl;
            lastValue_ = builder_->createIntConstant(0);
            return;
        }

        std::cerr << "  DEBUG CALL: Calling constructor " << constructorName << " with " << args.size() << " args" << std::endl;
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] && args[i]->type) {
                std::cerr << "    call_arg[" << i << "] type->kind = " << static_cast<int>(args[i]->type->kind) << std::endl;
            }
        }

        lastValue_ = builder_->createCall(constructorFunc.get(), args, "new_instance");
        std::cerr << "  DEBUG: Created call to constructor: " << constructorName << std::endl;

        // Find and attach the struct type to the result
        hir::HIRStructType* structType = nullptr;
        for (auto* type : module_->types) {
            if (type->kind == hir::HIRType::Kind::Struct) {
                auto* candidateStruct = static_cast<hir::HIRStructType*>(type);
                if (candidateStruct->name == actualClassName) {
                    structType = candidateStruct;
                    std::cerr << "  DEBUG: Found struct type for class: " << actualClassName << std::endl;
                    break;
                }
            }
        }

        // Attach the struct type to the instance value
        if (structType && lastValue_) {
            lastValue_->type = std::make_shared<hir::HIRStructType>(*structType);
            std::cerr << "  DEBUG: Attached struct type to new instance" << std::endl;
        } else {
            std::cerr << "  WARNING: Could not find struct type for class: " << actualClassName << std::endl;
        }
    }
    

void HIRGenerator::visit(ThisExpr& node) {
        (void)node;
        std::cerr << "\n=== TRACE: ThisExpr visitor ===" << std::endl;
        std::cerr << "  currentThis_ pointer: " << currentThis_ << std::endl;

        if (currentThis_) {
            lastValue_ = currentThis_;
            std::cerr << "  TRACE: Set lastValue_ = currentThis_" << std::endl;
            std::cerr << "  TRACE: lastValue_ pointer: " << lastValue_ << std::endl;
            if (lastValue_->type) {
                std::cerr << "  TRACE: lastValue_->type->kind = " << static_cast<int>(lastValue_->type->kind) << std::endl;
                std::cerr << "  TRACE: lastValue_->type pointer: " << lastValue_->type.get() << std::endl;
            } else {
                std::cerr << "  TRACE ERROR: lastValue_->type is NULL!" << std::endl;
            }
            std::cerr << "=== END TRACE ===" << std::endl;
        } else {
            std::cerr << "  TRACE ERROR: 'this' used outside of method context!" << std::endl;
            std::cerr << "=== END TRACE ===" << std::endl;
            // Create placeholder to avoid crash
            lastValue_ = builder_->createIntConstant(0);
        }
    }
    

void HIRGenerator::visit(SuperExpr& node) {
        (void)node;
        // super reference
    }
    

void HIRGenerator::visit(ClassDecl& node) {
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing class declaration: " << node.name << std::endl;

        // Register class name for static method call detection
        classNames_.insert(node.name);

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

        // 1. Create struct type for class data (instance properties + inherited fields)
        std::vector<hir::HIRStructType::Field> fields;
        std::set<std::string> fieldNames;  // Track field names to avoid duplicates

        // INHERITANCE: If this class extends another, include parent fields first
        if (!node.superclass.empty()) {
            std::cerr << "  DEBUG: Class " << node.name << " extends " << node.superclass << std::endl;

            // Store inheritance relationship
            classInheritance_[node.name] = node.superclass;

            // Look up parent class struct type
            auto parentStructIt = classStructTypes_.find(node.superclass);
            if (parentStructIt != classStructTypes_.end()) {
                hir::HIRStructType* parentStruct = parentStructIt->second;
                std::cerr << "  DEBUG: Found parent struct with " << parentStruct->fields.size() << " fields" << std::endl;

                // Copy all parent fields to derived class
                for (const auto& parentField : parentStruct->fields) {
                    fields.push_back(parentField);
                    fieldNames.insert(parentField.name);
                    std::cerr << "  DEBUG: Inherited field: " << parentField.name << std::endl;
                }
            } else {
                std::cerr << "  WARNING: Parent class " << node.superclass << " not found! Define parent before child." << std::endl;
            }
        }

        // Add own instance properties
        for (const auto& prop : node.properties) {
            if (prop.isStatic) {
                // Handle static property - store initial value
                std::string propKey = node.name + "_" + prop.name;
                std::cerr << "  DEBUG: Creating static property: " << propKey << std::endl;

                // Evaluate initializer if present
                int64_t initValue = 0;
                if (prop.initializer) {
                    if (auto* numLit = dynamic_cast<NumberLiteral*>(prop.initializer.get())) {
                        initValue = static_cast<int64_t>(numLit->value);
                    }
                }

                // Store static property value
                staticPropertyValues_[propKey] = initValue;

                // Track which properties are static for this class
                classStaticProps_[node.name].insert(prop.name);
            } else {
                // Instance property - add to struct fields
                hir::HIRType::Kind typeKind = hir::HIRType::Kind::I64;
                if (prop.type) {
                    typeKind = convertTypeKind(prop.type->kind);
                }
                auto fieldType = std::make_shared<hir::HIRType>(typeKind);
                fields.push_back({prop.name, fieldType, true});
                fieldNames.insert(prop.name);
                std::cerr << "  DEBUG: Added field: " << prop.name << std::endl;
            }
        }

        // Also scan constructor for this.property assignments to auto-add fields
        for (auto& method : node.methods) {
            if (method.kind == ClassDecl::Method::Kind::Constructor && method.body) {
                BlockStmt* bodyBlock = dynamic_cast<BlockStmt*>(method.body.get());
                if (bodyBlock) {
                    for (auto& stmt : bodyBlock->statements) {
                        ExprStmt* exprStmt = dynamic_cast<ExprStmt*>(stmt.get());
                        if (exprStmt) {
                            AssignmentExpr* assignExpr = dynamic_cast<AssignmentExpr*>(exprStmt->expression.get());
                            if (assignExpr) {
                                MemberExpr* memberExpr = dynamic_cast<MemberExpr*>(assignExpr->left.get());
                                if (memberExpr) {
                                    ThisExpr* thisExpr = dynamic_cast<ThisExpr*>(memberExpr->object.get());
                                    if (thisExpr) {
                                        Identifier* propIdent = dynamic_cast<Identifier*>(memberExpr->property.get());
                                        if (propIdent) {
                                            std::string propName = propIdent->name;
                                            if (fieldNames.find(propName) == fieldNames.end()) {
                                                // Infer field type from RHS of assignment
                                                hir::HIRType::Kind typeKind = hir::HIRType::Kind::I64; // default

                                                // Check what's being assigned
                                                if (auto* stringLit = dynamic_cast<StringLiteral*>(assignExpr->right.get())) {
                                                    typeKind = hir::HIRType::Kind::String;
                                                    std::cerr << "  DEBUG: Field '" << propName << "' inferred as String from string literal" << std::endl;
                                                } else if (auto* numLit = dynamic_cast<NumberLiteral*>(assignExpr->right.get())) {
                                                    typeKind = hir::HIRType::Kind::I64;
                                                } else if (auto* ident = dynamic_cast<Identifier*>(assignExpr->right.get())) {
                                                    // Constructor parameter - use Any for dynamic typing
                                                    // Any type can hold numbers, strings, objects, etc.
                                                    typeKind = hir::HIRType::Kind::Any;
                                                    std::cerr << "  DEBUG: Field '" << propName << "' inferred as Any from parameter '" << ident->name << "'" << std::endl;
                                                }

                                                auto fieldType = std::make_shared<hir::HIRType>(typeKind);
                                                fields.push_back({propName, fieldType, true});
                                                fieldNames.insert(propName);
                                                std::cerr << "  DEBUG: Auto-added field '" << propName << "' from constructor" << std::endl;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        auto structType = module_->createStructType(node.name);
        structType->fields = fields;
        std::cerr << "  DEBUG: Created struct type with " << fields.size() << " fields" << std::endl;

        // Store struct type for inheritance lookups
        classStructTypes_[node.name] = structType;

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
        } else {
            // Generate default constructor if none is defined
            std::cerr << "  DEBUG: Generating default constructor" << std::endl;
            generateDefaultConstructor(node.name, structType);
        }

        // 3. Generate method functions (including static, getters, setters)
        for (const auto& method : node.methods) {
            if (method.kind == ClassDecl::Method::Kind::Method) {
                if (method.isStatic) {
                    std::cerr << "  DEBUG: Generating static method: " << method.name << std::endl;
                    generateStaticMethodFunction(node.name, method, convertTypeKind);
                } else {
                    std::cerr << "  DEBUG: Generating method: " << method.name << std::endl;
                    generateMethodFunction(node.name, method, structType, convertTypeKind);
                    classOwnMethods_[node.name].insert(method.name);
                }
            } else if (method.kind == ClassDecl::Method::Kind::Get) {
                std::cerr << "  DEBUG: Generating getter: " << method.name << std::endl;
                generateGetterFunction(node.name, method, structType, convertTypeKind);
                classGetters_[node.name].insert(method.name);
            } else if (method.kind == ClassDecl::Method::Kind::Set) {
                std::cerr << "  DEBUG: Generating setter: " << method.name << std::endl;
                generateSetterFunction(node.name, method, structType, convertTypeKind);
                classSetters_[node.name].insert(method.name);
            }
        }

        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Completed class declaration: " << node.name << std::endl;
    }

void HIRGenerator::generateConstructorFunction(const std::string& className,
                                     const ClassDecl::Method& constructor,
                                     hir::HIRStructType* structType,
                                     [[maybe_unused]] std::function<HIRType::Kind(Type::Kind)> convertTypeKind) {
        // Constructor function name: ClassName_constructor
        std::string funcName = className + "_constructor";

        // Parameter types (from constructor parameters)
        // Use Any type to accept both strings and numbers (JavaScript is dynamically typed)
        std::vector<hir::HIRTypePtr> paramTypes;
        for (size_t i = 0; i < constructor.params.size(); ++i) {
            paramTypes.push_back(std::make_shared<hir::HIRType>(hir::HIRType::Kind::Any));
        }

        // Return type is pointer to struct (use Any for now)
        auto returnType = std::make_shared<hir::HIRType>(hir::HIRType::Kind::Any);

        // Create function type
        auto funcType = new HIRFunctionType(paramTypes, returnType);

        // Create HIR function
        auto func = module_->createFunction(funcName, funcType);

        // Save current function and class context
        HIRFunction* savedFunction = currentFunction_;
        hir::HIRStructType* savedClassStructType = currentClassStructType_;
        currentFunction_ = func.get();
        currentClassStructType_ = structType;  // Track current class struct type

        // Create entry block
        auto entryBlock = func->createBasicBlock("entry");

        // Create builder
        builder_ = std::make_unique<HIRBuilder>(module_, func.get());
        builder_->setInsertPoint(entryBlock.get());

        // Add parameters to symbol table
        for (size_t i = 0; i < constructor.params.size(); ++i) {
            symbolTable_[constructor.params[i]] = func->parameters[i];
        }

        // Check if constructor starts with super() call
        bool hasSuperCall = false;
        if (constructor.body) {
            BlockStmt* bodyBlock = dynamic_cast<BlockStmt*>(constructor.body.get());
            if (bodyBlock && !bodyBlock->statements.empty()) {
                ExprStmt* firstStmt = dynamic_cast<ExprStmt*>(bodyBlock->statements[0].get());
                if (firstStmt) {
                    CallExpr* callExpr = dynamic_cast<CallExpr*>(firstStmt->expression.get());
                    if (callExpr && dynamic_cast<SuperExpr*>(callExpr->callee.get())) {
                        hasSuperCall = true;
                        std::cerr << "    DEBUG: Constructor has super() call - will use parent's instance" << std::endl;
                    }
                }
            }
        }

        HIRValue* instancePtr = nullptr;

        if (hasSuperCall) {
            // Don't allocate memory - super() will allocate and we'll use its result
            std::cerr << "    DEBUG: Skipping malloc - will use instance from super()" << std::endl;
        } else {
            // Allocate memory for class instance using malloc
            std::cerr << "    DEBUG: Allocating memory for class instance: " << className << std::endl;

            // Get or create malloc function declaration
            HIRFunction* mallocFunc = nullptr;
            auto& functions = module_->functions;
            for (auto& f : functions) {
                if (f->name == "malloc") {
                    mallocFunc = f.get();
                    break;
                }
            }

            if (!mallocFunc) {
                // Create malloc: void* malloc(size_t size)
                // HIR signature: pointer malloc(i64 size)
                std::vector<HIRTypePtr> mallocParamTypes;
                mallocParamTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::I64));
                auto mallocReturnType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                HIRFunctionType* mallocFuncType = new HIRFunctionType(mallocParamTypes, mallocReturnType);
                HIRFunctionPtr mallocFuncPtr = module_->createFunction("malloc", mallocFuncType);
                mallocFuncPtr->linkage = HIRFunction::Linkage::External;
                mallocFunc = mallocFuncPtr.get();
                std::cerr << "    DEBUG: Created external malloc function declaration" << std::endl;
            }

            // Calculate struct size (ObjectHeader + number of fields * 8 bytes for i64)
            // ObjectHeader is 24 bytes: size_t size (8) + uint32_t type_id (4) + bool is_marked (1) + padding + ObjectHeader* next (8)
            // IMPORTANT: Allocate for MAX 8 fields to match LLVM struct layout
            const int MAX_FIELDS = 8;
            size_t structSize = 24 + (MAX_FIELDS * 8);
            auto sizeValue = builder_->createIntConstant(structSize);
            std::cerr << "    DEBUG: Struct size: " << structSize << " bytes (24-byte ObjectHeader + " << MAX_FIELDS << " fields max, actual=" << structType->fields.size() << ")" << std::endl;

            // Call malloc to allocate memory
            std::vector<HIRValue*> mallocArgs = { sizeValue };
            instancePtr = builder_->createCall(mallocFunc, mallocArgs, "instance");
            std::cerr << "    DEBUG: Created malloc call for instance allocation" << std::endl;
        }

        // Save current 'this' context (for nested classes)
        hir::HIRValue* savedThis = currentThis_;

        // Set 'this' to the allocated instance (or null if we'll use super's instance)
        if (!hasSuperCall) {
            currentThis_ = instancePtr;
        }

        // NEW: Apply parent field initializations first (if this is a derived class)
        auto inheritIt = classInheritance_.find(className);
        if (inheritIt != classInheritance_.end()) {
            std::string parentClass = inheritIt->second;
            std::cerr << "    DEBUG: Applying parent field initializations from " << parentClass << std::endl;

            // Recursively apply all ancestor initializations
            std::vector<std::string> ancestors;
            std::string currentParent = parentClass;
            while (!currentParent.empty()) {
                ancestors.push_back(currentParent);
                auto parentIt = classInheritance_.find(currentParent);
                if (parentIt != classInheritance_.end()) {
                    currentParent = parentIt->second;
                } else {
                    break;
                }
            }

            // Apply from oldest ancestor to immediate parent
            for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it) {
                auto fieldValsIt = classFieldInitialValues_.find(*it);
                if (fieldValsIt != classFieldInitialValues_.end()) {
                    for (const auto& pair : fieldValsIt->second) {
                        const std::string& fieldName = pair.first;
                        const FieldInitValue& initValue = pair.second;

                        // Find field index
                        uint32_t fieldIndex = 0;
                        bool found = false;
                        for (size_t i = 0; i < structType->fields.size(); ++i) {
                            if (structType->fields[i].name == fieldName) {
                                fieldIndex = static_cast<uint32_t>(i);
                                found = true;
                                break;
                            }
                        }

                        if (found) {
                            // Create HIRValue from stored literal
                            HIRValue* fieldValue = nullptr;
                            if (initValue.kind == FieldInitValue::Kind::String) {
                                fieldValue = builder_->createStringConstant(initValue.stringValue);
                            } else {
                                fieldValue = builder_->createIntConstant(static_cast<int64_t>(initValue.numberValue));
                            }

                            builder_->createSetField(instancePtr, fieldIndex, fieldValue, fieldName);
                            std::cerr << "      DEBUG: Initialized inherited field '" << fieldName
                                     << "' from " << *it << std::endl;
                        }
                    }
                }
            }
        }

        // Process constructor body
        if (constructor.body) {
            constructor.body->accept(*this);
        }

        // If constructor has super(), use the instance returned by super()
        if (hasSuperCall) {
            instancePtr = lastValue_;
            currentThis_ = instancePtr;
            std::cerr << "    DEBUG: Using instance from super() call: " << instancePtr << std::endl;
        }

        // NEW: Store field initial values for this class (for future child classes)
        // Scan constructor body for this.field = literal assignments
        if (constructor.body) {
            BlockStmt* bodyBlock = dynamic_cast<BlockStmt*>(constructor.body.get());
            if (bodyBlock) {
                std::unordered_map<std::string, FieldInitValue> fieldValues;

                for (auto& stmt : bodyBlock->statements) {
                    ExprStmt* exprStmt = dynamic_cast<ExprStmt*>(stmt.get());
                    if (!exprStmt) continue;

                    AssignmentExpr* assignExpr = dynamic_cast<AssignmentExpr*>(exprStmt->expression.get());
                    if (!assignExpr) continue;

                    MemberExpr* memberExpr = dynamic_cast<MemberExpr*>(assignExpr->left.get());
                    if (!memberExpr) continue;

                    ThisExpr* thisExpr = dynamic_cast<ThisExpr*>(memberExpr->object.get());
                    if (!thisExpr) continue;

                    Identifier* propIdent = dynamic_cast<Identifier*>(memberExpr->property.get());
                    if (!propIdent) continue;

                    // Check if right side is a literal (string or number)
                    if (StringLiteral* strLit = dynamic_cast<StringLiteral*>(assignExpr->right.get())) {
                        FieldInitValue initVal;
                        initVal.kind = FieldInitValue::Kind::String;
                        initVal.stringValue = strLit->value;
                        fieldValues[propIdent->name] = initVal;
                        std::cerr << "      DEBUG: Stored string literal for field '"
                                 << propIdent->name << "'" << std::endl;
                    } else if (NumberLiteral* numLit = dynamic_cast<NumberLiteral*>(assignExpr->right.get())) {
                        FieldInitValue initVal;
                        initVal.kind = FieldInitValue::Kind::Number;
                        initVal.numberValue = numLit->value;
                        fieldValues[propIdent->name] = initVal;
                        std::cerr << "      DEBUG: Stored number literal for field '"
                                 << propIdent->name << "'" << std::endl;
                    }
                }

                if (!fieldValues.empty()) {
                    classFieldInitialValues_[className] = fieldValues;
                    std::cerr << "    DEBUG: Stored " << fieldValues.size()
                             << " field initial values for " << className << std::endl;
                }
            }
        }

        // Add implicit return of instance if needed
        if (!entryBlock->hasTerminator()) {
            if (instancePtr) {
                std::cerr << "    DEBUG: Adding implicit return of instancePtr: " << instancePtr << std::endl;
                builder_->createReturn(instancePtr);
            } else {
                std::cerr << "    ERROR: instancePtr is NULL in implicit return!" << std::endl;
                // Return 0 as fallback (this should not happen)
                builder_->createReturn(builder_->createIntConstant(0));
            }
        }

        // Restore 'this' context and class struct type
        currentThis_ = savedThis;
        currentClassStructType_ = savedClassStructType;
        currentFunction_ = savedFunction;

        std::cerr << "    DEBUG: Created constructor function: " << funcName << std::endl;
    }

void HIRGenerator::generateDefaultConstructor(const std::string& className,
                                    hir::HIRStructType* structType) {
        // Default constructor function name: ClassName_constructor
        std::string funcName = className + "_constructor";

        // Check if this class has a parent - if so, match parent constructor signature
        std::vector<hir::HIRTypePtr> paramTypes;
        std::string parentClass = "";
        auto inheritIt = classInheritance_.find(className);
        if (inheritIt != classInheritance_.end()) {
            parentClass = inheritIt->second;
            std::cerr << "  DEBUG: Generating default constructor for " << className << " (extends " << parentClass << ")" << std::endl;

            // Look up parent constructor to match its parameters
            std::string parentInitName = parentClass + "_constructor";
            auto parentInit = module_->getFunction(parentInitName);
            if (parentInit) {
                // Match parent constructor parameters (skip first param which is 'this')
                for (size_t i = 1; i < parentInit.get()->parameters.size(); ++i) {
                    paramTypes.push_back(parentInit.get()->parameters[i]->type);
                }
                std::cerr << "  DEBUG: Parent constructor takes " << (parentInit.get()->parameters.size() - 1) << " params" << std::endl;
            } else {
                std::cerr << "  WARNING: Parent constructor " << parentInitName << " not found!" << std::endl;
            }
        }

        // Return type is pointer to struct (use Any for now)
        auto returnType = std::make_shared<hir::HIRType>(hir::HIRType::Kind::Any);

        // Create function type
        auto funcType = new HIRFunctionType(paramTypes, returnType);

        // Create HIR function
        auto func = module_->createFunction(funcName, funcType);

        // Save current function and class context
        HIRFunction* savedFunction = currentFunction_;
        hir::HIRStructType* savedClassStructType = currentClassStructType_;
        currentFunction_ = func.get();
        currentClassStructType_ = structType;

        // Create entry block
        auto entryBlock = func->createBasicBlock("entry");

        // Create builder
        builder_ = std::make_unique<HIRBuilder>(module_, func.get());
        builder_->setInsertPoint(entryBlock.get());

        // Get or create malloc function declaration
        HIRFunction* mallocFunc = nullptr;
        auto& functions = module_->functions;
        for (auto& f : functions) {
            if (f->name == "malloc") {
                mallocFunc = f.get();
                break;
            }
        }

        if (!mallocFunc) {
            std::vector<HIRTypePtr> mallocParamTypes;
            mallocParamTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::I64));
            auto mallocReturnType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            HIRFunctionType* mallocFuncType = new HIRFunctionType(mallocParamTypes, mallocReturnType);
            HIRFunctionPtr mallocFuncPtr = module_->createFunction("malloc", mallocFuncType);
            mallocFuncPtr->linkage = HIRFunction::Linkage::External;
            mallocFunc = mallocFuncPtr.get();
        }

        // Calculate struct size (number of fields * 8 bytes for i64)
        size_t structSize = structType->fields.size() * 8;
        if (structSize == 0) structSize = 8;  // Minimum allocation
        auto sizeValue = builder_->createIntConstant(structSize);

        // Call malloc to allocate memory
        std::vector<HIRValue*> mallocArgs = { sizeValue };
        auto instancePtr = builder_->createCall(mallocFunc, mallocArgs, "instance");

        // Initialize all fields to 0 (including inherited fields)
        // NOTE: Don't automatically call parent constructor - that requires explicit super()
        // and a different architecture (separate _init functions)
        for (size_t i = 0; i < structType->fields.size(); ++i) {
            auto zero = builder_->createIntConstant(0);
            builder_->createSetField(instancePtr, static_cast<uint32_t>(i), zero, structType->fields[i].name);
            if(NOVA_DEBUG) std::cerr << "    DEBUG: Initialized field " << i << " (" << structType->fields[i].name << ") to 0" << std::endl;
        }

        // Return the instance
        builder_->createReturn(instancePtr);

        // Restore context
        currentClassStructType_ = savedClassStructType;
        currentFunction_ = savedFunction;

        std::cerr << "    DEBUG: Created default constructor function: " << funcName << std::endl;
    }

void HIRGenerator::generateMethodFunction(const std::string& className,
                                const ClassDecl::Method& method,
                                hir::HIRStructType* structType,
                                std::function<HIRType::Kind(Type::Kind)> convertTypeKind) {
        // Method function name: ClassName_methodName
        std::string funcName = className + "_" + method.name;

        // Parameter types: 'this' pointer + method parameters
        std::vector<hir::HIRTypePtr> paramTypes;
        // First parameter is 'this' (pointer to struct) - use proper struct type
        paramTypes.push_back(std::make_shared<hir::HIRPointerType>(
            std::shared_ptr<hir::HIRStructType>(structType, [](hir::HIRStructType*){}), true));

        // Add method parameters (use Any for dynamic typing)
        for (size_t i = 0; i < method.params.size(); ++i) {
            paramTypes.push_back(std::make_shared<hir::HIRType>(hir::HIRType::Kind::Any));
        }

        // Return type - infer from return statements if not explicitly typed
        hir::HIRTypePtr returnType;
        if (method.returnType) {
            returnType = std::make_shared<hir::HIRType>(convertTypeKind(method.returnType->kind));
        } else {
            // Infer return type by scanning method body for return statements
            // Use Any as default for dynamic typing (JavaScript is dynamically typed)
            hir::HIRType::Kind inferredType = hir::HIRType::Kind::Any; // default

            if (method.body) {
                // Simple heuristic: check if body is a block with a return statement
                if (auto* block = dynamic_cast<BlockStmt*>(method.body.get())) {
                    for (auto& stmt : block->statements) {
                        if (auto* retStmt = dynamic_cast<ReturnStmt*>(stmt.get())) {
                            if (retStmt->argument) {
                                // Check type of returned expression
                                if (dynamic_cast<StringLiteral*>(retStmt->argument.get())) {
                                    inferredType = hir::HIRType::Kind::String;
                                    std::cerr << "    DEBUG: Inferred method return type as String from return statement" << std::endl;
                                } else if (dynamic_cast<NumberLiteral*>(retStmt->argument.get())) {
                                    inferredType = hir::HIRType::Kind::I64;
                                }
                            }
                            break; // Use first return statement for inference
                        }
                    }
                }
            }

            returnType = std::make_shared<hir::HIRType>(inferredType);
        }

        // Create function type
        auto funcType = new HIRFunctionType(paramTypes, returnType);

        // Create HIR function
        auto func = module_->createFunction(funcName, funcType);

        // Save current function and class context
        HIRFunction* savedFunction = currentFunction_;
        hir::HIRStructType* savedClassStructType = currentClassStructType_;
        currentFunction_ = func.get();
        currentClassStructType_ = structType;  // Track current class struct type

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

        // Infer return type from actual HIR return statements if still Any
        if (func->functionType && func->functionType->returnType &&
            func->functionType->returnType->kind == hir::HIRType::Kind::Any) {
            if(NOVA_DEBUG) std::cerr << "    DEBUG: Method " << method.name << " has Any return type, inferring from HIR..." << std::endl;
            // Scan blocks for return statements to infer type from actual return values
            for (auto& block : func->basicBlocks) {
                for (auto& inst : block->instructions) {
                    if (inst->opcode == HIRInstruction::Opcode::Return && !inst->operands.empty()) {
                        auto retVal = inst->operands[0].get();
                        if (retVal && retVal->type && retVal->type->kind != HIRType::Kind::Void) {
                            func->functionType->returnType = retVal->type;
                            if(NOVA_DEBUG) std::cerr << "    DEBUG: Inferred return type for method " << method.name
                                                      << " from HIR return statement: kind " << static_cast<int>(retVal->type->kind) << std::endl;
                            break;
                        }
                    }
                }
            }
        }

        // Add implicit return if needed
        if (!entryBlock->hasTerminator()) {
            builder_->createReturn(nullptr);
        }

        // Restore 'this' context and class struct type
        currentThis_ = savedThis;
        currentClassStructType_ = savedClassStructType;
        currentFunction_ = savedFunction;

        std::cerr << "    DEBUG: Created method function: " << funcName << std::endl;
    }

void HIRGenerator::generateStaticMethodFunction(const std::string& className,
                                      const ClassDecl::Method& method,
                                      std::function<HIRType::Kind(Type::Kind)> convertTypeKind) {
        // Static method function name: ClassName_methodName
        std::string funcName = className + "_" + method.name;
        
        // Register as static method
        staticMethods_.insert(funcName);

        // Parameter types: NO 'this' pointer for static methods
        std::vector<hir::HIRTypePtr> paramTypes;
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

        // Save current function context
        HIRFunction* savedFunction = currentFunction_;
        currentFunction_ = func.get();

        // Create entry block
        auto entryBlock = func->createBasicBlock("entry");

        // Create builder
        builder_ = std::make_unique<HIRBuilder>(module_, func.get());
        builder_->setInsertPoint(entryBlock.get());

        // Add method parameters to symbol table (starting from index 0, no 'this')
        for (size_t i = 0; i < method.params.size(); ++i) {
            symbolTable_[method.params[i]] = func->parameters[i];
        }

        // Process method body
        if (method.body) {
            method.body->accept(*this);
        }

        // Add implicit return if needed
        if (!entryBlock->hasTerminator()) {
            builder_->createReturn(nullptr);
        }

        // Restore function context
        currentFunction_ = savedFunction;

        std::cerr << "    DEBUG: Created static method function: " << funcName << std::endl;
    }

void HIRGenerator::generateGetterFunction(const std::string& className,
                                const ClassDecl::Method& method,
                                hir::HIRStructType* structType,
                                std::function<HIRType::Kind(Type::Kind)> convertTypeKind) {
        // Getter function name: ClassName_get_propertyName
        std::string funcName = className + "_get_" + method.name;

        // Parameter types: only 'this' pointer
        std::vector<hir::HIRTypePtr> paramTypes;
        paramTypes.push_back(std::make_shared<hir::HIRType>(hir::HIRType::Kind::Any));

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

        // Save current function and class context
        HIRFunction* savedFunction = currentFunction_;
        hir::HIRStructType* savedClassStructType = currentClassStructType_;
        currentFunction_ = func.get();
        currentClassStructType_ = structType;

        // Create entry block
        auto entryBlock = func->createBasicBlock("entry");

        // Create builder
        builder_ = std::make_unique<HIRBuilder>(module_, func.get());
        builder_->setInsertPoint(entryBlock.get());

        // First parameter is 'this'
        symbolTable_["this"] = func->parameters[0];

        // Save current 'this' context
        hir::HIRValue* savedThis = currentThis_;
        currentThis_ = func->parameters[0];

        // Process getter body
        if (method.body) {
            method.body->accept(*this);
        }

        // Add implicit return if needed
        if (!entryBlock->hasTerminator()) {
            builder_->createReturn(nullptr);
        }

        // Restore context
        currentThis_ = savedThis;
        currentClassStructType_ = savedClassStructType;
        currentFunction_ = savedFunction;

        std::cerr << "    DEBUG: Created getter function: " << funcName << std::endl;
    }

void HIRGenerator::generateSetterFunction(const std::string& className,
                                const ClassDecl::Method& method,
                                hir::HIRStructType* structType,
                                [[maybe_unused]] std::function<HIRType::Kind(Type::Kind)> convertTypeKind) {
        // Setter function name: ClassName_set_propertyName
        std::string funcName = className + "_set_" + method.name;

        // Parameter types: 'this' pointer + value parameter
        std::vector<hir::HIRTypePtr> paramTypes;
        paramTypes.push_back(std::make_shared<hir::HIRType>(hir::HIRType::Kind::Any));
        // Add parameter for the value
        if (method.params.size() > 0) {
            paramTypes.push_back(std::make_shared<hir::HIRType>(hir::HIRType::Kind::I64));
        }

        // Setters return void
        auto returnType = std::make_shared<hir::HIRType>(hir::HIRType::Kind::Void);

        // Create function type
        auto funcType = new HIRFunctionType(paramTypes, returnType);

        // Create HIR function
        auto func = module_->createFunction(funcName, funcType);

        // Save current function and class context
        HIRFunction* savedFunction = currentFunction_;
        hir::HIRStructType* savedClassStructType = currentClassStructType_;
        currentFunction_ = func.get();
        currentClassStructType_ = structType;

        // Create entry block
        auto entryBlock = func->createBasicBlock("entry");

        // Create builder
        builder_ = std::make_unique<HIRBuilder>(module_, func.get());
        builder_->setInsertPoint(entryBlock.get());

        // First parameter is 'this'
        symbolTable_["this"] = func->parameters[0];

        // Add value parameter to symbol table
        if (method.params.size() > 0) {
            symbolTable_[method.params[0]] = func->parameters[1];
        }

        // Save current 'this' context
        hir::HIRValue* savedThis = currentThis_;
        currentThis_ = func->parameters[0];

        // Process setter body
        if (method.body) {
            method.body->accept(*this);
        }

        // Add implicit return if needed
        if (!entryBlock->hasTerminator()) {
            builder_->createReturn(nullptr);
        }

        // Restore context
        currentThis_ = savedThis;
        currentClassStructType_ = savedClassStructType;
        currentFunction_ = savedFunction;

        std::cerr << "    DEBUG: Created setter function: " << funcName << std::endl;
    }

// Resolve method to the actual class that implements it (walking inheritance chain)
std::string HIRGenerator::resolveMethodToClass(const std::string& className,
                                                const std::string& methodName) {
    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Resolving method '" << methodName
                              << "' for class '" << className << "'" << std::endl;

    std::string currentClass = className;
    std::unordered_set<std::string> visitedClasses;  // Prevent infinite loops

    // Walk up the inheritance chain
    while (!currentClass.empty()) {
        // Prevent infinite inheritance loops
        if (visitedClasses.count(currentClass) > 0) {
            std::cerr << "ERROR HIRGen: Circular inheritance detected for class '"
                      << currentClass << "'" << std::endl;
            return "";
        }
        visitedClasses.insert(currentClass);

        // Check if this class defines the method
        auto methodsIt = classOwnMethods_.find(currentClass);
        if (methodsIt != classOwnMethods_.end()) {
            if (methodsIt->second.count(methodName) > 0) {
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Found method '" << methodName
                                          << "' in class '" << currentClass << "'" << std::endl;
                return currentClass;
            }
        }

        // Check getters (instance properties with get keyword)
        auto gettersIt = classGetters_.find(currentClass);
        if (gettersIt != classGetters_.end()) {
            if (gettersIt->second.count(methodName) > 0) {
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Found getter '" << methodName
                                          << "' in class '" << currentClass << "'" << std::endl;
                return currentClass;
            }
        }

        // Check setters
        auto settersIt = classSetters_.find(currentClass);
        if (settersIt != classSetters_.end()) {
            if (settersIt->second.count(methodName) > 0) {
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Found setter '" << methodName
                                          << "' in class '" << currentClass << "'" << std::endl;
                return currentClass;
            }
        }

        // Move to parent class
        auto inheritIt = classInheritance_.find(currentClass);
        if (inheritIt != classInheritance_.end()) {
            currentClass = inheritIt->second;
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Moving up to parent class '"
                                      << currentClass << "'" << std::endl;
        } else {
            // No parent class, method not found
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: No parent class found, method '"
                                      << methodName << "' does not exist" << std::endl;
            break;
        }
    }

    // Method not found in inheritance chain
    return "";
}

} // namespace nova::hir
