// HIRGen_Operators.cpp - Operator expression visitors
// Extracted from HIRGen.cpp for better code organization

#include "nova/HIR/HIRGen_Internal.h"
#define NOVA_DEBUG 0

namespace nova::hir {

HIRValue* HIRGenerator::toJSValue(HIRValue* value) {
        auto jsType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
        if (!value || !value->type) {
            return new HIRConstant(jsType, HIRConstant::Kind::Integer,
                static_cast<int64_t>(0x7ff9000000000000ULL));
        }
        if (auto* constant = dynamic_cast<HIRConstant*>(value)) {
            if (constant->kind == HIRConstant::Kind::Null ||
                constant->kind == HIRConstant::Kind::Undefined) {
                const uint64_t bits = constant->kind == HIRConstant::Kind::Null
                    ? 0x7ffa000000000000ULL : 0x7ff9000000000000ULL;
                return new HIRConstant(jsType, HIRConstant::Kind::Integer,
                    static_cast<int64_t>(bits));
            }
        }

        if (value->type->kind == HIRType::Kind::JSValue) return value;

        std::string functionName;
        HIRTypePtr parameterType = value->type;
        switch (value->type->kind) {
            case HIRType::Kind::F32:
            case HIRType::Kind::F64: functionName = "nova_value_from_f64"; break;
            case HIRType::Kind::Bool: functionName = "nova_value_from_bool"; break;
            case HIRType::Kind::String: functionName = "nova_value_from_string"; break;
            case HIRType::Kind::Pointer:
            case HIRType::Kind::Reference:
            case HIRType::Kind::Array:
            case HIRType::Kind::Tuple:
            case HIRType::Kind::Struct:
            case HIRType::Kind::Function:
            case HIRType::Kind::Closure:
                functionName = "nova_value_from_object";
                parameterType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                break;
            default: functionName = "nova_value_from_i64"; break;
        }

        auto existing = module_->getFunction(functionName);
        HIRFunction* function = existing ? existing.get() : nullptr;
        if (!function) {
            HIRFunctionType* functionType = new HIRFunctionType({parameterType}, jsType);
            HIRFunctionPtr created = module_->createFunction(functionName, functionType);
            created->linkage = HIRFunction::Linkage::External;
            function = created.get();
        }
        return builder_->createCall(function, {value}, "box.jsvalue");
}

HIRValue* HIRGenerator::toBoolean(HIRValue* value) {
        if (!value || !value->type) {
            return builder_->createBoolConstant(false);
        }

        // Literal truthiness can be decided without emitting runtime work.
        if (auto* constant = dynamic_cast<HIRConstant*>(value)) {
            switch (constant->kind) {
                case HIRConstant::Kind::Integer:
                    return builder_->createBoolConstant(
                        std::get<int64_t>(constant->value) != 0);
                case HIRConstant::Kind::Float: {
                    const double number = std::get<double>(constant->value);
                    return builder_->createBoolConstant(number != 0.0 && number == number);
                }
                case HIRConstant::Kind::Boolean:
                    return builder_->createBoolConstant(std::get<bool>(constant->value));
                case HIRConstant::Kind::String:
                    return builder_->createBoolConstant(
                        !std::get<std::string>(constant->value).empty());
                case HIRConstant::Kind::Null:
                case HIRConstant::Kind::Undefined:
                    return builder_->createBoolConstant(false);
            }
        }

        using Kind = HIRType::Kind;
        switch (value->type->kind) {
            case Kind::Bool:
                return value;
            case Kind::I8:
            case Kind::I16:
            case Kind::I32:
            case Kind::I64:
            case Kind::ISize:
            case Kind::U8:
            case Kind::U16:
            case Kind::U32:
            case Kind::U64:
            case Kind::USize:
            case Kind::Char:
                return builder_->createNe(value, builder_->createIntConstant(0), "tobool.int");
            case Kind::F32:
            case Kind::F64: {
                // Ordered self-comparison rejects NaN; comparison with zero
                // rejects both +0 and -0.
                auto* ordered = builder_->createEq(value, value, "tobool.ordered");
                auto* nonZero = builder_->createNe(
                    value, builder_->createFloatConstant(0.0), "tobool.nonzero");
                return builder_->createAnd(ordered, nonZero, "tobool.number");
            }
            case Kind::String:
                return builder_->createNe(
                    value, builder_->createStringConstant(""), "tobool.string");
            case Kind::JSValue: {
                auto jsType = std::make_shared<HIRType>(Kind::JSValue);
                auto intType = std::make_shared<HIRType>(Kind::I64);
                auto existing = module_->getFunction("nova_value_to_boolean");
                HIRFunction* function = existing ? existing.get() : nullptr;
                if (!function) {
                    HIRFunctionType* functionType = new HIRFunctionType({jsType}, intType);
                    HIRFunctionPtr created = module_->createFunction(
                        "nova_value_to_boolean", functionType);
                    created->linkage = HIRFunction::Linkage::External;
                    function = created.get();
                }
                return builder_->createCall(function, {value}, "tobool.jsvalue");
            }
            case Kind::Void:
            case Kind::Never:
            case Kind::Unit:
            case Kind::Optional:
            case Kind::Unknown:
                return builder_->createBoolConstant(false);
            case Kind::Pointer:
            case Kind::Reference:
            case Kind::Array:
            case Kind::Tuple:
            case Kind::Struct:
            case Kind::Function:
            case Kind::Closure:
            case Kind::Result:
            case Kind::Any:
                // All concrete ECMAScript objects are truthy. Nullable and
                // dynamically tagged values need the unified Value model.
                return builder_->createBoolConstant(true);
        }

        return builder_->createBoolConstant(false);
}

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

