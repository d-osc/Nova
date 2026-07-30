// HIRGen_Advanced.cpp - Advanced feature visitors
// Extracted from HIRGen.cpp for better code organization
// Contains: async/await, generators, spread, imports, JSX, patterns, decorators, TypeScript, declarations

#include "nova/HIR/HIRGen_Internal.h"
#include "nova/Frontend/Lexer.h"
#include "nova/Frontend/Parser.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>
#define NOVA_DEBUG 0

namespace nova::hir {

void HIRGenerator::visit(SpreadExpr& node) {
        // Spread operator: ...expr
        // Evaluate the argument - the array/iterable to spread
        // The actual unpacking/spreading is handled by the context:
        // - ArrayExpr: Handled by ArrayExpr visitor (see HIRGen_Arrays.cpp)
        // - CallExpr: Would be handled by CallExpr visitor (for spreading arguments)
        // - ObjectExpr: Would be handled by ObjectExpr visitor (for object spread)
        // This visitor just evaluates the argument and passes it through
        if (node.argument) {
            node.argument->accept(*this);
            if (auto* identifier =
                    dynamic_cast<Identifier*>(node.argument.get());
                identifier && setVars_.count(identifier->name) > 0) {
                auto pointerType =
                    std::make_shared<HIRType>(HIRType::Kind::Pointer);
                auto existing = module_->getFunction("nova_set_values");
                HIRFunction* function =
                    existing ? existing.get() : nullptr;
                if (!function) {
                    auto* functionType =
                        new HIRFunctionType({pointerType}, pointerType);
                    auto created = module_->createFunction(
                        "nova_set_values", functionType);
                    created->linkage = HIRFunction::Linkage::External;
                    function = created.get();
                }
                lastValue_ = builder_->createCall(
                    function, {lastValue_}, "set.spread.array");
                lastValue_->type = pointerType;
                lastWasRuntimeArray_ = true;
                return;
            }
            bool generatorSpread = false;
            if (auto* call =
                    dynamic_cast<CallExpr*>(node.argument.get())) {
                if (auto* callee =
                        dynamic_cast<Identifier*>(call->callee.get())) {
                    generatorSpread =
                        generatorFuncs_.count(callee->name) > 0;
                }
            } else if (auto* identifier =
                           dynamic_cast<Identifier*>(node.argument.get())) {
                generatorSpread =
                    generatorVars_.count(identifier->name) > 0;
            }
            if (generatorSpread) {
                auto pointerType =
                    std::make_shared<HIRType>(HIRType::Kind::Pointer);
                auto existing =
                    module_->getFunction("nova_generator_to_array");
                HIRFunction* function =
                    existing ? existing.get() : nullptr;
                if (!function) {
                    auto* functionType =
                        new HIRFunctionType({pointerType}, pointerType);
                    auto created = module_->createFunction(
                        "nova_generator_to_array", functionType);
                    created->linkage = HIRFunction::Linkage::External;
                    function = created.get();
                }
                lastValue_ = builder_->createCall(
                    function, {lastValue_}, "generator.spread.array");
                lastValue_->type = pointerType;
                lastWasRuntimeArray_ = true;
            }
            // lastValue_ now contains the array/iterable to spread
            // The parent expression (ArrayExpr, CallExpr, etc.) is responsible for unpacking
        }
    }
    
void HIRGenerator::visit(TemplateLiteralExpr& node) {
        // Template literal: `Hello ${name}!` becomes "Hello " + name + "!"
        // quasis: ["Hello ", "!"]
        // expressions: [name]

        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing template literal with " << node.quasis.size()
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

            // Convert non-string values to strings using runtime function
            if (exprValue && exprValue->type) {
                if(NOVA_DEBUG) {
                    std::cerr << "  Template expr type: " << static_cast<int>(exprValue->type->kind) << std::endl;
                    std::cerr << "  I64 enum value: " << static_cast<int>(HIRType::Kind::I64) << std::endl;
                    std::cerr << "  Match? " << (exprValue->type->kind == HIRType::Kind::I64 ? "YES" : "NO") << std::endl;
                }
                if (exprValue->type->kind == HIRType::Kind::I64 ||
                    exprValue->type->kind == HIRType::Kind::I32) {
                    if(NOVA_DEBUG) std::cerr << "  Converting number to string" << std::endl;
                    // Convert number to string using nova_i64_to_string
                    auto stringType = std::make_shared<HIRType>(HIRType::Kind::String);
                    auto i64Type = std::make_shared<HIRType>(HIRType::Kind::I64);
                    std::vector<HIRTypePtr> paramTypes = {i64Type};

                    HIRFunction* toStringFunc = nullptr;
                    auto existingFunc = module_->getFunction("nova_i64_to_string");
                    if (existingFunc) {
                        toStringFunc = existingFunc.get();
                    } else {
                        HIRFunctionType* funcType = new HIRFunctionType(paramTypes, stringType);
                        HIRFunctionPtr funcPtr = module_->createFunction("nova_i64_to_string", funcType);
                        funcPtr->linkage = HIRFunction::Linkage::External;
                        toStringFunc = funcPtr.get();
                    }

                    std::vector<HIRValue*> args = {exprValue};
                    auto* callInst = builder_->createCall(toStringFunc, args, "num_to_str");
                    callInst->type = stringType;
                    // Update lastValue_ so MIR/LLVM can resolve the result
                    lastValue_ = callInst;
                    exprValue = lastValue_;

                    if(NOVA_DEBUG) {
                        std::cerr << "  *** TEMPLATE DEBUG: Created nova_i64_to_string call, inst ptr=" << callInst << std::endl;
                        std::cerr << "  *** TEMPLATE DEBUG: exprValue now points to: " << exprValue << std::endl;
                    }
                } else if (exprValue->type->kind == HIRType::Kind::F64 ||
                           exprValue->type->kind == HIRType::Kind::F32) {
                    if(NOVA_DEBUG) std::cerr << "  Converting float to string" << std::endl;
                    auto stringType = std::make_shared<HIRType>(HIRType::Kind::String);
                    auto f64Type = std::make_shared<HIRType>(HIRType::Kind::F64);
                    std::vector<HIRTypePtr> paramTypes = {f64Type};

                    HIRFunction* toStringFunc = nullptr;
                    auto existingFunc = module_->getFunction("nova_f64_to_string");
                    if (existingFunc) {
                        toStringFunc = existingFunc.get();
                    } else {
                        HIRFunctionType* funcType = new HIRFunctionType(paramTypes, stringType);
                        HIRFunctionPtr funcPtr = module_->createFunction("nova_f64_to_string", funcType);
                        funcPtr->linkage = HIRFunction::Linkage::External;
                        toStringFunc = funcPtr.get();
                    }

                    std::vector<HIRValue*> args = {exprValue};
                    auto* callInst = builder_->createCall(toStringFunc, args, "float_to_str");
                    callInst->type = stringType;
                    lastValue_ = callInst;
                    exprValue = lastValue_;
                } else if (exprValue->type->kind == HIRType::Kind::Bool) {
                    // Convert boolean to "true" / "false" string via runtime.
                    auto stringType = std::make_shared<HIRType>(HIRType::Kind::String);
                    auto i64Type = std::make_shared<HIRType>(HIRType::Kind::I64);

                    // Widen bool to i64 for the C ABI.
                    HIRValue* boolAsI64 = builder_->createCast(exprValue, i64Type.get());

                    HIRFunction* toStringFunc = nullptr;
                    auto existingFunc = module_->getFunction("nova_bool_to_string");
                    if (existingFunc) {
                        toStringFunc = existingFunc.get();
                    } else {
                        HIRFunctionType* funcType = new HIRFunctionType({i64Type}, stringType);
                        HIRFunctionPtr funcPtr = module_->createFunction("nova_bool_to_string", funcType);
                        funcPtr->linkage = HIRFunction::Linkage::External;
                        toStringFunc = funcPtr.get();
                    }

                    auto* callInst = builder_->createCall(toStringFunc, {boolAsI64}, "bool_to_str");
                    callInst->type = stringType;
                    lastValue_ = callInst;
                    exprValue = lastValue_;
                } else if (exprValue->type->kind == HIRType::Kind::Pointer) {
                    // Inspect the pointee to choose between array vs. object
                    // string conversion. Pointers with no known pointee default
                    // to the array path (the common case for `${arr}`).
                    bool isObjectPointer = false;
                    if (auto* ptrTy = dynamic_cast<HIRPointerType*>(exprValue->type.get())) {
                        if (ptrTy->pointeeType &&
                            ptrTy->pointeeType->kind == HIRType::Kind::Struct) {
                            isObjectPointer = true;
                        }
                    }
                    auto stringType = std::make_shared<HIRType>(HIRType::Kind::String);
                    auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

                    if (isObjectPointer) {
                        std::vector<HIRTypePtr> paramTypes = {ptrType};
                        HIRFunction* toStringFunc = nullptr;
                        const std::string runtimeName = "nova_object_toString";
                        if (auto existing = module_->getFunction(runtimeName)) {
                            toStringFunc = existing.get();
                        } else {
                            auto* funcType = new HIRFunctionType(paramTypes, stringType);
                            auto funcPtr = module_->createFunction(runtimeName, funcType);
                            funcPtr->linkage = HIRFunction::Linkage::External;
                            toStringFunc = funcPtr.get();
                        }
                        std::vector<HIRValue*> args = {exprValue};
                        auto* callInst = builder_->createCall(toStringFunc, args, "obj_to_str");
                        callInst->type = stringType;
                        lastValue_ = callInst;
                        exprValue = lastValue_;
                    } else {
                        std::vector<HIRTypePtr> paramTypes = {ptrType, ptrType};
                        HIRFunction* toStringFunc = nullptr;
                        const std::string runtimeName = "nova_value_array_join";
                        if (auto existing = module_->getFunction(runtimeName)) {
                            toStringFunc = existing.get();
                        } else {
                            auto* funcType = new HIRFunctionType(paramTypes, stringType);
                            auto funcPtr = module_->createFunction(runtimeName, funcType);
                            funcPtr->linkage = HIRFunction::Linkage::External;
                            toStringFunc = funcPtr.get();
                        }
                        auto* delim = builder_->createStringConstant(",");
                        std::vector<HIRValue*> args = {exprValue, delim};
                        auto* callInst = builder_->createCall(toStringFunc, args, "array_to_str");
                        callInst->type = stringType;
                        lastValue_ = callInst;
                        exprValue = lastValue_;
                    }
                } else if (exprValue->type->kind == HIRType::Kind::Struct ||
                           exprValue->type->kind == HIRType::Kind::Reference) {
                    // Object literals become "[object Object]".
                    auto stringType = std::make_shared<HIRType>(HIRType::Kind::String);
                    auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                    std::vector<HIRTypePtr> paramTypes = {ptrType};

                    HIRFunction* toStringFunc = nullptr;
                    const std::string runtimeName = "nova_object_toString";
                    if (auto existing = module_->getFunction(runtimeName)) {
                        toStringFunc = existing.get();
                    } else {
                        auto* funcType = new HIRFunctionType(paramTypes, stringType);
                        auto funcPtr = module_->createFunction(runtimeName, funcType);
                        funcPtr->linkage = HIRFunction::Linkage::External;
                        toStringFunc = funcPtr.get();
                    }

                    std::vector<HIRValue*> args = {exprValue};
                    auto* callInst = builder_->createCall(toStringFunc, args, "obj_to_str");
                    callInst->type = stringType;
                    lastValue_ = callInst;
                    exprValue = lastValue_;
                }
            }

            // Concatenate result with the expression using nova_string_concat
            auto stringType = std::make_shared<HIRType>(HIRType::Kind::String);
            std::vector<HIRTypePtr> concatParamTypes = {stringType, stringType};

            HIRFunction* concatFunc = nullptr;
            auto existingConcatFunc = module_->getFunction("nova_string_concat");
            if (existingConcatFunc) {
                concatFunc = existingConcatFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(concatParamTypes, stringType);
                HIRFunctionPtr funcPtr = module_->createFunction("nova_string_concat", funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                concatFunc = funcPtr.get();
            }

            std::vector<HIRValue*> concatArgs = {result, exprValue};
            result = builder_->createCall(concatFunc, concatArgs, "str_concat");
            result->type = stringType;

            // Concatenate with the next quasi (string after this ${})
            if (i + 1 < node.quasis.size() && !node.quasis[i + 1].empty()) {
                HIRValue* nextQuasi = builder_->createStringConstant(node.quasis[i + 1]);
                std::vector<HIRValue*> concatArgs2 = {result, nextQuasi};
                result = builder_->createCall(concatFunc, concatArgs2, "str_concat");
                result->type = stringType;
            }
        }

        lastValue_ = result;
    }
    
HIRValue* HIRGenerator::createResolvedPromise(HIRValue* value) {
        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
        auto jsValueType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
        HIRValue* payload = toJSValue(value);

        HIRFunction* resolveFunction = nullptr;
        if (auto existing = module_->getFunction("nova_promise_resolve")) {
            resolveFunction = existing.get();
        } else {
            auto* functionType = new HIRFunctionType({jsValueType}, ptrType);
            auto function = module_->createFunction("nova_promise_resolve", functionType);
            function->linkage = HIRFunction::Linkage::External;
            resolveFunction = function.get();
        }
        auto* promise = builder_->createCall(resolveFunction, {payload}, "async.promise");
        promise->type = ptrType;
        return promise;
    }

void HIRGenerator::visit(AwaitExpr& node) {
        // Awaiting a non-Promise still yields the value. Known Promise-producing
        // expressions are unwrapped through the runtime.
        lastWasPromise_ = false;
        node.argument->accept(*this);
        HIRValue* awaited = lastValue_;
        const bool isPromise = lastWasPromise_;
        lastWasPromise_ = false;
        if (!isPromise || !awaited) return;

        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
        auto jsValueType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
        HIRFunction* awaitFunction = nullptr;
        if (auto existing = module_->getFunction("nova_promise_await")) {
            awaitFunction = existing.get();
        } else {
            auto* functionType = new HIRFunctionType({ptrType}, jsValueType);
            auto function = module_->createFunction("nova_promise_await", functionType);
            function->linkage = HIRFunction::Linkage::External;
            awaitFunction = function.get();
        }
        lastValue_ = builder_->createCall(awaitFunction, {awaited}, "await.value");
        lastValue_->type = jsValueType;
    }
    
void HIRGenerator::visit(YieldExpr& node) {
        // Check if this is yield* (delegation)
        if (node.isDelegate) {
            generateYieldDelegate(node);
            return;
        }

        // Regular yield expression - set state, call nova_generator_yield(genPtr, value), then RETURN to suspend
        HIRValue* yieldValue = nullptr;
        if (node.argument) {
            node.argument->accept(*this);
            yieldValue = lastValue_;
        } else {
            yieldValue = builder_->createIntConstant(0);
        }

        // Get the current generator pointer
        if (currentGeneratorPtr_) {
            // Load genPtr from the stored location
            auto* genPtr = builder_->createLoad(currentGeneratorPtr_);

            // Get or create nova_generator_yield function
            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
            auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
            std::vector<HIRTypePtr> paramTypes = {ptrType, intType};

            std::string runtimeFuncName = "nova_generator_yield";
            auto existingFunc = module_->getFunction(runtimeFuncName);
            HIRFunction* yieldFunc = nullptr;
            if (existingFunc) {
                yieldFunc = existingFunc.get();
            } else {
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, voidType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFuncName, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                yieldFunc = funcPtr.get();
            }

            // Increment yield state counter - this yield will be state N+1
            yieldStateCounter_++;
            int thisYieldState = yieldStateCounter_;
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Yield #" << thisYieldState << " in generator" << std::endl;

            // Set state BEFORE yielding so next() knows where to resume
            if (currentSetStateFunc_) {
                auto* stateConst = builder_->createIntConstant(thisYieldState);
                std::vector<HIRValue*> setStateArgs = {genPtr, stateConst};
                builder_->createCall(currentSetStateFunc_, setStateArgs);
            }

            // Call yield function to store the yielded value
            std::vector<HIRValue*> args = {genPtr, yieldValue};
            builder_->createCall(yieldFunc, args);

            // RETURN to suspend execution!
            builder_->createReturn(nullptr);

            // Create resume block for code after this yield
            auto* resumeBlock = currentFunction_->createBasicBlock(
                "resume_" + std::to_string(thisYieldState)).get();

            // Add to resume blocks vector (indexed by state-1)
            yieldResumeBlocks_.push_back(resumeBlock);

            // Continue code generation in the resume block
            builder_->setInsertPoint(resumeBlock);

            // The second hidden generator parameter is the value supplied by
            // the resuming `next(value)` call.  Yield is an expression, so
            // this must become its result after control reaches the resume
            // block (rather than the historical hard-coded zero).
            if (currentFunction_ &&
                currentFunction_->parameters.size() > 1) {
                lastValue_ = currentFunction_->parameters[1];
            } else {
                lastValue_ = builder_->createIntConstant(0);
            }
        } else {
            // Fallback: yield without generator context
            lastValue_ = yieldValue;
        }
    }
    

void HIRGenerator::generateYieldDelegate(YieldExpr& node) {
        // yield* delegation - iterate inner generator and yield each value
        // Uses generator local storage to persist inner iterator across suspensions
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing yield* delegation" << std::endl;

        if (!currentGeneratorPtr_) {
            if (node.argument) {
                node.argument->accept(*this);
            }
            lastValue_ = builder_->createIntConstant(0);
            return;
        }

        // Evaluate the inner iterable (generator)
        node.argument->accept(*this);
        HIRValue* innerIterator = lastValue_;

        // Get outer generator pointer
        auto* outerGenPtr = builder_->createLoad(currentGeneratorPtr_);

        // Create types
        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
        auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
        auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);

