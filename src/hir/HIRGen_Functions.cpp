// HIRGen_Functions.cpp - Function expression and declaration visitors
// Extracted from HIRGen.cpp for better code organization

#include "nova/HIR/HIRGen_Internal.h"
#define NOVA_DEBUG 1

namespace nova::hir {

void HIRGenerator::visit(FunctionExpr& node) {
        // Function expression: let f = function(a, b) { return a + b; }

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
            // FunctionExpr doesn't have paramTypes in AST, use Any for now
            paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::Any));
        }

        // Return type
        HIRType::Kind retTypeKind = HIRType::Kind::Any;
        if (node.returnType) {
            retTypeKind = convertTypeKind(node.returnType->kind);
        }
        auto retType = std::make_shared<HIRType>(retTypeKind);

        auto funcType = new HIRFunctionType(paramTypes, retType);

        // Generate unique name for function expression
        static int funcExprCounter = 0;
        std::string funcName = node.name.empty() ?
            "__func_" + std::to_string(funcExprCounter++) : node.name;

        // Create function
        auto func = module_->createFunction(funcName, funcType);
        func->isAsync = node.isAsync;
        func->isGenerator = node.isGenerator;

        // Save current function context
        HIRFunction* savedFunction = currentFunction_;
        currentFunction_ = func.get();

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

        // Clear symbol table for the new function scope
        symbolTable_.clear();

        // Add parameters to symbol table
        for (size_t i = 0; i < node.params.size(); ++i) {
            symbolTable_[node.params[i]] = func->parameters[i];
        }

        // Generate function body
        if (node.body) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Generating function body for " << funcName << std::endl;
            node.body->accept(*this);
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Function body generated, checking for terminator..." << std::endl;

            // Add implicit return if needed
            if (!entryBlock->hasTerminator()) {
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Adding implicit return to " << funcName << std::endl;
                builder_->createReturn(nullptr);
            } else {
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Function " << funcName << " already has terminator" << std::endl;
            }
        } else {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: WARNING - No body for function " << funcName << std::endl;
        }

        // Create closure environment AFTER body generation (so captures are detected)
        std::cout << "[ENV-CHECK] Checking if function '" << funcName << "' needs environment..." << std::endl;
        hir::HIRStructType* envStruct = createClosureEnvironment(funcName);
        if (envStruct) {
            std::cout << "[ENV-CREATE] Creating environment for '" << funcName << "'" << std::endl;
            closureEnvironments_[funcName] = envStruct;
            module_->closureEnvironments[funcName] = envStruct;
            if (environmentFieldNames_.count(funcName)) {
                module_->closureCapturedVars[funcName] = environmentFieldNames_[funcName];
            }
            if (environmentFieldValues_.count(funcName)) {
                module_->closureCapturedVarValues[funcName] = environmentFieldValues_[funcName];
            }

            // Add environment parameter to the function
            auto envPtrType = std::make_shared<HIRPointerType>(
                std::shared_ptr<HIRType>(envStruct),
                true  // mutable
            );
            auto envParam = new HIRParameter(envPtrType, "__env", func->parameters.size());
            func->parameters.push_back(envParam);
            func->functionType->paramTypes.push_back(envPtrType);

            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Added environment parameter to " << funcName << " after body generation" << std::endl;
        }

        // Restore context
        scopeStack_.pop_back();
        symbolTable_ = savedSymbolTable;
        builder_ = std::move(savedBuilder);
        currentFunction_ = savedFunction;

        // Restore function name (but keep funcName available for variable association)
        // Note: We keep the current funcName in lastFunctionName_ so it can be associated with a variable
        // The savedFunctionName will be used for the parent function's context
        lastFunctionName_ = funcName;  // Keep current for variable association
        // savedFunctionName is not restored here because we want the inner function name to persist

        // Return a string constant with the function name
        // This will be used by MIRGen to identify the function and allocate closure if needed
        lastValue_ = builder_->createStringConstant(funcName);
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Function " << funcName << " reference created" << std::endl;
    }
    

