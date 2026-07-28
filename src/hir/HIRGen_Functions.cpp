// HIRGen_Functions.cpp - Function expression and declaration visitors
// Extracted from HIRGen.cpp for better code organization

#include "nova/HIR/HIRGen_Internal.h"
#include <algorithm>
#define NOVA_DEBUG 0

namespace nova::hir {

void HIRGenerator::visit(FunctionExpr& node) {
        const bool savedFunctionIsArrow = currentFunctionIsArrow_;
        HIRValue* savedCurrentThis = currentThis_;
        const bool savedOrdinaryFunctionUsesThis =
            currentOrdinaryFunctionUsesThis_;
        currentFunctionIsArrow_ = false;
        currentOrdinaryFunctionUsesThis_ = false;
        const auto savedDynamicBindingNames = dynamicBindingNames_;
        const auto savedDynamicObjectVars = dynamicObjectVars_;
        const auto savedForcedDynamic = forcedDynamicObjectVars_;
        dynamicObjectVars_.clear();
        if (node.body) {
            forcedDynamicObjectVars_ = scanForcedDynamicObjects(node.body.get());
            for (const auto& forced : forcedDynamicObjectVars_) {
                dynamicObjectVars_.insert(forced);
            }
        }
        dynamicBindingNames_ = analyzeDynamicBindings(node.body.get());
        // Function expression: let f = function(a, b) { return a + b; }

        // Helper to convert AST Type::Kind to HIR HIRType::Kind
        auto convertTypeKind = [](Type::Kind astKind) -> HIRType::Kind {
            switch (astKind) {
                case Type::Kind::Void: return HIRType::Kind::Void;
                case Type::Kind::Number: return HIRType::Kind::I64;
                case Type::Kind::String: return HIRType::Kind::String;
                case Type::Kind::Boolean: return HIRType::Kind::Bool;
                case Type::Kind::BigInt:
                case Type::Kind::Symbol: return HIRType::Kind::Pointer;
                case Type::Kind::Any: return HIRType::Kind::Any;
                case Type::Kind::Union: return HIRType::Kind::JSValue;
                default: return HIRType::Kind::Any;
            }
        };

        // Create function type with parameter types
        std::vector<HIRTypePtr> paramTypes;
        const bool supportsDynamicThis = !node.isGenerator &&
            !forcePromiseExecutorABI_ && !forceTaggedFunctionABI_;
        if (supportsDynamicThis) {
            paramTypes.push_back(
                std::make_shared<HIRType>(HIRType::Kind::JSValue));
        }
        for (size_t i = 0; i < node.params.size(); ++i) {
            if (i < node.paramPatterns.size() && node.paramPatterns[i]) {
                appendPatternParameterTypes(
                    node.paramPatterns[i].get(), paramTypes);
                continue;
            }
            HIRType::Kind typeKind = HIRType::Kind::JSValue;
            if (!forceTaggedFunctionABI_ && !forcePromiseExecutorABI_ &&
                i < node.paramTypes.size() && node.paramTypes[i]) {
                typeKind = convertTypeKind(node.paramTypes[i]->kind);
            }
            paramTypes.push_back(std::make_shared<HIRType>(typeKind));
        }
        if (!node.restParam.empty()) {
            auto restElementType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
            auto restArrayType = std::make_shared<HIRArrayType>(restElementType, 0);
            paramTypes.push_back(std::make_shared<HIRPointerType>(restArrayType, true));
        }

        // Return type
        HIRType::Kind retTypeKind = HIRType::Kind::Any;
        if (forcePromiseExecutorABI_) {
            retTypeKind = HIRType::Kind::Void;
        } else if (forceTaggedFunctionABI_) {
            retTypeKind = HIRType::Kind::JSValue;
        } else if (node.isAsync && !node.isGenerator) {
            retTypeKind = HIRType::Kind::Pointer;
        } else if (node.returnType) {
            retTypeKind = convertTypeKind(node.returnType->kind);
        } else if (!node.isGenerator && hasHeterogeneousReturns(node.body.get())) {
            retTypeKind = HIRType::Kind::JSValue;
        }
        auto retType = std::make_shared<HIRType>(retTypeKind);

        auto funcType = new HIRFunctionType(paramTypes, retType);

        // Generate unique name for function expression
        static int funcExprCounter = 0;
        std::string funcName = node.name.empty() ?
            "__func_" + std::to_string(funcExprCounter++) : node.name;
        functionParamCounts_[funcName] =
            static_cast<int64_t>(node.params.size());
        if (!node.restParam.empty()) {
            module_->functionRestParams[funcName] = {
                node.restParam, node.params.size()};
        }

        // Create function
        auto func = module_->createFunction(funcName, funcType);
        HIRParameter* tentativeThisParam = nullptr;
        if (supportsDynamicThis && !func->parameters.empty()) {
            tentativeThisParam = func->parameters.front();
            tentativeThisParam->name = "__this";
        }
        functionParameterPatterns_[funcName] = node.paramPatterns;
        if (!node.defaultValues.empty()) {
            functionDefaultValues_[funcName] = &node.defaultValues;
        }
        func->isAsync = node.isAsync;
        func->isGenerator = node.isGenerator;

        // Save current function context
        HIRFunction* savedFunction = currentFunction_;
        currentFunction_ = func.get();
        currentThis_ = tentativeThisParam;

        // Create entry block
        auto entryBlock = func->createBasicBlock("entry");

        // Save current builder
        auto savedBuilder = std::move(builder_);
        builder_ = std::make_unique<HIRBuilder>(module_, func.get());
        builder_->setInsertPoint(entryBlock.get());

        // Save current symbol table and push to scope stack for closure support
        auto savedSymbolTable = symbolTable_;
        scopeStack_.push_back(savedSymbolTable);

        // Save and set function name for closure tracking (must be before body generation)
        auto savedFunctionName = lastFunctionName_;
        lastFunctionName_ = funcName;

        // Add tentative __env parameter BEFORE body generation for closure support
        // This allows the Identifier visitor to create GetField instructions during body generation
        HIRParameter* tentativeEnvParam = nullptr;
        if (!savedSymbolTable.empty()) {
            // Only add __env if we're in a nested function (have parent scope)
            // Create temporary empty struct type
            auto tempEnvStruct = new HIRStructType("__temp_env_" + funcName, {});
            std::shared_ptr<HIRType> tempEnvType(tempEnvStruct);
            tentativeEnvParam = new HIRParameter(tempEnvType, "__env", func->parameters.size());
            func->parameters.push_back(tentativeEnvParam);
            func->functionType->paramTypes.push_back(tempEnvType);
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: FunctionExpr - Added tentative __env parameter to '"
                                      << funcName << "' before body generation" << std::endl;
        }

        // Clear symbol table for the new function scope
        symbolTable_.clear();
        if (tentativeThisParam) {
            symbolTable_["this"] = tentativeThisParam;
        }

        // Add parameters to symbol table (skip tentative __env)
        size_t parameterCursor = tentativeThisParam ? 1 : 0;
        for (size_t i = 0; i < node.params.size(); ++i) {
            if (i < node.paramPatterns.size() && node.paramPatterns[i]) {
                Expr* parameterDefault = i < node.defaultValues.size()
                    ? node.defaultValues[i].get() : nullptr;
                bindPatternParameters(
                    node.paramPatterns[i].get(), func->parameters,
                    parameterCursor, parameterDefault);
            } else if (parameterCursor < func->parameters.size()) {
                symbolTable_[node.params[i]] =
                    func->parameters[parameterCursor++];
            }
        }
        if (!node.restParam.empty() &&
            parameterCursor < func->parameters.size()) {
            symbolTable_[node.restParam] = func->parameters[parameterCursor];
        }

        if (symbolTable_.count("arguments") == 0 &&
            std::none_of(node.paramPatterns.begin(), node.paramPatterns.end(),
                [](const auto& pattern) { return pattern != nullptr; })) {
            std::vector<HIRValue*> argumentValues;
            const size_t argumentCount = std::min(
                node.params.size(),
                func->parameters.size() - (tentativeThisParam ? 1 : 0));
            argumentValues.reserve(argumentCount);
            for (size_t i = 0; i < argumentCount; ++i) {
                argumentValues.push_back(
                    func->parameters[i + (tentativeThisParam ? 1 : 0)]);
            }
            symbolTable_["arguments"] = builder_->createArrayConstruct(
                argumentValues, "arguments");
        }

        // Generate function body
        if (node.body) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Generating function body for " << funcName << std::endl;
            node.body->accept(*this);
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Function body generated, checking for terminator..." << std::endl;

            if (retTypeKind == HIRType::Kind::Any) {
                for (auto& block : func->basicBlocks) {
                    auto terminator = block->getTerminator();
                    if (terminator && terminator->opcode == HIRInstruction::Opcode::Return &&
                        !terminator->operands.empty() && terminator->operands[0]->type) {
                        func->functionType->returnType = terminator->operands[0]->type;
                        break;
                    }
                }
            }

            // Add implicit return if needed
            if (!entryBlock->hasTerminator()) {
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Adding implicit return to " << funcName << std::endl;
                builder_->createReturn(node.isAsync
                    ? createResolvedPromise(nullptr) : nullptr);
            } else {
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Function " << funcName << " already has terminator" << std::endl;
            }
        } else {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: WARNING - No body for function " << funcName << std::endl;
        }