        // Get or create nova_generator_store_local function
        std::vector<HIRTypePtr> storeLocalParamTypes = {ptrType, intType, intType};
        HIRFunction* storeLocalFunc = nullptr;
        auto existingStoreLocal = module_->getFunction("nova_generator_store_local");
        if (existingStoreLocal) {
            storeLocalFunc = existingStoreLocal.get();
        } else {
            HIRFunctionType* funcType = new HIRFunctionType(storeLocalParamTypes, voidType);
            HIRFunctionPtr funcPtr = module_->createFunction("nova_generator_store_local", funcType);
            funcPtr->linkage = HIRFunction::Linkage::External;
            storeLocalFunc = funcPtr.get();
        }

        // Get or create nova_generator_load_local function
        std::vector<HIRTypePtr> loadLocalParamTypes = {ptrType, intType};
        HIRFunction* loadLocalFunc = nullptr;
        auto existingLoadLocal = module_->getFunction("nova_generator_load_local");
        if (existingLoadLocal) {
            loadLocalFunc = existingLoadLocal.get();
        } else {
            HIRFunctionType* funcType = new HIRFunctionType(loadLocalParamTypes, intType);
            HIRFunctionPtr funcPtr = module_->createFunction("nova_generator_load_local", funcType);
            funcPtr->linkage = HIRFunction::Linkage::External;
            loadLocalFunc = funcPtr.get();
        }

