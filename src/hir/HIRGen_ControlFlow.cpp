// HIRGen_ControlFlow.cpp - Control flow statement visitors
// Extracted from HIRGen.cpp for better code organization

#include "nova/HIR/HIRGen_Internal.h"
#define NOVA_DEBUG 0

namespace nova::hir {

void HIRGenerator::visit(IfStmt& node) {
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
        if (!thenBlock->hasBreakOrContinue) {
            builder_->createBr(endBlock);
        }
        
        // Generate else block
        if (elseBlock) {
            builder_->setInsertPoint(elseBlock);
            node.alternate->accept(*this);
            
            // Only add branch to end block if the else block doesn't end with a return, break, or continue
            if (!elseBlock->hasBreakOrContinue) {
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
    
void HIRGenerator::visit(WhileStmt& node) {
        if(NOVA_DEBUG) std::cerr << "DEBUG: Entering WhileStmt generation" << std::endl;

        // Include label in block names if we're inside a labeled statement
        std::string labelSuffix = currentLabel_.empty() ? "" : "#" + currentLabel_;
        // Clear label after using it - label only applies to this loop, not nested loops
        currentLabel_.clear();

        auto* condBlock = currentFunction_->createBasicBlock("while.cond" + labelSuffix).get();
        auto* bodyBlock = currentFunction_->createBasicBlock("while.body" + labelSuffix).get();
        auto* endBlock = currentFunction_->createBasicBlock("while.end" + labelSuffix).get();

        if(NOVA_DEBUG) std::cerr << "DEBUG: Created while loop blocks: cond=" << condBlock << ", body=" << bodyBlock << ", end=" << endBlock << std::endl;

        // Push break and continue targets onto stacks
        breakTargetStack_.push_back(endBlock);
        continueTargetStack_.push_back(condBlock);

        // Jump to condition
        builder_->createBr(condBlock);

        // Condition block
        builder_->setInsertPoint(condBlock);
        if(NOVA_DEBUG) std::cerr << "DEBUG: Evaluating while condition" << std::endl;
        node.test->accept(*this);
        if(NOVA_DEBUG) std::cerr << "DEBUG: While condition evaluated, lastValue_=" << lastValue_ << std::endl;
        builder_->createCondBr(lastValue_, bodyBlock, endBlock);

        // Body block
        builder_->setInsertPoint(bodyBlock);
        if(NOVA_DEBUG) std::cerr << "DEBUG: Executing while body" << std::endl;
        node.body->accept(*this);
        if(NOVA_DEBUG) std::cerr << "DEBUG: While body executed" << std::endl;

        // Check if body block has a terminator (break/continue/return all set hasBreakOrContinue flag)
        if (!bodyBlock->hasBreakOrContinue) {
            if(NOVA_DEBUG) std::cerr << "DEBUG: Creating branch back to condition" << std::endl;
            builder_->createBr(condBlock);
        } else {
            if(NOVA_DEBUG) std::cerr << "DEBUG: Body block ends with terminator, not adding branch back to condition" << std::endl;
        }

        // Pop break and continue targets from stacks
        breakTargetStack_.pop_back();
        continueTargetStack_.pop_back();

        // End block
        builder_->setInsertPoint(endBlock);
        if(NOVA_DEBUG) std::cerr << "DEBUG: While loop generation completed" << std::endl;
    }
    
void HIRGenerator::visit(DoWhileStmt& node) {
        // Create basic blocks for the do-while loop
        // Include label in block names if we're inside a labeled statement
        std::string labelSuffix = currentLabel_.empty() ? "" : "#" + currentLabel_;
        // Clear label after using it - label only applies to this loop, not nested loops
        currentLabel_.clear();

        auto* bodyBlock = currentFunction_->createBasicBlock("do-while.body" + labelSuffix).get();
        auto* condBlock = currentFunction_->createBasicBlock("do-while.cond" + labelSuffix).get();
        auto* endBlock = currentFunction_->createBasicBlock("do-while.end" + labelSuffix).get();
        
        // Jump to body block (do-while always executes at least once)
        builder_->createBr(bodyBlock);
        
        // Body block
        builder_->setInsertPoint(bodyBlock);
        node.body->accept(*this);
        
        // Check if body block has a terminator
        if (!bodyBlock->hasBreakOrContinue) {
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
    
void HIRGenerator::visit(ForStmt& node) {
        if(NOVA_DEBUG) std::cerr << "DEBUG: Entering ForStmt generation" << std::endl;

        // Include label in block names if we're inside a labeled statement
        std::string labelSuffix = currentLabel_.empty() ? "" : "#" + currentLabel_;
        // Clear label after using it - label only applies to this loop, not nested loops
        currentLabel_.clear();

        // Create basic blocks for the for loop
        auto* initBlock = currentFunction_->createBasicBlock("for.init" + labelSuffix).get();
        auto* condBlock = currentFunction_->createBasicBlock("for.cond" + labelSuffix).get();
        auto* bodyBlock = currentFunction_->createBasicBlock("for.body" + labelSuffix).get();
        auto* updateBlock = currentFunction_->createBasicBlock("for.update" + labelSuffix).get();
        auto* endBlock = currentFunction_->createBasicBlock("for.end" + labelSuffix).get();
        
        if(NOVA_DEBUG) std::cerr << "DEBUG: Created for loop blocks: init=" << initBlock << ", cond=" << condBlock
                  << ", body=" << bodyBlock << ", update=" << updateBlock << ", end=" << endBlock << std::endl;

        // Push break and continue targets onto stacks
        breakTargetStack_.push_back(endBlock);
        continueTargetStack_.push_back(updateBlock);

        // Branch to init block
        builder_->createBr(initBlock);
        
        // Init block - execute initializer
        builder_->setInsertPoint(initBlock);
        if(NOVA_DEBUG) std::cerr << "DEBUG: Executing for init" << std::endl;
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
        if(NOVA_DEBUG) std::cerr << "DEBUG: For init executed" << std::endl;
        // Branch to condition
        builder_->createBr(condBlock);
        
        // Condition block
        builder_->setInsertPoint(condBlock);
        if(NOVA_DEBUG) std::cerr << "DEBUG: Evaluating for condition" << std::endl;
        if (node.test) {
            node.test->accept(*this);
            auto* condition = lastValue_;
            if(NOVA_DEBUG) std::cerr << "DEBUG: For condition evaluated, condition=" << condition << std::endl;
            builder_->createCondBr(condition, bodyBlock, endBlock);
        } else {
            // No condition means infinite loop
            if(NOVA_DEBUG) std::cerr << "DEBUG: No for condition, creating infinite loop" << std::endl;
            builder_->createBr(bodyBlock);
        }
        
        // Body block
        builder_->setInsertPoint(bodyBlock);
        if(NOVA_DEBUG) std::cerr << "DEBUG: Executing for body" << std::endl;
        node.body->accept(*this);
        if(NOVA_DEBUG) std::cerr << "DEBUG: For body executed" << std::endl;

        // Check if the body block itself ends with a terminator instruction
        // (break, continue, or return). If it does, don't add a branch.
        // Otherwise, always add the branch to update block to maintain proper CFG.
        bool needsBranch = true;
        if (!bodyBlock->instructions.empty()) {
            auto lastOpcode = bodyBlock->instructions.back()->opcode;
            if (lastOpcode == hir::HIRInstruction::Opcode::Break ||
                lastOpcode == hir::HIRInstruction::Opcode::Continue ||
                lastOpcode == hir::HIRInstruction::Opcode::Return) {
                needsBranch = false;
                if(NOVA_DEBUG) std::cerr << "DEBUG: Body block ends with terminator, not adding branch to update" << std::endl;
            }
        }

        if (needsBranch) {
            // Always branch to update block to maintain proper CFG and enable loop detection
            if(NOVA_DEBUG) std::cerr << "DEBUG: Creating branch from body to update block" << std::endl;
            builder_->createBr(updateBlock);
        }
        
        // Update block
        builder_->setInsertPoint(updateBlock);
        if(NOVA_DEBUG) std::cerr << "DEBUG: Executing for update" << std::endl;
        if (node.update) {
            node.update->accept(*this);
            // Result of update expression is ignored
        }
        if(NOVA_DEBUG) std::cerr << "DEBUG: For update executed" << std::endl;
        // Branch back to condition
        builder_->createBr(condBlock);
        
        // End block
        builder_->setInsertPoint(endBlock);

        // Pop break and continue targets from stacks
        breakTargetStack_.pop_back();
        continueTargetStack_.pop_back();

        if(NOVA_DEBUG) std::cerr << "DEBUG: For loop generation completed" << std::endl;
    }
    
void HIRGenerator::visit(ForInStmt& node) {
        if(NOVA_DEBUG) std::cerr << "DEBUG: Generating for-in loop" << std::endl;

        // For-in loop: for (let key in obj) { body }
        // Desugar to:
        //   let __keys = nova_object_keys(obj);  // Get array of property keys
        //   let __iter_idx = 0;
        //   while (__iter_idx < __keys.length) {
        //       let key = __keys[__iter_idx];  // Extract key string from array
        //       body;
        //       __iter_idx = __iter_idx + 1;
        //   }

        // Create basic blocks (with label suffix if inside labeled statement)
        std::string labelSuffix = currentLabel_.empty() ? "" : "#" + currentLabel_;
        // Clear label after using it - label only applies to this loop, not nested loops
        currentLabel_.clear();

        auto* initBlock = currentFunction_->createBasicBlock("forin.init" + labelSuffix).get();
        auto* condBlock = currentFunction_->createBasicBlock("forin.cond" + labelSuffix).get();
        auto* bodyBlock = currentFunction_->createBasicBlock("forin.body" + labelSuffix).get();
        auto* updateBlock = currentFunction_->createBasicBlock("forin.update" + labelSuffix).get();
        auto* endBlock = currentFunction_->createBasicBlock("forin.end" + labelSuffix).get();

        // Branch to init block
        builder_->createBr(initBlock);

        // Init block: evaluate the iterable expression and get its keys
        builder_->setInsertPoint(initBlock);
        if(NOVA_DEBUG) std::cerr << "DEBUG: ForIn - evaluating iterable" << std::endl;
        node.right->accept(*this);  // Evaluate object/array expression
        auto* objectValue = lastValue_;

        // Call nova_object_keys() to get array of property keys
        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
        auto i64Type = std::make_shared<HIRType>(HIRType::Kind::I64);

        // Get or create nova_object_keys function
        std::vector<HIRTypePtr> keysParamTypes = {ptrType};
        HIRFunction* objectKeysFunc = nullptr;
        auto existingKeysFunc = module_->getFunction("nova_object_keys");
        if (existingKeysFunc) {
            objectKeysFunc = existingKeysFunc.get();
        } else {
            HIRFunctionType* funcType = new HIRFunctionType(keysParamTypes, ptrType);
            HIRFunctionPtr funcPtr = module_->createFunction("nova_object_keys", funcType);
            funcPtr->linkage = HIRFunction::Linkage::External;
            objectKeysFunc = funcPtr.get();
        }

        // Call nova_object_keys(objectValue) to get keys array
        std::vector<HIRValue*> keysArgs = {objectValue};
        auto* keysArray = builder_->createCall(objectKeysFunc, keysArgs, "__forin_keys");

        // Create iterator index variable: let __iter_idx = 0
        auto* indexType = new hir::HIRType(hir::HIRType::Kind::I64);
        auto* indexVar = builder_->createAlloca(indexType, "__forin_idx");
        auto* zeroConst = builder_->createIntConstant(0);
        builder_->createStore(zeroConst, indexVar);

        // Branch to condition
        builder_->createBr(condBlock);

        // Condition block: __iter_idx < keys.length
        builder_->setInsertPoint(condBlock);
        if(NOVA_DEBUG) std::cerr << "DEBUG: ForIn - checking condition" << std::endl;

        // Load current index
        auto* currentIndex = builder_->createLoad(indexVar);

        // Get keys array length (field 1 is length)
        auto* keysLength = builder_->createGetField(keysArray, 1);

        // Compare: __iter_idx < keys.length
        auto* condition = builder_->createLt(currentIndex, keysLength);
        builder_->createCondBr(condition, bodyBlock, endBlock);

        // Body block: let key = keys[__iter_idx]; body;
        builder_->setInsertPoint(bodyBlock);
        if(NOVA_DEBUG) std::cerr << "DEBUG: ForIn - executing body" << std::endl;

        // Load current index
        auto* indexForKey = builder_->createLoad(indexVar);

        // Get or create value_array_get function to extract key from array
        std::vector<HIRTypePtr> getParamTypes = {ptrType, i64Type};
        HIRFunction* getFunc = nullptr;
        auto existingGetFunc = module_->getFunction("value_array_get");
        if (existingGetFunc) {
            getFunc = existingGetFunc.get();
        } else {
            HIRFunctionType* funcType = new HIRFunctionType(getParamTypes, i64Type);
            HIRFunctionPtr funcPtr = module_->createFunction("value_array_get", funcType);
            funcPtr->linkage = HIRFunction::Linkage::External;
            getFunc = funcPtr.get();
        }

        // Extract the key string from keys array: keys[index]
        std::vector<HIRValue*> getArgs = {keysArray, indexForKey};
        auto* keyString = builder_->createCall(getFunc, getArgs, "__forin_key_str");

        // Declare loop variable and assign the key string
        // node.left is the variable name (e.g., "key" in "for (let key in obj)")
        auto* keyType = new hir::HIRType(hir::HIRType::Kind::I64);  // String represented as i64
        auto* loopVar = builder_->createAlloca(keyType, node.left);

        // Store the key string in the loop variable (key = keyString, not index!)
        builder_->createStore(keyString, loopVar);

        // Track the variable
        symbolTable_[node.left] = loopVar;

        // Push break and continue targets onto stacks for nested break/continue support
        breakTargetStack_.push_back(endBlock);
        continueTargetStack_.push_back(updateBlock);

        // Execute loop body
        node.body->accept(*this);

        // Pop break and continue targets from stacks
        breakTargetStack_.pop_back();
        continueTargetStack_.pop_back();

        // Check if body ends with terminator
        bool needsBranch = true;
        if (!bodyBlock->instructions.empty()) {
            auto lastOpcode = bodyBlock->instructions.back()->opcode;
            if (lastOpcode == hir::HIRInstruction::Opcode::Break ||
                lastOpcode == hir::HIRInstruction::Opcode::Continue ||
                lastOpcode == hir::HIRInstruction::Opcode::Return) {
                needsBranch = false;
            }
        }

        if (needsBranch) {
            builder_->createBr(updateBlock);
        }

        // Update block: __iter_idx = __iter_idx + 1
        builder_->setInsertPoint(updateBlock);
        if(NOVA_DEBUG) std::cerr << "DEBUG: ForIn - incrementing index" << std::endl;

        auto* currentIndexForIncr = builder_->createLoad(indexVar);
        auto* oneConst = builder_->createIntConstant(1);
        auto* nextIndex = builder_->createAdd(currentIndexForIncr, oneConst);
        builder_->createStore(nextIndex, indexVar);

        // Branch back to condition
        builder_->createBr(condBlock);

        // End block
        builder_->setInsertPoint(endBlock);

        if(NOVA_DEBUG) std::cerr << "DEBUG: ForIn loop generation completed" << std::endl;
    }
    
void HIRGenerator::visit(ForOfStmt& node) {
        if(NOVA_DEBUG) std::cerr << "DEBUG: Generating for-of loop" << std::endl;

        // Check if iterating over a generator or async generator
        bool isGeneratorIteration = false;
        bool isAsyncGeneratorIteration = false;
        if (auto* identExpr = dynamic_cast<Identifier*>(node.right.get())) {
            if (asyncGeneratorVars_.count(identExpr->name) > 0) {
                isAsyncGeneratorIteration = true;
                isGeneratorIteration = true;  // Async generators are also generators
                if(NOVA_DEBUG) std::cerr << "DEBUG: ForOf - iterating over async generator: " << identExpr->name << std::endl;
            } else if (generatorVars_.count(identExpr->name) > 0) {
                isGeneratorIteration = true;
                if(NOVA_DEBUG) std::cerr << "DEBUG: ForOf - iterating over generator: " << identExpr->name << std::endl;
            }
        }

        // Handle for await...of (async iteration)
        if (node.isAwait && !isAsyncGeneratorIteration) {
            std::cerr << "NOTE: 'for await...of' on non-async-generator compiled as synchronous iteration" << std::endl;
        }

        if (isGeneratorIteration) {
            // Generator iteration using iterator protocol:
            //   let result = gen.next(0);
            //   while (!result.done) {
            //       let item = result.value;
            //       body;
            //       result = gen.next(0);
            //   }

            std::string labelSuffix = currentLabel_.empty() ? "" : "#" + currentLabel_;
            currentLabel_.clear();

            auto* initBlock = currentFunction_->createBasicBlock("forof_gen.init" + labelSuffix).get();
            auto* condBlock = currentFunction_->createBasicBlock("forof_gen.cond" + labelSuffix).get();
            auto* bodyBlock = currentFunction_->createBasicBlock("forof_gen.body" + labelSuffix).get();
            auto* updateBlock = currentFunction_->createBasicBlock("forof_gen.update" + labelSuffix).get();
            auto* endBlock = currentFunction_->createBasicBlock("forof_gen.end" + labelSuffix).get();

            builder_->createBr(initBlock);

            // Init block: evaluate generator and get first result
            builder_->setInsertPoint(initBlock);
            node.right->accept(*this);
            auto* genValue = lastValue_;

            // Create result variable to hold IteratorResult
            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
            auto* resultVar = builder_->createAlloca(ptrType.get(), "__iter_result");

            // Call gen.next(0) for first iteration
            // Use async generator functions for async generators
            std::string nextFuncName = isAsyncGeneratorIteration ?
                "nova_async_generator_next" : "nova_generator_next";
            auto existingNextFunc = module_->getFunction(nextFuncName);
            HIRFunction* nextFunc = nullptr;
            if (existingNextFunc) {
                nextFunc = existingNextFunc.get();
            } else {
                std::vector<HIRTypePtr> paramTypes = {ptrType, intType};
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                HIRFunctionPtr funcPtr = module_->createFunction(nextFuncName, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                nextFunc = funcPtr.get();
            }

            if (isAsyncGeneratorIteration) {
                if(NOVA_DEBUG) std::cerr << "DEBUG: ForOf - using async generator next()" << std::endl;
            }

            auto* zeroConst = builder_->createIntConstant(0);
            std::vector<HIRValue*> nextArgs = {genValue, zeroConst};
            auto* firstResult = builder_->createCall(nextFunc, nextArgs, "iter_result");
            firstResult->type = ptrType;
            builder_->createStore(firstResult, resultVar);

            builder_->createBr(condBlock);

            // Condition block: check !result.done
            builder_->setInsertPoint(condBlock);

            auto* currentResult = builder_->createLoad(resultVar);

            // Get result.done
            std::string doneFuncName = "nova_iterator_result_done";
            auto existingDoneFunc = module_->getFunction(doneFuncName);
            HIRFunction* doneFunc = nullptr;
            if (existingDoneFunc) {
                doneFunc = existingDoneFunc.get();
            } else {
                std::vector<HIRTypePtr> paramTypes = {ptrType};
                auto boolType = std::make_shared<HIRType>(HIRType::Kind::Bool);
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, boolType);
                HIRFunctionPtr funcPtr = module_->createFunction(doneFuncName, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                doneFunc = funcPtr.get();
            }

            std::vector<HIRValue*> doneArgs = {currentResult};
            auto* isDone = builder_->createCall(doneFunc, doneArgs, "is_done");

            // If done, exit loop; otherwise continue to body
            // done == 0 means not done, done != 0 means done
            auto* zeroForCmp = builder_->createIntConstant(0);
            auto* notDone = builder_->createEq(isDone, zeroForCmp);
            builder_->createCondBr(notDone, bodyBlock, endBlock);

            // Body block: let item = result.value; body;
            builder_->setInsertPoint(bodyBlock);

            auto* resultForValue = builder_->createLoad(resultVar);

            // Get result.value
            std::string valueFuncName = "nova_iterator_result_value";
            auto existingValueFunc = module_->getFunction(valueFuncName);
            HIRFunction* valueFunc = nullptr;
            if (existingValueFunc) {
                valueFunc = existingValueFunc.get();
            } else {
                std::vector<HIRTypePtr> paramTypes = {ptrType};
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, intType);
                HIRFunctionPtr funcPtr = module_->createFunction(valueFuncName, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                valueFunc = funcPtr.get();
            }

            std::vector<HIRValue*> valueArgs = {resultForValue};
            auto* itemValue = builder_->createCall(valueFunc, valueArgs, "iter_value");
            itemValue->type = intType;

            // Create loop variable and assign
            auto* varType = new hir::HIRType(hir::HIRType::Kind::I64);
            auto* loopVar = builder_->createAlloca(varType, node.left);
            builder_->createStore(itemValue, loopVar);
            symbolTable_[node.left] = loopVar;

            // Execute loop body
            node.body->accept(*this);

            // Check if body ends with terminator
            bool needsBranch = true;
            if (!bodyBlock->instructions.empty()) {
                auto lastOpcode = bodyBlock->instructions.back()->opcode;
                if (lastOpcode == hir::HIRInstruction::Opcode::Break ||
                    lastOpcode == hir::HIRInstruction::Opcode::Continue ||
                    lastOpcode == hir::HIRInstruction::Opcode::Return) {
                    needsBranch = false;
                }
            }

            if (needsBranch) {
                builder_->createBr(updateBlock);
            }

            // Update block: result = gen.next(0)
            builder_->setInsertPoint(updateBlock);

            // Re-evaluate the generator expression to get its current value
            node.right->accept(*this);
            auto* genValueAgain = lastValue_;

            auto* zeroForNext = builder_->createIntConstant(0);
            std::vector<HIRValue*> nextArgs2 = {genValueAgain, zeroForNext};
            auto* nextResult = builder_->createCall(nextFunc, nextArgs2, "next_result");
            nextResult->type = ptrType;
            builder_->createStore(nextResult, resultVar);

            builder_->createBr(condBlock);

            // End block
            builder_->setInsertPoint(endBlock);

            if(NOVA_DEBUG) std::cerr << "DEBUG: ForOf generator loop generation completed" << std::endl;
            return;
        }

        // For-of loop: for (let item of array) { body }
        // Desugar to:
        //   let __iter_idx = 0;
        //   while (__iter_idx < array.length) {
        //       let item = array[__iter_idx];
        //       body;
        //       __iter_idx = __iter_idx + 1;
        //   }

        // Create basic blocks (with label suffix if inside labeled statement)
        std::string labelSuffix = currentLabel_.empty() ? "" : "#" + currentLabel_;
        // Clear label after using it - label only applies to this loop, not nested loops
        currentLabel_.clear();

        auto* initBlock = currentFunction_->createBasicBlock("forof.init" + labelSuffix).get();
        auto* condBlock = currentFunction_->createBasicBlock("forof.cond" + labelSuffix).get();
        auto* bodyBlock = currentFunction_->createBasicBlock("forof.body" + labelSuffix).get();
        auto* updateBlock = currentFunction_->createBasicBlock("forof.update" + labelSuffix).get();
        auto* endBlock = currentFunction_->createBasicBlock("forof.end" + labelSuffix).get();

        // Branch to init block
        builder_->createBr(initBlock);

        // Init block: evaluate the iterable expression and create iterator index
        builder_->setInsertPoint(initBlock);
        if(NOVA_DEBUG) std::cerr << "DEBUG: ForOf - evaluating iterable" << std::endl;
        node.right->accept(*this);  // Evaluate array expression
        auto* arrayValue = lastValue_;

        // Create iterator index variable: let __iter_idx = 0
        auto* indexType = new hir::HIRType(hir::HIRType::Kind::I64);
        auto* indexVar = builder_->createAlloca(indexType, "__iter_idx");
        auto* zeroConst = builder_->createIntConstant(0);
        builder_->createStore(zeroConst, indexVar);

        // Branch to condition
        builder_->createBr(condBlock);

        // Condition block: __iter_idx < array.length
        builder_->setInsertPoint(condBlock);
        if(NOVA_DEBUG) std::cerr << "DEBUG: ForOf - checking condition" << std::endl;

        // Load current index
        auto* currentIndex = builder_->createLoad(indexVar);

        // Check if iterating over a runtime array
        bool isRuntimeArrayForLength = false;
        if (auto* identExpr = dynamic_cast<Identifier*>(node.right.get())) {
            if (runtimeArrayVars_.count(identExpr->name) > 0) {
                isRuntimeArrayForLength = true;
            }
        }

        // Get array.length
        HIRValue* arrayLength = nullptr;
        if (isRuntimeArrayForLength) {
            if(NOVA_DEBUG) std::cerr << "DEBUG: ForOf - using runtime array length function" << std::endl;
            // Use nova_value_array_length for runtime arrays
            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

            std::string runtimeFunc = "nova_value_array_length";
            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                std::vector<HIRTypePtr> paramTypes = {ptrType};
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, intType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            std::vector<HIRValue*> args = {arrayValue};
            arrayLength = builder_->createCall(func, args, "array_len");
            arrayLength->type = intType;
        } else {
            // Use GetField for regular arrays
            arrayLength = builder_->createGetField(arrayValue, 1);  // field 1 is length
        }

        // Compare: __iter_idx < array.length
        auto* condition = builder_->createLt(currentIndex, arrayLength);
        builder_->createCondBr(condition, bodyBlock, endBlock);

        // Body block: let item = array[__iter_idx]; body;
        builder_->setInsertPoint(bodyBlock);
        if(NOVA_DEBUG) std::cerr << "DEBUG: ForOf - executing body" << std::endl;

        // Load current index for array access
        auto* indexForAccess = builder_->createLoad(indexVar);

        // Get current element: array[__iter_idx]
        HIRValue* currentElement = nullptr;

        // Check if iterating over a runtime array (from keys(), values(), entries())
        bool isRuntimeArray = false;
        if (auto* identExpr = dynamic_cast<Identifier*>(node.right.get())) {
            if (runtimeArrayVars_.count(identExpr->name) > 0) {
                isRuntimeArray = true;
                if(NOVA_DEBUG) std::cerr << "DEBUG: ForOf - using runtime array element access for " << identExpr->name << std::endl;
            }
        }

        if (isRuntimeArray) {
            // Use nova_value_array_at for runtime arrays
            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

            std::string runtimeFunc = "nova_value_array_at";
            auto existingFunc = module_->getFunction(runtimeFunc);
            HIRFunction* func = nullptr;
            if (existingFunc) {
                func = existingFunc.get();
            } else {
                std::vector<HIRTypePtr> paramTypes = {ptrType, intType};
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, intType);
                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                func = funcPtr.get();
            }

            std::vector<HIRValue*> args = {arrayValue, indexForAccess};
            currentElement = builder_->createCall(func, args, "iter_elem");
            currentElement->type = intType;
        } else {
            // Use GetElement for regular arrays
            currentElement = builder_->createGetElement(arrayValue, indexForAccess, "iter_elem");
        }

        // Declare loop variable and assign current element
        // node.left is the variable name (e.g., "value" in "for (let value of arr)")
        auto* varType = new hir::HIRType(hir::HIRType::Kind::I64);
        auto* loopVar = builder_->createAlloca(varType, node.left);

        // Store the current element in the loop variable
        builder_->createStore(currentElement, loopVar);

        // Track the variable
        symbolTable_[node.left] = loopVar;

        // Execute loop body
        node.body->accept(*this);

        // Check if body ends with terminator
        bool needsBranch = true;
        if (!bodyBlock->instructions.empty()) {
            auto lastOpcode = bodyBlock->instructions.back()->opcode;
            if (lastOpcode == hir::HIRInstruction::Opcode::Break ||
                lastOpcode == hir::HIRInstruction::Opcode::Continue ||
                lastOpcode == hir::HIRInstruction::Opcode::Return) {
                needsBranch = false;
            }
        }

        if (needsBranch) {
            builder_->createBr(updateBlock);
        }

        // Update block: __iter_idx = __iter_idx + 1
        builder_->setInsertPoint(updateBlock);
        if(NOVA_DEBUG) std::cerr << "DEBUG: ForOf - incrementing index" << std::endl;

        auto* currentIndexForIncr = builder_->createLoad(indexVar);
        auto* oneConst = builder_->createIntConstant(1);
        auto* nextIndex = builder_->createAdd(currentIndexForIncr, oneConst);
        builder_->createStore(nextIndex, indexVar);

        // Branch back to condition
        builder_->createBr(condBlock);

        // End block
        builder_->setInsertPoint(endBlock);

        if(NOVA_DEBUG) std::cerr << "DEBUG: ForOf loop generation completed" << std::endl;
    }
    
void HIRGenerator::visit(ReturnStmt& node) {
        // For generator functions, return should call nova_generator_complete
        if (currentGeneratorPtr_) {
            HIRValue* returnValue = nullptr;
            if (node.argument) {
                node.argument->accept(*this);
                returnValue = lastValue_;
            } else {
                returnValue = builder_->createIntConstant(0);
            }

            // Get types
            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
            auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);

            // Get or create nova_generator_complete function
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

            // Load genPtr and call complete
            auto* genPtr = builder_->createLoad(currentGeneratorPtr_);
            std::vector<HIRValue*> args = {genPtr, returnValue};
            builder_->createCall(completeFunc, args);

            // Return from function
            builder_->createReturn(nullptr);
        } else {
            // Normal return
            if (node.argument) {
                node.argument->accept(*this);
                builder_->createReturn(lastValue_);
            } else {
                builder_->createReturn(nullptr);
            }
        }
    }
    
void HIRGenerator::visit(BreakStmt& node) {
        // Create break instruction with optional label
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing break statement";
        if (!node.label.empty()) {
            std::cerr << " with label: " << node.label;
        }
        std::cerr << std::endl;

        // Get the break target from the stack
        if (breakTargetStack_.empty()) {
            std::cerr << "ERROR: break statement outside of loop/switch" << std::endl;
            return;
        }

        auto* breakTarget = breakTargetStack_.back();
        builder_->createBr(breakTarget);

        auto* currentBlock = builder_->getInsertBlock();
        currentBlock->hasBreakOrContinue = true;
    }

void HIRGenerator::visit(ContinueStmt& node) {
        // Create continue instruction with optional label
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing continue statement";
        if (!node.label.empty()) {
            std::cerr << " with label: " << node.label;
        }
        std::cerr << std::endl;

        // Get the continue target from the stack
        if (continueTargetStack_.empty()) {
            std::cerr << "ERROR: continue statement outside of loop" << std::endl;
            return;
        }

        auto* continueTarget = continueTargetStack_.back();
        builder_->createBr(continueTarget);

        auto* currentBlock = builder_->getInsertBlock();
        currentBlock->hasBreakOrContinue = true;
    }
    
void HIRGenerator::visit(ThrowStmt& node) {
        // throw statement - call nova_throw runtime function
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing throw statement" << std::endl;

        // Evaluate the exception value
        node.argument->accept(*this);
        auto* exceptionValue = lastValue_;

        // Setup function signature for nova_throw(int64_t)
        std::string runtimeFuncName = "nova_throw";
        std::vector<HIRTypePtr> paramTypes;
        paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::I64));
        auto returnType = std::make_shared<HIRType>(HIRType::Kind::Void);

        // Find or create runtime function
        HIRFunction* runtimeFunc = nullptr;
        auto& functions = module_->functions;
        for (auto& func : functions) {
            if (func->name == runtimeFuncName) {
                runtimeFunc = func.get();
                break;
            }
        }

        if (!runtimeFunc) {
            HIRFunctionType* funcType = new HIRFunctionType(paramTypes, returnType);
            HIRFunctionPtr funcPtr = module_->createFunction(runtimeFuncName, funcType);
            funcPtr->linkage = HIRFunction::Linkage::External;
            runtimeFunc = funcPtr.get();
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Created external function: " << runtimeFuncName << std::endl;
        }

        // Create call to nova_throw
        std::vector<HIRValue*> args = {exceptionValue};
        builder_->createCall(runtimeFunc, args, "");
        // If we are inside a try block, jump to the catch block
        if (currentCatchBlock_) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Throw jumping to catch block" << std::endl;
            builder_->createBr(currentCatchBlock_);
        }
        // If no catch block, nova_throw will handle uncaught exception and exit
    }
    
void HIRGenerator::visit(TryStmt& node) {
        // try-catch-finally implementation
        // For now, implements basic control flow without actual exception handling
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing try-catch-finally statement" << std::endl;

        // Create blocks for try, catch, finally, and end
        auto tryBlock = currentFunction_->createBasicBlock("try");
        auto catchBlock = node.handler ? currentFunction_->createBasicBlock("catch") : nullptr;
        auto finallyBlock = node.finalizer ? currentFunction_->createBasicBlock("finally") : nullptr;
        auto endBlock = currentFunction_->createBasicBlock("try.end");
        // Save previous catch block and set new one
        auto* prevCatchBlock = currentCatchBlock_;
        currentCatchBlock_ = catchBlock ? catchBlock.get() : nullptr;
        // Call nova_try_begin() to increment try depth
        {
            std::string funcName = "nova_try_begin";
            HIRFunction* runtimeFunc = nullptr;
            auto& functions = module_->functions;
            for (auto& func : functions) {
                if (func->name == funcName) {
                    runtimeFunc = func.get();
                    break;
                }
            }
            if (!runtimeFunc) {
                std::vector<HIRTypePtr> paramTypes;
                auto returnType = std::make_shared<HIRType>(HIRType::Kind::Void);
                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, returnType);
                HIRFunctionPtr funcPtr = module_->createFunction(funcName, funcType);
                funcPtr->linkage = HIRFunction::Linkage::External;
                runtimeFunc = funcPtr.get();
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Created external function: " << funcName << std::endl;
            }
            std::vector<HIRValue*> args;
            builder_->createCall(runtimeFunc, args, "");
        }

        // Jump to try block
        builder_->createBr(tryBlock.get());

        // Generate try block
        builder_->setInsertPoint(tryBlock.get());
        if (node.block) {
            node.block->accept(*this);
        }

        // After try block, jump to finally or end
        if (!builder_->getInsertBlock()->hasBreakOrContinue) {
            if (finallyBlock) {
                builder_->createBr(finallyBlock.get());
            } else {
                builder_->createBr(endBlock.get());
            }
        }

        // Generate catch block
        if (catchBlock) {
            builder_->setInsertPoint(catchBlock.get());

            // Get exception value via nova_get_exception()
            HIRValue* exceptionValue = nullptr;
            {
                std::string funcName = "nova_get_exception";
                HIRFunction* runtimeFunc = nullptr;
                auto& functions = module_->functions;
                for (auto& func : functions) {
                    if (func->name == funcName) {
                        runtimeFunc = func.get();
                        break;
                    }
                }
                if (!runtimeFunc) {
                    std::vector<HIRTypePtr> paramTypes;
                    auto returnType = std::make_shared<HIRType>(HIRType::Kind::I64);
                    HIRFunctionType* funcType = new HIRFunctionType(paramTypes, returnType);
                    HIRFunctionPtr funcPtr = module_->createFunction(funcName, funcType);
                    funcPtr->linkage = HIRFunction::Linkage::External;
                    runtimeFunc = funcPtr.get();
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Created external function: " << funcName << std::endl;
                }
                std::vector<HIRValue*> args;
                exceptionValue = builder_->createCall(runtimeFunc, args, "exception_value");
            }

            // Add catch parameter to symbol table
            if (node.handler && !node.handler->param.empty()) {
                symbolTable_[node.handler->param] = exceptionValue;
            }

            if (node.handler && node.handler->body) {
                node.handler->body->accept(*this);
            }

            // After catch, jump to finally or end
            if (!builder_->getInsertBlock()->hasBreakOrContinue) {
                if (finallyBlock) {
                    builder_->createBr(finallyBlock.get());
                } else {
                    builder_->createBr(endBlock.get());
                }
            }
        }

        // Generate finally block
        if (finallyBlock) {
            builder_->setInsertPoint(finallyBlock.get());
            if (node.finalizer) {
                node.finalizer->accept(*this);
            }
            // After finally, jump to end
            if (!builder_->getInsertBlock()->hasBreakOrContinue) {
                builder_->createBr(endBlock.get());
            }
        }

        // Continue at end block
        builder_->setInsertPoint(endBlock.get());
        // Restore previous catch block
        currentCatchBlock_ = prevCatchBlock;
    }
    
void HIRGenerator::visit(SwitchStmt& node) {
        // For now, implement switch as a series of if-else statements
        // Evaluate discriminant once
        node.discriminant->accept(*this);
        auto discriminantValue = lastValue_;

        // Create end block
        auto endBlock = currentFunction_->createBasicBlock("switch.end");

        // Push end block onto break target stack so break statements know where to jump
        breakTargetStack_.push_back(endBlock.get());

        // Find default case if it exists
        size_t defaultCaseIndex = node.cases.size();
        for (size_t i = 0; i < node.cases.size(); ++i) {
            if (!node.cases[i]->test) {
                defaultCaseIndex = i;
                break;
            }
        }

        // Generate if-else chain for each case
        for (size_t i = 0; i < node.cases.size(); ++i) {
            if (node.cases[i]->test) {  // Regular case (not default)
                // Evaluate test value
                node.cases[i]->test->accept(*this);
                auto testValue = lastValue_;

                // Compare
                auto cmp = builder_->createEq(discriminantValue, testValue);

                // Create then and else blocks
                auto thenBlock = currentFunction_->createBasicBlock("case.then");
                auto elseBlock = currentFunction_->createBasicBlock("case.else");

                builder_->createCondBr(cmp, thenBlock.get(), elseBlock.get());

                // Generate then block (case body)
                builder_->setInsertPoint(thenBlock.get());
                for (auto& stmt : node.cases[i]->consequent) {
                    stmt->accept(*this);
                }

                // Jump to end if no break
                if (!builder_->getInsertBlock()->hasBreakOrContinue) {
                    builder_->createBr(endBlock.get());
                }

                // Continue with else block
                builder_->setInsertPoint(elseBlock.get());
            }
        }

        // Generate default case if it exists
        if (defaultCaseIndex < node.cases.size()) {
            for (auto& stmt : node.cases[defaultCaseIndex]->consequent) {
                stmt->accept(*this);
            }

            if (!builder_->getInsertBlock()->hasBreakOrContinue) {
                builder_->createBr(endBlock.get());
            }
        } else {
            // No default case, just jump to end
            builder_->createBr(endBlock.get());
        }

        // Pop break target from stack
        breakTargetStack_.pop_back();

        // Continue with end block
        builder_->setInsertPoint(endBlock.get());
    }

} // namespace nova::hir
