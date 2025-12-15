// HIRGen_Arrays.cpp - Array expression visitors
// Extracted from HIRGen.cpp for better code organization

#include "nova/HIR/HIRGen_Internal.h"

namespace nova::hir {

// Static counter to detect infinite recursion
static int arrayVisitorDepth = 0;
static const int MAX_ARRAY_VISITOR_DEPTH = 10;

void HIRGenerator::visit(ArrayExpr& node) {
        // Recursion guard to prevent infinite loops
        arrayVisitorDepth++;
        if (arrayVisitorDepth > MAX_ARRAY_VISITOR_DEPTH) {
            arrayVisitorDepth--;
            lastValue_ = builder_->createIntConstant(0);
            return;
        }

        // Array literal construction
        // Check if any elements are spread expressions
        bool hasSpread = false;
        for (const auto& elem : node.elements) {
            if (dynamic_cast<SpreadExpr*>(elem.get())) {
                hasSpread = true;
                break;
            }
        }

        // If no spread operators, use simple array construction
        if (!hasSpread) {
            std::vector<HIRValue*> elementValues;
            for (const auto& elem : node.elements) {
                elem->accept(*this);
                if (lastValue_) {
                    elementValues.push_back(lastValue_);
                }
            }
            lastValue_ = builder_->createArrayConstruct(elementValues, "arr");
            arrayVisitorDepth--;
            return;
        }

        // Special case: single spread element (e.g., [...arr])
        // This is the most common case and can use simple array copy
        if (node.elements.size() == 1) {
            auto* spreadExpr = dynamic_cast<SpreadExpr*>(node.elements[0].get());
            if (spreadExpr) {
                // Evaluate the source array
                spreadExpr->argument->accept(*this);

                HIRValue* sourceArray = lastValue_;

                // Get or create nova_array_copy function
                auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

                // Create proper Pointer-to-Array type for the result
                auto arrayElementType = std::make_shared<HIRType>(HIRType::Kind::I64);
                auto arrayType = std::make_shared<HIRArrayType>(arrayElementType, 0); // Dynamic size
                auto ptrToArrayType = std::make_shared<HIRPointerType>(arrayType, true);

                std::vector<HIRTypePtr> copyParamTypes = {ptrType};
                HIRFunction* copyFunc = nullptr;
                auto existingCopyFunc = module_->getFunction("nova_array_copy");
                if (existingCopyFunc) {
                    copyFunc = existingCopyFunc.get();
                } else {
                    HIRFunctionType* funcType = new HIRFunctionType(copyParamTypes, ptrToArrayType);
                    HIRFunctionPtr funcPtr = module_->createFunction("nova_array_copy", funcType);
                    funcPtr->linkage = HIRFunction::Linkage::External;
                    copyFunc = funcPtr.get();
                }

                // Call nova_array_copy
                std::vector<HIRValue*> copyArgs = {sourceArray};
                lastValue_ = builder_->createCall(copyFunc, copyArgs, "spread_copy");
                // Ensure the result has the correct Pointer-to-Array type
                if (lastValue_) {
                    lastValue_->type = ptrToArrayType;
                }

                arrayVisitorDepth--;
                return;
            }
        }

        // Array with spread operators - need dynamic construction
        // Step 1: Calculate total length needed
        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
        auto i64Type = std::make_shared<HIRType>(HIRType::Kind::I64);

        // Get or create value_array_length function
        std::vector<HIRTypePtr> lengthParamTypes = {ptrType};
        HIRFunction* lengthFunc = nullptr;
        auto existingLengthFunc = module_->getFunction("value_array_length");
        if (existingLengthFunc) {
            lengthFunc = existingLengthFunc.get();
        } else {
            HIRFunctionType* funcType = new HIRFunctionType(lengthParamTypes, i64Type);
            HIRFunctionPtr funcPtr = module_->createFunction("value_array_length", funcType);
            funcPtr->linkage = HIRFunction::Linkage::External;
            lengthFunc = funcPtr.get();
        }

        // Calculate total length by summing up lengths of spread arrays and counting regular elements
        HIRValue* totalLength = builder_->createIntConstant(0);

        for (const auto& elem : node.elements) {
            if (auto* spreadExpr = dynamic_cast<SpreadExpr*>(elem.get())) {
                // Spread element - get its length
                spreadExpr->argument->accept(*this);
                HIRValue* spreadArray = lastValue_;

                // Call value_array_length to get length
                std::vector<HIRValue*> lengthArgs = {spreadArray};
                HIRValue* arrayLength = builder_->createCall(lengthFunc, lengthArgs, "spread_len");

                // Add to total
                totalLength = builder_->createAdd(totalLength, arrayLength);
            } else {
                // Regular element - count as 1
                HIRValue* one = builder_->createIntConstant(1);
                totalLength = builder_->createAdd(totalLength, one);
            }
        }

        // Step 2: Create new array with calculated total length
        // Create proper Pointer-to-Array type for the result
        auto arrayElementType = std::make_shared<HIRType>(HIRType::Kind::I64);
        auto arrayType = std::make_shared<HIRArrayType>(arrayElementType, 0); // Dynamic size
        auto ptrToArrayType = std::make_shared<HIRPointerType>(arrayType, true);

        // Get or create create_value_array function
        std::vector<HIRTypePtr> createArrayParamTypes = {i64Type};
        HIRFunction* createArrayFunc = nullptr;
        auto existingCreateFunc = module_->getFunction("create_value_array");
        if (existingCreateFunc) {
            createArrayFunc = existingCreateFunc.get();
        } else {
            HIRFunctionType* funcType = new HIRFunctionType(createArrayParamTypes, ptrToArrayType);
            HIRFunctionPtr funcPtr = module_->createFunction("create_value_array", funcType);
            funcPtr->linkage = HIRFunction::Linkage::External;
            createArrayFunc = funcPtr.get();
        }

        std::vector<HIRValue*> createArgs = {totalLength};
        HIRValue* resultArray = builder_->createCall(createArrayFunc, createArgs, "spread_arr");
        // Ensure the result has the correct Pointer-to-Array type
        if (resultArray) {
            resultArray->type = ptrToArrayType;
        }

        // CRITICAL FIX: Set the array length field directly
        // create_value_array creates array with length=0, but we need totalLength
        // Get or create a helper function to set array length
        std::vector<HIRTypePtr> setLengthParams = {ptrType, i64Type};
        HIRFunction* setLengthFunc = nullptr;
        auto existingSetLengthFunc = module_->getFunction("nova_array_set_length");
        if (existingSetLengthFunc) {
            setLengthFunc = existingSetLengthFunc.get();
        } else {
            auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
            HIRFunctionType* funcType = new HIRFunctionType(setLengthParams, voidType);
            HIRFunctionPtr funcPtr = module_->createFunction("nova_array_set_length", funcType);
            funcPtr->linkage = HIRFunction::Linkage::External;
            setLengthFunc = funcPtr.get();
        }
        // Call nova_array_set_length to set the length before copying elements
        std::vector<HIRValue*> setLengthArgs = {resultArray, totalLength};
        builder_->createCall(setLengthFunc, setLengthArgs);

        // Step 3: Get or create value_array_get and value_array_set functions
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

        std::vector<HIRTypePtr> setParamTypes = {ptrType, i64Type, i64Type};
        auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
        HIRFunction* setFunc = nullptr;
        auto existingSetFunc = module_->getFunction("value_array_set");
        if (existingSetFunc) {
            setFunc = existingSetFunc.get();
        } else {
            HIRFunctionType* funcType = new HIRFunctionType(setParamTypes, voidType);
            HIRFunctionPtr funcPtr = module_->createFunction("value_array_set", funcType);
            funcPtr->linkage = HIRFunction::Linkage::External;
            setFunc = funcPtr.get();
        }

        // Step 4: Copy elements from each source (spread or regular) into result array
        // Create destIndex as an alloca so it can be updated across basic blocks
        HIRValue* destIndexAlloca = builder_->createAlloca(i64Type.get(), "dest_index");
        HIRValue* initialDestIndex = builder_->createIntConstant(0);
        builder_->createStore(initialDestIndex, destIndexAlloca);

        for (const auto& elem : node.elements) {
            if (auto* spreadExpr = dynamic_cast<SpreadExpr*>(elem.get())) {
                // Spread element - copy all elements from source array
                // Evaluate the spread source array
                spreadExpr->argument->accept(*this);
                HIRValue* sourceArray = lastValue_;

                // Get source array length
                std::vector<HIRValue*> lengthArgs = {sourceArray};
                HIRValue* sourceLength = builder_->createCall(lengthFunc, lengthArgs, "src_len");

                // CRITICAL FIX: Create loop variable alloca BEFORE branching
                // Allocas must be created at the current insert point, not inside loop blocks
                HIRValue* loopVar = builder_->createAlloca(i64Type.get(), "i");

                // Create loop to copy elements
                // Loop: for (i = 0; i < sourceLength; i++)
                auto* loopInit = currentFunction_->createBasicBlock("spread_loop_init").get();
                auto* loopCond = currentFunction_->createBasicBlock("spread_loop_cond").get();
                auto* loopBody = currentFunction_->createBasicBlock("spread_loop_body").get();
                auto* loopEnd = currentFunction_->createBasicBlock("spread_loop_end").get();
                // CRITICAL FIX: Create continuation block for code after array construction
                auto* continuationBlock = currentFunction_->createBasicBlock("spread_continue").get();

                // Branch to loop initialization
                builder_->createBr(loopInit);

                // Loop init: i = 0
                builder_->setInsertPoint(loopInit);
                HIRValue* initIndex = builder_->createIntConstant(0);
                builder_->createStore(initIndex, loopVar);
                builder_->createBr(loopCond);

                // Loop condition: i < sourceLength
                builder_->setInsertPoint(loopCond);
                HIRValue* currentI = builder_->createLoad(loopVar);
                HIRValue* cond = builder_->createLt(currentI, sourceLength);
                builder_->createCondBr(cond, loopBody, loopEnd);

                // Loop body: resultArray[destIndex] = sourceArray[i]; destIndex++; i++
                builder_->setInsertPoint(loopBody);
                HIRValue* currentIBody = builder_->createLoad(loopVar);

                // Get element from source array
                std::vector<HIRValue*> getArgs = {sourceArray, currentIBody};
                HIRValue* element = builder_->createCall(getFunc, getArgs, "elem");

                // Load current destIndex and set element in destination array
                HIRValue* currentDestIndex = builder_->createLoad(destIndexAlloca);
                std::vector<HIRValue*> setArgs = {resultArray, currentDestIndex, element};
                builder_->createCall(setFunc, setArgs);

                // Increment destIndex and store back
                HIRValue* one = builder_->createIntConstant(1);
                HIRValue* nextDestIndex = builder_->createAdd(currentDestIndex, one);
                builder_->createStore(nextDestIndex, destIndexAlloca);

                // Increment i
                HIRValue* nextI = builder_->createAdd(currentIBody, one);
                builder_->createStore(nextI, loopVar);
                builder_->createBr(loopCond);

                // CRITICAL FIX: loopEnd must branch to continuation block
                builder_->setInsertPoint(loopEnd);
                builder_->createBr(continuationBlock);

                // Continue building code in continuation block
                builder_->setInsertPoint(continuationBlock);

            } else {
                // Regular element - just add it at current destIndex
                elem->accept(*this);
                HIRValue* element = lastValue_;

                // Load current destIndex and set element in destination array
                HIRValue* currentDestIndex = builder_->createLoad(destIndexAlloca);
                std::vector<HIRValue*> setArgs = {resultArray, currentDestIndex, element};
                builder_->createCall(setFunc, setArgs);

                // Increment destIndex and store back
                HIRValue* one = builder_->createIntConstant(1);
                HIRValue* nextDestIndex = builder_->createAdd(currentDestIndex, one);
                builder_->createStore(nextDestIndex, destIndexAlloca);
            }
        }

        lastValue_ = resultArray;

        // Decrement recursion counter on successful exit
        arrayVisitorDepth--;
    }


} // namespace nova::hir