        // Store inner iterator in generator local storage slot 0
        auto* zero = builder_->createIntConstant(0);
        std::vector<HIRValue*> storeArgs = {outerGenPtr, zero, innerIterator};
        builder_->createCall(storeLocalFunc, storeArgs);

        // Create blocks for delegation loop
        auto* loopHeaderBlock = currentFunction_->createBasicBlock("yield_delegate_header").get();
        auto* loopBodyBlock = currentFunction_->createBasicBlock("yield_delegate_body").get();
        auto* loopExitBlock = currentFunction_->createBasicBlock("yield_delegate_exit").get();

        // Jump to loop header
        builder_->createBr(loopHeaderBlock);
        builder_->setInsertPoint(loopHeaderBlock);

        // Load inner iterator from generator local storage
        auto* outerGenPtrInLoop = builder_->createLoad(currentGeneratorPtr_);
        std::vector<HIRValue*> loadArgs = {outerGenPtrInLoop, zero};
        auto* innerIter = builder_->createCall(loadLocalFunc, loadArgs);

        // Get or create nova_generator_next function
        std::vector<HIRTypePtr> nextParamTypes = {ptrType, intType};
        HIRFunction* nextFunc = nullptr;
        auto existingNextFunc = module_->getFunction("nova_generator_next");
        if (existingNextFunc) {
            nextFunc = existingNextFunc.get();
        } else {
            HIRFunctionType* funcType = new HIRFunctionType(nextParamTypes, ptrType);
            HIRFunctionPtr funcPtr = module_->createFunction("nova_generator_next", funcType);
            funcPtr->linkage = HIRFunction::Linkage::External;
            nextFunc = funcPtr.get();
        }

        // Get or create nova_iterator_result_done function
        std::vector<HIRTypePtr> doneParamTypes = {ptrType};
        HIRFunction* doneFunc = nullptr;
        auto existingDoneFunc = module_->getFunction("nova_iterator_result_done");
        if (existingDoneFunc) {
            doneFunc = existingDoneFunc.get();
        } else {
            HIRFunctionType* funcType = new HIRFunctionType(doneParamTypes, intType);
            HIRFunctionPtr funcPtr = module_->createFunction("nova_iterator_result_done", funcType);
            funcPtr->linkage = HIRFunction::Linkage::External;
            doneFunc = funcPtr.get();
        }

        // Get or create nova_iterator_result_value function
        HIRFunction* valueFunc = nullptr;
        auto existingValueFunc = module_->getFunction("nova_iterator_result_value");
        if (existingValueFunc) {
            valueFunc = existingValueFunc.get();
        } else {
            HIRFunctionType* funcType = new HIRFunctionType(doneParamTypes, intType);
            HIRFunctionPtr funcPtr = module_->createFunction("nova_iterator_result_value", funcType);
            funcPtr->linkage = HIRFunction::Linkage::External;
            valueFunc = funcPtr.get();
        }

        // Call inner.next(0)
        std::vector<HIRValue*> nextArgs = {innerIter, zero};
        auto* result = builder_->createCall(nextFunc, nextArgs);

        // Check if done
        std::vector<HIRValue*> doneArgs = {result};
        auto* doneVal = builder_->createCall(doneFunc, doneArgs);
        auto* doneCondition = builder_->createNe(doneVal, zero);

        // Store result in generator local storage slot 1 for body access
        auto* one = builder_->createIntConstant(1);
        std::vector<HIRValue*> storeResultArgs = {outerGenPtrInLoop, one, result};
        builder_->createCall(storeLocalFunc, storeResultArgs);

        // If done, exit loop; otherwise, process value
        builder_->createCondBr(doneCondition, loopExitBlock, loopBodyBlock);

        // Loop body: get value and yield it
        builder_->setInsertPoint(loopBodyBlock);

        // Load result from generator local storage slot 1
        auto* outerGenPtrInBody = builder_->createLoad(currentGeneratorPtr_);
        std::vector<HIRValue*> loadResultArgs = {outerGenPtrInBody, one};
        auto* storedResult = builder_->createCall(loadLocalFunc, loadResultArgs);

        std::vector<HIRValue*> valueArgs = {storedResult};
        auto* yieldValue = builder_->createCall(valueFunc, valueArgs);

        // Get or create nova_generator_yield function
        std::vector<HIRTypePtr> yieldParamTypes = {ptrType, intType};
        HIRFunction* yieldFunc = nullptr;
        auto existingYieldFunc = module_->getFunction("nova_generator_yield");
        if (existingYieldFunc) {
            yieldFunc = existingYieldFunc.get();
        } else {
            HIRFunctionType* funcType = new HIRFunctionType(yieldParamTypes, voidType);
            HIRFunctionPtr funcPtr = module_->createFunction("nova_generator_yield", funcType);
            funcPtr->linkage = HIRFunction::Linkage::External;
            yieldFunc = funcPtr.get();
        }

        // Increment yield state counter - this yield* will be a single state
        yieldStateCounter_++;
        int thisYieldState = yieldStateCounter_;
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Yield* delegation state #" << thisYieldState << std::endl;

        // Set state BEFORE yielding
        if (currentSetStateFunc_) {
            auto* stateConst = builder_->createIntConstant(thisYieldState);
            std::vector<HIRValue*> setStateArgs = {outerGenPtrInBody, stateConst};
            builder_->createCall(currentSetStateFunc_, setStateArgs);
        }

        // Call yield function to store the yielded value
        std::vector<HIRValue*> yieldArgs = {outerGenPtrInBody, yieldValue};
        builder_->createCall(yieldFunc, yieldArgs);

        // RETURN to suspend execution
        builder_->createReturn(nullptr);

        // Create resume block that branches BACK to loop header
        auto* resumeBlock = currentFunction_->createBasicBlock(
            "yield_delegate_resume_" + std::to_string(thisYieldState)).get();
        yieldResumeBlocks_.push_back(resumeBlock);

        // Resume block branches back to loop header to continue iteration
        builder_->setInsertPoint(resumeBlock);
        builder_->createBr(loopHeaderBlock);

        // Continue code generation after yield* in the exit block
        builder_->setInsertPoint(loopExitBlock);

        // The result of yield* is the final return value of the inner generator
        lastValue_ = builder_->createIntConstant(0);
    }