void HIRGenerator::visit(ArrowFunctionExpr& node) {
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

        // Save and set function name for closure tracking (must be before body generation)
        auto savedFunctionName = lastFunctionName_;
        lastFunctionName_ = funcName;

        // Save current symbol table and push to scope stack for closure support
        auto savedSymbolTable = symbolTable_;
        scopeStack_.push_back(savedSymbolTable);  // Push for closure access

        // Clear symbol table for the new function scope
        symbolTable_.clear();

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

                builder_->createReturn(lastValue_);
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
                        builder_->createReturn(nullptr);
                        if(NOVA_DEBUG) std::cerr << "DEBUG: Added return terminator to block '" << block->label << "'" << std::endl;
                    }
                }
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

        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Created arrow function '" << funcName << "' with "
                  << node.params.size() << " parameters" << std::endl;
    }
    

void HIRGenerator::visit(FunctionDecl& node) {
        std::cerr << "DEBUG: Entering visit(FunctionDecl), function name: " << node.name << std::endl;
        std::cerr << "DEBUG: Default values count: " << node.defaultValues.size() << std::endl;

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

        // For generator functions, add implicit genPtr and input parameters
        if (node.isGenerator) {
            paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::Pointer));  // genPtr
            paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::I64));      // input
        }

        for (size_t i = 0; i < node.params.size(); ++i) {
            // Default to I64 for better type inference in closures
            // JavaScript numbers are typically 64-bit floats, but we use i64 for integers
            HIRType::Kind typeKind = HIRType::Kind::I64;  // Default to I64 instead of Any

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

        // Track generator functions
        if (node.isGenerator && node.isAsync) {
            // AsyncGenerator (ES2018) - async function*
            asyncGeneratorFuncs_.insert(node.name);
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered AsyncGenerator function: " << node.name << std::endl;
        } else if (node.isGenerator) {
            // Regular Generator (ES2015) - function*
            generatorFuncs_.insert(node.name);
        }

        // Track all functions for call/apply/bind support
        functionVars_.insert(node.name);
        functionParamCounts_[node.name] = static_cast<int64_t>(node.params.size());
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered function: " << node.name << " with " << node.params.size() << " params" << std::endl;

        currentFunction_ = func.get();

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

        // Add parameters to symbol table
        // For generators, parameters are loaded from local slots (set at call site)
        if (!node.isGenerator) {
            for (size_t i = 0; i < node.params.size(); ++i) {
                if (i < func->parameters.size()) {
                    symbolTable_[node.params[i]] = func->parameters[i];
                }
            }
        }
        // For generators, parameter loading happens after state machine setup

        // Handle rest parameter (...args)
        if (!node.restParam.empty()) {
            // Create an array to hold rest arguments
            // For now, create an empty array - full implementation would collect varargs
            auto* arrayType = new hir::HIRType(hir::HIRType::Kind::Array);
            auto* restArray = builder_->createAlloca(arrayType, node.restParam);
            symbolTable_[node.restParam] = restArray;
            std::cerr << "NOTE: Rest parameter '" << node.restParam << "' created (varargs collection not fully implemented)" << std::endl;
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
            builder_->createReturn(nullptr);
        }

        // After body generation, update closure environment struct type if this function captures variables
        if (tentativeEnvParam && capturedVariables_.count(node.name) && !capturedVariables_[node.name].empty()) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: FunctionDecl - Updating __env struct type for '"
                                      << node.name << "' (captured " << capturedVariables_[node.name].size()
                                      << " variables)" << std::endl;

            // Create closure environment struct with actual captured variables
            auto* envStruct = createClosureEnvironment(node.name);
            if (envStruct) {
                // Update the tentative __env parameter's type with the real struct
                std::shared_ptr<HIRType> envType(envStruct);
                tentativeEnvParam->type = envType;

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

        // Restore context
        if (!savedSymbolTable.empty()) {
            scopeStack_.pop_back();  // Pop closure scope
        }
        symbolTable_ = savedSymbolTable;
        builder_ = std::move(savedBuilder);
        currentFunction_ = nullptr;
        lastFunctionName_ = savedFunctionName;  // Restore function name context
    }
    

} // namespace nova::hir