        propagateTransitiveCaptures(
            savedFunctionName, savedSymbolTable, funcName);

        // After body generation, update closure environment struct type if this function captures variables
        if (tentativeEnvParam && capturedVariables_.count(funcName) && !capturedVariables_[funcName].empty()) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: FunctionExpr - Updating __env struct type for '"
                                      << funcName << "' (captured " << capturedVariables_[funcName].size()
                                      << " variables)" << std::endl;

            // Create closure environment struct with actual captured variables
            auto* envStruct = createClosureEnvironment(funcName);
            if (envStruct) {
                // Update the tentative __env parameter's type with the real struct
                auto envPtrType = std::make_shared<HIRPointerType>(
                    std::shared_ptr<HIRType>(envStruct),
                    true  // mutable
                );
                tentativeEnvParam->type = envPtrType;

                // Update function type as well
                func->functionType->paramTypes.back() = envPtrType;

                closureEnvironments_[funcName] = envStruct;
                module_->closureEnvironments[funcName] = envStruct;
                if (environmentFieldNames_.count(funcName)) {
                    module_->closureCapturedVars[funcName] = environmentFieldNames_[funcName];
                }
                if (environmentFieldValues_.count(funcName)) {
                    module_->closureCapturedVarValues[funcName] = environmentFieldValues_[funcName];
                }

                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: FunctionExpr - Updated __env parameter type for '"
                                          << funcName << "' with " << envStruct->fields.size() << " fields" << std::endl;

                // IMPORTANT: Mark this function as returning a closure
                // This tells MIRGen that calls to this function return closure pointers
                // The parent function (caller) should be tracked
                if (savedFunction) {
                    module_->closureReturnedBy[savedFunction->name] = funcName;
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Marked that '" << savedFunction->name
                                             << "' returns closure '" << funcName << "'" << std::endl;
                }
            }
        } else if (tentativeEnvParam && (!capturedVariables_.count(funcName) || capturedVariables_[funcName].empty())) {
            // No variables captured, remove the tentative __env parameter
            if (!func->parameters.empty() && func->parameters.back() == tentativeEnvParam) {
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: FunctionExpr - Removing unused __env parameter from '"
                                          << funcName << "'" << std::endl;
                func->parameters.pop_back();
                func->functionType->paramTypes.pop_back();
                delete tentativeEnvParam;
            }
        }

        if (tentativeThisParam) {
            if (currentOrdinaryFunctionUsesThis_) {
                dynamicThisFunctions_.insert(funcName);
            } else if (!func->parameters.empty() &&
                       func->parameters.front() == tentativeThisParam) {
                func->parameters.erase(func->parameters.begin());
                func->functionType->paramTypes.erase(
                    func->functionType->paramTypes.begin());
                delete tentativeThisParam;
                for (size_t i = 0; i < func->parameters.size(); ++i) {
                    func->parameters[i]->index = static_cast<uint32_t>(i);
                }
            }
        }

        // Restore context
        scopeStack_.pop_back();
        symbolTable_ = savedSymbolTable;
        builder_ = std::move(savedBuilder);
        currentFunction_ = savedFunction;
        currentThis_ = savedCurrentThis;

        // Restore function name (but keep funcName available for variable association)
        // Note: We keep the current funcName in lastFunctionName_ so it can be associated with a variable
        // The savedFunctionName will be used for the parent function's context
        lastFunctionName_ = funcName;  // Keep current for variable association
        // savedFunctionName is not restored here because we want the inner function name to persist

        // Return a string constant with the function name
        // This will be used by MIRGen to identify the function and allocate closure if needed
        lastValue_ = builder_->createStringConstant(funcName);
        dynamicBindingNames_ = savedDynamicBindingNames;
        dynamicObjectVars_ = savedDynamicObjectVars;
        forcedDynamicObjectVars_ = savedForcedDynamic;
        currentFunctionIsArrow_ = savedFunctionIsArrow;
        currentOrdinaryFunctionUsesThis_ = savedOrdinaryFunctionUsesThis;
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Function " << funcName << " reference created" << std::endl;
    }
    