void HIRGenerator::visit(AsExpr& node) {
        // Type assertion - just evaluate expression
        node.expression->accept(*this);

        if(NOVA_DEBUG) {
            std::cerr << "DEBUG AsExpr: target name='" << (node.targetType ? node.targetType->name : "<null>")
                      << "', lastValue_ type kind="
                      << (lastValue_ && lastValue_->type ? static_cast<int>(lastValue_->type->kind) : -1)
                      << std::endl;
        }

        // When casting a JSValue (e.g. an exception caught via `catch (e)`)
        // to a class type, generate an unbox so subsequent field accesses
        // resolve through the class's struct layout instead of falling
        // through to the "property not found" path.
        if (node.targetType && lastValue_ && lastValue_->type &&
            lastValue_->type->kind == HIRType::Kind::JSValue &&
            !node.targetType->name.empty()) {
            auto structIt = classStructTypes_.find(node.targetType->name);
            if(NOVA_DEBUG) {
                std::cerr << "DEBUG AsExpr: looking up '" << node.targetType->name
                          << "' in classStructTypes_, found="
                          << (structIt != classStructTypes_.end() ? "YES" : "NO")
                          << ", total entries=" << classStructTypes_.size() << std::endl;
            }
            if (structIt != classStructTypes_.end()) {
                auto jsValueType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
                auto structTypePtr = std::shared_ptr<HIRType>(
                    structIt->second, [](HIRType*){});
                auto ptrType = std::make_shared<HIRPointerType>(
                    structTypePtr, true);
                auto* function = [&]() -> HIRFunction* {
                    if (auto existing = module_->getFunction("nova_value_to_object")) {
                        return existing.get();
                    }
                    auto* functionType = new HIRFunctionType({jsValueType}, ptrType);
                    auto created = module_->createFunction("nova_value_to_object", functionType);
                    created->linkage = HIRFunction::Linkage::External;
                    return created.get();
                }();
                auto* unboxed = builder_->createCall(
                    function, {lastValue_}, "as_class_unbox");
                unboxed->type = ptrType;
                lastValue_ = unboxed;
            }
        }
    }
    
void HIRGenerator::visit(SatisfiesExpr& node) {
        // Satisfies operator - just evaluate expression
        node.expression->accept(*this);
    }
    
void HIRGenerator::visit(NonNullExpr& node) {
        // Non-null assertion - just evaluate expression
        node.expression->accept(*this);
    }
    
void HIRGenerator::visit(TaggedTemplateExpr& node) {
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing tagged template expression" << std::endl;

        // Build arguments: first create the strings array, then each expression value
        std::vector<HIRValue*> args;

        // Create strings array from quasis
        std::vector<HIRValue*> quasiValues;
        for (const auto& quasi : node.quasis) {
            quasiValues.push_back(builder_->createStringConstant(quasi));
        }
        // Pass strings array as first argument (as a runtime array)
        auto* stringsArray = builder_->createArrayConstruct(quasiValues, "tagged_strings");
        args.push_back(stringsArray);

        // Evaluate each expression now so we know their values.
        std::vector<HIRValue*> exprValues;
        for (auto& expr : node.expressions) {
            expr->accept(*this);
            exprValues.push_back(lastValue_);
        }

        // Determine if the callee has a rest parameter. When it does, the
        // function signature is (strings, values_array) — we must collect
        // expressions into a single runtime array. Otherwise pass them as
        // individual arguments.
        std::string calleeName;
        if (auto* ident = dynamic_cast<Identifier*>(node.tag.get())) {
            calleeName = ident->name;
        }
        bool hasRest = false;
        if (!calleeName.empty()) {
            auto restIt = module_->functionRestParams.find(calleeName);
            if (restIt != module_->functionRestParams.end()) {
                hasRest = true;
            }
        }

        if (hasRest) {
            for (auto*& value : exprValues) {
                value = toJSValue(value);
            }
            auto* valuesArray = builder_->createArrayConstruct(
                exprValues, "tagged_values");
            args.push_back(valuesArray);
        } else {
            for (auto* v : exprValues) args.push_back(v);
        }

        // Call the tag function with (strings, ...values)
        if (auto* ident = dynamic_cast<Identifier*>(node.tag.get())) {
            auto func = module_->getFunction(ident->name);
            if (func) {
                lastValue_ = builder_->createCall(func.get(), args, "tagged_template");
                return;
            }
        }

        // Fallback: treat as regular template literal by concatenating quasis and expressions
        if (!node.quasis.empty()) {
            lastValue_ = builder_->createStringConstant(node.quasis[0]);
            for (size_t i = 0; i < node.expressions.size(); ++i) {
                node.expressions[i]->accept(*this);
                // The concatenation will be handled by the MIR/codegen as string add operations
                if (i + 1 < node.quasis.size() && !node.quasis[i + 1].empty()) {
                    // Append the next quasi string
                    lastValue_ = builder_->createStringConstant(node.quasis[i + 1]);
                }
            }
        } else {
            lastValue_ = builder_->createStringConstant("");
        }
    }
    
void HIRGenerator::visit(SequenceExpr& node) {
        // Comma operator - evaluate all, return last
        for (auto& expr : node.expressions) {
            expr->accept(*this);
        }
    }
    
void HIRGenerator::visit(ParenthesizedExpr& node) {
        node.expression->accept(*this);
    }
    
void HIRGenerator::visit(MetaProperty& node) {
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing meta property: " << node.meta << "." << node.property << std::endl;

        if (node.meta == "new" && node.property == "target") {
            // new.target - returns undefined outside constructors, constructor function inside
            auto undefinedType = std::make_shared<HIRType>(HIRType::Kind::Unknown);
            lastValue_ = builder_->createUndefinedConstant(undefinedType.get());
        } else if (node.meta == "import" && node.property == "meta") {
            // import.meta - return an object with url property
            // For compiled code, this is typically the file path
            lastValue_ = builder_->createStringConstant("file://compiled");
        } else {
            lastValue_ = builder_->createIntConstant(0);
        }
    }
    
void HIRGenerator::visit(ImportExpr& node) {
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing dynamic import() expression" << std::endl;
        // Dynamic import() - evaluate the source path
        node.source->accept(*this);
        // For now, dynamic import returns the source path as a promise placeholder
        // Full implementation requires async module loading at runtime
        // lastValue_ is already set from the source expression
    }
    
void HIRGenerator::visit(Decorator& node) {
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing decorator @" << node.name << std::endl;

        // Decorators are functions that wrap/modify class or method definitions
        // For now, evaluate the decorator function and its arguments
        // The actual decoration happens in ClassDecl processing

        // Store decorator info for later use when processing the class/method
        // Evaluate decorator arguments if any
        for (auto& arg : node.arguments) {
            arg->accept(*this);
        }
    }
    
    // JSX/TSX Expressions
void HIRGenerator::visit(JSXElement& node) {
        static size_t jsxCounter = 0;
        const size_t id = jsxCounter++;

        std::vector<HIRStructType::Field> propFields;
        std::vector<HIRValue*> propValues;
        for (auto& attribute : node.attributes) {
            if (attribute->value) {
                attribute->value->accept(*this);
            } else {
                lastValue_ = builder_->createBoolConstant(true);
            }
            propFields.push_back({
                attribute->name, lastValue_->type, true
            });
            propValues.push_back(lastValue_);
        }
        for (size_t index = 0;
             index < node.spreadAttributes.size(); ++index) {
            node.spreadAttributes[index]->expression->accept(*this);
            propFields.push_back({
                "$spread" + std::to_string(index),
                lastValue_->type, true
            });
            propValues.push_back(lastValue_);
        }
        auto* propsType = new HIRStructType(
            "__jsx_props_" + std::to_string(id), propFields);
        HIRValue* props = builder_->createStructConstruct(
            propsType, propValues,
            "__jsx_props_" + std::to_string(id));

        std::vector<HIRValue*> children;
        children.reserve(node.children.size());
        for (auto& child : node.children) {
            child->accept(*this);
            children.push_back(lastValue_);
        }
        HIRValue* childArray = builder_->createArrayConstruct(
            children, "__jsx_children_" + std::to_string(id));
        HIRValue* tag = builder_->createStringConstant(node.tagName);

        // The default emit is a stable, framework-neutral virtual-node
        // record. A host can opt into a factory ABI by setting
        // NOVA_JSX_FACTORY to an externally linked function accepting
        // (tag, props, children).
        if (const char* factoryName = std::getenv("NOVA_JSX_FACTORY");
            factoryName && *factoryName) {
            auto existing = module_->getFunction(factoryName);
            HIRFunction* factory = existing ? existing.get() : nullptr;
            if (!factory) {
                auto stringType = std::make_shared<HIRType>(
                    HIRType::Kind::String);
                auto anyType = std::make_shared<HIRType>(
                    HIRType::Kind::Any);
                auto* factoryType = new HIRFunctionType(
                    {stringType, props->type, childArray->type}, anyType);
                auto created = module_->createFunction(
                    factoryName, factoryType);
                created->linkage = HIRFunction::Linkage::External;
                factory = created.get();
            }
            lastValue_ = builder_->createCall(
                factory, {tag, props, childArray}, "jsx.factory");
            return;
        }

        std::vector<HIRStructType::Field> vnodeFields = {
            {"type", tag->type, true},
            {"props", props->type, true},
            {"children", childArray->type, true}
        };
        auto* vnodeType = new HIRStructType(
            "__jsx_vnode_" + std::to_string(id), vnodeFields);
        lastValue_ = builder_->createStructConstruct(
            vnodeType, {tag, props, childArray},
            "__jsx_vnode_" + std::to_string(id));
    }
    
