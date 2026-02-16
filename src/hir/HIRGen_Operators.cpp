// HIRGen_Operators.cpp - Operator expression visitors
// Extracted from HIRGen.cpp for better code organization

#include "nova/HIR/HIRGen_Internal.h"
#define NOVA_DEBUG 0

namespace nova::hir {

void HIRGenerator::visit(BinaryExpr& node) {
        using Op = BinaryExpr::Op;

        // Handle logical operators with proper short-circuit evaluation
        // JavaScript semantics:
        //   && returns left if falsy, otherwise right
        //   || returns left if truthy, otherwise right
        if (node.op == Op::LogicalAnd || node.op == Op::LogicalOr) {
            // Evaluate left operand
            node.left->accept(*this);
            auto lhs = lastValue_;

            // Create basic blocks for short-circuit
            auto* evalRightBlock = currentFunction_->createBasicBlock("sc.right").get();
            auto* mergeBlock = currentFunction_->createBasicBlock("sc.merge").get();

            // Check if left operand is truthy/falsy
            auto zero = builder_->createIntConstant(0);
            auto lhsBool = builder_->createNe(lhs, zero);

            if (node.op == Op::LogicalAnd) {
                // AND: if left is falsy, short-circuit (return left/false)
                // if left is truthy, evaluate right
                builder_->createCondBr(lhsBool, evalRightBlock, mergeBlock);
            } else {
                // OR: if left is truthy, short-circuit (return left/true)
                // if left is falsy, evaluate right
                builder_->createCondBr(lhsBool, mergeBlock, evalRightBlock);
            }

            // Evaluate right operand (only reached if not short-circuited)
            builder_->setInsertPoint(evalRightBlock);
            node.right->accept(*this);
            auto rhs = lastValue_;

            // Create a boolean from rhs for the merge
            auto rhsBool = builder_->createNe(rhs, zero);
            builder_->createBr(mergeBlock);

            // Merge block: result is lhsBool if short-circuited, rhsBool if not
            builder_->setInsertPoint(mergeBlock);

            // Since HIR doesn't have PHI nodes, use a simpler approach:
            // For &&: result = lhsBool AND rhsBool (lhsBool * rhsBool works since if we reach merge
            //         from left, lhsBool is 0, so result is 0; from right, it's rhsBool)
            // For ||: result = lhsBool OR rhsBool
            if (node.op == Op::LogicalAnd) {
                lastValue_ = builder_->createMul(lhsBool, rhsBool);
            } else {
                // OR: a + b - a*b
                auto product = builder_->createMul(lhsBool, rhsBool);
                auto sum = builder_->createAdd(lhsBool, rhsBool);
                lastValue_ = builder_->createSub(sum, product);
            }

            if (lastValue_) {
                lastValue_->type = std::make_shared<HIRType>(HIRType::Kind::Bool);
            }

            return;
        }

        // Handle nullish coalescing operator (??)
        // Returns left operand if it's not null/undefined, otherwise returns right operand
        // Since Nova doesn't have null/undefined types yet, we always return the left operand
        // TODO: Implement proper null/undefined checking when those types are added
        if (node.op == Op::NullishCoalescing) {
            // For now, just evaluate and return the left operand
            // This is correct behavior when left is never null/undefined
            node.left->accept(*this);
            // Right operand is not evaluated (short-circuit)
            return;
        }

        // For non-logical operators, evaluate both operands normally
        // Generate left operand
        node.left->accept(*this);
        auto lhs = lastValue_;

        // Generate right operand
        node.right->accept(*this);
        auto rhs = lastValue_;

        // Convert boolean operands to integers for arithmetic operations
        auto convertBoolToInt = [this](HIRValue* value) -> HIRValue* {
            if (value && value->type && value->type->kind == HIRType::Kind::Bool) {
                // Convert boolean to integer: true -> 1, false -> 0
                // Use ZExt (zero extension) to convert i1 to i64
                auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                return builder_->createCast(value, intType.get());
            }
            return value;
        };

        // Generate operation based on operator
        switch (node.op) {
            case Op::Add: {
                // Check if this is string concatenation (at least one operand is a string)
                bool lhsIsString = (lhs && lhs->type && lhs->type->kind == HIRType::Kind::String);
                bool rhsIsString = (rhs && rhs->type && rhs->type->kind == HIRType::Kind::String);

                // For string concatenation, preserve boolean types (don't convert to int)
                // This allows LLVM codegen to properly convert boolean to "true"/"false" strings
                if (!lhsIsString && !rhsIsString) {
                    // Pure numeric addition: convert booleans to integers
                    lhs = convertBoolToInt(lhs);
                    rhs = convertBoolToInt(rhs);
                }
                // else: String concatenation - keep booleans as-is for proper string conversion

                lastValue_ = builder_->createAdd(lhs, rhs);
                break;
            }
            case Op::Sub:
                lhs = convertBoolToInt(lhs);
                rhs = convertBoolToInt(rhs);
                lastValue_ = builder_->createSub(lhs, rhs);
                break;
            case Op::Mul:
                lastValue_ = builder_->createMul(lhs, rhs);
                break;
            case Op::Div:
                lastValue_ = builder_->createDiv(lhs, rhs);
                break;
            case Op::Mod:
                lastValue_ = builder_->createRem(lhs, rhs);
                break;
            case Op::Pow:
                lastValue_ = builder_->createPow(lhs, rhs);
                break;
            case Op::BitAnd:
                lastValue_ = builder_->createAnd(lhs, rhs);
                break;
            case Op::BitOr:
                lastValue_ = builder_->createOr(lhs, rhs);
                break;
            case Op::BitXor:
                lastValue_ = builder_->createXor(lhs, rhs);
                break;
            case Op::LeftShift:
                lastValue_ = builder_->createShl(lhs, rhs);
                break;
            case Op::RightShift:
                lastValue_ = builder_->createShr(lhs, rhs);
                break;
            case Op::UnsignedRightShift:
                lastValue_ = builder_->createUShr(lhs, rhs);
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
    

void HIRGenerator::visit(UnaryExpr& node) {
        node.operand->accept(*this);
        auto operand = lastValue_;

        using Op = UnaryExpr::Op;
        switch (node.op) {
            case Op::Plus:
                // Unary plus - convert to number (for numbers, it's a no-op)
                // In JavaScript, +x converts x to a number
                // For our integer types, we just use the value as-is
                lastValue_ = operand;
                break;
            case Op::Minus:
                // Negate
                {
                    auto zero = builder_->createIntConstant(0);
                    lastValue_ = builder_->createSub(zero, operand);
                }
                break;
            case Op::Not:
                // Logical not - compare with zero
                // !x is true if x is 0 (falsy), false otherwise
                {
                    auto zero = builder_->createIntConstant(0);
                    lastValue_ = builder_->createEq(operand, zero);
                }
                break;
            case Op::BitNot:
                // Bitwise NOT
                lastValue_ = builder_->createNot(operand);
                break;
            case Op::Typeof:
                // typeof operator - return string representation of type
                {
                    std::string typeStr = "unknown";

                    if (operand && operand->type) {
                        switch (operand->type->kind) {
                            case HIRType::Kind::I64:
                            case HIRType::Kind::I32:
                            case HIRType::Kind::I8:
                                typeStr = "number";
                                break;
                            case HIRType::Kind::String:
                                typeStr = "string";
                                break;
                            case HIRType::Kind::Bool:
                                typeStr = "boolean";
                                break;
                            case HIRType::Kind::Array:
                            case HIRType::Kind::Struct:
                            case HIRType::Kind::Pointer:
                                typeStr = "object";
                                break;
                            case HIRType::Kind::Function:
                                typeStr = "function";
                                break;
                            case HIRType::Kind::Void:
                                typeStr = "undefined";
                                break;
                            default:
                                typeStr = "unknown";
                                break;
                        }
                    }

                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: typeof operator returns '" << typeStr << "'" << std::endl;
                    lastValue_ = builder_->createStringConstant(typeStr);
                }
                break;
            case Op::Void:
                // void operator - evaluates operand and returns undefined (0 for integers)
                // The operand has already been evaluated above
                // In JavaScript, void always returns undefined
                // For our integer-only compiler, we return 0
                lastValue_ = builder_->createIntConstant(0);
                break;
            default:
                // Other operators
                break;
        }
    }
    

void HIRGenerator::visit(UpdateExpr& node) {
        // Increment/Decrement operators: ++x, x++, --x, x--
        // The argument must be a variable (identifier)
        auto* identifier = dynamic_cast<Identifier*>(node.argument.get());
        if (!identifier) {
            std::cerr << "ERROR: UpdateExpr argument must be an identifier" << std::endl;
            return;
        }

        // Get the variable's current value (with closure support)
        HIRValue* varAlloca = lookupVariable(identifier->name);
        if (!varAlloca) {
            std::cerr << "ERROR: Undefined variable: " << identifier->name << std::endl;
            return;
        }

        // Load current value
        auto currentValue = builder_->createLoad(varAlloca);

        // Create constant 1 for increment/decrement
        auto one = builder_->createIntConstant(1);

        // Calculate new value
        HIRValue* newValue;
        if (node.op == UpdateExpr::Op::Increment) {
            newValue = builder_->createAdd(currentValue, one);
        } else {  // Decrement
            newValue = builder_->createSub(currentValue, one);
        }

        // Store new value back to variable
        builder_->createStore(newValue, varAlloca);

        // Return value depends on prefix vs postfix
        if (node.isPrefix) {
            // Prefix: ++x or --x returns new value
            lastValue_ = newValue;
        } else {
            // Postfix: x++ or x-- returns old value
            lastValue_ = currentValue;
        }
    }
    

void HIRGenerator::visit(ConditionalExpr& node) {
        // Ternary operator: test ? consequent : alternate
        // FIXED: Evaluate branches INSIDE the then/else blocks, not before branching
        // FIXED: Properly handle type inference for strings and other types

        // Step 1: Evaluate condition
        node.test->accept(*this);
        auto cond = lastValue_;

        // Step 2: Determine result type by peeking at consequent
        // Save current insertion point
        auto* savedBlock = builder_->getInsertBlock();

        // Create temporary block for type inference
        auto* typeInferBlock = currentFunction_->createBasicBlock("ternary.typeinfer").get();
        builder_->setInsertPoint(typeInferBlock);

        // Evaluate consequent to determine type
        node.consequent->accept(*this);
        HIRType* resultType = lastValue_ && lastValue_->type ? lastValue_->type.get() : new HIRType(HIRType::Kind::I64);

        // Restore insertion point (discard type inference block)
        builder_->setInsertPoint(savedBlock);

        // Step 3: Create temporary variable to store result with correct type
        auto* resultAlloca = builder_->createAlloca(resultType, "ternary.result");

        // Step 4: Create basic blocks
        auto* thenBlock = currentFunction_->createBasicBlock("ternary.then").get();
        auto* elseBlock = currentFunction_->createBasicBlock("ternary.else").get();
        auto* endBlock = currentFunction_->createBasicBlock("ternary.end").get();

        // Step 5: Branch on condition
        builder_->createCondBr(cond, thenBlock, elseBlock);

        // Step 6: Generate THEN block - evaluate consequent HERE (not before!)
        builder_->setInsertPoint(thenBlock);
        node.consequent->accept(*this);  // Evaluate INSIDE then block
        auto consequentValue = lastValue_;
        builder_->createStore(consequentValue, resultAlloca);
        builder_->createBr(endBlock);

        // Step 7: Generate ELSE block - evaluate alternate HERE (not before!)
        builder_->setInsertPoint(elseBlock);
        node.alternate->accept(*this);  // Evaluate INSIDE else block
        auto alternateValue = lastValue_;
        builder_->createStore(alternateValue, resultAlloca);
        builder_->createBr(endBlock);

        // Step 8: Continue at end block
        builder_->setInsertPoint(endBlock);

        // Step 9: Load result from temporary variable
        lastValue_ = builder_->createLoad(resultAlloca, "ternary.result");
    }
    

void HIRGenerator::visit(AssignmentExpr& node) {
        std::cerr << "=== ASSIGNMENT EXPRESSION VISITOR CALLED ===" << std::endl;
        hir::HIRValue* value = nullptr;

        // Handle logical assignment operators with short-circuit evaluation
        if (node.op == AssignmentExpr::Op::LogicalAndAssign ||
            node.op == AssignmentExpr::Op::LogicalOrAssign ||
            node.op == AssignmentExpr::Op::NullishCoalescingAssign) {

            // Get current value of left side
            node.left->accept(*this);
            auto leftValue = lastValue_;

            // Create temporary variable to store result
            auto i64Type = new HIRType(HIRType::Kind::I64);
            auto* resultAlloca = builder_->createAlloca(i64Type, "logical_assign.result");

            // Create blocks
            auto* evalRightBlock = currentFunction_->createBasicBlock("logical_assign.eval_right").get();
            auto* skipBlock = currentFunction_->createBasicBlock("logical_assign.skip").get();
            auto* endBlock = currentFunction_->createBasicBlock("logical_assign.end").get();

            // Create condition based on operator type
            HIRValue* condition = nullptr;
            auto zero = builder_->createIntConstant(0);
            if (node.op == AssignmentExpr::Op::LogicalAndAssign) {
                // &&= : evaluate right if left is truthy (left != 0)
                condition = builder_->createNe(leftValue, zero);
            } else if (node.op == AssignmentExpr::Op::LogicalOrAssign) {
                // ||= : evaluate right if left is falsy (left == 0)
                condition = builder_->createEq(leftValue, zero);
            } else {
                // ??= : In a proper implementation, this should only assign if left is null/undefined
                // Since we don't have null/undefined tracking in our simple type system,
                // we treat all values as non-nullish, so ??= always keeps the left value
                // This means the condition is always false
                condition = builder_->createIntConstant(0);  // Always false
            }

            // Branch based on condition
            builder_->createCondBr(condition, evalRightBlock, skipBlock);

            // Evaluate right side and store
            builder_->setInsertPoint(evalRightBlock);
            node.right->accept(*this);
            auto rightValue = lastValue_;
            builder_->createStore(rightValue, resultAlloca);
            builder_->createBr(endBlock);

            // Skip evaluation of right side, keep left value
            builder_->setInsertPoint(skipBlock);
            builder_->createStore(leftValue, resultAlloca);
            builder_->createBr(endBlock);

            // Continue at end block
            builder_->setInsertPoint(endBlock);

            // Load result
            value = builder_->createLoad(resultAlloca);
        } else {
            // Handle regular and arithmetic compound assignments
            // Generate right side
            node.right->accept(*this);
            auto rightValue = lastValue_;

            // For compound assignments (+=, -=, etc.), need to read current value first
            hir::HIRValue* finalValue = rightValue;
            if (node.op != AssignmentExpr::Op::Assign) {
                // Get current value of left side
                node.left->accept(*this);
                auto leftValue = lastValue_;

                // Perform the binary operation
                switch (node.op) {
                    case AssignmentExpr::Op::AddAssign:
                        finalValue = builder_->createAdd(leftValue, rightValue);
                        break;
                    case AssignmentExpr::Op::SubAssign:
                        finalValue = builder_->createSub(leftValue, rightValue);
                        break;
                    case AssignmentExpr::Op::MulAssign:
                        finalValue = builder_->createMul(leftValue, rightValue);
                        break;
                    case AssignmentExpr::Op::DivAssign:
                        finalValue = builder_->createDiv(leftValue, rightValue);
                        break;
                    case AssignmentExpr::Op::ModAssign:
                        finalValue = builder_->createRem(leftValue, rightValue);
                        break;
                    case AssignmentExpr::Op::PowAssign:
                        finalValue = builder_->createPow(leftValue, rightValue);
                        break;
                    case AssignmentExpr::Op::BitAndAssign:
                        finalValue = builder_->createAnd(leftValue, rightValue);
                        break;
                    case AssignmentExpr::Op::BitOrAssign:
                        finalValue = builder_->createOr(leftValue, rightValue);
                        break;
                    case AssignmentExpr::Op::BitXorAssign:
                        finalValue = builder_->createXor(leftValue, rightValue);
                        break;
                    case AssignmentExpr::Op::LeftShiftAssign:
                        finalValue = builder_->createShl(leftValue, rightValue);
                        break;
                    case AssignmentExpr::Op::RightShiftAssign:
                        finalValue = builder_->createShr(leftValue, rightValue);
                        break;
                    case AssignmentExpr::Op::UnsignedRightShiftAssign:
                        finalValue = builder_->createUShr(leftValue, rightValue);
                        break;
                    default:
                        std::cerr << "Warning: Unsupported compound assignment operator" << std::endl;
                        break;
                }
            }
            value = finalValue;
        }

        // Store to left side
        if (auto* id = dynamic_cast<Identifier*>(node.left.get())) {
            // Simple variable assignment - check current scope and parent scopes (for closures)
            HIRValue* target = lookupVariable(id->name);
            if (target) {
                builder_->createStore(value, target);
            }
        } else if (auto* memberExpr = dynamic_cast<MemberExpr*>(node.left.get())) {
            // Get the object/array
            memberExpr->object->accept(*this);
            auto object = lastValue_;

            if (memberExpr->isComputed) {
                // Array element assignment: arr[index] = value
                memberExpr->property->accept(*this);
                auto index = lastValue_;

                // Check if this is TypedArray element assignment
                if (auto* objIdent = dynamic_cast<Identifier*>(memberExpr->object.get())) {
                    auto typeIt = typedArrayTypes_.find(objIdent->name);
                    if (typeIt != typedArrayTypes_.end()) {
                        std::string typedArrayType = typeIt->second;
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: TypedArray element assignment on " << objIdent->name
                                  << " (type: " << typedArrayType << ")" << std::endl;

                        // Determine runtime function name
                        std::string runtimeFunc;
                        if (typedArrayType == "Int8Array") runtimeFunc = "nova_int8array_set";
                        else if (typedArrayType == "Uint8Array") runtimeFunc = "nova_uint8array_set";
                        else if (typedArrayType == "Uint8ClampedArray") runtimeFunc = "nova_uint8clampedarray_set";
                        else if (typedArrayType == "Int16Array") runtimeFunc = "nova_int16array_set";
                        else if (typedArrayType == "Uint16Array") runtimeFunc = "nova_uint16array_set";
                        else if (typedArrayType == "Int32Array") runtimeFunc = "nova_int32array_set";
                        else if (typedArrayType == "Uint32Array") runtimeFunc = "nova_uint32array_set";
                        else if (typedArrayType == "Float32Array") runtimeFunc = "nova_float32array_set";
                        else if (typedArrayType == "Float64Array") runtimeFunc = "nova_float64array_set";
                        else if (typedArrayType == "BigInt64Array") runtimeFunc = "nova_bigint64array_set";
                        else if (typedArrayType == "BigUint64Array") runtimeFunc = "nova_biguint64array_set";

                        if (!runtimeFunc.empty()) {
                            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                            auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);

                            // Determine value type (double for Float32/Float64, i64 otherwise)
                            HIRTypePtr valueType;
                            if (typedArrayType == "Float32Array" || typedArrayType == "Float64Array") {
                                valueType = std::make_shared<HIRType>(HIRType::Kind::F64);
                            } else {
                                valueType = std::make_shared<HIRType>(HIRType::Kind::I64);
                            }

                            std::vector<HIRTypePtr> paramTypes = {ptrType, intType, valueType};
                            auto existingFunc = module_->getFunction(runtimeFunc);
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, voidType);
                                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            std::vector<HIRValue*> args = {object, index, value};
                            builder_->createCall(func, args, "");
                            lastValue_ = value;
                            return;
                        }
                    }
                }

                // Use SetElement to store value directly to regular array element
                builder_->createSetElement(object, index, value);
            } else if (auto propExpr = dynamic_cast<Identifier*>(memberExpr->property.get())) {
                // Object property assignment: obj.x = value or this.x = value
                std::string propertyName = propExpr->name;

                // Find field index
                uint32_t fieldIndex = 0;
                bool found = false;
                hir::HIRStructType* structType = nullptr;

                // Check if this is a 'this' property assignment
                if (object == currentThis_ && currentClassStructType_) {
                    // Use the current class struct type directly
                    structType = currentClassStructType_;
                    std::cerr << "  DEBUG: Using currentClassStructType_ for 'this' property assignment" << std::endl;
                } else if (object && object->type) {
                    // First check if object is directly a struct type
                    if (object->type->kind == hir::HIRType::Kind::Struct) {
                        structType = dynamic_cast<hir::HIRStructType*>(object->type.get());
                    }
                    // Otherwise try pointer to struct
                    else if (auto ptrType = dynamic_cast<hir::HIRPointerType*>(object->type.get())) {
                        structType = dynamic_cast<hir::HIRStructType*>(ptrType->pointeeType.get());
                    }
                }

                // Find the field in the struct type
                if (structType) {
                    for (size_t i = 0; i < structType->fields.size(); ++i) {
                        if (structType->fields[i].name == propertyName) {
                            fieldIndex = static_cast<uint32_t>(i);
                            found = true;
                            std::cerr << "  DEBUG: Found field '" << propertyName << "' at index " << fieldIndex << std::endl;
                            break;
                        }
                    }
                }

                // Check if this property has a setter
                if (structType) {
                    std::string className = structType->name;
                    auto setterClassIt = classSetters_.find(className);
                    if (setterClassIt != classSetters_.end()) {
                        if (setterClassIt->second.find(propertyName) != setterClassIt->second.end()) {
                            // This property has a setter - call the setter function
                            std::string setterName = className + "_set_" + propertyName;
                            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Calling setter " << setterName << std::endl;
                            
                            auto setterFunc = module_->getFunction(setterName);
                            if (setterFunc) {
                                std::vector<HIRValue*> args = { object, value };
                                builder_->createCall(setterFunc.get(), args, "setter_result");
                                lastValue_ = value;
                                return;
                            }
                        }
                    }
                }

                if (found) {
                    // Use SetField to store value directly to the field
                    builder_->createSetField(object, fieldIndex, value, propertyName);
                    std::cerr << "  DEBUG: Created SetField for property '" << propertyName << "'" << std::endl;
                } else {
                    std::cerr << "Warning: Property '" << propertyName << "' not found for assignment" << std::endl;
                    if (currentClassStructType_) {
                        std::cerr << "  DEBUG: Current class has " << currentClassStructType_->fields.size() << " fields:" << std::endl;
                        for (const auto& field : currentClassStructType_->fields) {
                            std::cerr << "    - " << field.name << std::endl;
                        }
                    }
                }
            }
        }
    }
    

} // namespace nova::hir
