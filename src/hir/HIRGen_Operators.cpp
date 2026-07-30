// HIRGen_Operators.cpp - Operator expression visitors
// Extracted from HIRGen.cpp for better code organization

#include "nova/HIR/HIRGen_Internal.h"
#include <unordered_map>
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

        // Handle instanceof operator - resolve statically when possible
        if (node.op == Op::Instanceof) {
            std::string rhsClassName;
            if (auto* rhsIdent = dynamic_cast<Identifier*>(node.right.get())) {
                rhsClassName = rhsIdent->name;
            }

            // Try to determine left side's class statically
            std::string lhsClassName;
            // Case 1: lhs is `new ClassName(...)` - inspect AST directly
            if (auto* lhsNew = dynamic_cast<NewExpr*>(node.left.get())) {
                if (auto* id = dynamic_cast<Identifier*>(lhsNew->callee.get())) {
                    lhsClassName = id->name;
                }
            }
            // Case 2: lhs is an Identifier - look up classReferences_ and variableKinds_
            if (lhsClassName.empty()) {
                if (auto* lhsIdent = dynamic_cast<Identifier*>(node.left.get())) {
                    auto it = classReferences_.find(lhsIdent->name);
                    if (it != classReferences_.end()) {
                        lhsClassName = it->second;
                    }
                    if (lhsClassName.empty()) {
                        auto kindIt = variableKinds_.find(lhsIdent->name);
                        if (kindIt != variableKinds_.end()) {
                            lhsClassName = kindIt->second;
                        }
                    }
                }
            }
            // Case 3: lhs is an Array literal -> Array
            if (lhsClassName.empty()) {
                if (dynamic_cast<ArrayExpr*>(node.left.get())) {
                    lhsClassName = "Array";
                }
            }
            // Case 4: lhs is an Object literal -> Object
            if (lhsClassName.empty()) {
                if (dynamic_cast<ObjectExpr*>(node.left.get())) {
                    lhsClassName = "Object";
                }
            }
            // Case 5: lhs is a Function/ArrowFunction -> Function
            if (lhsClassName.empty()) {
                if (dynamic_cast<ArrowFunctionExpr*>(node.left.get()) ||
                    dynamic_cast<FunctionExpr*>(node.left.get())) {
                    lhsClassName = "Function";
                }
            }
            // Case 6: function references via identifier
            if (lhsClassName.empty()) {
                if (auto* lhsIdent = dynamic_cast<Identifier*>(node.left.get())) {
                    if (functionReferences_.count(lhsIdent->name)) {
                        lhsClassName = "Function";
                    }
                }
            }

            // If we know both classes, resolve at compile time
            if (!lhsClassName.empty() && !rhsClassName.empty()) {
                // Special: everything is an instance of Object
                static const std::unordered_set<std::string> objectLike = {
                    "Array", "Function", "Object", "Error",
                    "TypeError", "RangeError", "ReferenceError",
                    "SyntaxError", "URIError", "EvalError", "InternalError"
                };
                if (rhsClassName == "Object" &&
                    (objectLike.count(lhsClassName) || classInheritance_.count(lhsClassName))) {
                    lastValue_ = builder_->createBoolConstant(true);
                    return;
                }
                // Error subclass checks: all builtin error types extend Error
                if (rhsClassName == "Error" && objectLike.count(lhsClassName) &&
                    lhsClassName != "Array" && lhsClassName != "Function" &&
                    lhsClassName != "Object") {
                    lastValue_ = builder_->createBoolConstant(true);
                    return;
                }
                bool result = false;
                std::string current = lhsClassName;
                std::unordered_set<std::string> visited;
                while (!current.empty() && visited.count(current) == 0) {
                    if (current == rhsClassName) {
                        result = true;
                        break;
                    }
                    visited.insert(current);
                    auto it = classInheritance_.find(current);
                    if (it == classInheritance_.end()) break;
                    current = it->second;
                }
                lastValue_ = builder_->createBoolConstant(result);
                return;
            }

            // Dynamic path: LHS class unknown statically (e.g. caught exception
            // value). When RHS is a known class, walk the inheritance chain at
            // compile time and emit one runtime comparison per candidate name
            // (the RHS plus each known ancestor), OR-ed together.
            if (lhsClassName.empty() && !rhsClassName.empty()) {
                auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

                // First, when RHS is a builtin Error subclass, evaluate LHS
                // to a pointer and dispatch to the matching nova_is_<type>
                // runtime helper. This handles values that were never thrown
                // (e.g. `aggregate.errors[0] instanceof RangeError`).
                static const std::unordered_map<std::string, std::string>
                    builtinErrorCheck = {
                        {"Error", "nova_is_error"},
                        {"TypeError", "nova_is_type_error"},
                        {"RangeError", "nova_is_range_error"},
                        {"ReferenceError", "nova_is_reference_error"},
                        {"SyntaxError", "nova_is_syntax_error"},
                        {"URIError", "nova_is_uri_error"},
                        {"AggregateError", "nova_is_aggregate_error"},
                        {"Promise", "nova_promise_isPromise"},
                    };
                auto builtinErrIt = builtinErrorCheck.find(rhsClassName);
                HIRValue* runtimeTypeCheck = nullptr;
                if (builtinErrIt != builtinErrorCheck.end()) {
                    // Evaluate LHS
                    node.left->accept(*this);
                    HIRValue* lhsValue = lastValue_;
                    if (lhsValue && lhsValue->type &&
                        lhsValue->type->kind == HIRType::Kind::I64) {
                        // Cast raw i64 (pointer bits) to pointer
                        lhsValue = builder_->createCast(
                            lhsValue, ptrType.get(), "instanceof.lhs.ptr");
                    } else if (lhsValue && lhsValue->type &&
                               lhsValue->type->kind == HIRType::Kind::JSValue) {
                        auto existingUnbox = module_->getFunction("nova_value_to_object");
                        HIRFunction* unbox = existingUnbox ? existingUnbox.get() : nullptr;
                        if (!unbox) {
                            auto jsValueType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
                            auto* type = new HIRFunctionType({jsValueType}, ptrType);
                            auto created = module_->createFunction("nova_value_to_object", type);
                            created->linkage = HIRFunction::Linkage::External;
                            unbox = created.get();
                        }
                        lhsValue = builder_->createCall(unbox, {lhsValue}, "instanceof.lhs.unbox");
                        lhsValue->type = ptrType;
                    }

                    HIRFunction* checkFn = module_->getFunction(builtinErrIt->second).get();
                    if (!checkFn) {
                        HIRFunctionType* ft = new HIRFunctionType({ptrType}, intType);
                        auto fp = module_->createFunction(builtinErrIt->second, ft);
                        fp->linkage = HIRFunction::Linkage::External;
                        checkFn = fp.get();
                    }
                    runtimeTypeCheck = builder_->createCall(
                        checkFn, {lhsValue}, "instanceof.builtin");
                    runtimeTypeCheck->type = intType;
                    // For specific builtin subclasses (TypeError, RangeError,
                    // etc.) and Promise, the runtime check is authoritative —
                    // a value either has that error-type tag / is a registered
                    // promise, or it doesn't. Return early so we don't OR in
                    // stale thrown-class state.
                    // For "Error" RHS we fall through and OR with the
                    // thrown-class-name candidates, because custom subclasses
                    // (e.g. `class MyErr extends Error`) are not NovaError
                    // instances and `nova_is_error` would miss them.
                    if (rhsClassName != "Error") {
                        lastValue_ = builder_->createNe(
                            runtimeTypeCheck, builder_->createIntConstant(0),
                            "instanceof.bool");
                        return;
                    }
                }

                // Get the runtime class-name tag (set at throw time)
                std::string getFuncName = "nova_get_thrown_class_name";
                HIRFunction* getFunc = module_->getFunction(getFuncName).get();
                if (!getFunc) {
                    HIRFunctionType* ft = new HIRFunctionType({}, ptrType);
                    auto fp = module_->createFunction(getFuncName, ft);
                    fp->linkage = HIRFunction::Linkage::External;
                    getFunc = fp.get();
                }
                auto* thrownName = builder_->createCall(getFunc, {}, "thrown.class");

                std::string eqFuncName = "nova_string_equals";
                HIRFunction* eqFunc = module_->getFunction(eqFuncName).get();
                if (!eqFunc) {
                    HIRFunctionType* ft = new HIRFunctionType({ptrType, ptrType}, intType);
                    auto fp = module_->createFunction(eqFuncName, ft);
                    fp->linkage = HIRFunction::Linkage::External;
                    eqFunc = fp.get();
                }

                // Build candidate list: RHS plus every class whose ancestor
                // chain includes RHS (i.e. all transitive subclasses). This
                // lets `e instanceof Parent` match a child instance.
                std::vector<std::string> candidates;
                candidates.push_back(rhsClassName);

                std::unordered_set<std::string> known;
                known.insert(rhsClassName);

                // Builtin Error hierarchy. Nova represents builtin error
                // subclasses (TypeError extends Error, etc.) only at runtime;
                // the static classInheritance_ table doesn't know about them.
                // Inject them here so `instanceof Error` and `instanceof
                // <BuiltinError>` see the full subtree.
                static const std::unordered_map<std::string,
                    std::vector<std::string>> builtinSubclasses = {
                    {"Error", {"TypeError", "RangeError", "ReferenceError",
                               "SyntaxError", "URIError", "InternalError",
                               "EvalError", "AggregateError"}},
                    {"RangeError", {"AggregateError"}},
                };
                auto builtinIt = builtinSubclasses.find(rhsClassName);
                if (builtinIt != builtinSubclasses.end()) {
                    for (const auto& sub : builtinIt->second) {
                        if (known.count(sub) == 0) {
                            known.insert(sub);
                            candidates.push_back(sub);
                        }
                    }
                }

                // Repeatedly scan classInheritance_ for new child classes until
                // a fixed point is reached. This handles multi-level chains
                // (C extends B extends A extends Error).
                bool changed = true;
                while (changed) {
                    changed = false;
                    for (const auto& kv : classInheritance_) {
                        // If the parent is already known, then the child is a
                        // (transitive) subclass of rhs.
                        if (known.count(kv.second) && !known.count(kv.first)) {
                            known.insert(kv.first);
                            candidates.push_back(kv.first);
                            changed = true;
                        }
                    }
                }

                // OR-ed runtime checks: result = eq(thrown, c0) | eq(thrown, c1) | ...
                HIRValue* acc = nullptr;
                for (const auto& name : candidates) {
                    auto* nameConst = builder_->createStringConstant(name);
                    auto* eq = builder_->createCall(eqFunc, {thrownName, nameConst},
                                                    "instanceof.cmp");
                    if (!acc) {
                        acc = eq;
                    } else {
                        acc = builder_->createOr(acc, eq, "instanceof.or");
                    }
                }

                if (!acc) {
                    if (runtimeTypeCheck) {
                        auto* zero = builder_->createIntConstant(0);
                        lastValue_ = builder_->createNe(
                            runtimeTypeCheck, zero, "instanceof.bool");
                    } else {
                        lastValue_ = builder_->createBoolConstant(false);
                    }
                } else {
                    if (runtimeTypeCheck) {
                        acc = builder_->createOr(
                            acc, runtimeTypeCheck, "instanceof.runtime_or");
                    }
                    auto* zero = builder_->createIntConstant(0);
                    lastValue_ = builder_->createNe(acc, zero, "instanceof.bool");
                }
                return;
            }

            // Fallback: emit false (unknown instanceof)
            lastValue_ = builder_->createBoolConstant(false);
            return;
        }

        // Handle `in` operator: `"key" in obj`.
        // Routes through nova_object_has(Object*, const char*) at runtime —
        // works for both runtime Objects (Object.create / dynamic literals)
        // and class instances reinterpreted as Object*.
        if (node.op == Op::In) {
            node.left->accept(*this);
            HIRValue* key = lastValue_;
            node.right->accept(*this);
            HIRValue* obj = lastValue_;

            auto pointerType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto stringType = std::make_shared<HIRType>(HIRType::Kind::String);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

            // Coerce key to string if needed.
            if (key && key->type && key->type->kind != HIRType::Kind::String) {
                // For JSValue keys, unbox to a C string via nova_value_to_string_ptr.
                if (key->type->kind == HIRType::Kind::JSValue) {
                    auto jsValueType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
                    auto existing = module_->getFunction("nova_value_to_string_ptr");
                    HIRFunction* fn = existing ? existing.get() : nullptr;
                    if (!fn) {
                        auto* type = new HIRFunctionType({jsValueType}, stringType);
                        auto created = module_->createFunction("nova_value_to_string_ptr", type);
                        created->linkage = HIRFunction::Linkage::External;
                        fn = created.get();
                    }
                    key = builder_->createCall(fn, {key}, "in.key.to_string");
                    key->type = stringType;
                } else if (key->type->kind == HIRType::Kind::I64 ||
                           key->type->kind == HIRType::Kind::I32) {
                    // Convert numeric key to string via a runtime helper that
                    // formats the integer. We bypass createCast here because
                    // pointer-keyed values would otherwise hit a bogus SExt.
                    auto existing = module_->getFunction("nova_value_key_to_string");
                    HIRFunction* fn = existing ? existing.get() : nullptr;
                    if (!fn) {
                        auto* type = new HIRFunctionType({intType}, stringType);
                        auto created = module_->createFunction("nova_value_key_to_string", type);
                        created->linkage = HIRFunction::Linkage::External;
                        fn = created.get();
                    }
                    // Widen to i64 if narrower (e.g. i32) — integer-to-integer
                    // cast is the only safe kind here.
                    HIRValue* asI64 = key;
                    if (key->type->kind == HIRType::Kind::I32) {
                        asI64 = builder_->createCast(key, intType.get());
                    }
                    key = builder_->createCall(fn, {asI64}, "in.key.numeric");
                    key->type = stringType;
                }
            }

            // Coerce object to pointer.
            if (obj && obj->type && obj->type->kind == HIRType::Kind::JSValue) {
                auto jsValueType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
                auto existingUnbox = module_->getFunction("nova_value_to_object");
                HIRFunction* unbox = existingUnbox ? existingUnbox.get() : nullptr;
                if (!unbox) {
                    auto* type = new HIRFunctionType({jsValueType}, pointerType);
                    auto created = module_->createFunction("nova_value_to_object", type);
                    created->linkage = HIRFunction::Linkage::External;
                    unbox = created.get();
                }
                obj = builder_->createCall(unbox, {obj}, "in.obj.unbox");
                obj->type = pointerType;
            }

            auto existingHas = module_->getFunction("nova_object_has");
            HIRFunction* hasFn = existingHas ? existingHas.get() : nullptr;
            if (!hasFn) {
                auto* type = new HIRFunctionType({pointerType, stringType}, intType);
                auto created = module_->createFunction("nova_object_has", type);
                created->linkage = HIRFunction::Linkage::External;
                hasFn = created.get();
            }

            lastValue_ = builder_->createCall(hasFn, {obj, key}, "in.has");
            return;
        }

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

        // Coerce object-literal operands to primitives when possible.
        // If an operand is an instance of an anonymous object literal struct
        // (named "__obj_N") and that object defines a `valueOf` (preferred) or
        // `toString` method, emit a direct call to that method so the binary
        // operator sees a primitive value. This is the static fast-path for
        // ECMAScript ToPrimitive(number) on object literals; the runtime path
        // for opaque objects is handled by nova_value_to_primitive.
        auto coerceObjectLiteral = [this](HIRValue* value) -> HIRValue* {
            if (!value || !value->type) return value;
            // Resolve to the underlying struct type — either directly, or
            // through a pointer-typed value (the common case after a load).
            hir::HIRStructType* structType = nullptr;
            HIRValue* receiver = value;
            if (auto* s = dynamic_cast<hir::HIRStructType*>(value->type.get())) {
                structType = s;
            } else if (auto* p = dynamic_cast<hir::HIRPointerType*>(value->type.get())) {
                if (p->pointeeType) {
                    structType = dynamic_cast<hir::HIRStructType*>(p->pointeeType.get());
                }
            }
            if (!structType) return value;
            if (structType->name.rfind("__obj_", 0) != 0) return value;
            auto objIt = objectMethodFunctions_.find(structType->name);
            if (objIt == objectMethodFunctions_.end()) return value;
            const auto& methods = objIt->second;

            // Prefer valueOf, fall back to toString (ToPrimitive default
            // hint for `+` and arithmetic is number).
            std::string methodName;
            auto valueOfIt = methods.find("valueOf");
            if (valueOfIt != methods.end()) {
                methodName = valueOfIt->second;
            } else {
                auto toStringIt = methods.find("toString");
                if (toStringIt == methods.end()) return value;
                methodName = toStringIt->second;
            }

            auto existing = module_->getFunction(methodName);
            HIRFunction* fn = existing ? existing.get() : nullptr;
            if (!fn) return value;

            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            std::vector<HIRTypePtr> paramVec = {ptrType};
            auto retType = std::make_shared<HIRType>(HIRType::Kind::Any);
            HIRFunctionType* fnType = new HIRFunctionType(paramVec, retType);
            HIRFunctionPtr created = module_->createFunction(methodName, fnType);
            created->linkage = HIRFunction::Linkage::External;
            return builder_->createCall(created.get(), {receiver}, "obj.primitive");
        };

        // Coercion applies to every arithmetic, comparison, and bitwise
        // operator below. We rewrite lhs/rhs in place before the switch so
        // every case benefits.
        if (node.op != Op::Add || !(lhs && lhs->type &&
                lhs->type->kind == HIRType::Kind::String)) {
            lhs = coerceObjectLiteral(lhs);
        }
        if (node.op != Op::Add || !(rhs && rhs->type &&
                rhs->type->kind == HIRType::Kind::String)) {
            rhs = coerceObjectLiteral(rhs);
        }

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
            case Op::Pow: {
                // Constant-fold integer ** integer when both operands are
                // literals. Promote to F64 when the result reaches
                // Number.MAX_SAFE_INTEGER so IEEE 754 semantics hold
                // (e.g. `2 ** 53 + 1 === 2 ** 53`).
                auto* lhsConst = dynamic_cast<HIRConstant*>(lhs);
                auto* rhsConst = dynamic_cast<HIRConstant*>(rhs);
                HIRValue* powResult = nullptr;
                if (lhsConst && rhsConst &&
                    lhsConst->kind == HIRConstant::Kind::Integer &&
                    rhsConst->kind == HIRConstant::Kind::Integer) {
                    const int64_t base = std::get<int64_t>(lhsConst->value);
                    const int64_t exp = std::get<int64_t>(rhsConst->value);
                    if (exp >= 0 && exp <= 62) {
                        int64_t acc = 1;
                        bool exceedsSafeInt = false;
                        for (int i = 0; i < exp; ++i) {
                            if (acc > (1LL << 53) / std::max<int64_t>(1, std::llabs(base))) {
                                exceedsSafeInt = true;
                                break;
                            }
                            acc *= base;
                        }
                        if (!exceedsSafeInt && std::llabs(acc) >= (1LL << 53)) {
                            exceedsSafeInt = true;
                        }
                        if (exceedsSafeInt) {
                            double floatResult = 1.0;
                            for (int i = 0; i < exp; ++i) {
                                floatResult *= static_cast<double>(base);
                            }
                            powResult = builder_->createFloatConstant(floatResult);
                        } else {
                            powResult = builder_->createIntConstant(acc);
                        }
                    }
                }
                if (!powResult) {
                    powResult = builder_->createPow(lhs, rhs);
                }
                lastValue_ = powResult;
                break;
            }
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
                    // For both strict AND abstract equality, when one side is
                    // a pointer-typed value (e.g. an Object* that may legitimately
                    // be null) and the other is a nullish literal, emit a runtime
                    // pointer==0 check instead of constant-folding. This matters
                    // for `Object.getOwnPropertyDescriptor(obj, "missing")` which
                    // returns nullptr; `result != undefined` must compare the
                    // pointer payload, not fold to `true`.
                    auto emitPointerNullCheck = [&](HIRValue* ptrVal) -> HIRValue* {
                        return builder_->createEq(
                            ptrVal, builder_->createIntConstant(0), "ptr.nullish_eq");
                    };
                    if (nullishKind(rhsConstant) && lhs && lhs->type &&
                        (lhs->type->kind == HIRType::Kind::Pointer ||
                         lhs->type->kind == HIRType::Kind::String ||
                         lhs->type->kind == HIRType::Kind::JSValue ||
                         lhs->type->kind == HIRType::Kind::Function ||
                         lhs->type->kind == HIRType::Kind::Closure ||
                         lhs->type->kind == HIRType::Kind::Any)) {
                        equality = emitPointerNullCheck(lhs);
                    } else if (nullishKind(lhsConstant) && rhs && rhs->type &&
                               (rhs->type->kind == HIRType::Kind::Pointer ||
                                rhs->type->kind == HIRType::Kind::String ||
                                rhs->type->kind == HIRType::Kind::JSValue ||
                                rhs->type->kind == HIRType::Kind::Function ||
                                rhs->type->kind == HIRType::Kind::Closure ||
                                rhs->type->kind == HIRType::Kind::Any)) {
                        equality = emitPointerNullCheck(rhs);
                    } else if (strict) {
                        // `ptr === null` or `null === ptr`: compare the pointer
                        // payload against 0. The null literal in Nova lowers to
                        // a zero constant, so this is a plain i64 equality.
                        if (nullishKind(rhsConstant) &&
                            lhs && lhs->type &&
                            (lhs->type->kind == HIRType::Kind::Pointer ||
                             lhs->type->kind == HIRType::Kind::String ||
                             lhs->type->kind == HIRType::Kind::JSValue ||
                             lhs->type->kind == HIRType::Kind::Function ||
                             lhs->type->kind == HIRType::Kind::Closure ||
                             lhs->type->kind == HIRType::Kind::Any)) {
                            equality = builder_->createEq(
                                lhs, builder_->createIntConstant(0), "strict.ptr_null");
                            // `equal` flag is no longer a compile-time constant.
                        } else if (nullishKind(lhsConstant) &&
                                   rhs && rhs->type &&
                                   (rhs->type->kind == HIRType::Kind::Pointer ||
                                    rhs->type->kind == HIRType::Kind::String ||
                                    rhs->type->kind == HIRType::Kind::JSValue ||
                                    rhs->type->kind == HIRType::Kind::Function ||
                                    rhs->type->kind == HIRType::Kind::Closure ||
                                    rhs->type->kind == HIRType::Kind::Any)) {
                            equality = builder_->createEq(
                                rhs, builder_->createIntConstant(0), "strict.null_ptr");
                        } else {
                            equality = builder_->createBoolConstant(equal);
                        }
                    } else {
                        equality = builder_->createBoolConstant(equal);
                    }
                } else if (strict && lhs && rhs && lhs->type && rhs->type) {
                    const bool lhsNumber = lhs->type->isNumeric();
                    const bool rhsNumber = rhs->type->isNumeric();
                    const bool lhsAny = lhs->type->kind == HIRType::Kind::Any;
                    const bool rhsAny = rhs->type->kind == HIRType::Kind::Any;
                    // Strings produced by runtime calls (nova_error_get_name,
                    // nova_value_to_string_alloc, etc.) arrive as Pointer-typed
                    // HIR values, while string literals are typed String. Treat
                    // Pointer/Function/Closure/Reference on one side and String
                    // on the other as a string equality via nova_string_equals,
                    // otherwise `err.name !== "TypeError"` gets constant-folded
                    // to `true` and the entire if-body is elided.
                    const auto isStringLike = [](HIRType::Kind kind) {
                        return kind == HIRType::Kind::String ||
                            kind == HIRType::Kind::Pointer ||
                            kind == HIRType::Kind::Reference ||
                            kind == HIRType::Kind::Function ||
                            kind == HIRType::Kind::Closure;
                    };
                    const bool lhsStringy = isStringLike(lhs->type->kind);
                    const bool rhsStringy = isStringLike(rhs->type->kind);
                    if (lhsNumber && rhsNumber) {
                        // Integer and floating HIR values are both ECMAScript Number.
                        equality = builder_->createEq(lhs, rhs, "strict.number");
                    } else if ((lhsAny || rhsAny) && (lhsNumber || rhsNumber)) {
                        // Generic type parameters erase to Any but carry a raw
                        // numeric payload at runtime; compare those bits
                        // numerically so `id<number>(42) === 42` holds.
                        equality = builder_->createEq(lhs, rhs, "strict.any_number");
                    } else if (lhsAny || rhsAny) {
                        equality = builder_->createEq(lhs, rhs, "strict.any_payload");
                    } else if (lhsStringy && rhsStringy &&
                               lhs->type->kind != rhs->type->kind) {
                        // Mixed String/Pointer kinds: route through runtime
                        // string equality instead of constant-folding.
                        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                        auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                        auto existing = module_->getFunction("nova_string_equals");
                        HIRFunction* strEq = existing ? existing.get() : nullptr;
                        if (!strEq) {
                            HIRFunctionType* ft = new HIRFunctionType(
                                {ptrType, ptrType}, intType);
                            HIRFunctionPtr created = module_->createFunction(
                                "nova_string_equals", ft);
                            created->linkage = HIRFunction::Linkage::External;
                            strEq = created.get();
                        }
                        equality = builder_->createCall(
                            strEq, {lhs, rhs}, "strict.string_eq");
                    } else if ((lhsStringy || rhsStringy) &&
                               (lhs->type->kind == HIRType::Kind::I64 ||
                                rhs->type->kind == HIRType::Kind::I64)) {
                        // Array elements / opaque runtime values often arrive as
                        // raw i64 carrying pointer bits. When the other operand
                        // is a string literal (String or Pointer-to-char), reinterpret
                        // the bits as a pointer and do a string comparison.
                        // Restriction: only enter the string-equals path when at
                        // least one operand is a genuine pointer-typed value
                        // (Pointer/Function/Closure/Reference) OR an i64 produced
                        // by a runtime Call (e.g., nova_value_array_at returning
                        // a tagged JS value as raw bits). Otherwise `"1" === 1`
                        // (String literal vs numeric i64 literal) would crash by
                        // reinterpreting the integer 1 as a pointer.
                        const auto isRealPtr = [](HIRType::Kind kind) {
                            return kind == HIRType::Kind::Pointer ||
                                kind == HIRType::Kind::Function ||
                                kind == HIRType::Kind::Closure ||
                                kind == HIRType::Kind::Reference;
                        };
                        const auto isOpaqueBits = [](HIRValue* v) {
                            if (!v) return false;
                            auto* instr = dynamic_cast<HIRInstruction*>(v);
                            if (!instr) return false;
                            return instr->opcode == HIRInstruction::Opcode::Call ||
                                instr->opcode == HIRInstruction::Opcode::Load ||
                                instr->opcode == HIRInstruction::Opcode::GetElement ||
                                instr->opcode == HIRInstruction::Opcode::Phi;
                        };
                        const bool lhsOpaque = isRealPtr(lhs->type->kind) ||
                            (lhs->type->kind == HIRType::Kind::I64 && isOpaqueBits(lhs));
                        const bool rhsOpaque = isRealPtr(rhs->type->kind) ||
                            (rhs->type->kind == HIRType::Kind::I64 && isOpaqueBits(rhs));
                        if (lhsOpaque || rhsOpaque) {
                            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                            auto existing = module_->getFunction("nova_string_equals");
                            HIRFunction* strEq = existing ? existing.get() : nullptr;
                            if (!strEq) {
                                HIRFunctionType* ft = new HIRFunctionType(
                                    {ptrType, ptrType}, intType);
                                HIRFunctionPtr created = module_->createFunction(
                                    "nova_string_equals", ft);
                                created->linkage = HIRFunction::Linkage::External;
                                strEq = created.get();
                            }
                            auto as_ptr = [&](HIRValue* v) -> HIRValue* {
                                if (!v || !v->type) return v;
                                if (v->type->kind == HIRType::Kind::String ||
                                    v->type->kind == HIRType::Kind::Pointer ||
                                    v->type->kind == HIRType::Kind::Function ||
                                    v->type->kind == HIRType::Kind::Closure ||
                                    v->type->kind == HIRType::Kind::Reference) {
                                    return v;
                                }
                                // int-to-ptr reinterpret via Cast (lowered as PtrToInt
                                // in reverse — actually the codegen treats int->ptr as
                                // an opaque bit cast on x64).
                                auto* asPtr = builder_->createCast(v, ptrType.get());
                                return asPtr;
                            };
                            equality = builder_->createCall(
                                strEq, {as_ptr(lhs), as_ptr(rhs)}, "strict.i64_string_eq");
                        } else {
                            // No real pointer involved — String vs numeric i64
                            // is a type mismatch in strict equality.
                            equality = builder_->createBoolConstant(false);
                        }
                    } else if (lhs->type->kind != rhs->type->kind) {
                        // Mixed kinds where one is a raw i64 (pointer bits) and
                        // the other is a pointer-typed value: compare as pointer
                        // identity. This covers `ownKeys[1] !== symbol` where
                        // ownKeys[1] is an i64 carrying a Symbol* and symbol is
                        // a Pointer-typed Symbol reference.
                        const auto isIntOrPtr = [](HIRType::Kind kind) {
                            return kind == HIRType::Kind::I64 ||
                                kind == HIRType::Kind::I32 ||
                                kind == HIRType::Kind::Pointer ||
                                kind == HIRType::Kind::Reference ||
                                kind == HIRType::Kind::Function ||
                                kind == HIRType::Kind::Closure ||
                                kind == HIRType::Kind::Any;
                        };
                        if (isIntOrPtr(lhs->type->kind) && isIntOrPtr(rhs->type->kind)) {
                            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                            auto lhsI = lhs;
                            auto rhsI = rhs;
                            if (lhs->type->kind != HIRType::Kind::I64) {
                                lhsI = builder_->createCast(lhs, intType.get());
                            }
                            if (rhs->type->kind != HIRType::Kind::I64) {
                                rhsI = builder_->createCast(rhs, intType.get());
                            }
                            equality = builder_->createEq(lhsI, rhsI, "strict.ptr_bits");
                        } else {
                            equality = builder_->createBoolConstant(false);
                        }
                    } else {
                        const auto kind = lhs->type->kind;
                        if (kind == HIRType::Kind::String) {
                            // Two String operands: compare by value (content)
                            // rather than by pointer. JS `===` semantics require
                            // "foo" === "foo" to be true even when the runtime
                            // allocates them at distinct addresses.
                            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                            auto existing = module_->getFunction("nova_string_equals");
                            HIRFunction* strEq = existing ? existing.get() : nullptr;
                            if (!strEq) {
                                HIRFunctionType* ft = new HIRFunctionType(
                                    {ptrType, ptrType}, intType);
                                HIRFunctionPtr created = module_->createFunction(
                                    "nova_string_equals", ft);
                                created->linkage = HIRFunction::Linkage::External;
                                strEq = created.get();
                            }
                            equality = builder_->createCall(
                                strEq, {lhs, rhs}, "strict.string_eq");
                        } else if (identityKind(kind)) {
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

        using Op = UnaryExpr::Op;

        // `delete object.property` — route through nova_object_delete when the
        // receiver is a dynamic Object. Returns true (1) on success or when the
        // property didn't exist; the runtime honors configurable=false by
        // returning false in strict mode semantics (we accept silent refusal
        // here since Nova codegen doesn't currently distinguish strict mode).
        if (node.op == Op::Delete) {
            auto* member = dynamic_cast<MemberExpr*>(node.operand.get());
            if (member) {
                std::string objectVariableName;
                if (auto* objIdent = dynamic_cast<Identifier*>(member->object.get())) {
                    objectVariableName = objIdent->name;
                }
                bool dynamicTypedObject = false;
                if (!objectVariableName.empty()) {
                    if (auto* binding =
                            lookupVariable(objectVariableName);
                        binding && binding->type) {
                        HIRType* type = binding->type.get();
                        const bool opaquePointer =
                            type->kind == HIRType::Kind::Pointer &&
                            dynamic_cast<HIRPointerType*>(type) == nullptr;
                        while (type &&
                               type->kind == HIRType::Kind::Pointer) {
                            auto* pointer =
                                dynamic_cast<HIRPointerType*>(type);
                            type = pointer && pointer->pointeeType
                                ? pointer->pointeeType.get()
                                : nullptr;
                        }
                        dynamicTypedObject =
                            opaquePointer ||
                            (type &&
                             (type->kind == HIRType::Kind::Any ||
                              type->kind == HIRType::Kind::JSValue ||
                              type->kind == HIRType::Kind::Unknown));
                    }
                }
                if (!dynamicTypedObject && member->isComputed) {
                    if (auto* keyIdentifier =
                            dynamic_cast<Identifier*>(
                                member->property.get())) {
                        if (auto* keyBinding =
                                lookupVariable(keyIdentifier->name);
                            keyBinding && keyBinding->type) {
                            dynamicTypedObject =
                                keyBinding->type->kind ==
                                    HIRType::Kind::String ||
                                keyBinding->type->kind ==
                                    HIRType::Kind::JSValue ||
                                keyBinding->type->kind ==
                                    HIRType::Kind::Any;
                        }
                    } else if (dynamic_cast<StringLiteral*>(
                                   member->property.get())) {
                        dynamicTypedObject = true;
                    }
                }
                bool intrinsicObject = false;
                std::string intrinsicObjectPath;
                if (auto* directIntrinsic =
                        dynamic_cast<Identifier*>(member->object.get());
                    directIntrinsic &&
                    (directIntrinsic->name == "Date" ||
                     directIntrinsic->name == "RegExp")) {
                    intrinsicObject = true;
                    intrinsicObjectPath = directIntrinsic->name;
                }
                if (auto* intrinsicMember =
                        dynamic_cast<MemberExpr*>(member->object.get())) {
                    auto* base = dynamic_cast<Identifier*>(
                        intrinsicMember->object.get());
                    auto* property = dynamic_cast<Identifier*>(
                        intrinsicMember->property.get());
                    intrinsicObject =
                        !intrinsicMember->isComputed && base && property &&
                        property->name == "prototype" &&
                        (base->name == "Date" ||
                         base->name == "RegExp");
                    if (intrinsicObject) {
                        intrinsicObjectPath =
                            base->name + ".prototype";
                    }
                }
                if (intrinsicObject || dynamicTypedObject ||
                    (!objectVariableName.empty() &&
                     dynamicObjectVars_.count(objectVariableName) > 0)) {
                    std::string propertyName;
                    if (!member->isComputed) {
                    if (auto* propIdent = dynamic_cast<Identifier*>(member->property.get())) {
                        propertyName = propIdent->name;
                    } else if (auto* propStr = dynamic_cast<StringLiteral*>(member->property.get())) {
                        propertyName = propStr->value;
                    } else if (auto* propNum = dynamic_cast<NumberLiteral*>(member->property.get())) {
                        propertyName = std::to_string(propNum->value);
                    }
                    }

                    if (!propertyName.empty() || member->isComputed) {
                        auto pointerType =
                            std::make_shared<HIRPointerType>(
                                std::make_shared<HIRType>(
                                    HIRType::Kind::Any),
                                true);
                        auto stringType = std::make_shared<HIRType>(HIRType::Kind::String);
                        auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                        HIRValue* objectValue = nullptr;
                        if (!intrinsicObjectPath.empty()) {
                            HIRFunction* getter = nullptr;
                            if (auto existing = module_->getFunction(
                                    "nova_intrinsic_object")) {
                                getter = existing.get();
                            } else {
                                auto* type = new HIRFunctionType(
                                    {stringType}, pointerType);
                                auto created = module_->createFunction(
                                    "nova_intrinsic_object", type);
                                created->linkage =
                                    HIRFunction::Linkage::External;
                                getter = created.get();
                            }
                            objectValue = builder_->createCall(
                                getter,
                                {builder_->createStringConstant(
                                    intrinsicObjectPath)},
                                "delete.intrinsic");
                            objectValue->type = pointerType;
                        } else {
                            member->object->accept(*this);
                            objectValue = lastValue_;
                        }

                        if (objectValue && objectValue->type &&
                            objectValue->type->kind == HIRType::Kind::JSValue) {
                            auto jsValueType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
                            auto existingUnbox = module_->getFunction("nova_value_to_object");
                            HIRFunction* unbox = existingUnbox ? existingUnbox.get() : nullptr;
                            if (!unbox) {
                                auto* type = new HIRFunctionType({jsValueType}, pointerType);
                                auto created = module_->createFunction("nova_value_to_object", type);
                                created->linkage = HIRFunction::Linkage::External;
                                unbox = created.get();
                            }
                            objectValue = builder_->createCall(
                                unbox, {objectValue}, "delete.unbox");
                            objectValue->type = pointerType;
                        }

                        HIRValue* keyValue = nullptr;
                        if (!propertyName.empty()) {
                            keyValue =
                                builder_->createStringConstant(propertyName);
                        } else {
                            member->property->accept(*this);
                            keyValue = lastValue_;
                            if (keyValue && keyValue->type &&
                                keyValue->type->kind ==
                                    HIRType::Kind::JSValue) {
                                auto jsValueType =
                                    std::make_shared<HIRType>(
                                        HIRType::Kind::JSValue);
                                HIRFunction* toString = nullptr;
                                if (auto existing =
                                        module_->getFunction(
                                            "nova_value_to_string_ptr")) {
                                    toString = existing.get();
                                } else {
                                    auto* type = new HIRFunctionType(
                                        {jsValueType}, stringType);
                                    auto created = module_->createFunction(
                                        "nova_value_to_string_ptr", type);
                                    created->linkage =
                                        HIRFunction::Linkage::External;
                                    toString = created.get();
                                }
                                keyValue = builder_->createCall(
                                    toString, {keyValue},
                                    "delete.key.string");
                            } else if (keyValue && keyValue->type &&
                                       keyValue->type->kind !=
                                           HIRType::Kind::String) {
                                HIRFunction* toString = nullptr;
                                if (auto existing =
                                        module_->getFunction(
                                            "nova_value_key_to_string")) {
                                    toString = existing.get();
                                } else {
                                    auto* type = new HIRFunctionType(
                                        {intType}, stringType);
                                    auto created = module_->createFunction(
                                        "nova_value_key_to_string", type);
                                    created->linkage =
                                        HIRFunction::Linkage::External;
                                    toString = created.get();
                                }
                                keyValue = builder_->createCall(
                                    toString, {keyValue},
                                    "delete.key.number");
                            }
                        }

                        auto existingDel = module_->getFunction("nova_object_delete");
                        HIRFunction* delFn = existingDel ? existingDel.get() : nullptr;
                        if (!delFn) {
                            auto* type = new HIRFunctionType({pointerType, stringType}, intType);
                            auto created = module_->createFunction("nova_object_delete", type);
                            created->linkage = HIRFunction::Linkage::External;
                            delFn = created.get();
                        }

                        // Spec returns true unconditionally for missing properties in
                        // non-strict mode, and our object_delete returns void, so wrap.
                        lastValue_ = builder_->createIntConstant(1);
                        builder_->createCall(delFn, {
                            objectValue,
                            keyValue,
                        }, "delete.property");
                        return;
                    }
                }
            }

            // Plain `delete identifier` or non-dynamic member: just evaluate the
            // operand for side effects and return true.
            node.operand->accept(*this);
            lastValue_ = builder_->createIntConstant(1);
            return;
        }

        node.operand->accept(*this);
        auto operand = lastValue_;
        const bool operandIsBigInt = lastWasBigInt_;
        const bool operandIsSymbol = lastWasSymbol_;
        lastWasBigInt_ = false;
        lastWasSymbol_ = false;
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
                // Preserve signed-zero semantics for `-0`: when the operand is
                // a literal integer 0, lower as f64 -0.0 so Object.is(0, -0)
                // returns false and 1 / -0 returns -Infinity per spec.
                if (auto* constOperand = dynamic_cast<HIRConstant*>(operand)) {
                    if (constOperand->kind == HIRConstant::Kind::Integer &&
                        std::get<int64_t>(constOperand->value) == 0) {
                        lastValue_ = builder_->createFloatConstant(-0.0);
                        break;
                    }
                }
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

                    // Special case: typeof of `resolvers.resolve` / `resolvers.reject`
                    // where `resolvers` is the result of Promise.withResolvers().
                    // Those properties are callable functions per spec.
                    if (auto* member = dynamic_cast<MemberExpr*>(node.operand.get())) {
                        if (auto* objIdent = dynamic_cast<Identifier*>(member->object.get())) {
                            if (promiseWithResolversVars_.count(objIdent->name) > 0) {
                                if (auto* propIdent = dynamic_cast<Identifier*>(member->property.get())) {
                                    if (propIdent->name == "resolve" || propIdent->name == "reject") {
                                        typeStr = "function";
                                    }
                                }
                            }
                        }
                    }

                    // Special case: typeof of an arrow function or function
                    // expression. In Nova's HIR, these are represented by a
                    // String constant containing the function name (see
                    // visit(ArrowFunctionExpr&) / visit(FunctionExpr&) in
                    // HIRGen_Functions.cpp). Detect that here so
                    // `typeof (() => 1) === "function"` works.
                    if (dynamic_cast<ArrowFunctionExpr*>(node.operand.get()) ||
                        dynamic_cast<FunctionExpr*>(node.operand.get())) {
                        typeStr = "function";
                    }

                    // First, check for literal null/undefined constants — both
                    // have type Kind::Unknown, but typeof differs (null → "object",
                    // undefined → "undefined"). The constant kind distinguishes them.
                    if (typeStr == "unknown") {
                    if (auto* constVal = dynamic_cast<HIRConstant*>(operand)) {
                        if (constVal->kind == HIRConstant::Kind::Null) {
                            typeStr = "object";  // JS quirk: typeof null === "object"
                        } else if (constVal->kind == HIRConstant::Kind::Undefined) {
                            typeStr = "undefined";
                        }
                    }
                    }

                    if (typeStr == "unknown") {
                    if (operand && operand->type &&
                        operand->type->kind ==
                            HIRType::Kind::JSValue) {
                        auto jsValueType =
                            std::make_shared<HIRType>(
                                HIRType::Kind::JSValue);
                        auto stringType =
                            std::make_shared<HIRType>(
                                HIRType::Kind::String);
                        HIRFunction* typeOf = nullptr;
                        if (auto existing =
                                module_->getFunction(
                                    "nova_value_typeof")) {
                            typeOf = existing.get();
                        } else {
                            auto* type = new HIRFunctionType(
                                {jsValueType}, stringType);
                            auto created = module_->createFunction(
                                "nova_value_typeof", type);
                            created->linkage =
                                HIRFunction::Linkage::External;
                            typeOf = created.get();
                        }
                        lastValue_ = builder_->createCall(
                            typeOf, {operand}, "typeof.dynamic");
                        break;
                    }
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
                            case HIRType::Kind::Closure:
                                typeStr = "function";
                                break;
                            case HIRType::Kind::Void:
                            case HIRType::Kind::Unknown:
                                // UndefinedLiteral produces Kind::Unknown in
                                // HIRGen_Literals.cpp; treat it the same as
                                // Void here so `typeof undefined === "undefined"`.
                                typeStr = "undefined";
                                break;
                            case HIRType::Kind::F32:
                            case HIRType::Kind::F64:
                            case HIRType::Kind::U8:
                            case HIRType::Kind::U16:
                            case HIRType::Kind::U32:
                            case HIRType::Kind::U64:
                            case HIRType::Kind::USize:
                                typeStr = "number";
                                break;
                            case HIRType::Kind::JSValue:
                                // For NaN-boxed values, the runtime decides.
                                // Most boxed values are objects, so default to that.
                                typeStr = "object";
                                break;
                            default:
                                typeStr = "unknown";
                                break;
                        }
                    }
                    }  // end if (typeStr == "unknown")

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
        // Supports identifiers AND member access (obj.field, obj[field]).
        auto* identifier = dynamic_cast<Identifier*>(node.argument.get());
        auto* member = dynamic_cast<MemberExpr*>(node.argument.get());

        HIRValue* varAlloca = nullptr;
        HIRValue* objectAlloca = nullptr;
        HIRValue* currentValue = nullptr;
        int generatorSlot = -1;

        if (identifier) {
            auto generatorSlotIt =
                generatorVarSlots_.find(identifier->name);
            if (currentGeneratorPtr_ && generatorLoadLocalFunc_ &&
                generatorSlotIt != generatorVarSlots_.end()) {
                generatorSlot = generatorSlotIt->second;
                auto* generator = builder_->createLoad(
                    currentGeneratorPtr_);
                currentValue = builder_->createCall(
                    generatorLoadLocalFunc_,
                    {generator,
                     builder_->createIntConstant(generatorSlot)},
                    identifier->name + ".generator.update");
                currentValue->type =
                    std::make_shared<HIRType>(HIRType::Kind::I64);
            }
            // Get the variable's current value (with closure support)
            if (generatorSlot < 0) {
                varAlloca = lookupVariable(identifier->name);
            }
            if (generatorSlot < 0 && !varAlloca &&
                currentFunction_ && !lastFunctionName_.empty() &&
                capturedVariables_[lastFunctionName_].count(
                    identifier->name) != 0) {
                varAlloca =
                    getCapturedVariableStorage(identifier->name);
            }
            if (generatorSlot < 0 && !varAlloca) {
                if (NOVA_DEBUG) std::cerr << "ERROR: Undefined variable: " << identifier->name << std::endl;
                return;
            }
            if (varAlloca) {
                currentValue = builder_->createLoad(varAlloca);
            }
        } else if (member) {
            // For member access: evaluate the object, then load the field.
            // We support `obj.field` (static field index) and `obj[field]`
            // (dynamic index). For now, support both by visiting the
            // member expression: that yields a value, but we need a
            // writable reference. The simplest path is to use assignment
            // semantics — load current value via normal member access,
            // compute new value, then write back via SetField.
            node.argument->accept(*this);
            currentValue = lastValue_;
            // Visit the object once more for the write-back target.
            member->object->accept(*this);
            objectAlloca = lastValue_;
        } else {
            if (NOVA_DEBUG) std::cerr << "ERROR: UpdateExpr argument must be an identifier or member expression" << std::endl;
            return;
        }

        // Stash identifier name (empty for member) for bigint lookup below.
        const std::string identName = identifier ? identifier->name : std::string();

        if (identifier && bigIntVars_.count(identifier->name) > 0) {
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

        // Store new value back to variable (or member field).
        if (generatorSlot >= 0 && generatorStoreLocalFunc_) {
            auto* generator = builder_->createLoad(currentGeneratorPtr_);
            builder_->createCall(
                generatorStoreLocalFunc_,
                {generator, builder_->createIntConstant(generatorSlot),
                 newValue});
        } else if (varAlloca) {
            builder_->createStore(newValue, varAlloca);
        } else if (member && objectAlloca) {
            // For member access, look up the field index in the current
            // class struct (handles `this.field` and `obj.field` when obj
            // is the current `this`).
            auto* memberIdent = dynamic_cast<Identifier*>(member->property.get());
            if (memberIdent && !member->isComputed && currentClassStructType_) {
                int fieldIdx = -1;
                for (size_t i = 0; i < currentClassStructType_->fields.size(); ++i) {
                    if (currentClassStructType_->fields[i].name == memberIdent->name) {
                        fieldIdx = (int)i;
                        break;
                    }
                }
                if (fieldIdx >= 0) {
                    builder_->createSetField(objectAlloca, (size_t)fieldIdx, newValue);
                }
            }
        }

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

        // Step 2: Determine result type by peeking at both branches. A
        // JavaScript conditional may produce heterogeneous values; selecting
        // the consequent's type alone truncates strings/objects into booleans
        // (for example `cond ? number : "unlikelyValue"`).
        // Save current insertion point
        auto* savedBlock = builder_->getInsertBlock();

        // Create temporary block for type inference
        auto* typeInferBlock = currentFunction_->createBasicBlock("ternary.typeinfer").get();
        builder_->setInsertPoint(typeInferBlock);

        // Evaluate consequent to determine type
        node.consequent->accept(*this);
        HIRType* consequentType =
            lastValue_ && lastValue_->type
            ? lastValue_->type.get()
            : nullptr;
        node.alternate->accept(*this);
        HIRType* alternateType =
            lastValue_ && lastValue_->type
            ? lastValue_->type.get()
            : nullptr;
        const bool heterogeneous =
            consequentType && alternateType &&
            consequentType->kind != alternateType->kind;
        HIRType* resultType = heterogeneous
            ? new HIRType(HIRType::Kind::JSValue)
            : (consequentType
                ? consequentType
                : (alternateType
                    ? alternateType
                    : new HIRType(HIRType::Kind::I64)));

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
        builder_->createStore(
            heterogeneous
                ? toJSValue(consequentValue)
                : consequentValue,
            resultAlloca);
        builder_->createBr(endBlock);

        // Step 7: Generate ELSE block - evaluate alternate HERE (not before!)
        builder_->setInsertPoint(elseBlock);
        node.alternate->accept(*this);  // Evaluate INSIDE else block
        auto alternateValue = lastValue_;
        builder_->createStore(
            heterogeneous
                ? toJSValue(alternateValue)
                : alternateValue,
            resultAlloca);
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
        bool assignmentIsDate = false;

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
            lastWasDate_ = false;
            node.right->accept(*this);
            auto rightValue = lastValue_;
            const bool rightIsBigInt = lastWasBigInt_;
            assignmentIsDate = lastWasDate_;
            lastWasBigInt_ = false;
            lastWasDate_ = false;

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
            auto generatorSlotIt = generatorVarSlots_.find(id->name);
            if (currentGeneratorPtr_ && generatorStoreLocalFunc_ &&
                generatorSlotIt != generatorVarSlots_.end()) {
                auto* generator = builder_->createLoad(
                    currentGeneratorPtr_);
                builder_->createCall(
                    generatorStoreLocalFunc_,
                    {generator,
                     builder_->createIntConstant(generatorSlotIt->second),
                     value});
                lastValue_ = value;
                return;
            }
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
                if (assignmentIsDate &&
                    node.op == AssignmentExpr::Op::Assign) {
                    // `var date; date = new Date(...)` must retain the raw
                    // Date pointer ABI. Widening the undefined placeholder to
                    // JSValue makes subsequent Date method calls treat the
                    // native Date storage as a generic Object and crash.
                    if (auto* pointerType = target->type
                        ? dynamic_cast<HIRPointerType*>(
                              target->type.get())
                        : nullptr) {
                        pointerType->pointeeType = value->type;
                    }
                    dateVars_.insert(id->name);
                } else if (!targetIsBigInt) {
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
            if (node.op == AssignmentExpr::Op::Assign &&
                !assignmentIsDate) {
                dateVars_.erase(id->name);
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
            bool explicitlyDynamicObject = false;
            Expr* assignmentObject = memberExpr->object.get();
            if (auto* assertion = dynamic_cast<AsExpr*>(assignmentObject)) {
                explicitlyDynamicObject = assertion->targetType &&
                    assertion->targetType->kind == Type::Kind::Any;
                assignmentObject = assertion->expression.get();
            }
            if (auto* objectIdentifier =
                    dynamic_cast<Identifier*>(assignmentObject)) {
                objectVariableName = objectIdentifier->name;
                if (auto* binding =
                        lookupVariable(objectVariableName);
                    binding && binding->type) {
                    HIRType* type = binding->type.get();
                    const bool opaquePointer =
                        type->kind == HIRType::Kind::Pointer &&
                        dynamic_cast<HIRPointerType*>(type) == nullptr;
                    while (type &&
                           type->kind == HIRType::Kind::Pointer) {
                        auto* pointer =
                            dynamic_cast<HIRPointerType*>(type);
                        type = pointer && pointer->pointeeType
                            ? pointer->pointeeType.get()
                            : nullptr;
                    }
                    explicitlyDynamicObject =
                        explicitlyDynamicObject ||
                        opaquePointer ||
                        (type &&
                         (type->kind == HIRType::Kind::Any ||
                          type->kind == HIRType::Kind::JSValue ||
                          type->kind == HIRType::Kind::Unknown));
                }
            }
            if (!explicitlyDynamicObject && memberExpr->isComputed) {
                if (auto* keyIdentifier =
                        dynamic_cast<Identifier*>(
                            memberExpr->property.get())) {
                    if (auto* keyBinding =
                            lookupVariable(keyIdentifier->name);
                        keyBinding && keyBinding->type) {
                        explicitlyDynamicObject =
                            keyBinding->type->kind ==
                                HIRType::Kind::String ||
                            keyBinding->type->kind ==
                                HIRType::Kind::JSValue ||
                            keyBinding->type->kind ==
                                HIRType::Kind::Any;
                    }
                } else if (dynamic_cast<StringLiteral*>(
                               memberExpr->property.get())) {
                    explicitlyDynamicObject = true;
                }
            }

            if (!memberExpr->isComputed &&
                regexVars_.count(objectVariableName) > 0) {
                auto* property = dynamic_cast<Identifier*>(
                    memberExpr->property.get());
                if (property && property->name == "lastIndex") {
                    memberExpr->object->accept(*this);
                    HIRValue* regex = lastValue_;
                    auto pointerType = std::make_shared<HIRType>(
                        HIRType::Kind::Pointer);
                    auto integerType = std::make_shared<HIRType>(
                        HIRType::Kind::I64);
                    auto voidType = std::make_shared<HIRType>(
                        HIRType::Kind::Void);
                    HIRValue* index = value;
                    if (index && index->type &&
                        index->type->kind != HIRType::Kind::I64) {
                        index = builder_->createCast(
                            index, integerType.get(),
                            "regex.lastIndex.value");
                    }
                    HIRFunction* setter = nullptr;
                    if (auto existing = module_->getFunction(
                            "nova_regex_set_lastIndex")) {
                        setter = existing.get();
                    } else {
                        auto* type = new HIRFunctionType(
                            {pointerType, integerType}, voidType);
                        auto created = module_->createFunction(
                            "nova_regex_set_lastIndex", type);
                        created->linkage =
                            HIRFunction::Linkage::External;
                        setter = created.get();
                    }
                    builder_->createCall(
                        setter, {regex, index},
                        "regex.lastIndex.set");
                    lastValue_ = value;
                    return;
                }
            }

            // Dynamic-object property write: if the base variable is in
            // dynamicObjectVars_ (forced-dynamic, Object.create result, etc.),
            // route the assignment through nova_dynamic_object_set_tagged.
            if (explicitlyDynamicObject ||
                (!objectVariableName.empty() &&
                 dynamicObjectVars_.count(objectVariableName) > 0)) {
                std::string propertyName;
                if (!memberExpr->isComputed) {
                    if (auto* propIdent = dynamic_cast<Identifier*>(memberExpr->property.get())) {
                        propertyName = propIdent->name;
                    } else if (auto* propStr = dynamic_cast<StringLiteral*>(memberExpr->property.get())) {
                        propertyName = propStr->value;
                    } else if (auto* propNum = dynamic_cast<NumberLiteral*>(memberExpr->property.get())) {
                        propertyName = std::to_string(propNum->value);
                    }
                }

                // For dynamic objects, even computed keys should route through
                // the property map rather than being treated as array index
                // writes. We materialize the key as a string at runtime when we
                // can't resolve it statically.
                if (!propertyName.empty() || memberExpr->isComputed) {
                    memberExpr->object->accept(*this);
                    HIRValue* objectValue = lastValue_;
                    auto pointerType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                    auto stringType = std::make_shared<HIRType>(HIRType::Kind::String);
                    auto jsValueType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
                    auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);

                    if (objectValue && objectValue->type &&
                        objectValue->type->kind == HIRType::Kind::JSValue) {
                        auto existingUnbox = module_->getFunction("nova_value_to_object");
                        HIRFunction* unbox = existingUnbox ? existingUnbox.get() : nullptr;
                        if (!unbox) {
                            auto* type = new HIRFunctionType({jsValueType}, pointerType);
                            auto created = module_->createFunction("nova_value_to_object", type);
                            created->linkage = HIRFunction::Linkage::External;
                            unbox = created.get();
                        }
                        objectValue = builder_->createCall(
                            unbox, {objectValue}, "dynamic.assign.unbox");
                        objectValue->type = pointerType;
                    }

                    auto existingSet = module_->getFunction("nova_dynamic_object_set_tagged");
                    HIRFunction* setFn = existingSet ? existingSet.get() : nullptr;
                    if (!setFn) {
                        auto* type = new HIRFunctionType(
                            {pointerType, stringType, jsValueType}, voidType);
                        auto created = module_->createFunction(
                            "nova_dynamic_object_set_tagged", type);
                        created->linkage = HIRFunction::Linkage::External;
                        setFn = created.get();
                    }

                    HIRValue* keyArg = nullptr;
                    if (!propertyName.empty()) {
                        keyArg = builder_->createStringConstant(propertyName);
                    } else {
                        // Computed key: evaluate the expression and convert
                        // to a string key at runtime. Symbol-typed keys use a
                        // dedicated runtime entry point so identity is
                        // preserved across ownKeys / === comparisons.
                        lastWasSymbol_ = false;
                        memberExpr->property->accept(*this);
                        HIRValue* computedKey = lastValue_;
                        const bool computedKeyIsSymbol = lastWasSymbol_;
                        lastWasSymbol_ = false;

                        if (computedKeyIsSymbol && computedKey &&
                            computedKey->type &&
                            computedKey->type->kind == HIRType::Kind::Pointer) {
                            // Symbol-keyed property write: bypass the string
                            // property map entirely so identity round-trips.
                            HIRValue* boxedVal = toJSValue(value);

                            auto existingSymSet =
                                module_->getFunction("nova_object_set_symbol");
                            HIRFunction* symSetFn =
                                existingSymSet ? existingSymSet.get() : nullptr;
                            if (!symSetFn) {
                                auto* type = new HIRFunctionType(
                                    {pointerType, pointerType, jsValueType}, voidType);
                                auto created = module_->createFunction(
                                    "nova_object_set_symbol", type);
                                created->linkage = HIRFunction::Linkage::External;
                                symSetFn = created.get();
                            }
                            builder_->createCall(
                                symSetFn,
                                {objectValue, computedKey, boxedVal},
                                "dynamic.symbol_assign");
                            lastValue_ = value;
                            return;
                        }

                        if (computedKey && computedKey->type &&
                            (computedKey->type->kind == HIRType::Kind::Pointer ||
                             computedKey->type->kind == HIRType::Kind::String)) {
                            // Symbol identity was handled above using the
                            // visitor's semantic symbol flag. A remaining
                            // pointer here is a C string property key. Do not
                            // reinterpret its first bytes as NovaSymbol::id:
                            // ordinary names such as "getYear" can
                            // accidentally satisfy that heuristic.
                            keyArg = computedKey;
                            keyArg->type = stringType;
                        } else if (computedKey && computedKey->type &&
                                   computedKey->type->kind == HIRType::Kind::JSValue) {
                            auto existingPtr = module_->getFunction("nova_value_to_string_ptr");
                            HIRFunction* strFn = existingPtr ? existingPtr.get() : nullptr;
                            if (!strFn) {
                                auto* type = new HIRFunctionType({jsValueType}, stringType);
                                auto created = module_->createFunction("nova_value_to_string_ptr", type);
                                created->linkage = HIRFunction::Linkage::External;
                                strFn = created.get();
                            }
                            keyArg = builder_->createCall(strFn, {computedKey}, "dynamic.jsvalue_key");
                            keyArg->type = stringType;
                        } else {
                            // Fallback: convert to a generic key string. Pointer
                            // values that aren't Symbols (e.g. function refs)
                            // land here.
                            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                            auto existingKey = module_->getFunction("nova_value_key_to_string");
                            HIRFunction* keyFn = existingKey ? existingKey.get() : nullptr;
                            if (!keyFn) {
                                auto* type = new HIRFunctionType({intType}, stringType);
                                auto created = module_->createFunction("nova_value_key_to_string", type);
                                created->linkage = HIRFunction::Linkage::External;
                                keyFn = created.get();
                            }
                            keyArg = builder_->createCall(keyFn, {computedKey}, "dynamic.computed_key");
                            keyArg->type = stringType;
                        }
                    }

                    HIRValue* boxed = toJSValue(value);
                    builder_->createCall(setFn, {
                        objectValue,
                        keyArg,
                        boxed,
                    }, "dynamic.assign");
                    lastValue_ = value;
                    return;
                }
            }

            // Static property write: ClassName.field = value.
            // Route through the runtime mutable store so changes persist.
            if (!memberExpr->isComputed && objectVariableName != "") {
                auto classIt = classStaticProps_.find(objectVariableName);
                if (classIt != classStaticProps_.end()) {
                    if (auto* propIdent = dynamic_cast<Identifier*>(memberExpr->property.get())) {
                        if (classIt->second.find(propIdent->name) != classIt->second.end()) {
                            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                            std::string setter = "nova_class_static_set_i64";
                            HIRFunction* setFunc = nullptr;
                            auto existingSet = module_->getFunction(setter);
                            if (existingSet) setFunc = existingSet.get();
                            else {
                                HIRFunctionType* sft = new HIRFunctionType({ptrType, ptrType, intType}, intType);
                                HIRFunctionPtr sfp = module_->createFunction(setter, sft);
                                sfp->linkage = HIRFunction::Linkage::External;
                                setFunc = sfp.get();
                            }
                            // Coerce value to i64 if needed (matches the i64 slot type).
                            HIRValue* i64Value = value;
                            if (value && value->type && value->type->kind != HIRType::Kind::I64) {
                                i64Value = builder_->createCast(value, intType.get(), "static_assign_cast");
                            }
                            HIRValue* classNameArg = builder_->createStringConstant(objectVariableName);
                            HIRValue* fieldNameArg = builder_->createStringConstant(propIdent->name);
                            builder_->createCall(setFunc, {classNameArg, fieldNameArg, i64Value}, "static_set");
                            lastValue_ = value;
                            return;
                        }
                    }
                }
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
                    if (NOVA_DEBUG) std::cerr << "  DEBUG: Using currentClassStructType_ for 'this' property assignment" << std::endl;
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
                            if (NOVA_DEBUG) std::cerr << "  DEBUG: Found field '" << propertyName << "' at index " << fieldIndex << std::endl;
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
                    // Convert F64/F32 to I64/I32 if field expects integer but value is float
                    if (structType && structType->fields[fieldIndex].type && value && value->type) {
                        auto fieldKind = structType->fields[fieldIndex].type->kind;
                        auto valueKind = value->type->kind;
                        if ((fieldKind == HIRType::Kind::I64 || fieldKind == HIRType::Kind::I32) &&
                            (valueKind == HIRType::Kind::F64 || valueKind == HIRType::Kind::F32)) {
                            auto targetTy = structType->fields[fieldIndex].type;
                            storedValue = builder_->createCast(value, targetTy.get(), "fltoint");
                        }
                    }
                    // Use SetField to store value directly to the field.
                    builder_->createSetField(object, fieldIndex, storedValue, propertyName);
                } else {
                    std::cerr << "Warning: Property '" << propertyName << "' not found for assignment" << std::endl;
                }
            }
        }
    }
    

} // namespace nova::hir