void HIRGenerator::visit(JSXFragment& node) {
        static size_t fragmentCounter = 0;
        std::vector<HIRValue*> children;
        children.reserve(node.children.size());
        for (auto& child : node.children) {
            child->accept(*this);
            children.push_back(lastValue_);
        }
        lastValue_ = builder_->createArrayConstruct(
            children,
            "__jsx_fragment_" +
                std::to_string(fragmentCounter++));
    }
    
void HIRGenerator::visit(JSXText& node) {
        // JSX text node - convert to string constant
        lastValue_ = builder_->createStringConstant(node.value);
    }
    
void HIRGenerator::visit(JSXExpressionContainer& node) {
        // JSX expression container - just evaluate inner expression
        node.expression->accept(*this);
    }
    
void HIRGenerator::visit(JSXAttribute& node) {
        if (node.value) {
            node.value->accept(*this);
        } else {
            lastValue_ = builder_->createBoolConstant(true);
        }
    }
    
void HIRGenerator::visit(JSXSpreadAttribute& node) {
        node.expression->accept(*this);
    }
    
    // Patterns (for destructuring)
void HIRGenerator::visit(ObjectPattern& node) {
        (void)node;
        // Object destructuring pattern
        // This is used in variable declarations and function parameters
        // TODO: Implement proper destructuring logic
    }
    
void HIRGenerator::visit(ArrayPattern& node) {
        (void)node;
        // Array destructuring pattern
        // TODO: Implement proper destructuring logic
    }
    
void HIRGenerator::visit(AssignmentPattern& node) {
        (void)node;
        // Pattern with default value
        // TODO: Implement default value assignment
    }
    
void HIRGenerator::visit(RestElement& node) {
        (void)node;
        // Rest element in destructuring (...rest)
        // TODO: Implement rest element collection
    }
    
void HIRGenerator::visit(IdentifierPattern& node) {
        // Simple identifier pattern - look up in symbol table
        auto it = symbolTable_.find(node.name);
        if (it != symbolTable_.end()) {
            lastValue_ = it->second;
        }
    }

void HIRGenerator::visit(InterfaceDecl& node) {
        (void)node;
        // Interface - type information only
    }
    
void HIRGenerator::visit(TypeAliasDecl& node) {
        (void)node;
        // Type alias - type information only
    }

void HIRGenerator::visit(NamespaceDecl& node) {
        (void)node;
        // Namespace declarations are erased from JavaScript output. Runtime
        // namespace values will be handled by the module/object lowering phase.
    }
    
void HIRGenerator::visit(EnumDecl& node) {
        // Enum declaration - store enum values in enumTable_
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing enum declaration: " << node.name << " with " << node.members.size() << " members" << std::endl;

        std::unordered_map<std::string, EnumValue> members;
        int64_t nextValue = 0;
        bool hasStringMember = false;

        for (const auto& member : node.members) {
            EnumValue ev;

            // If member has explicit initializer, evaluate it
            if (member.initializer) {
                if (auto* strLit = dynamic_cast<StringLiteral*>(member.initializer.get())) {
                    ev.kind = EnumValue::Kind::String;
                    ev.stringValue = strLit->value;
                    hasStringMember = true;
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: StringLiteral value = \"" << ev.stringValue << "\"" << std::endl;
                } else if (auto* numLit = dynamic_cast<NumberLiteral*>(member.initializer.get())) {
                    ev.kind = EnumValue::Kind::Number;
                    ev.numberValue = static_cast<int64_t>(numLit->value);
                    nextValue = ev.numberValue + 1;
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: NumberLiteral value = " << ev.numberValue << std::endl;
                } else {
                    // Fallback: treat as number with auto-value
                    ev.kind = EnumValue::Kind::Number;
                    ev.numberValue = nextValue;
                    nextValue++;
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Initializer is non-literal, using auto-value" << std::endl;
                }
            } else {
                // Auto-increment numeric value
                ev.kind = EnumValue::Kind::Number;
                ev.numberValue = nextValue;
                nextValue++;
            }

            members[member.name] = ev;
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Enum member " << node.name << "." << member.name << " = "
                                     << (ev.kind == EnumValue::Kind::String ? "\"" + ev.stringValue + "\"" : std::to_string(ev.numberValue))
                                     << std::endl;
        }

        enumTable_[node.name] = members;
        enumIsString_[node.name] = hasStringMember;
    }
    