void HIRGenerator::visit(ArrowFunctionExpr& node) {
        const bool savedFunctionIsArrow = currentFunctionIsArrow_;
        currentFunctionIsArrow_ = true;
        const auto savedDynamicBindingNames = dynamicBindingNames_;
        const auto savedDynamicObjectVars = dynamicObjectVars_;
        const auto savedForcedDynamic = forcedDynamicObjectVars_;
        dynamicObjectVars_.clear();
        if (node.body) {
            forcedDynamicObjectVars_ = scanForcedDynamicObjects(node.body.get());
            for (const auto& forced : forcedDynamicObjectVars_) {
                dynamicObjectVars_.insert(forced);
            }
        }
        dynamicBindingNames_ = analyzeDynamicBindings(node.body.get());
        // Arrow function: (a, b) => a + b
        // For now, treat as anonymous function with auto-generated name

        // Helper to convert AST Type::Kind to HIR HIRType::Kind
        auto convertTypeKind = [](Type::Kind astKind) -> HIRType::Kind {
            switch (astKind) {
                case Type::Kind::Void: return HIRType::Kind::Void;
                case Type::Kind::Number: return HIRType::Kind::I64;
                case Type::Kind::String: return HIRType::Kind::String;
                case Type::Kind::Boolean: return HIRType::Kind::Bool;
                case Type::Kind::BigInt:
                case Type::Kind::Symbol: return HIRType::Kind::Pointer;
                case Type::Kind::Any: return HIRType::Kind::Any;
                case Type::Kind::Union: return HIRType::Kind::JSValue;
                default: return HIRType::Kind::Any;
            }
        };

        // Create function type with parameter types
        std::vector<HIRTypePtr> paramTypes;
        for (size_t i = 0; i < node.params.size(); ++i) {
            if (i < node.paramPatterns.size() && node.paramPatterns[i]) {
                appendPatternParameterTypes(
                    node.paramPatterns[i].get(), paramTypes);
                continue;
            }
            HIRType::Kind typeKind = HIRType::Kind::JSValue;
            if (!forceTaggedFunctionABI_ && !forcePromiseExecutorABI_ &&
                i < node.paramTypes.size() && node.paramTypes[i]) {
                typeKind = convertTypeKind(node.paramTypes[i]->kind);
            }
            paramTypes.push_back(std::make_shared<HIRType>(typeKind));
        }

        if (!node.restParam.empty()) {
            auto restElementType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
            auto restArrayType = std::make_shared<HIRArrayType>(restElementType, 0);
            paramTypes.push_back(std::make_shared<HIRPointerType>(restArrayType, true));
        }

        // Return type
        HIRType::Kind retTypeKind = HIRType::Kind::Any;
        if (forcePromiseExecutorABI_) {
            retTypeKind = HIRType::Kind::Void;
        } else if (forceTaggedFunctionABI_) {
            retTypeKind = HIRType::Kind::JSValue;
        } else if (node.isAsync) {
            retTypeKind = HIRType::Kind::Pointer;
        } else if (node.returnType) {
            retTypeKind = convertTypeKind(node.returnType->kind);
        } else if (hasHeterogeneousReturns(node.body.get())) {
            retTypeKind = HIRType::Kind::JSValue;
        }
        auto retType = std::make_shared<HIRType>(retTypeKind);

        auto funcType = new HIRFunctionType(paramTypes, retType);

        // Generate unique name for arrow function
        static int arrowFuncCounter = 0;
        std::string funcName = "__arrow_" + std::to_string(arrowFuncCounter++);
        functionParamCounts_[funcName] =
            static_cast<int64_t>(node.params.size());
        if (!node.restParam.empty()) {
            module_->functionRestParams[funcName] = {
                node.restParam, node.params.size()};
        }

        // Create function
        auto func = module_->createFunction(funcName, funcType);
        functionParameterPatterns_[funcName] = node.paramPatterns;
        if (!node.defaultValues.empty()) {
            functionDefaultValues_[funcName] = &node.defaultValues;
        }
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

        // Save and set function name for closure tracking (must be before body generation)
        auto savedFunctionName = lastFunctionName_;
        lastFunctionName_ = funcName;

        // Save current symbol table and push to scope stack for closure support
        auto savedSymbolTable = symbolTable_;
        scopeStack_.push_back(savedSymbolTable);  // Push for closure access

        HIRParameter* tentativeEnvParam = nullptr;
        if (!savedSymbolTable.empty()) {
            auto tempEnvStruct = new HIRStructType("__temp_env_" + funcName, {});
            std::shared_ptr<HIRType> tempEnvType(tempEnvStruct);
            tentativeEnvParam = new HIRParameter(
                tempEnvType, "__env", func->parameters.size());
            func->parameters.push_back(tentativeEnvParam);
            func->functionType->paramTypes.push_back(tempEnvType);
        }

        // Clear symbol table for the new function scope
        symbolTable_.clear();

        // Add parameters to symbol table
        size_t parameterCursor = 0;
        for (size_t i = 0; i < node.params.size(); ++i) {
            if (i < node.paramPatterns.size() && node.paramPatterns[i]) {
                Expr* parameterDefault = i < node.defaultValues.size()
                    ? node.defaultValues[i].get() : nullptr;
                bindPatternParameters(
                    node.paramPatterns[i].get(), func->parameters,
                    parameterCursor, parameterDefault);
                continue;
            }
            if (parameterCursor >= func->parameters.size()) break;
            symbolTable_[node.params[i]] = func->parameters[parameterCursor++];
            if (i < node.paramTypes.size() && node.paramTypes[i] &&
                (node.paramTypes[i]->kind == Type::Kind::Array ||
                 node.paramTypes[i]->kind == Type::Kind::Tuple)) {
                runtimeArrayVars_.insert(node.params[i]);
                if (forceTaggedFunctionABI_) {
                    taggedRuntimeArrayVars_.insert(node.params[i]);
                }
            }
        }

        if (!node.restParam.empty() &&
            parameterCursor < func->parameters.size()) {
            symbolTable_[node.restParam] = func->parameters[parameterCursor];
        }

        // Generate function body
        if (node.body) {
            // Check if body is an expression statement (implicit return)
            if (auto* exprStmt = dynamic_cast<ExprStmt*>(node.body.get())) {
                // Arrow function with expression body: x => x + 1
                // This should return the expression value
                exprStmt->expression->accept(*this);

                // Infer return type from actual returned value
                if (lastValue_ && lastValue_->type && retTypeKind == HIRType::Kind::Any) {
                    // Update function return type based on actual return value
                    // SPECIAL CASE: Convert Bool to I64 for callback compatibility
                    // JavaScript represents booleans as numbers (0/1), and C++ runtime
                    // callbacks expect int64_t returns, not bool
                    if (lastValue_->type->kind == HIRType::Kind::Bool) {
                        func->functionType->returnType = std::make_shared<HIRType>(HIRType::Kind::I64);
                        if(NOVA_DEBUG) std::cerr << "DEBUG: Arrow function bool return converted to I64 for callback compatibility" << std::endl;
                    } else {
                        func->functionType->returnType = lastValue_->type;
                        if(NOVA_DEBUG) std::cerr << "DEBUG: Arrow function inferred return type from expression: "
                                                  << static_cast<int>(lastValue_->type->kind) << std::endl;
                    }
                }

                builder_->createReturn(node.isAsync
                    ? createResolvedPromise(lastValue_) : lastValue_);
            } else {
                // Arrow function with block body: x => { return x + 1; }
                node.body->accept(*this);

                // Infer return type from explicit return statements in block body
                if (retTypeKind == HIRType::Kind::Any) {
                    // Scan blocks for return statements to infer type
                    for (auto& block : func->basicBlocks) {
                        auto terminator = block->getTerminator();
                        if (terminator && terminator->opcode == HIRInstruction::Opcode::Return) {
                            // Return instruction's first operand is the return value
                            if (!terminator->operands.empty() && terminator->operands[0]->type) {
                                // SPECIAL CASE: Convert Bool to I64 for callback compatibility
                                if (terminator->operands[0]->type->kind == HIRType::Kind::Bool) {
                                    func->functionType->returnType = std::make_shared<HIRType>(HIRType::Kind::I64);
                                    if(NOVA_DEBUG) std::cerr << "DEBUG: Arrow function bool return converted to I64 for callback compatibility" << std::endl;
                                } else {
                                    func->functionType->returnType = terminator->operands[0]->type;
                                    if(NOVA_DEBUG) std::cerr << "DEBUG: Arrow function inferred return type from block return: "
                                                              << static_cast<int>(terminator->operands[0]->type->kind) << std::endl;
                                }
                                break;  // Use first return statement's type
                            }
                        }
                    }
                }

                // Add implicit return to ALL blocks that don't have terminators
                // This is needed because closure variable access may create additional blocks
                if(NOVA_DEBUG) std::cerr << "DEBUG: Arrow function has " << func->basicBlocks.size() << " blocks" << std::endl;
                for (auto& block : func->basicBlocks) {
                    if(NOVA_DEBUG) std::cerr << "DEBUG: Block '" << block->label << "' hasTerminator=" << block->hasTerminator() << std::endl;
                    if (!block->hasTerminator()) {
                        // Set insert point to this block
                        builder_->setInsertPoint(block.get());
                        builder_->createReturn(node.isAsync
                            ? createResolvedPromise(nullptr) : nullptr);
                        if(NOVA_DEBUG) std::cerr << "DEBUG: Added return terminator to block '" << block->label << "'" << std::endl;
                    }
                }
            }
        }

        propagateTransitiveCaptures(
            savedFunctionName, savedSymbolTable, funcName);

        if (tentativeEnvParam && capturedVariables_.count(funcName) &&
            !capturedVariables_[funcName].empty()) {
            if (auto* envStruct = createClosureEnvironment(funcName)) {
                auto envPtrType = std::make_shared<HIRPointerType>(
                    std::shared_ptr<HIRType>(envStruct), true);
                tentativeEnvParam->type = envPtrType;
                func->functionType->paramTypes.back() = envPtrType;
                closureEnvironments_[funcName] = envStruct;
                module_->closureEnvironments[funcName] = envStruct;
                if (environmentFieldNames_.count(funcName)) {
                    module_->closureCapturedVars[funcName] =
                        environmentFieldNames_[funcName];
                }
                if (environmentFieldValues_.count(funcName)) {
                    module_->closureCapturedVarValues[funcName] =
                        environmentFieldValues_[funcName];
                }
                if (savedFunction) {
                    module_->closureReturnedBy[savedFunction->name] = funcName;
                }
            }
        } else if (tentativeEnvParam) {
            if (!func->parameters.empty() &&
                func->parameters.back() == tentativeEnvParam) {
                func->parameters.pop_back();
                func->functionType->paramTypes.pop_back();
                delete tentativeEnvParam;
            }
        }

        // Restore context
        scopeStack_.pop_back();  // Pop closure scope
        symbolTable_ = savedSymbolTable;
        builder_ = std::move(savedBuilder);
        currentFunction_ = savedFunction;

        // Restore function name
        lastFunctionName_ = funcName;  // Keep current for variable association

        // Return a string constant with the function name
        // This will be used by MIRGen to identify the function and allocate closure if needed
        lastValue_ = builder_->createStringConstant(funcName);
        dynamicBindingNames_ = savedDynamicBindingNames;
        dynamicObjectVars_ = savedDynamicObjectVars;
        forcedDynamicObjectVars_ = savedForcedDynamic;
        currentFunctionIsArrow_ = savedFunctionIsArrow;
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Created arrow function '" << funcName << "' with "
                  << node.params.size() << " parameters" << std::endl;
    }
    