            // Store the selected operand in memory so both CFG paths merge
            // without an SSA phi. This also preserves JavaScript's operand-
            // returning semantics instead of collapsing the result to bool.
            auto* resultAlloca = builder_->createAlloca(
                lhs && lhs->type ? lhs->type.get() : nullptr, "logical.result");
            builder_->createStore(lhs, resultAlloca);

            // Create basic blocks for short-circuit
            auto* evalRightBlock = currentFunction_->createBasicBlock("sc.right").get();
            auto* mergeBlock = currentFunction_->createBasicBlock("sc.merge").get();

            auto lhsBool = toBoolean(lhs);

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
            builder_->createStore(rhs, resultAlloca);
            builder_->createBr(mergeBlock);

            builder_->setInsertPoint(mergeBlock);
            lastValue_ = builder_->createLoad(resultAlloca, "logical.result");

            return;
        }

        // Handle nullish coalescing operator (??)
        // Returns left operand if it's not null/undefined, otherwise returns right operand
        if (node.op == Op::NullishCoalescing) {
            node.left->accept(*this);
            auto* lhs = lastValue_;
            if (lhs && lhs->type && lhs->type->kind == HIRType::Kind::JSValue) {
                auto jsType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
                auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                auto existing = module_->getFunction("nova_value_is_nullish");
                HIRFunction* function = existing ? existing.get() : nullptr;
                if (!function) {
                    HIRFunctionType* functionType = new HIRFunctionType({jsType}, intType);
                    HIRFunctionPtr created = module_->createFunction(
                        "nova_value_is_nullish", functionType);
                    created->linkage = HIRFunction::Linkage::External;
                    function = created.get();
                }
                // createAlloca currently keeps a non-owning pointee type. Use
                // the operand's module-owned JSValue type instead of the local
                // helper shared_ptr, which would otherwise dangle and make MIR
                // randomly reinterpret the merge slot as bool/i64.
                auto* result = builder_->createAlloca(lhs->type.get(), "nullish.result");
                builder_->createStore(lhs, result);
                auto* rightBlock = currentFunction_->createBasicBlock("nullish.right").get();
                auto* mergeBlock = currentFunction_->createBasicBlock("nullish.merge").get();
                auto* isNullish = builder_->createCall(function, {lhs}, "nullish.test");
                builder_->createCondBr(isNullish, rightBlock, mergeBlock);
                builder_->setInsertPoint(rightBlock);
                node.right->accept(*this);
                builder_->createStore(toJSValue(lastValue_), result);
                builder_->createBr(mergeBlock);
                builder_->setInsertPoint(mergeBlock);
                lastValue_ = builder_->createLoad(result, "nullish.value");
                return;
            }
            auto* constant = dynamic_cast<HIRConstant*>(lastValue_);
            const bool isNullishLiteral = constant &&
                (constant->kind == HIRConstant::Kind::Null ||
                 constant->kind == HIRConstant::Kind::Undefined);
            if (isNullishLiteral) {
                node.right->accept(*this);
            }
            return;
        }

        // For non-logical operators, evaluate both operands normally
        // Generate left operand
        lastWasBigInt_ = false;
        node.left->accept(*this);
        auto lhs = lastValue_;
        const bool lhsIsBigInt = lastWasBigInt_;

        // Generate right operand
        lastWasBigInt_ = false;
        node.right->accept(*this);
        auto rhs = lastValue_;
        const bool rhsIsBigInt = lastWasBigInt_;
        lastWasBigInt_ = false;

        if (lhsIsBigInt && rhsIsBigInt) {
            auto pointerType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
            if (node.op == Op::LeftShift || node.op == Op::RightShift) {
                auto convertExisting = module_->getFunction("nova_bigint_toInt64");
                HIRFunction* convert = convertExisting ? convertExisting.get() : nullptr;
                if (!convert) {
                    HIRFunctionType* convertType = new HIRFunctionType(
                        {pointerType}, intType);
                    HIRFunctionPtr created = module_->createFunction(
                        "nova_bigint_toInt64", convertType);
                    created->linkage = HIRFunction::Linkage::External;
                    convert = created.get();
                }
                auto* shiftAmount = builder_->createCall(
                    convert, {rhs}, "bigint.shift_amount");
                const std::string functionName = node.op == Op::LeftShift
                    ? "nova_bigint_shl" : "nova_bigint_shr";
                auto shiftExisting = module_->getFunction(functionName);
                HIRFunction* shift = shiftExisting ? shiftExisting.get() : nullptr;
                if (!shift) {
                    HIRFunctionType* shiftType = new HIRFunctionType(
                        {pointerType, intType}, pointerType);
                    HIRFunctionPtr created = module_->createFunction(
                        functionName, shiftType);
                    created->linkage = HIRFunction::Linkage::External;
                    shift = created.get();
                }
                lastValue_ = builder_->createCall(
                    shift, {lhs, shiftAmount}, "bigint.shift");
                lastWasBigInt_ = true;
                return;
            }
            auto emitBigIntCall = [&](const std::string& functionName,
                                      HIRTypePtr resultType,
                                      std::vector<HIRValue*> arguments) -> HIRValue* {
                std::vector<HIRTypePtr> parameters(arguments.size(), pointerType);
                auto existing = module_->getFunction(functionName);
                HIRFunction* function = existing ? existing.get() : nullptr;
                if (!function) {
                    HIRFunctionType* functionType = new HIRFunctionType(
                        parameters, resultType);
                    HIRFunctionPtr created = module_->createFunction(
                        functionName, functionType);
                    created->linkage = HIRFunction::Linkage::External;
                    function = created.get();
                }
                return builder_->createCall(function, arguments, "bigint.binary");
            };

            std::string runtimeFunction;
            bool returnsBigInt = true;
            switch (node.op) {
                case Op::Add: runtimeFunction = "nova_bigint_add"; break;
                case Op::Sub: runtimeFunction = "nova_bigint_sub"; break;
                case Op::Mul: runtimeFunction = "nova_bigint_mul"; break;
                case Op::Div: runtimeFunction = "nova_bigint_div"; break;
                case Op::Mod: runtimeFunction = "nova_bigint_mod"; break;
                case Op::Pow: runtimeFunction = "nova_bigint_pow"; break;
                case Op::BitAnd: runtimeFunction = "nova_bigint_and"; break;
                case Op::BitOr: runtimeFunction = "nova_bigint_or"; break;
                case Op::BitXor: runtimeFunction = "nova_bigint_xor"; break;
                case Op::Equal:
                case Op::StrictEqual:
                    runtimeFunction = "nova_bigint_equals";
                    returnsBigInt = false;
                    break;
                case Op::NotEqual:
                case Op::StrictNotEqual:
                    runtimeFunction = "nova_bigint_equals";
                    returnsBigInt = false;
                    break;
                case Op::Less: runtimeFunction = "nova_bigint_lt"; returnsBigInt = false; break;
                case Op::LessEqual: runtimeFunction = "nova_bigint_le"; returnsBigInt = false; break;
                case Op::Greater: runtimeFunction = "nova_bigint_gt"; returnsBigInt = false; break;
                case Op::GreaterEqual: runtimeFunction = "nova_bigint_ge"; returnsBigInt = false; break;
                default: break;
            }

            if (!runtimeFunction.empty()) {
                lastValue_ = emitBigIntCall(
                    runtimeFunction, returnsBigInt ? pointerType : intType, {lhs, rhs});
                if (node.op == Op::NotEqual || node.op == Op::StrictNotEqual) {
                    lastValue_ = builder_->createEq(
                        lastValue_, builder_->createIntConstant(0), "bigint.not_equal");
                }
                lastWasBigInt_ = returnsBigInt;
                return;
            }
        }

        const bool hasJSValueOperand = lhs && rhs && lhs->type && rhs->type &&
            (lhs->type->kind == HIRType::Kind::JSValue ||
             rhs->type->kind == HIRType::Kind::JSValue);
        const bool bothStringOperands = lhs && rhs && lhs->type && rhs->type &&
            lhs->type->kind == HIRType::Kind::String &&
            rhs->type->kind == HIRType::Kind::String;
        const bool relationalOperator = node.op == Op::Less ||
            node.op == Op::LessEqual || node.op == Op::Greater ||
            node.op == Op::GreaterEqual;
        if (hasJSValueOperand || (bothStringOperands && relationalOperator)) {
            auto jsType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
            const auto emitCall = [&](const std::string& functionName,
                                      std::vector<HIRTypePtr> parameters,
                                      HIRTypePtr resultType,
                                      std::vector<HIRValue*> arguments) -> HIRValue* {
                auto existing = module_->getFunction(functionName);
                HIRFunction* function = existing ? existing.get() : nullptr;
                if (!function) {
                    HIRFunctionType* functionType = new HIRFunctionType(
                        std::move(parameters), std::move(resultType));
                    HIRFunctionPtr created = module_->createFunction(
                        functionName, functionType);
                    created->linkage = HIRFunction::Linkage::External;
                    function = created.get();
                }
                return builder_->createCall(function, arguments, "jsvalue.binary");
            };

            if (node.op == Op::Add) {
                lastValue_ = emitCall("nova_value_add", {jsType, jsType}, jsType,
                    {toJSValue(lhs), toJSValue(rhs)});
                return;
            }

            int64_t numericOperation = -1;
            switch (node.op) {
                case Op::Sub: numericOperation = 0; break;
                case Op::Mul: numericOperation = 1; break;
                case Op::Div: numericOperation = 2; break;
                case Op::Mod: numericOperation = 3; break;
                case Op::Pow: numericOperation = 4; break;
                case Op::BitAnd: numericOperation = 5; break;
                case Op::BitOr: numericOperation = 6; break;
                case Op::BitXor: numericOperation = 7; break;
                case Op::LeftShift: numericOperation = 8; break;
                case Op::RightShift: numericOperation = 9; break;
                case Op::UnsignedRightShift: numericOperation = 10; break;
                default: break;
            }
            if (numericOperation >= 0) {
                lastValue_ = emitCall("nova_value_binary_numeric",
                    {jsType, jsType, intType}, jsType,
                    {toJSValue(lhs), toJSValue(rhs),
                     builder_->createIntConstant(numericOperation)});
                return;
            }

            int64_t comparisonOperation = -1;
            if (node.op == Op::Less) comparisonOperation = 0;
            else if (node.op == Op::LessEqual) comparisonOperation = 1;
            else if (node.op == Op::Greater) comparisonOperation = 2;
            else if (node.op == Op::GreaterEqual) comparisonOperation = 3;
            if (comparisonOperation >= 0) {
                lastValue_ = emitCall("nova_value_compare",
                    {jsType, jsType, intType}, intType,
                    {toJSValue(lhs), toJSValue(rhs),
                     builder_->createIntConstant(comparisonOperation)});
                return;
            }
        }

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
            case Op::NotEqual:
            case Op::StrictEqual:
            case Op::StrictNotEqual: {
                const bool strict = node.op == Op::StrictEqual ||
                    node.op == Op::StrictNotEqual;
                const bool negated = node.op == Op::NotEqual ||
                    node.op == Op::StrictNotEqual;
                auto* lhsConstant = dynamic_cast<HIRConstant*>(lhs);
                auto* rhsConstant = dynamic_cast<HIRConstant*>(rhs);
                const auto nullishKind = [](HIRConstant* constant) {
                    return constant &&
                        (constant->kind == HIRConstant::Kind::Null ||
                         constant->kind == HIRConstant::Kind::Undefined);
                };
                const auto identityKind = [](HIRType::Kind kind) {
                    return kind == HIRType::Kind::Pointer ||
                        kind == HIRType::Kind::Reference ||
                        kind == HIRType::Kind::Array ||
                        kind == HIRType::Kind::Tuple ||
                        kind == HIRType::Kind::Struct ||
                        kind == HIRType::Kind::Function ||
                        kind == HIRType::Kind::Closure;
                };
                const auto emitIdentityEquality = [this](HIRValue* left,
                                                         HIRValue* right) -> HIRValue* {
                    auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                    auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                    auto existing = module_->getFunction("nova_object_is_identity");
                    HIRFunction* identity = existing ? existing.get() : nullptr;
                    if (!identity) {
                        HIRFunctionType* functionType = new HIRFunctionType(
                            {ptrType, ptrType}, intType);
                        HIRFunctionPtr function = module_->createFunction(
                            "nova_object_is_identity", functionType);
                        function->linkage = HIRFunction::Linkage::External;
                        identity = function.get();
                    }
                    return builder_->createCall(
                        identity, {left, right}, "equality.identity");
                };
                const auto emitNumberStringEquality = [this](
                    HIRValue* number, HIRValue* string) -> HIRValue* {
                    auto floatType = std::make_shared<HIRType>(HIRType::Kind::F64);
                    auto stringType = std::make_shared<HIRType>(HIRType::Kind::String);
                    auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                    auto existing = module_->getFunction(
                        "nova_abstract_equal_number_string");
                    HIRFunction* function = existing ? existing.get() : nullptr;
                    if (!function) {
                        HIRFunctionType* functionType = new HIRFunctionType(
                            {floatType, stringType}, intType);
                        HIRFunctionPtr created = module_->createFunction(
                            "nova_abstract_equal_number_string", functionType);
                        created->linkage = HIRFunction::Linkage::External;
                        function = created.get();
                    }
                    return builder_->createCall(
                        function, {number, string}, "abstract.number_string");
                };

                HIRValue* equality = nullptr;
                const bool hasJSValue = lhs && rhs && lhs->type && rhs->type &&
                    (lhs->type->kind == HIRType::Kind::JSValue ||
                     rhs->type->kind == HIRType::Kind::JSValue);
                if (hasJSValue) {
                    auto jsType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
                    auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                    const std::string functionName = strict
                        ? "nova_value_strict_equal" : "nova_value_abstract_equal";
                    auto existing = module_->getFunction(functionName);
                    HIRFunction* function = existing ? existing.get() : nullptr;
                    if (!function) {
                        HIRFunctionType* functionType = new HIRFunctionType(
                            {jsType, jsType}, intType);
                        HIRFunctionPtr created = module_->createFunction(
                            functionName, functionType);
                        created->linkage = HIRFunction::Linkage::External;
                        function = created.get();
                    }
                    equality = builder_->createCall(
                        function, {toJSValue(lhs), toJSValue(rhs)}, "equality.jsvalue");
                } else if (nullishKind(lhsConstant) || nullishKind(rhsConstant)) {
                    bool equal = false;
                    if (nullishKind(lhsConstant) && nullishKind(rhsConstant)) {
                        equal = !strict || lhsConstant->kind == rhsConstant->kind;
                    }
                    equality = builder_->createBoolConstant(equal);
                } else if (strict && lhs && rhs && lhs->type && rhs->type) {
                    const bool lhsNumber = lhs->type->isNumeric();
                    const bool rhsNumber = rhs->type->isNumeric();
                    if (lhsNumber && rhsNumber) {
                        // Integer and floating HIR values are both ECMAScript Number.
                        equality = builder_->createEq(lhs, rhs, "strict.number");
                    } else if (lhs->type->kind != rhs->type->kind) {
                        equality = builder_->createBoolConstant(false);
                    } else {
                        const auto kind = lhs->type->kind;
                        if (identityKind(kind)) {
                            equality = emitIdentityEquality(lhs, rhs);
                        } else {
                            equality = builder_->createEq(lhs, rhs, "strict.equal");
                        }
                    }
                } else if (lhs && rhs && lhs->type && rhs->type) {
                    HIRValue* abstractLhs = lhs;
                    HIRValue* abstractRhs = rhs;
                    auto lhsKind = abstractLhs->type->kind;
                    auto rhsKind = abstractRhs->type->kind;

                    if (lhsKind == HIRType::Kind::Bool) {
                        auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                        abstractLhs = builder_->createCast(abstractLhs, intType.get());
                        lhsKind = HIRType::Kind::I64;
                    }
                    if (rhsKind == HIRType::Kind::Bool) {
                        auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                        abstractRhs = builder_->createCast(abstractRhs, intType.get());
                        rhsKind = HIRType::Kind::I64;
                    }

                    const bool lhsNumber = abstractLhs->type->isNumeric();
                    const bool rhsNumber = abstractRhs->type->isNumeric();
                    if (lhsNumber && rhsKind == HIRType::Kind::String) {
                        equality = emitNumberStringEquality(abstractLhs, abstractRhs);
                    } else if (rhsNumber && lhsKind == HIRType::Kind::String) {
                        equality = emitNumberStringEquality(abstractRhs, abstractLhs);
                    } else if (lhsNumber && rhsNumber) {
                        equality = builder_->createEq(
                            abstractLhs, abstractRhs, "abstract.number");
                    } else if (lhsKind == HIRType::Kind::Any ||
                               rhsKind == HIRType::Kind::Any) {
                        // Mixed arrays currently erase their per-slot type to
                        // Any while retaining the raw i64 payload. Preserve
                        // equality for those payloads until tagged Value slots
                        // replace this representation.
                        equality = builder_->createEq(
                            abstractLhs, abstractRhs, "abstract.any_payload");
                    } else if (lhsKind != rhsKind) {
                        equality = builder_->createBoolConstant(false);
                    } else if (identityKind(lhsKind)) {
                        equality = emitIdentityEquality(abstractLhs, abstractRhs);
                    } else {
                        equality = builder_->createEq(
                            abstractLhs, abstractRhs, "abstract.equal");
                    }
                } else {
                    equality = builder_->createBoolConstant(false);
                }

                // Runtime helpers use an i64 C ABI, but every JavaScript
                // equality operator produces a Boolean value. Normalize the
                // helper result here so concatenation/string coercion prints
                // "true"/"false" rather than "1"/"0".
                if (equality && equality->type &&
                    equality->type->kind != HIRType::Kind::Bool) {
                    equality = builder_->createNe(
                        equality, builder_->createIntConstant(0),
                        "equality.boolean");
                }
                lastValue_ = negated
                    ? builder_->createEq(
                          equality, builder_->createBoolConstant(false),
                          "equality.not")
                    : equality;
                break;
            }
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
        lastWasBigInt_ = false;
        lastWasSymbol_ = false;
        node.operand->accept(*this);
        auto operand = lastValue_;
        const bool operandIsBigInt = lastWasBigInt_;
        const bool operandIsSymbol = lastWasSymbol_;
        lastWasBigInt_ = false;
        lastWasSymbol_ = false;

        using Op = UnaryExpr::Op;
        if (operandIsBigInt &&
            (node.op == Op::Minus || node.op == Op::BitNot)) {
            auto pointerType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            const std::string functionName = node.op == Op::Minus
                ? "nova_bigint_unary_minus" : "nova_bigint_not";
            auto existing = module_->getFunction(functionName);
            HIRFunction* function = existing ? existing.get() : nullptr;
            if (!function) {
                HIRFunctionType* functionType = new HIRFunctionType(
                    {pointerType}, pointerType);
                HIRFunctionPtr created = module_->createFunction(
                    functionName, functionType);
                created->linkage = HIRFunction::Linkage::External;
                function = created.get();
            }
            lastValue_ = builder_->createCall(function, {operand}, "bigint.unary");
            lastWasBigInt_ = true;
            return;
        }
        if (operand && operand->type && operand->type->kind == HIRType::Kind::JSValue &&
            (node.op == Op::Plus || node.op == Op::Minus || node.op == Op::BitNot)) {
            auto jsType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
            auto existing = module_->getFunction("nova_value_unary_numeric");
            HIRFunction* function = existing ? existing.get() : nullptr;
            if (!function) {
                HIRFunctionType* functionType = new HIRFunctionType(
                    {jsType, intType}, jsType);
                HIRFunctionPtr created = module_->createFunction(
                    "nova_value_unary_numeric", functionType);
                created->linkage = HIRFunction::Linkage::External;
                function = created.get();
            }
            const int64_t operation = node.op == Op::Plus ? 0
                : node.op == Op::Minus ? 1 : 2;
            lastValue_ = builder_->createCall(function,
                {operand, builder_->createIntConstant(operation)}, "jsvalue.unary");
            return;
        }
        switch (node.op) {
            case Op::Plus:
                // Unary plus - convert to number (for numbers, it's a no-op)
                // In JavaScript, +x converts x to a number
                // For our integer types, we just use the value as-is
                lastValue_ = operand;
                break;
            case Op::Minus:
                lastValue_ = builder_->createNeg(operand, "unary_minus");
                break;
            case Op::Not:
                lastValue_ = builder_->createEq(
                    toBoolean(operand), builder_->createBoolConstant(false),
                    "logical_not");
                break;
            case Op::BitNot:
                // Bitwise NOT
                lastValue_ = builder_->createNot(operand);
                break;
            case Op::Typeof:
                // typeof operator - return string representation of type
                {
                    std::string typeStr = "unknown";

                    if (operandIsBigInt) {
                        typeStr = "bigint";
                    } else if (operandIsSymbol) {
                        typeStr = "symbol";
                    } else if (operand && operand->type) {
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

        if (bigIntVars_.count(identifier->name) > 0) {
            auto pointerType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            const std::string functionName =
                node.op == UpdateExpr::Op::Increment
                    ? "nova_bigint_inc" : "nova_bigint_dec";
            auto existing = module_->getFunction(functionName);
            HIRFunction* function = existing ? existing.get() : nullptr;
            if (!function) {
                HIRFunctionType* functionType = new HIRFunctionType(
                    {pointerType}, pointerType);
                HIRFunctionPtr created = module_->createFunction(
                    functionName, functionType);
                created->linkage = HIRFunction::Linkage::External;
                function = created.get();
            }
            auto* newValue = builder_->createCall(
                function, {currentValue}, "bigint.update");
            builder_->createStore(newValue, varAlloca);
            lastValue_ = node.isPrefix ? newValue : currentValue;
            lastWasBigInt_ = true;
            return;
        }

        // Create constant 1 for increment/decrement
        auto one = builder_->createIntConstant(1);

        // Calculate new value
        HIRValue* newValue;
        if (currentValue && currentValue->type &&
            currentValue->type->kind == HIRType::Kind::JSValue) {
            auto jsType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
            const bool increment = node.op == UpdateExpr::Op::Increment;
            const std::string functionName = increment
                ? "nova_value_add" : "nova_value_binary_numeric";
            auto existing = module_->getFunction(functionName);
            HIRFunction* function = existing ? existing.get() : nullptr;
            if (!function) {
                std::vector<HIRTypePtr> parameters = {jsType, jsType};
                if (!increment) {
                    parameters.push_back(
                        std::make_shared<HIRType>(HIRType::Kind::I64));
                }
                HIRFunctionType* functionType = new HIRFunctionType(
                    parameters, jsType);
                HIRFunctionPtr created = module_->createFunction(
                    functionName, functionType);
                created->linkage = HIRFunction::Linkage::External;
                function = created.get();
            }
            auto* boxedOne = toJSValue(one);
            newValue = increment
                ? builder_->createCall(function, {currentValue, boxedOne}, "jsvalue.inc")
                : builder_->createCall(function,
                    {currentValue, boxedOne, builder_->createIntConstant(0)},
                    "jsvalue.dec");
        } else if (node.op == UpdateExpr::Op::Increment) {
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
        auto cond = toBoolean(lastValue_);

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
        if (node.pattern && node.op == AssignmentExpr::Op::Assign) {
            node.right->accept(*this);
            HIRValue* assigned = lastValue_;
            assignDestructuringPattern(node.pattern.get(), assigned);
            lastValue_ = assigned;
            return;
        }
        hir::HIRValue* value = nullptr;

        // Handle logical assignment operators with short-circuit evaluation
        if (node.op == AssignmentExpr::Op::LogicalAndAssign ||
            node.op == AssignmentExpr::Op::LogicalOrAssign ||
            node.op == AssignmentExpr::Op::NullishCoalescingAssign) {

            // Get current value of left side
            node.left->accept(*this);
            auto leftValue = lastValue_;

            // Create temporary variable to store result
            auto* resultAlloca = builder_->createAlloca(
                leftValue && leftValue->type ? leftValue->type.get() : nullptr,
                "logical_assign.result");

            // Create blocks
            auto* evalRightBlock = currentFunction_->createBasicBlock("logical_assign.eval_right").get();
            auto* skipBlock = currentFunction_->createBasicBlock("logical_assign.skip").get();
            auto* endBlock = currentFunction_->createBasicBlock("logical_assign.end").get();

            // Create condition based on operator type
            HIRValue* condition = nullptr;
            auto* truthy = toBoolean(leftValue);
            if (node.op == AssignmentExpr::Op::LogicalAndAssign) {
                condition = truthy;
            } else if (node.op == AssignmentExpr::Op::LogicalOrAssign) {
                condition = builder_->createEq(
                    truthy, builder_->createBoolConstant(false),
                    "logical_assign.falsy");
            } else {
                if (leftValue && leftValue->type &&
                    leftValue->type->kind == HIRType::Kind::JSValue) {
                    auto jsType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
                    auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                    auto existing = module_->getFunction("nova_value_is_nullish");
                    HIRFunction* function = existing ? existing.get() : nullptr;
                    if (!function) {
                        HIRFunctionType* functionType = new HIRFunctionType(
                            {jsType}, intType);
                        HIRFunctionPtr created = module_->createFunction(
                            "nova_value_is_nullish", functionType);
                        created->linkage = HIRFunction::Linkage::External;
                        function = created.get();
                    }
                    condition = builder_->createCall(
                        function, {leftValue}, "logical_assign.nullish");
                } else if (auto* constant = dynamic_cast<HIRConstant*>(leftValue)) {
                    condition = builder_->createBoolConstant(
                        constant->kind == HIRConstant::Kind::Null ||
                        constant->kind == HIRConstant::Kind::Undefined);
                } else {
                    condition = builder_->createBoolConstant(false);
                }
            }

            // Branch based on condition
            builder_->createCondBr(condition, evalRightBlock, skipBlock);

            // Evaluate right side and store
            builder_->setInsertPoint(evalRightBlock);
            node.right->accept(*this);
            auto rightValue = lastValue_;
            builder_->createStore(
                leftValue && leftValue->type &&
                    leftValue->type->kind == HIRType::Kind::JSValue
                    ? toJSValue(rightValue) : rightValue,
                resultAlloca);
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
            lastWasBigInt_ = false;
            node.right->accept(*this);
            auto rightValue = lastValue_;
            const bool rightIsBigInt = lastWasBigInt_;
            lastWasBigInt_ = false;

            // For compound assignments (+=, -=, etc.), need to read current value first
            hir::HIRValue* finalValue = rightValue;
            if (node.op != AssignmentExpr::Op::Assign) {
                // Get current value of left side
                node.left->accept(*this);
                auto leftValue = lastValue_;

                auto* leftIdentifier = dynamic_cast<Identifier*>(node.left.get());
                const bool leftIsBigInt = leftIdentifier &&
                    bigIntVars_.count(leftIdentifier->name) > 0;

                if (leftIsBigInt && rightIsBigInt) {
                    auto pointerType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                    auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                    std::string functionName;
                    switch (node.op) {
                        case AssignmentExpr::Op::AddAssign:
                            functionName = "nova_bigint_add"; break;
                        case AssignmentExpr::Op::SubAssign:
                            functionName = "nova_bigint_sub"; break;
                        case AssignmentExpr::Op::MulAssign:
                            functionName = "nova_bigint_mul"; break;
                        case AssignmentExpr::Op::DivAssign:
                            functionName = "nova_bigint_div"; break;
                        case AssignmentExpr::Op::ModAssign:
                            functionName = "nova_bigint_mod"; break;
                        case AssignmentExpr::Op::PowAssign:
                            functionName = "nova_bigint_pow"; break;
                        case AssignmentExpr::Op::BitAndAssign:
                            functionName = "nova_bigint_and"; break;
                        case AssignmentExpr::Op::BitOrAssign:
                            functionName = "nova_bigint_or"; break;
                        case AssignmentExpr::Op::BitXorAssign:
                            functionName = "nova_bigint_xor"; break;
                        case AssignmentExpr::Op::LeftShiftAssign:
                            functionName = "nova_bigint_shl"; break;
                        case AssignmentExpr::Op::RightShiftAssign:
                            functionName = "nova_bigint_shr"; break;
                        default:
                            break;
                    }

                    if (!functionName.empty()) {
                        HIRValue* secondArgument = rightValue;
                        std::vector<HIRTypePtr> parameterTypes = {
                            pointerType, pointerType};
                        if (node.op == AssignmentExpr::Op::LeftShiftAssign ||
                            node.op == AssignmentExpr::Op::RightShiftAssign) {
                            auto convertExisting = module_->getFunction(
                                "nova_bigint_toInt64");
                            HIRFunction* convert = convertExisting
                                ? convertExisting.get() : nullptr;
                            if (!convert) {
                                HIRFunctionType* convertType = new HIRFunctionType(
                                    {pointerType}, intType);
                                HIRFunctionPtr created = module_->createFunction(
                                    "nova_bigint_toInt64", convertType);
                                created->linkage = HIRFunction::Linkage::External;
                                convert = created.get();
                            }
                            secondArgument = builder_->createCall(
                                convert, {rightValue}, "bigint.shift_amount");
                            parameterTypes[1] = intType;
                        }

                        auto existing = module_->getFunction(functionName);
                        HIRFunction* function = existing ? existing.get() : nullptr;
                        if (!function) {
                            HIRFunctionType* functionType = new HIRFunctionType(
                                parameterTypes, pointerType);
                            HIRFunctionPtr created = module_->createFunction(
                                functionName, functionType);
                            created->linkage = HIRFunction::Linkage::External;
                            function = created.get();
                        }
                        finalValue = builder_->createCall(
                            function, {leftValue, secondArgument},
                            "bigint.compound_assign");
                        lastWasBigInt_ = true;
                    }
                }

                // Perform the binary operation. Dynamic bindings must use
                // ECMAScript coercion helpers instead of operating on tag bits.
                if (leftIsBigInt && rightIsBigInt) {
                    // The BigInt operation above already produced finalValue.
                } else if (leftValue && leftValue->type &&
                    leftValue->type->kind == HIRType::Kind::JSValue) {
                    auto jsType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
                    if (node.op == AssignmentExpr::Op::AddAssign) {
                        auto existing = module_->getFunction("nova_value_add");
                        HIRFunction* function = existing ? existing.get() : nullptr;
                        if (!function) {
                            HIRFunctionType* functionType = new HIRFunctionType(
                                {jsType, jsType}, jsType);
                            HIRFunctionPtr created = module_->createFunction(
                                "nova_value_add", functionType);
                            created->linkage = HIRFunction::Linkage::External;
                            function = created.get();
                        }
                        finalValue = builder_->createCall(function,
                            {leftValue, toJSValue(rightValue)}, "jsvalue.add_assign");
                    } else {
                        int64_t operation = 0;
                        switch (node.op) {
                            case AssignmentExpr::Op::SubAssign: operation = 0; break;
                            case AssignmentExpr::Op::MulAssign: operation = 1; break;
                            case AssignmentExpr::Op::DivAssign: operation = 2; break;
                            case AssignmentExpr::Op::ModAssign: operation = 3; break;
                            case AssignmentExpr::Op::PowAssign: operation = 4; break;
                            case AssignmentExpr::Op::BitAndAssign: operation = 5; break;
                            case AssignmentExpr::Op::BitOrAssign: operation = 6; break;
                            case AssignmentExpr::Op::BitXorAssign: operation = 7; break;
                            case AssignmentExpr::Op::LeftShiftAssign: operation = 8; break;
                            case AssignmentExpr::Op::RightShiftAssign: operation = 9; break;
                            case AssignmentExpr::Op::UnsignedRightShiftAssign: operation = 10; break;
                            default: break;
                        }
                        auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                        auto existing = module_->getFunction("nova_value_binary_numeric");
                        HIRFunction* function = existing ? existing.get() : nullptr;
                        if (!function) {
                            HIRFunctionType* functionType = new HIRFunctionType(
                                {jsType, jsType, intType}, jsType);
                            HIRFunctionPtr created = module_->createFunction(
                                "nova_value_binary_numeric", functionType);
                            created->linkage = HIRFunction::Linkage::External;
                            function = created.get();
                        }
                        finalValue = builder_->createCall(function,
                            {leftValue, toJSValue(rightValue),
                             builder_->createIntConstant(operation)},
                            "jsvalue.compound_assign");
                    }
                } else switch (node.op) {
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
            if (!target && currentFunction_ && !lastFunctionName_.empty()) {
                auto captured = capturedVariables_.find(lastFunctionName_);
                if (captured != capturedVariables_.end() &&
                    captured->second.count(id->name) != 0) {
                    target = getCapturedVariableStorage(id->name);
                }
            }
            bool targetIsJSValue = false;
            if (target) {
                HIRValue* storedValue = value;
                const bool targetIsBigInt = bigIntVars_.count(id->name) > 0;
                if (!targetIsBigInt) {
                    if (auto* pointerType = target->type
                        ? dynamic_cast<HIRPointerType*>(target->type.get())
                        : nullptr;
                        pointerType && pointerType->pointeeType && value && value->type) {
                        if (pointerType->pointeeType->kind != HIRType::Kind::JSValue &&
                            pointerType->pointeeType->kind != value->type->kind) {
                            pointerType->pointeeType = std::make_shared<HIRType>(
                                HIRType::Kind::JSValue);
                        }
                        if (pointerType->pointeeType->kind == HIRType::Kind::JSValue) {
                            targetIsJSValue = true;
                            storedValue = toJSValue(value);
                        }
                    }
                } else {
                    lastWasBigInt_ = true;
                }
                builder_->createStore(storedValue, target);
            }
            auto* constant = dynamic_cast<HIRConstant*>(value);
            if (!targetIsJSValue && constant &&
                (constant->kind == HIRConstant::Kind::Null ||
                 constant->kind == HIRConstant::Kind::Undefined)) {
                staticNullishVariables_[id->name] = constant->kind;
            } else {
                staticNullishVariables_.erase(id->name);
            }
        } else if (auto* memberExpr = dynamic_cast<MemberExpr*>(node.left.get())) {
            std::string objectVariableName;
            if (auto* objectIdentifier = dynamic_cast<Identifier*>(memberExpr->object.get())) {
                objectVariableName = objectIdentifier->name;
            }

            // Get the object/array
            memberExpr->object->accept(*this);
            auto object = lastValue_;

            if (memberExpr->isComputed) {
                // Array element assignment: arr[index] = value
                if (auto* keyLiteral =
                        dynamic_cast<StringLiteral*>(memberExpr->property.get())) {
                    HIRStructType* structType = nullptr;
                    if (object && object->type) {
                        structType = dynamic_cast<HIRStructType*>(object->type.get());
                        if (!structType) {
                            if (auto* pointerType =
                                    dynamic_cast<HIRPointerType*>(object->type.get())) {
                                structType = dynamic_cast<HIRStructType*>(
                                    pointerType->pointeeType.get());
                            }
                        }
                    }
                    if (structType) {
                        for (size_t i = 0; i < structType->fields.size(); ++i) {
                            if (structType->fields[i].name == keyLiteral->value) {
                                bool canWrite = objectVariableName.empty() ||
                                    frozenObjectVars_.count(objectVariableName) == 0;
                                if (canWrite && !objectVariableName.empty()) {
                                    auto objectAttributes =
                                        propertyWritable_.find(objectVariableName);
                                    if (objectAttributes != propertyWritable_.end()) {
                                        auto attribute = objectAttributes->second.find(
                                            keyLiteral->value);
                                        if (attribute != objectAttributes->second.end()) {
                                            canWrite = attribute->second;
                                        }
                                    }
                                }
                                if (canWrite && value && value->type &&
                                    structType->fields[i].type) {
                                    HIRValue* storedValue = value;
                                    if (structType->name.rfind("__obj_", 0) == 0 &&
                                        value->type->kind != structType->fields[i].type->kind) {
                                        structType->fields[i].type = std::make_shared<HIRType>(
                                            HIRType::Kind::JSValue);
                                    }
                                    if (structType->fields[i].type->kind ==
                                        HIRType::Kind::JSValue) {
                                        storedValue = toJSValue(value);
                                    }
                                    builder_->createSetField(
                                        object, static_cast<uint32_t>(i), storedValue,
                                        keyLiteral->value);
                                }
                                lastValue_ = value;
                                return;
                            }
                        }
                        // Adding a computed field is impossible on a fixed layout.
                        lastValue_ = value;
                        return;
                    }
                }

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
                    // Assignment to a frozen static object is ignored in Nova's
                    // current non-strict execution mode. The expression still
                    // evaluates to the right-hand value, matching JavaScript.
                    if (!objectVariableName.empty() &&
                        frozenObjectVars_.count(objectVariableName) > 0) {
                        lastValue_ = value;
                        return;
                    }
                    if (!objectVariableName.empty()) {
                        auto objectAttributes = propertyWritable_.find(objectVariableName);
                        if (objectAttributes != propertyWritable_.end()) {
                            auto propertyAttribute =
                                objectAttributes->second.find(propertyName);
                            if (propertyAttribute != objectAttributes->second.end() &&
                                !propertyAttribute->second) {
                                lastValue_ = value;
                                return;
                            }
                        }
                    }

                    HIRValue* storedValue = value;
                    if (structType && structType->name.rfind("__obj_", 0) == 0 &&
                        structType->fields[fieldIndex].type && value && value->type &&
                        value->type->kind != structType->fields[fieldIndex].type->kind) {
                        structType->fields[fieldIndex].type = std::make_shared<HIRType>(
                            HIRType::Kind::JSValue);
                    }
                    if (structType && structType->fields[fieldIndex].type &&
                        structType->fields[fieldIndex].type->kind == HIRType::Kind::JSValue) {
                        storedValue = toJSValue(value);
                    }
                    // Use SetField to store value directly to the field.
                    builder_->createSetField(object, fieldIndex, storedValue, propertyName);
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