void HIRGenerator::visit(ImportDecl& node) {
        // Import declaration - module system
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing import from '" << node.source << "'" << std::endl;

        // Check for nova: built-in modules
        if (node.source.substr(0, 5) == "nova:") {
            std::string moduleName = node.source.substr(5); // e.g., "fs", "test", "path", "os"
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Built-in module import: nova:" << moduleName << std::endl;

            // Register namespace import
            if (!node.namespaceImport.empty()) {
                builtinModuleImports_[node.namespaceImport] = "nova:" + moduleName;
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered namespace '" << node.namespaceImport << "' -> nova:" << moduleName << std::endl;
            }

            // Register named imports to their runtime functions
            for (const auto& spec : node.specifiers) {
                std::string runtimeFunc = getBuiltinFunctionName(moduleName, spec.imported);
                builtinFunctionImports_[spec.local] = runtimeFunc;
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered '" << spec.local << "' -> " << runtimeFunc << std::endl;
            }

            return; // Don't process further for built-in modules
        }

        // User-file module imports
        // Resolve the file path relative to current file
        std::string sourcePath = node.source;

        // Try common extensions
        std::vector<std::string> extensions = {".ts", ".js", ".tsx", ".jsx", ""};
        std::string resolvedPath;

        for (const auto& ext : extensions) {
            std::string candidate = sourcePath + ext;
            // Try relative path from current file directory
            if (!currentFilePath_.empty()) {
                std::filesystem::path currentDir = std::filesystem::path(currentFilePath_).parent_path();
                std::filesystem::path resolved = currentDir / candidate;
                if (std::filesystem::exists(resolved)) {
                    resolvedPath = resolved.string();
                    break;
                }
            }
            // Try as-is
            if (std::filesystem::exists(candidate)) {
                resolvedPath = candidate;
                break;
            }
        }

        if (resolvedPath.empty()) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Module file not found: " << sourcePath << std::endl;

            // Check if it's a Node.js built-in module name (without nova: prefix)
            // Map bare module names like "http", "fs", "path" to nova: built-in modules
            static const std::vector<std::string> builtinModuleNames = {
                "http", "https", "http2", "fs", "path", "os", "url", "util",
                "events", "stream", "buffer", "crypto", "net", "dns", "tls",
                "child_process", "cluster", "dgram", "readline", "repl",
                "string_decoder", "timers", "tty", "v8", "vm", "worker_threads",
                "zlib", "assert", "console", "process", "querystring", "perf_hooks"
            };

            bool isBuiltinModule = false;
            for (const auto& mod : builtinModuleNames) {
                if (sourcePath == mod) {
                    isBuiltinModule = true;
                    break;
                }
            }

            if (isBuiltinModule) {
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Treating '" << sourcePath << "' as built-in module nova:" << sourcePath << std::endl;

                // Register namespace import
                if (!node.namespaceImport.empty()) {
                    builtinModuleImports_[node.namespaceImport] = "nova:" + sourcePath;
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered namespace '" << node.namespaceImport << "' -> nova:" << sourcePath << std::endl;
                }

                // Register named imports to their runtime functions
                for (const auto& spec : node.specifiers) {
                    std::string runtimeFunc = getBuiltinFunctionName(sourcePath, spec.imported);
                    builtinFunctionImports_[spec.local] = runtimeFunc;
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered '" << spec.local << "' -> " << runtimeFunc << std::endl;
                }

                // Register default import if present
                if (!node.defaultImport.empty()) {
                    builtinModuleImports_[node.defaultImport] = "nova:" + sourcePath;
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered default '" << node.defaultImport << "' -> nova:" << sourcePath << std::endl;
                }

                return;
            }

            // Not a file import and not a built-in - might be a npm package, skip gracefully
            return;
        }

        // Check if already imported (prevent circular imports)
        if (importedModules_.find(resolvedPath) != importedModules_.end()) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Module already imported: " << resolvedPath << std::endl;
            // Re-bind specifiers from already-loaded module
            for (const auto& spec : node.specifiers) {
                auto it = symbolTable_.find(spec.imported);
                if (it != symbolTable_.end()) {
                    symbolTable_[spec.local] = it->second;
                }
            }
            return;
        }
        importedModules_.insert(resolvedPath);

        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Loading module from: " << resolvedPath << std::endl;

        // Read the imported file
        std::ifstream file(resolvedPath);
        if (!file.is_open()) {
            if (NOVA_DEBUG) std::cerr << "ERROR HIRGen: Cannot open module file: " << resolvedPath << std::endl;
            return;
        }
        std::stringstream ss;
        ss << file.rdbuf();
        std::string importedSource = ss.str();
        file.close();

        // Parse the imported file
        nova::Lexer lexer(resolvedPath, importedSource);
        if (lexer.hasErrors()) {
            if (NOVA_DEBUG) std::cerr << "ERROR HIRGen: Lexer errors in imported module: " << resolvedPath << std::endl;
            return;
        }

        nova::Parser parser(lexer);
        auto ast = parser.parseProgram();
        if (parser.hasErrors()) {
            if (NOVA_DEBUG) std::cerr << "ERROR HIRGen: Parser errors in imported module: " << resolvedPath << std::endl;
            return;
        }

        std::unordered_map<std::string, double> moduleNumbers;
        std::unordered_map<std::string, std::string> moduleStrings;
        std::unordered_map<std::string, bool> moduleBooleans;
        std::unordered_map<std::string, std::string> moduleFunctions;

        // Process the imported module's declarations
        // Save current state
        std::string savedFilePath = currentFilePath_;
        currentFilePath_ = resolvedPath;

        // During the program hoisting prepass there is no active function for
        // module-level allocas. Hoist exported functions and retain literal
        // constants as compiler bindings that can be materialized in each
        // importing function.
        const bool modulePrepass = currentFunction_ == nullptr;
        for (auto& stmt : ast->body) {
            if (!stmt) continue;
            if (!modulePrepass) {
                stmt->accept(*this);
                continue;
            }

            auto* declarationStatement = dynamic_cast<DeclStmt*>(stmt.get());
            auto* exported = declarationStatement
                ? dynamic_cast<ExportDecl*>(declarationStatement->declaration.get())
                : nullptr;
            if (!exported) {
                if (declarationStatement && declarationStatement->declaration &&
                    dynamic_cast<FunctionDecl*>(declarationStatement->declaration.get())) {
                    declarationStatement->declaration->accept(*this);
                }
                continue;
            }

            if (!exported->source.empty()) {
                ImportDecl reexport;
                reexport.source = exported->source;
                reexport.location = exported->location;
                for (const auto& specifier : exported->specifiers) {
                    ImportDecl::Specifier imported;
                    imported.imported = specifier.local;
                    imported.local = specifier.exported;
                    reexport.specifiers.push_back(std::move(imported));
                }
                visit(reexport);
            }

            if (exported->exportedDecl) {
                exported->exportedDecl->accept(*this);
                if (auto* function =
                        dynamic_cast<FunctionDecl*>(exported->exportedDecl.get())) {
                    moduleFunctions[function->name] = function->name;
                }
            }
            if (exported->isDefault && exported->declaration) {
                if (auto* number =
                        dynamic_cast<NumberLiteral*>(exported->declaration.get())) {
                    importedNumberConstants_["default"] = number->value;
                } else if (auto* string =
                               dynamic_cast<StringLiteral*>(exported->declaration.get())) {
                    importedStringConstants_["default"] = string->value;
                } else if (auto* boolean =
                               dynamic_cast<BooleanLiteral*>(exported->declaration.get())) {
                    importedBooleanConstants_["default"] = boolean->value;
                }
            }
            auto* variables = exported->exportedStmt
                ? dynamic_cast<VarDeclStmt*>(exported->exportedStmt.get()) : nullptr;
            if (!variables) continue;
            for (auto& declarator : variables->declarations) {
                if (auto* number = dynamic_cast<NumberLiteral*>(declarator.init.get())) {
                    importedNumberConstants_[declarator.name] = number->value;
                    moduleNumbers[declarator.name] = number->value;
                } else if (auto* string =
                               dynamic_cast<StringLiteral*>(declarator.init.get())) {
                    importedStringConstants_[declarator.name] = string->value;
                    moduleStrings[declarator.name] = string->value;
                } else if (auto* boolean =
                               dynamic_cast<BooleanLiteral*>(declarator.init.get())) {
                    importedBooleanConstants_[declarator.name] = boolean->value;
                    moduleBooleans[declarator.name] = boolean->value;
                }
            }
        }

        // Restore state
        currentFilePath_ = savedFilePath;

        // Bind imported names to local aliases
        for (const auto& spec : node.specifiers) {
            auto it = symbolTable_.find(spec.imported);
            if (it != symbolTable_.end()) {
                symbolTable_[spec.local] = it->second;
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Bound imported '" << spec.imported << "' as '" << spec.local << "'" << std::endl;
            } else {
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Warning - imported symbol not found: " << spec.imported << std::endl;
            }
            if (auto number = importedNumberConstants_.find(spec.imported);
                number != importedNumberConstants_.end()) {
                importedNumberConstants_[spec.local] = number->second;
            }
            if (auto string = importedStringConstants_.find(spec.imported);
                string != importedStringConstants_.end()) {
                importedStringConstants_[spec.local] = string->second;
            }
            if (auto boolean = importedBooleanConstants_.find(spec.imported);
                boolean != importedBooleanConstants_.end()) {
                importedBooleanConstants_[spec.local] = boolean->second;
            }
            if (module_->getFunction(spec.imported) && spec.local != spec.imported) {
                functionReferences_[spec.local] = spec.imported;
            } else if (auto function = functionReferences_.find(spec.imported);
                       function != functionReferences_.end()) {
                functionReferences_[spec.local] = function->second;
            }
        }

        // Handle default imports
        if (!node.defaultImport.empty()) {
            auto it = symbolTable_.find("default");
            if (it != symbolTable_.end()) {
                symbolTable_[node.defaultImport] = it->second;
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Bound default import as '" << node.defaultImport << "'" << std::endl;
            }
            if (auto number = importedNumberConstants_.find("default");
                number != importedNumberConstants_.end()) {
                importedNumberConstants_[node.defaultImport] = number->second;
            }
            if (auto string = importedStringConstants_.find("default");
                string != importedStringConstants_.end()) {
                importedStringConstants_[node.defaultImport] = string->second;
            }
            if (auto boolean = importedBooleanConstants_.find("default");
                boolean != importedBooleanConstants_.end()) {
                importedBooleanConstants_[node.defaultImport] = boolean->second;
            }
        }

        // Handle namespace imports
        if (!node.namespaceImport.empty()) {
            moduleNamespaceNumberConstants_[node.namespaceImport] =
                std::move(moduleNumbers);
            moduleNamespaceStringConstants_[node.namespaceImport] =
                std::move(moduleStrings);
            moduleNamespaceBooleanConstants_[node.namespaceImport] =
                std::move(moduleBooleans);
            moduleNamespaceFunctions_[node.namespaceImport] =
                std::move(moduleFunctions);
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Namespace import as '" << node.namespaceImport << "'" << std::endl;
        }
    }

// Helper to get runtime function name for built-in module functions
std::string HIRGenerator::getBuiltinFunctionName(const std::string& module, const std::string& funcName) {
        // Map module:function to nova_module_function
        // nova:fs -> nova_fs_*
        // nova:test -> nova_test_*
        // nova:path -> nova_path_*
        // nova:os -> nova_os_*
        return "nova_" + module + "_" + funcName;
    }

// Check if a function call is to a built-in module function
bool HIRGenerator::isBuiltinFunctionCall(const std::string& name) {
        return builtinFunctionImports_.find(name) != builtinFunctionImports_.end();
    }

// Get the runtime function name for a built-in import
std::string HIRGenerator::getBuiltinRuntimeFunction(const std::string& name) {
        auto it = builtinFunctionImports_.find(name);
        if (it != builtinFunctionImports_.end()) {
            return it->second;
        }
        return "";
    }