void HIRGenerator::visit(FunctionDecl& node) {
        // Ambient declarations and overload signatures describe types only;
        // only the implementation signature owns executable code.
        if (node.isDeclare || node.isOverload || !node.body) {
            return;
        }
        const bool savedFunctionIsArrow = currentFunctionIsArrow_;
        HIRValue* savedCurrentThis = currentThis_;
        const bool savedOrdinaryFunctionUsesThis =
            currentOrdinaryFunctionUsesThis_;
        currentFunctionIsArrow_ = false;
        currentOrdinaryFunctionUsesThis_ = false;
        const auto savedDynamicBindingNames = dynamicBindingNames_;
        const auto savedDynamicObjectVars = dynamicObjectVars_;
        const auto savedForcedDynamic = forcedDynamicObjectVars_;
        dynamicObjectVars_.clear();
        forcedDynamicObjectVars_ = scanForcedDynamicObjects(node.body.get());
        for (const auto& forced : forcedDynamicObjectVars_) {
            dynamicObjectVars_.insert(forced);
        }
        dynamicBindingNames_ = analyzeDynamicBindings(node.body.get());

        // Helper to convert AST Type::Kind to HIR HIRType::Kind
        auto convertTypeKind = [](Type::Kind astKind) -> HIRType::Kind {
            switch (astKind) {
                case Type::Kind::Void: return HIRType::Kind::Void;
                case Type::Kind::Number: return HIRType::Kind::I64;  // Default to i64 for numbers
                case Type::Kind::String: return HIRType::Kind::String;
                case Type::Kind::Boolean: return HIRType::Kind::Bool;
                case Type::Kind::BigInt:
                case Type::Kind::Symbol: return HIRType::Kind::Pointer;
                case Type::Kind::Any: return HIRType::Kind::Any;
                case Type::Kind::Unknown: return HIRType::Kind::Unknown;
                case Type::Kind::Never: return HIRType::Kind::Never;
                case Type::Kind::Null: return HIRType::Kind::Any;  // Map to Any for now
                case Type::Kind::Undefined: return HIRType::Kind::Any;  // Map to Any for now
                case Type::Kind::Union: return HIRType::Kind::JSValue;
                default: return HIRType::Kind::Any;
            }
        };

        // Create function type with actual parameter types
        std::vector<HIRTypePtr> paramTypes;

        const bool supportsDynamicThis = !node.isGenerator;
        if (supportsDynamicThis) {
            paramTypes.push_back(
                std::make_shared<HIRType>(HIRType::Kind::JSValue));
        }

        // For generator functions, add implicit genPtr and input parameters
        if (node.isGenerator) {
            paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::Pointer));  // genPtr
            paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::I64));      // input
        }

        // Generator bodies are resumed by nova_generator_next through the
        // fixed (genPtr, input) ABI. Source arguments live in generator local
        // slots populated when the generator object is created.
        if (!node.isGenerator) {
            for (size_t i = 0; i < node.params.size(); ++i) {
                if (i < node.paramPatterns.size() && node.paramPatterns[i]) {
                    appendPatternParameterTypes(
                        node.paramPatterns[i].get(), paramTypes);
                    continue;
                }

                if (i < node.paramTypes.size() && node.paramTypes[i] &&
                    node.paramTypes[i]->kind == Type::Kind::Array) {
                    HIRType::Kind elementKind = HIRType::Kind::JSValue;
                    if (node.paramTypes[i]->elementType) {
                        elementKind = convertTypeKind(
                            node.paramTypes[i]->elementType->kind);
                        if (elementKind == HIRType::Kind::Any ||
                            elementKind == HIRType::Kind::Unknown) {
                            elementKind = HIRType::Kind::JSValue;
                        }
                    }
                    auto elementType =
                        std::make_shared<HIRType>(elementKind);
                    auto arrayType =
                        std::make_shared<HIRArrayType>(elementType, 0);
                    paramTypes.push_back(
                        std::make_shared<HIRPointerType>(arrayType, true));
                    continue;
                }

                HIRType::Kind typeKind = HIRType::Kind::I64;

                if (i < node.paramTypes.size() && node.paramTypes[i]) {
                    typeKind = convertTypeKind(node.paramTypes[i]->kind);
                } else if (auto inferred =
                               inferredFunctionParameterTypes_.find(node.name);
                           inferred != inferredFunctionParameterTypes_.end() &&
                           i < inferred->second.size()) {
                    typeKind = inferred->second[i];
                }

                paramTypes.push_back(std::make_shared<HIRType>(typeKind));
            }
        }

        // Lower a JavaScript rest parameter as one explicit trailing array
        // parameter. Call sites package all excess arguments into this array,
        // avoiding a platform-specific C varargs ABI while preserving the
        // JavaScript-visible rest value.
        if (!node.isGenerator && !node.restParam.empty()) {
            auto restElementType = std::make_shared<HIRType>(
                HIRType::Kind::JSValue);
            auto restArrayType = std::make_shared<HIRArrayType>(
                restElementType, 0);
            paramTypes.push_back(std::make_shared<HIRPointerType>(
                restArrayType, true));
        }

        // Use return type annotation if available
        HIRType::Kind retTypeKind = HIRType::Kind::Any;  // Default to Any
        if (node.isAsync && !node.isGenerator) {
            retTypeKind = HIRType::Kind::Pointer;
        } else if (node.returnType) {
            retTypeKind = convertTypeKind(node.returnType->kind);
        } else if (!node.isGenerator && hasHeterogeneousReturns(node.body.get())) {
            retTypeKind = HIRType::Kind::JSValue;
        }
        auto retType = std::make_shared<HIRType>(retTypeKind);

        auto funcType = new HIRFunctionType(paramTypes, retType);
        
        // Create function
        auto func = module_->createFunction(node.name, funcType);
        HIRParameter* tentativeThisParam = nullptr;
        if (supportsDynamicThis && !func->parameters.empty()) {
            tentativeThisParam = func->parameters.front();
            tentativeThisParam->name = "__this";
        }
        functionParameterPatterns_[node.name] = node.paramPatterns;
        func->isAsync = node.isAsync;
        func->isGenerator = node.isGenerator;

        // Track generator functions
        if (node.isGenerator && node.isAsync) {
            // AsyncGenerator (ES2018) - async function*
            asyncGeneratorFuncs_.insert(node.name);
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered AsyncGenerator function: " << node.name << std::endl;
        } else if (node.isGenerator) {
            // Regular Generator (ES2015) - function*
            generatorFuncs_.insert(node.name);
        } else if (node.isAsync) {
            asyncFuncs_.insert(node.name);
        }

        // Track all functions for call/apply/bind support
        functionVars_.insert(node.name);
        functionParamCounts_[node.name] = static_cast<int64_t>(node.params.size());
        // Mark this name as a Function-typed value for instanceof resolution
        functionReferences_[node.name] = node.name;
        variableKinds_[node.name] = "Function";
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered function: " << node.name << " with " << node.params.size() << " params" << std::endl;

        // Track rest parameters
        if (!node.restParam.empty()) {
            module_->functionRestParams[node.name] = {node.restParam, node.params.size()};
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered rest parameter '" << node.restParam
                                      << "' for function '" << node.name << "' after " << node.params.size() << " params" << std::endl;
        }

        auto savedCurrentFunction = currentFunction_;
        currentFunction_ = func.get();
        currentThis_ = tentativeThisParam;

        // Store default parameter values for this function
        if (!node.defaultValues.empty()) {
            functionDefaultValues_[node.name] = &node.defaultValues;
        }

        // Create entry block
        auto entryBlock = func->createBasicBlock("entry");

        // Save current builder for nested functions
        auto savedBuilder = std::move(builder_);
        builder_ = std::make_unique<HIRBuilder>(module_, func.get());
        builder_->setInsertPoint(entryBlock.get());

        // Save and set function name for closure tracking (must be before body generation)
        auto savedFunctionName = lastFunctionName_;
        lastFunctionName_ = node.name;

        // Save current symbol table first (needed for checking if we're in nested function)
        auto savedSymbolTable = symbolTable_;

        // Add tentative __env parameter BEFORE body generation for closure support
        // We'll populate the struct type after body generation when we know which variables are captured
        // This allows the Identifier visitor to create GetField instructions during body generation
        HIRParameter* tentativeEnvParam = nullptr;
        if (!savedSymbolTable.empty()) {
            // Only add __env if we're in a nested function (have parent scope)
            // Create temporary empty struct type
            auto tempEnvStruct = new HIRStructType("__temp_env_" + node.name, {});
            std::shared_ptr<HIRType> tempEnvType(tempEnvStruct);
            tentativeEnvParam = new HIRParameter(tempEnvType, "__env", func->parameters.size());
            func->parameters.push_back(tentativeEnvParam);
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Added tentative __env parameter to function '"
                                      << node.name << "' before body generation" << std::endl;
        }

        // Push to scope stack for closure support
        if (!savedSymbolTable.empty()) {
            scopeStack_.push_back(savedSymbolTable);  // Push for closure access
        }

        // Clear symbol table for the new function scope
        symbolTable_.clear();
        if (tentativeThisParam) {
            symbolTable_["this"] = tentativeThisParam;
        }

        // Add parameters to symbol table
        // For generators, parameters are loaded from local slots (set at call site)
        size_t parameterCursor = node.isGenerator ? 2 :
            (tentativeThisParam ? 1 : 0);
        if (!node.isGenerator) {
            for (size_t i = 0; i < node.params.size(); ++i) {
                if (i < node.paramPatterns.size() && node.paramPatterns[i]) {
                    Expr* parameterDefault = i < node.defaultValues.size()
                        ? node.defaultValues[i].get() : nullptr;
                    bindPatternParameters(
                        node.paramPatterns[i].get(), func->parameters,
                        parameterCursor, parameterDefault);
                } else if (parameterCursor < func->parameters.size()) {
                    symbolTable_[node.params[i]] =
                        func->parameters[parameterCursor++];
                }
            }
        }
        // For generators, parameter loading happens after state machine setup

        // Handle rest parameter (...args). The trailing function parameter is
        // the materialized array created by the call-site lowering.
        if (!node.restParam.empty()) {
            const size_t restIndex = parameterCursor;
            if (restIndex < func->parameters.size()) {
                symbolTable_[node.restParam] = func->parameters[restIndex];
            }
        }

        if (!node.isGenerator && symbolTable_.count("arguments") == 0 &&
            std::none_of(node.paramPatterns.begin(), node.paramPatterns.end(),
                [](const auto& pattern) { return pattern != nullptr; })) {
            std::vector<HIRValue*> argumentValues;
            const size_t argumentCount = std::min(
                node.params.size(),
                func->parameters.size() - (tentativeThisParam ? 1 : 0));
            argumentValues.reserve(argumentCount);
            for (size_t i = 0; i < argumentCount; ++i) {
                argumentValues.push_back(
                    func->parameters[i + (tentativeThisParam ? 1 : 0)]);
            }
            symbolTable_["arguments"] = builder_->createArrayConstruct(
                argumentValues, "arguments");
        }

        // For generator functions, set up state machine
        if (node.isGenerator) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Setting up generator state machine for " << node.name << std::endl;

            // Reset state machine variables for this generator
            yieldStateCounter_ = 0;
            yieldResumeBlocks_.clear();
            generatorBodyBlock_ = nullptr;
            currentSetStateFunc_ = nullptr;
            generatorVarSlots_.clear();
            generatorNextLocalSlot_ = 0;
            generatorStoreLocalFunc_ = nullptr;
            generatorLoadLocalFunc_ = nullptr;

            // Generator function receives (genPtr, input) as implicit first two parameters
            auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
            // Use proper HIRPointerType for generator pointer (pointer to opaque void)
            auto ptrType = std::make_shared<HIRPointerType>(voidType, false);

            // Create a local to store genPtr - need pointer-to-pointer type for alloca
            auto ptrToPtrType = std::make_shared<HIRPointerType>(ptrType, false);
            auto* genPtrVar = builder_->createAlloca(ptrToPtrType.get(), "__genPtr");

            // Store genPtr (from first parameter) for later use
            if (!func->parameters.empty()) {
                builder_->createStore(func->parameters[0], genPtrVar);
                currentGeneratorPtr_ = genPtrVar;
            }

            // Get or create nova_generator_get_state function
            std::string getStateFuncName = "nova_generator_get_state";
            auto existingGetStateFunc = module_->getFunction(getStateFuncName);
            HIRFunction* getStateFunc = nullptr;
            if (existingGetStateFunc) {
                getStateFunc = existingGetStateFunc.get();
            } else {
                std::vector<HIRTypePtr> getStateParamTypes = {ptrType};
                HIRFunctionType* getStateFuncType = new HIRFunctionType(getStateParamTypes, intType);
                HIRFunctionPtr getStateFuncPtr = module_->createFunction(getStateFuncName, getStateFuncType);
                getStateFuncPtr->linkage = HIRFunction::Linkage::External;
                getStateFunc = getStateFuncPtr.get();
            }

            // Get or create nova_generator_set_state function
            std::string setStateFuncName = "nova_generator_set_state";
            auto existingSetStateFunc = module_->getFunction(setStateFuncName);
            if (existingSetStateFunc) {
                currentSetStateFunc_ = existingSetStateFunc.get();
            } else {
                std::vector<HIRTypePtr> setStateParamTypes = {ptrType, intType};
                HIRFunctionType* setStateFuncType = new HIRFunctionType(setStateParamTypes, voidType);
                HIRFunctionPtr setStateFuncPtr = module_->createFunction(setStateFuncName, setStateFuncType);
                setStateFuncPtr->linkage = HIRFunction::Linkage::External;
                currentSetStateFunc_ = setStateFuncPtr.get();
            }

            // Get or create nova_generator_store_local function (ptr, index, value) -> void
            std::string storeLocalFuncName = "nova_generator_store_local";
            auto existingStoreLocal = module_->getFunction(storeLocalFuncName);
            if (existingStoreLocal) {
                generatorStoreLocalFunc_ = existingStoreLocal.get();
            } else {
                std::vector<HIRTypePtr> storeLocalParamTypes = {ptrType, intType, intType};
                HIRFunctionType* storeLocalFuncType = new HIRFunctionType(storeLocalParamTypes, voidType);
                HIRFunctionPtr storeLocalFuncPtr = module_->createFunction(storeLocalFuncName, storeLocalFuncType);
                storeLocalFuncPtr->linkage = HIRFunction::Linkage::External;
                generatorStoreLocalFunc_ = storeLocalFuncPtr.get();
            }

            // Get or create nova_generator_load_local function (ptr, index) -> i64
            std::string loadLocalFuncName = "nova_generator_load_local";
            auto existingLoadLocal = module_->getFunction(loadLocalFuncName);
            if (existingLoadLocal) {
                generatorLoadLocalFunc_ = existingLoadLocal.get();
            } else {
                std::vector<HIRTypePtr> loadLocalParamTypes = {ptrType, intType};
                HIRFunctionType* loadLocalFuncType = new HIRFunctionType(loadLocalParamTypes, intType);
                HIRFunctionPtr loadLocalFuncPtr = module_->createFunction(loadLocalFuncName, loadLocalFuncType);
                loadLocalFuncPtr->linkage = HIRFunction::Linkage::External;
                generatorLoadLocalFunc_ = loadLocalFuncPtr.get();
            }

            // Get current state
            auto* genPtrLoaded = builder_->createLoad(genPtrVar);
            std::vector<HIRValue*> getStateArgs = {genPtrLoaded};
            auto* currentState = builder_->createCall(getStateFunc, getStateArgs, "state");

            // Save state value for later dispatch
            generatorStateValue_ = currentState;

            // Create blocks for state dispatch
            // State 0 = initial entry (body), State N = resume after yield N
            generatorDispatchBlock_ = func->createBasicBlock("dispatch").get();
            generatorBodyBlock_ = func->createBasicBlock("body").get();

            // Branch from entry to dispatch
            builder_->createBr(generatorDispatchBlock_);

            // Set insert point to dispatch but DON'T add terminator yet
            // We'll add the if-else chain after processing body to know all resume blocks
            builder_->setInsertPoint(generatorDispatchBlock_);
            // Leave dispatch block open - terminator will be added after body processing

            // Set insert point to body block for main code generation
            builder_->setInsertPoint(generatorBodyBlock_);

            // Load generator function parameters from local slots (stored at call site)
            // Parameters are stored in slots 100, 101, 102, etc.
            for (size_t i = 0; i < node.params.size(); ++i) {
                int slotIndex = 100 + static_cast<int>(i);
                generatorVarSlots_[node.params[i]] = slotIndex;
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Generator parameter '" << node.params[i]
                          << "' mapped to slot " << slotIndex << std::endl;
            }
        }

        // Generate function body
        if (node.body) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: FunctionDecl - Generating function body for " << node.name << std::endl;
            node.body->accept(*this);
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: FunctionDecl - Function body generated for " << node.name << std::endl;
        } else {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: WARNING - FunctionDecl has NO BODY for " << node.name << std::endl;
        }

        // Infer return type for regular functions if not explicitly annotated
        if (!node.isGenerator && retTypeKind == HIRType::Kind::Any) {
            // Scan blocks for return statements to infer type from actual return values
            for (auto& block : func->basicBlocks) {
                for (auto& inst : block->instructions) {
                    if (inst->opcode == HIRInstruction::Opcode::Return && !inst->operands.empty()) {
                        auto retVal = inst->operands[0].get();
                        if (retVal && retVal->type && retVal->type->kind != HIRType::Kind::Void) {
                            func->functionType->returnType = retVal->type;
                            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Inferred return type for " << node.name
                                                      << " from return statement: kind " << static_cast<int>(retVal->type->kind) << std::endl;
                            break;
                        }
                    }
                }
            }
        }

        // For generator functions, mark completion at the end and wire up dispatch
        if (node.isGenerator && currentGeneratorPtr_) {
            // Only add implicit completion if current block doesn't have a terminator
            // (i.e., no explicit return statement in the generator)
            auto* currentBlock = builder_->getInsertBlock();
            if (currentBlock && !currentBlock->hasTerminator()) {
                auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);

                std::string completeFuncName = "nova_generator_complete";
                auto existingCompleteFunc = module_->getFunction(completeFuncName);
                HIRFunction* completeFunc = nullptr;
                if (existingCompleteFunc) {
                    completeFunc = existingCompleteFunc.get();
                } else {
                    std::vector<HIRTypePtr> completeParamTypes = {ptrType, intType};
                    HIRFunctionType* completeFuncType = new HIRFunctionType(completeParamTypes, voidType);
                    HIRFunctionPtr completeFuncPtr = module_->createFunction(completeFuncName, completeFuncType);
                    completeFuncPtr->linkage = HIRFunction::Linkage::External;
                    completeFunc = completeFuncPtr.get();
                }

                auto* genPtr = builder_->createLoad(currentGeneratorPtr_);
                auto* zeroVal = builder_->createIntConstant(0);
                std::vector<HIRValue*> args = {genPtr, zeroVal};
                builder_->createCall(completeFunc, args);

                // Add return
                builder_->createReturn(nullptr);
            }

            // Now generate dispatch logic - we know all resume blocks
            // Go back to dispatch block and add the if-else chain
            if (generatorDispatchBlock_ && generatorStateValue_) {
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Generating dispatch for " << yieldResumeBlocks_.size() << " resume blocks" << std::endl;

                // Save current insert point
                auto* savedBlock = builder_->getInsertBlock();

                // Set insert point to dispatch block
                builder_->setInsertPoint(generatorDispatchBlock_);

                // Generate if-else chain for state dispatch:
                // if state == 1 goto resume_1
                // if state == 2 goto resume_2
                // ... else goto body
                [[maybe_unused]] HIRBasicBlock* currentCheckBlock = generatorDispatchBlock_;

                for (size_t i = 0; i < yieldResumeBlocks_.size(); ++i) {
                    int stateNum = static_cast<int>(i + 1);  // States are 1-indexed
                    auto* stateConst = builder_->createIntConstant(stateNum);
                    auto* isThisState = builder_->createEq(
                        generatorStateValue_, stateConst, "is_state_" + std::to_string(stateNum));

                    if (i < yieldResumeBlocks_.size() - 1) {
                        // More states to check - create next check block
                        auto* nextCheckBlock = func->createBasicBlock(
                            "dispatch_check_" + std::to_string(i + 2)).get();
                        builder_->createCondBr(isThisState, yieldResumeBlocks_[i], nextCheckBlock);
                        builder_->setInsertPoint(nextCheckBlock);
                        currentCheckBlock = nextCheckBlock;
                    } else {
                        // Last state - else goes to body
                        builder_->createCondBr(isThisState, yieldResumeBlocks_[i], generatorBodyBlock_);
                    }
                }

                // If no resume blocks, just branch to body
                if (yieldResumeBlocks_.empty()) {
                    builder_->createBr(generatorBodyBlock_);
                }

                // Restore insert point
                builder_->setInsertPoint(savedBlock);
            }

            // Reset generator state machine variables
            generatorDispatchBlock_ = nullptr;
            generatorStateValue_ = nullptr;
            generatorBodyBlock_ = nullptr;
            yieldResumeBlocks_.clear();
            yieldStateCounter_ = 0;
            currentSetStateFunc_ = nullptr;
            currentGeneratorPtr_ = nullptr;
        }

        // Add implicit return if needed
        if (!entryBlock->hasTerminator()) {
            builder_->createReturn(node.isAsync && !node.isGenerator
                ? createResolvedPromise(nullptr) : nullptr);
        }

        propagateTransitiveCaptures(
            savedFunctionName, savedSymbolTable, node.name);

        // After body generation, update closure environment struct type if this function captures variables
        if (tentativeEnvParam && capturedVariables_.count(node.name) && !capturedVariables_[node.name].empty()) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: FunctionDecl - Updating __env struct type for '"
                                      << node.name << "' (captured " << capturedVariables_[node.name].size()
                                      << " variables)" << std::endl;

            // Create closure environment struct with actual captured variables
            auto* envStruct = createClosureEnvironment(node.name);
            if (envStruct) {
                // Update the tentative __env parameter's type with the real struct (as pointer-to-struct)
                auto envPtrType = std::make_shared<HIRPointerType>(
                    std::shared_ptr<HIRType>(envStruct),
                    true  // mutable
                );
                tentativeEnvParam->type = envPtrType;

                // Update function type as well
                if (func->functionType && !func->functionType->paramTypes.empty()) {
                    func->functionType->paramTypes.back() = envPtrType;
                }

                closureEnvironments_[node.name] = envStruct;
                module_->closureEnvironments[node.name] = envStruct;
                if (environmentFieldNames_.count(node.name)) {
                    module_->closureCapturedVars[node.name] = environmentFieldNames_[node.name];
                }
                if (environmentFieldValues_.count(node.name)) {
                    module_->closureCapturedVarValues[node.name] = environmentFieldValues_[node.name];
                }

                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: FunctionDecl - Updated __env parameter type for function '"
                                          << node.name << "' with " << envStruct->fields.size() << " fields" << std::endl;
            }
        } else if (tentativeEnvParam && (!capturedVariables_.count(node.name) || capturedVariables_[node.name].empty())) {
            // No variables captured, remove the tentative __env parameter
            if (!func->parameters.empty() && func->parameters.back() == tentativeEnvParam) {
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: FunctionDecl - Removing unused __env parameter from '"
                                          << node.name << "'" << std::endl;
                func->parameters.pop_back();
                delete tentativeEnvParam;
            }
        }

        if (tentativeThisParam) {
            if (currentOrdinaryFunctionUsesThis_) {
                dynamicThisFunctions_.insert(node.name);
            } else if (!func->parameters.empty() &&
                       func->parameters.front() == tentativeThisParam) {
                func->parameters.erase(func->parameters.begin());
                func->functionType->paramTypes.erase(
                    func->functionType->paramTypes.begin());
                delete tentativeThisParam;
                for (size_t i = 0; i < func->parameters.size(); ++i) {
                    func->parameters[i]->index = static_cast<uint32_t>(i);
                }
            }
        }

        // Restore context
        if (!savedSymbolTable.empty()) {
            if (!scopeStack_.empty()) {
                scopeStack_.pop_back();  // Pop closure scope
            }
        }
        symbolTable_ = savedSymbolTable;
        builder_ = std::move(savedBuilder);
        currentFunction_ = savedCurrentFunction;
        currentThis_ = savedCurrentThis;
        lastFunctionName_ = savedFunctionName;  // Restore function name context
        dynamicBindingNames_ = savedDynamicBindingNames;
        dynamicObjectVars_ = savedDynamicObjectVars;
        forcedDynamicObjectVars_ = savedForcedDynamic;
        currentFunctionIsArrow_ = savedFunctionIsArrow;
        currentOrdinaryFunctionUsesThis_ = savedOrdinaryFunctionUsesThis;
    }
    

} // namespace nova::hir