void HIRGenerator::visit(ExportDecl& node) {
        // Export declaration - process any exported declaration
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing export declaration" << std::endl;

        if (node.isDefault) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Export default" << std::endl;
        }

        // If there's an exported declaration (e.g., export function foo() {}), process it
        if (node.exportedDecl) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing exported declaration" << std::endl;
            node.exportedDecl->accept(*this);
        }

        if (node.exportedStmt) {
            node.exportedStmt->accept(*this);
        }

        // If there's a declaration expression (e.g., export default someExpr), evaluate it
        if (node.declaration) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing export declaration expression" << std::endl;
            node.declaration->accept(*this);
        }

        // Log re-exports
        if (!node.source.empty()) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Re-export from '" << node.source << "'" << std::endl;
        }

        for (const auto& spec : node.specifiers) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Export '" << spec.local << "' as '" << spec.exported << "'" << std::endl;
        }
    }
    
void HIRGenerator::visit(Program& node) {
        const auto savedDynamicBindingNames = dynamicBindingNames_;
        const auto savedInferredFunctionParameterTypes = inferredFunctionParameterTypes_;
        BlockStmt topLevelBody(node.body);
        dynamicBindingNames_ = analyzeDynamicBindings(&topLevelBody);
        inferredFunctionParameterTypes_ = analyzeFunctionParameterTypes(node);
        // Collect function/class declarations first (hoisting)
        std::vector<size_t> importDeclIndices;
        std::vector<size_t> functionDeclIndices;
        std::vector<size_t> topLevelIndices;

        for (size_t i = 0; i < node.body.size(); ++i) {
            auto& stmt = node.body[i];
            if (!stmt) continue;

            // Check if this is a DeclStmt containing a function or class declaration
            bool isImport = false;
            bool isDeclaration = false;
            if (auto* declStmt = dynamic_cast<DeclStmt*>(stmt.get())) {
                if (declStmt->declaration) {
                    isImport = dynamic_cast<ImportDecl*>(
                        declStmt->declaration.get()) != nullptr;
                    if (dynamic_cast<FunctionDecl*>(declStmt->declaration.get()) ||
                        dynamic_cast<ClassDecl*>(declStmt->declaration.get()) ||
                        dynamic_cast<InterfaceDecl*>(declStmt->declaration.get()) ||
                        dynamic_cast<TypeAliasDecl*>(declStmt->declaration.get()) ||
                        dynamic_cast<EnumDecl*>(declStmt->declaration.get())) {
                        isDeclaration = true;
                    }
                }
            }

            if (isImport) {
                importDeclIndices.push_back(i);
            } else if (isDeclaration) {
                functionDeclIndices.push_back(i);
            } else {
                topLevelIndices.push_back(i);
            }
        }

        // Hoisted function bodies may reference primordial built-ins captured
        // by top-level variables declared earlier in source. Since Nova emits
        // all FunctionDecl bodies before top-level initializers, pre-register
        // these aliases here so calls inside those bodies do not collapse to
        // placeholders.
        auto memberPath = [](Expr* expression) {
            std::vector<std::string> parts;
            Expr* cursor = expression;
            while (auto* member =
                       dynamic_cast<MemberExpr*>(cursor)) {
                if (member->isComputed) {
                    parts.clear();
                    break;
                }
                auto* property = dynamic_cast<Identifier*>(
                    member->property.get());
                if (!property) {
                    parts.clear();
                    break;
                }
                parts.push_back(property->name);
                cursor = member->object.get();
            }
            if (auto* base = dynamic_cast<Identifier*>(cursor)) {
                parts.push_back(base->name);
            } else {
                parts.clear();
            }
            std::reverse(parts.begin(), parts.end());
            std::string path;
            for (const auto& part : parts) {
                if (!path.empty()) path += ".";
                path += part;
            }
            return path;
        };
        auto pointerType =
            std::make_shared<HIRType>(HIRType::Kind::Pointer);
        auto stringType =
            std::make_shared<HIRType>(HIRType::Kind::String);
        auto integerType =
            std::make_shared<HIRType>(HIRType::Kind::I64);
        auto booleanType =
            std::make_shared<HIRType>(HIRType::Kind::Bool);
        auto jsValueType =
            std::make_shared<HIRType>(HIRType::Kind::JSValue);
        auto registerPrimordial =
            [&](const std::string& alias,
                const std::string& runtimeName,
                std::vector<HIRTypePtr> parameters,
                HIRTypePtr result) {
                if (!module_->getFunction(runtimeName)) {
                    auto* type =
                        new HIRFunctionType(parameters, result);
                    auto function =
                        module_->createFunction(runtimeName, type);
                    function->linkage =
                        HIRFunction::Linkage::External;
                }
                functionReferences_[alias] = runtimeName;
            };
        for (const auto& statement : node.body) {
            auto* variables =
                dynamic_cast<VarDeclStmt*>(statement.get());
            if (!variables) continue;
            for (const auto& declaration : variables->declarations) {
                if (declaration.name.empty() || !declaration.init) {
                    continue;
                }
                const std::string direct =
                    memberPath(declaration.init.get());
                if (direct == "Object.getOwnPropertyDescriptor") {
                    registerPrimordial(
                        declaration.name,
                        "nova_object_getOwnPropertyDescriptor",
                        {pointerType, stringType}, pointerType);
                } else if (direct == "Object.getOwnPropertyNames") {
                    registerPrimordial(
                        declaration.name,
                        "nova_object_getOwnPropertyNames",
                        {pointerType}, pointerType);
                } else if (direct == "Object.defineProperty") {
                    registerPrimordial(
                        declaration.name,
                        "nova_object_defineProperty",
                        {pointerType, stringType, pointerType},
                        pointerType);
                } else if (direct == "Array.isArray") {
                    registerPrimordial(
                        declaration.name,
                        "nova_value_is_array",
                        {jsValueType}, booleanType);
                } else if (auto* bind =
                               dynamic_cast<CallExpr*>(
                                   declaration.init.get())) {
                    if (memberPath(bind->callee.get()) !=
                            "Function.prototype.call.bind" ||
                        bind->arguments.empty()) {
                        continue;
                    }
                    const std::string target =
                        memberPath(bind->arguments.front().get());
                    if (target ==
                        "Object.prototype.hasOwnProperty") {
                        registerPrimordial(
                            declaration.name,
                            "nova_object_hasOwnProperty",
                            {pointerType, stringType}, booleanType);
                    } else if (target ==
                               "Object.prototype.propertyIsEnumerable") {
                        registerPrimordial(
                            declaration.name,
                            "nova_object_propertyIsEnumerable",
                            {pointerType, stringType}, booleanType);
                    } else if (target ==
                               "Array.prototype.push") {
                        registerPrimordial(
                            declaration.name,
                            "nova_value_array_push",
                            {pointerType, integerType}, integerType);
                    } else if (target ==
                               "Array.prototype.join") {
                        registerPrimordial(
                            declaration.name,
                            "nova_value_array_join",
                            {pointerType, stringType}, stringType);
                    }
                }
            }
        }

        // Resolve imports before local function bodies so imported bindings are
        // visible while those bodies are generated.
        for (size_t idx : importDeclIndices) {
            node.body[idx]->accept(*this);
        }

        // Emit ordinary function declarations in dependency order. The
        // previous single hoisting pass generated a caller body before a
        // later-declared callee existed in the HIR module, so valid forward
        // calls silently reused a stale expression value. A dependency walk
        // preserves JavaScript function hoisting while retaining the existing
        // one-pass body generator.
        std::unordered_map<std::string, size_t> functionIndexByName;
        for (size_t idx : functionDeclIndices) {
            auto* statement =
                dynamic_cast<DeclStmt*>(node.body[idx].get());
            auto* function = statement
                ? dynamic_cast<FunctionDecl*>(
                      statement->declaration.get())
                : nullptr;
            if (function) {
                functionIndexByName[function->name] = idx;
                // Classes are emitted before ordinary function bodies so
                // their layouts are available to `main`. Register the AST
                // declarations up front as well, allowing class decorators
                // to inspect a decorator declared later in the source.
                functionDeclarations_[function->name] = function;
            }
        }
        std::unordered_map<size_t, std::unordered_set<std::string>>
            functionDependencies;
        std::function<void(Expr*, std::unordered_set<std::string>&)>
            scanExpression;
        std::function<void(Stmt*, std::unordered_set<std::string>&)>
            scanStatement;
        scanExpression =
            [&](Expr* expression,
                std::unordered_set<std::string>& dependencies) {
                if (!expression) return;
                if (auto* call =
                        dynamic_cast<CallExpr*>(expression)) {
                    if (auto* identifier =
                            dynamic_cast<Identifier*>(
                                call->callee.get())) {
                        if (functionIndexByName.count(
                                identifier->name) > 0) {
                            dependencies.insert(identifier->name);
                        }
                    }
                    scanExpression(call->callee.get(), dependencies);
                    for (auto& argument : call->arguments) {
                        scanExpression(argument.get(), dependencies);
                    }
                } else if (auto* binary =
                               dynamic_cast<BinaryExpr*>(expression)) {
                    scanExpression(binary->left.get(), dependencies);
                    scanExpression(binary->right.get(), dependencies);
                } else if (auto* unary =
                               dynamic_cast<UnaryExpr*>(expression)) {
                    scanExpression(unary->operand.get(), dependencies);
                } else if (auto* assignment =
                               dynamic_cast<AssignmentExpr*>(expression)) {
                    scanExpression(assignment->left.get(), dependencies);
                    scanExpression(assignment->right.get(), dependencies);
                } else if (auto* update =
                               dynamic_cast<UpdateExpr*>(expression)) {
                    scanExpression(update->argument.get(), dependencies);
                } else if (auto* member =
                               dynamic_cast<MemberExpr*>(expression)) {
                    scanExpression(member->object.get(), dependencies);
                    scanExpression(member->property.get(), dependencies);
                } else if (auto* conditional =
                               dynamic_cast<ConditionalExpr*>(expression)) {
                    scanExpression(conditional->test.get(), dependencies);
                    scanExpression(
                        conditional->consequent.get(), dependencies);
                    scanExpression(
                        conditional->alternate.get(), dependencies);
                } else if (auto* array =
                               dynamic_cast<ArrayExpr*>(expression)) {
                    for (auto& element : array->elements) {
                        scanExpression(element.get(), dependencies);
                    }
                } else if (auto* object =
                               dynamic_cast<ObjectExpr*>(expression)) {
                    for (auto& property : object->properties) {
                        scanExpression(
                            property.value.get(), dependencies);
                    }
                } else if (auto* parenthesized =
                               dynamic_cast<ParenthesizedExpr*>(
                                   expression)) {
                    scanExpression(
                        parenthesized->expression.get(),
                        dependencies);
                } else if (auto* sequence =
                               dynamic_cast<SequenceExpr*>(expression)) {
                    for (auto& item : sequence->expressions) {
                        scanExpression(item.get(), dependencies);
                    }
                } else if (auto* awaited =
                               dynamic_cast<AwaitExpr*>(expression)) {
                    scanExpression(awaited->argument.get(), dependencies);
                } else if (auto* yielded =
                               dynamic_cast<YieldExpr*>(expression)) {
                    scanExpression(yielded->argument.get(), dependencies);
                }
            };
        scanStatement =
            [&](Stmt* statement,
                std::unordered_set<std::string>& dependencies) {
                if (!statement) return;
                if (auto* block = dynamic_cast<BlockStmt*>(statement)) {
                    for (auto& item : block->statements) {
                        scanStatement(item.get(), dependencies);
                    }
                } else if (auto* expression =
                               dynamic_cast<ExprStmt*>(statement)) {
                    scanExpression(
                        expression->expression.get(), dependencies);
                } else if (auto* variables =
                               dynamic_cast<VarDeclStmt*>(statement)) {
                    for (auto& declaration : variables->declarations) {
                        scanExpression(
                            declaration.init.get(), dependencies);
                    }
                } else if (auto* conditional =
                               dynamic_cast<IfStmt*>(statement)) {
                    scanExpression(
                        conditional->test.get(), dependencies);
                    scanStatement(
                        conditional->consequent.get(), dependencies);
                    scanStatement(
                        conditional->alternate.get(), dependencies);
                } else if (auto* whileLoop =
                               dynamic_cast<WhileStmt*>(statement)) {
                    scanExpression(
                        whileLoop->test.get(), dependencies);
                    scanStatement(
                        whileLoop->body.get(), dependencies);
                } else if (auto* forLoop =
                               dynamic_cast<ForStmt*>(statement)) {
                    scanStatement(
                        forLoop->init.get(), dependencies);
                    scanExpression(
                        forLoop->test.get(), dependencies);
                    scanExpression(
                        forLoop->update.get(), dependencies);
                    scanStatement(
                        forLoop->body.get(), dependencies);
                } else if (auto* returned =
                               dynamic_cast<ReturnStmt*>(statement)) {
                    scanExpression(
                        returned->argument.get(), dependencies);
                } else if (auto* thrown =
                               dynamic_cast<ThrowStmt*>(statement)) {
                    scanExpression(thrown->argument.get(), dependencies);
                } else if (auto* attempted =
                               dynamic_cast<TryStmt*>(statement)) {
                    scanStatement(attempted->block.get(), dependencies);
                    if (attempted->handler) {
                        scanStatement(
                            attempted->handler->body.get(),
                            dependencies);
                    }
                    scanStatement(
                        attempted->finalizer.get(), dependencies);
                }
            };
        for (const auto& entry : functionIndexByName) {
            auto* declaration = dynamic_cast<DeclStmt*>(
                node.body[entry.second].get());
            auto* function = declaration
                ? dynamic_cast<FunctionDecl*>(
                      declaration->declaration.get())
                : nullptr;
            if (function) {
                scanStatement(
                    function->body.get(),
                    functionDependencies[entry.second]);
            }
        }
        std::vector<size_t> orderedFunctionDeclIndices;
        // Class/enum/type metadata must exist before any function body that
        // constructs or inspects those declarations. They do not participate
        // in the ordinary-function dependency graph.
        for (size_t idx : functionDeclIndices) {
            auto* declaration =
                dynamic_cast<DeclStmt*>(node.body[idx].get());
            if (!declaration ||
                !dynamic_cast<FunctionDecl*>(
                    declaration->declaration.get())) {
                orderedFunctionDeclIndices.push_back(idx);
            }
        }
        std::unordered_set<size_t> visiting;
        std::unordered_set<size_t> visited;
        std::function<void(size_t)> visitDependency =
            [&](size_t index) {
                if (visited.count(index) > 0) return;
                if (visiting.count(index) > 0) return;
                visiting.insert(index);
                for (const auto& dependency :
                     functionDependencies[index]) {
                    auto found =
                        functionIndexByName.find(dependency);
                    if (found != functionIndexByName.end() &&
                        found->second != index) {
                        visitDependency(found->second);
                    }
                }
                visiting.erase(index);
                visited.insert(index);
                orderedFunctionDeclIndices.push_back(index);
            };
        for (size_t idx : functionDeclIndices) {
            if (functionDependencies.count(idx) > 0 ||
                std::any_of(
                    functionIndexByName.begin(),
                    functionIndexByName.end(),
                    [&](const auto& item) {
                        return item.second == idx;
                    })) {
                visitDependency(idx);
            }
        }
        // Process function/class declarations first (they can be used anywhere)
        for (size_t idx : orderedFunctionDeclIndices) {
            node.body[idx]->accept(*this);
        }

        // If there are top-level statements (not just declarations), create an implicit main function
        if (!topLevelIndices.empty()) {
            // Create main function signature: int __nova_main()
            std::vector<HIRTypePtr> paramTypes;
            auto returnType = std::make_shared<HIRType>(HIRType::Kind::I32);

            HIRFunctionType* funcType = new HIRFunctionType(paramTypes, returnType);
            HIRFunctionPtr mainFunc = module_->createFunction("__nova_main", funcType);
            mainFunc->linkage = HIRFunction::Linkage::Public;

            // Create entry block
            auto entryBlock = mainFunc->createBasicBlock("entry");

            // Set up the builder for main function
            auto savedFunction = currentFunction_;
            currentFunction_ = mainFunc.get();
            builder_ = std::make_unique<HIRBuilder>(module_, mainFunc.get());
            builder_->setInsertPoint(entryBlock.get());

            // Process top-level statements inside main
            for (size_t idx : topLevelIndices) {
                node.body[idx]->accept(*this);
            }

            // Add return 0 at the end of main
            auto* zeroValue = builder_->createIntConstant(0);
            builder_->createReturn(zeroValue);

            // Restore state
            currentFunction_ = savedFunction;
            builder_.reset();
        }
        dynamicBindingNames_ = savedDynamicBindingNames;
        inferredFunctionParameterTypes_ = savedInferredFunctionParameterTypes;
    }

} // namespace nova::hir
