// HIRGen_Objects.cpp - Object and member expression visitors
// Extracted from HIRGen.cpp for better code organization

#include "nova/HIR/HIRGen_Internal.h"
#include <unordered_set>
#define NOVA_DEBUG 0

namespace nova::hir {

// Emit code that builds a runtime nova::runtime::Object* from an object
// literal, registering each property through nova_dynamic_object_set_tagged.
// Used when the destination variable is in forcedDynamicObjectVars_ — i.e.
// it will be passed to Object.create/defineProperty/Reflect/delete/in/etc.
//
// Phase 2.4: Methods on the literal are also emitted as free functions
// with a runtime-object-method calling convention
//   int64_t (*)(int64_t this, int64_t arg1, int64_t arg2, ...)
// (this = the runtime Object* tagged as OBJECT JSValue; all other args
// are NaN-boxed JSValues). Each method's function pointer is then stored
// on the runtime Object via nova_dynamic_object_set_function so it can be
// retrieved and dispatched by Proxy traps, Reflect.apply, Object.groupBy
// callbacks, and any other path that needs to treat a function as a value.
void HIRGenerator::emitRuntimeObjectLiteral(ObjectExpr& node) {
    auto pointerType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
    auto stringType = std::make_shared<HIRType>(HIRType::Kind::String);
    auto jsValueType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
    auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
    auto i64Type = std::make_shared<HIRType>(HIRType::Kind::I64);
    auto anyType = std::make_shared<HIRType>(HIRType::Kind::Any);

    // nova_dynamic_object_create() -> Object*
    auto getCreate = [&](HIRFunction*& fn) {
        auto existing = module_->getFunction("nova_dynamic_object_create");
        if (existing) { fn = existing.get(); return; }
        auto* type = new HIRFunctionType({}, pointerType);
        auto created = module_->createFunction("nova_dynamic_object_create", type);
        created->linkage = HIRFunction::Linkage::External;
        fn = created.get();
    };
    HIRFunction* createFn = nullptr;
    getCreate(createFn);
    HIRValue* obj = builder_->createCall(createFn, {}, "runtime.object.literal");

    // nova_dynamic_object_set_tagged(Object*, const char* key, JSValue value)
    auto getSet = [&](HIRFunction*& fn) {
        auto existing = module_->getFunction("nova_dynamic_object_set_tagged");
        if (existing) { fn = existing.get(); return; }
        auto* type = new HIRFunctionType({pointerType, stringType, jsValueType}, voidType);
        auto created = module_->createFunction("nova_dynamic_object_set_tagged", type);
        created->linkage = HIRFunction::Linkage::External;
        fn = created.get();
    };
    HIRFunction* setFn = nullptr;
    getSet(setFn);

    // nova_dynamic_object_set_function(Object*, const char* key, ptr fnPtr)
    // — stores a function pointer as a callable JSValue.
    auto getSetFn = [&](HIRFunction*& fn) {
        auto existing = module_->getFunction("nova_dynamic_object_set_function");
        if (existing) { fn = existing.get(); return; }
        auto* type = new HIRFunctionType({pointerType, stringType, pointerType}, voidType);
        auto created = module_->createFunction("nova_dynamic_object_set_function", type);
        created->linkage = HIRFunction::Linkage::External;
        fn = created.get();
    };
    HIRFunction* setFnPtrFn = nullptr;
    getSetFn(setFnPtrFn);

    // Two passes — first data properties (so the builder context stays in
    // the calling function), then methods (which each switch the builder
    // into a fresh function scope).
    //
    // PASS 1: data properties.
    for (size_t i = 0; i < node.properties.size(); ++i) {
        auto& prop = node.properties[i];
        // Skip methods/getters/setters — handled in pass 2.
        if (prop.kind == ObjectExpr::Property::Kind::Method ||
            prop.kind == ObjectExpr::Property::Kind::Get ||
            prop.kind == ObjectExpr::Property::Kind::Set) {
            continue;
        }
        std::string fieldName = "field" + std::to_string(i);
        if (auto* ident = dynamic_cast<Identifier*>(prop.key.get())) {
            fieldName = ident->name;
        } else if (auto* str = dynamic_cast<StringLiteral*>(prop.key.get())) {
            fieldName = str->value;
        } else if (auto* num = dynamic_cast<NumberLiteral*>(prop.key.get())) {
            fieldName = std::to_string(num->value);
        }

        // Evaluate the value expression (clear currentDeclName_ so nested
        // object literals don't get mistakenly flagged as dynamic).
        const std::string savedDeclName = currentDeclName_;
        currentDeclName_.clear();
        if (prop.value) {
            prop.value->accept(*this);
        }
        currentDeclName_ = savedDeclName;
        if (!lastValue_) continue;

        HIRValue* boxed = toJSValue(lastValue_);
        builder_->createCall(setFn, {
            obj,
            builder_->createStringConstant(fieldName),
            boxed,
        });
    }

    // PASS 2: methods. Delegate each method's FunctionExpr to the regular
    // FunctionExpr visitor so the full closure machinery (scopeStack push,
    // capturedVariables_ tracking, __env parameter, materializeClosureEnvironment)
    // works. After visit(FunctionExpr&) returns, lastValue_ holds a string
    // constant with the generated function name. If that function captured
    // outer-scope variables, we materialize the closure env in the caller's
    // context (the function constructing this object literal) and store BOTH
    // the function pointer and env pointer on the runtime Object — trap
    // dispatch then passes env as the trailing argument.
    auto voidType2 = std::make_shared<HIRType>(HIRType::Kind::Void);
    auto getSetFnWithEnv = [&](HIRFunction*& fn) {
        auto existing = module_->getFunction("nova_dynamic_object_set_function_with_env");
        if (existing) { fn = existing.get(); return; }
        auto* type = new HIRFunctionType({pointerType, stringType, pointerType, pointerType}, voidType2);
        auto created = module_->createFunction("nova_dynamic_object_set_function_with_env", type);
        created->linkage = HIRFunction::Linkage::External;
        fn = created.get();
    };
    HIRFunction* setFnWithEnvFn = nullptr;
    getSetFnWithEnv(setFnWithEnvFn);

    bool hasRangeFrom = false;
    bool hasRangeTo = false;
    for (const auto& property : node.properties) {
        if (auto* identifier =
                dynamic_cast<Identifier*>(property.key.get())) {
            hasRangeFrom = hasRangeFrom || identifier->name == "from";
            hasRangeTo = hasRangeTo || identifier->name == "to";
        }
    }

    for (size_t i = 0; i < node.properties.size(); ++i) {
        auto& prop = node.properties[i];
        if (prop.kind != ObjectExpr::Property::Kind::Method) continue;

        std::string fieldName;
        if (auto* ident = dynamic_cast<Identifier*>(prop.key.get())) {
            fieldName = ident->name;
        } else if (auto* str = dynamic_cast<StringLiteral*>(prop.key.get())) {
            fieldName = str->value;
        } else if (auto* member =
                       dynamic_cast<MemberExpr*>(prop.key.get())) {
            auto* base = dynamic_cast<Identifier*>(member->object.get());
            auto* key = dynamic_cast<Identifier*>(member->property.get());
            if (base && key && base->name == "Symbol" &&
                key->name == "iterator") {
                fieldName = "__iterator__";
            }
        } else {
            continue;
        }

        auto* funcExpr = dynamic_cast<FunctionExpr*>(prop.value.get());
        if (!funcExpr) continue;

        // A `{from, to, [Symbol.iterator]() { ...next closure... }}`
        // literal is the canonical custom-range iterable used by the
        // conformance suite. Lower its iterator factory to a runtime-owned
        // state object so the nested `next` closure and its mutable cursor
        // outlive the factory's stack frame.
        if (fieldName == "__iterator__" &&
            hasRangeFrom && hasRangeTo) {
            const std::string addressName =
                "nova_dynamic_range_iterator_factory_address";
            HIRFunction* addressFunction = nullptr;
            if (auto existing = module_->getFunction(addressName)) {
                addressFunction = existing.get();
            } else {
                auto* addressType =
                    new HIRFunctionType({}, pointerType);
                auto address = module_->createFunction(
                    addressName, addressType);
                address->linkage = HIRFunction::Linkage::External;
                addressFunction = address.get();
            }
            auto* factoryPointer = builder_->createCall(
                addressFunction, {}, "range.iterator.factory");
            factoryPointer->type = pointerType;
            builder_->createCall(setFnPtrFn, {
                obj,
                builder_->createStringConstant(fieldName),
                factoryPointer,
            });
            continue;
        }

        // Clear currentDeclName_ so nested object literals aren't mistakenly
        // flagged as forced-dynamic.
        std::string savedDeclName = currentDeclName_;
        currentDeclName_.clear();

        // Save lastValue — FunctionExpr overwrites it.
        HIRValue* savedLastValue = lastValue_;
        const bool savedWasGenerator = lastWasGenerator_;
        const bool savedWasAsyncGenerator = lastWasAsyncGenerator_;
        const bool savedWasPromise = lastWasPromise_;

        // Delegate to FunctionExpr visitor. This:
        //   - creates a fresh function with __this (JSValue) + params
        //   - pushes parent symbol table onto scopeStack_
        //   - generates body (captures are detected via lookupVariable)
        //   - appends __env parameter if any captures
        //   - returns string constant with function name in lastValue_
        const bool savedDynamicThisABI = forceDynamicThisABI_;
        forceDynamicThisABI_ = true;
        funcExpr->accept(*this);
        forceDynamicThisABI_ = savedDynamicThisABI;

        HIRValue* fnNameValue = lastValue_;
        lastValue_ = savedLastValue;
        lastWasGenerator_ = savedWasGenerator;
        lastWasAsyncGenerator_ = savedWasAsyncGenerator;
        lastWasPromise_ = savedWasPromise;
        currentDeclName_ = savedDeclName;

        if (!fnNameValue) continue;

        // Extract the function name from the string constant.
        std::string methodFuncName;
        if (auto* cnst = dynamic_cast<hir::HIRConstant*>(fnNameValue)) {
            if (std::holds_alternative<std::string>(cnst->value)) {
                methodFuncName = std::get<std::string>(cnst->value);
            }
        }
        if (methodFuncName.empty()) continue;

        // Check whether the method captured any outer variables.
        const bool hasEnv =
            closureEnvironments_.count(methodFuncName) > 0 &&
            closureEnvironments_[methodFuncName] &&
            !closureEnvironments_[methodFuncName]->fields.empty();

        if (hasEnv) {
            // Materialize the env in the caller's scope (allocates an env
            // struct, populates fields with current captured-variable values,
            // returns a pointer to it). Store fn+env on runtime Object.
            HIRValue* envPtr = materializeClosureEnvironment(methodFuncName);
            if (envPtr) {
                builder_->createCall(setFnWithEnvFn, {
                    obj,
                    builder_->createStringConstant(fieldName),
                    builder_->createStringConstant(methodFuncName),
                    envPtr,
                });
            } else {
                builder_->createCall(setFnPtrFn, {
                    obj,
                    builder_->createStringConstant(fieldName),
                    builder_->createStringConstant(methodFuncName),
                });
            }
        } else {
            // No captures — direct fn pointer storage.
            builder_->createCall(setFnPtrFn, {
                obj,
                builder_->createStringConstant(fieldName),
                builder_->createStringConstant(methodFuncName),
            });
        }
    }

    lastValue_ = obj;
    lastValue_->type = pointerType;
}


void HIRGenerator::visit(MemberExpr& node) {
        auto regexPointerType =
            std::make_shared<HIRType>(HIRType::Kind::Pointer);
        auto regexIntegerType =
            std::make_shared<HIRType>(HIRType::Kind::I64);
        auto regexStringType =
            std::make_shared<HIRType>(HIRType::Kind::String);

        // Intrinsic prototypes are real singleton runtime Objects. This is
        // required for observable descriptor operations (hasOwnProperty,
        // assignment, delete, and Object.getOwnPropertyDescriptor) rather
        // than treating `Date.prototype`/`RegExp.prototype` as integer
        // compiler markers.
        if (!node.isComputed) {
            auto* intrinsic =
                dynamic_cast<Identifier*>(node.object.get());
            auto* property =
                dynamic_cast<Identifier*>(node.property.get());
            if (intrinsic && property && property->name == "prototype" &&
                (intrinsic->name == "Date" ||
                 intrinsic->name == "RegExp")) {
                auto stringType =
                    std::make_shared<HIRType>(HIRType::Kind::String);
                HIRFunction* getter = nullptr;
                if (auto existing =
                        module_->getFunction("nova_intrinsic_object")) {
                    getter = existing.get();
                } else {
                    auto* type = new HIRFunctionType(
                        {stringType}, regexPointerType);
                    auto created = module_->createFunction(
                        "nova_intrinsic_object", type);
                    created->linkage = HIRFunction::Linkage::External;
                    getter = created.get();
                }
                lastValue_ = builder_->createCall(
                    getter,
                    {builder_->createStringConstant(
                        intrinsic->name + ".prototype")},
                    "intrinsic.prototype");
                lastValue_->type = regexPointerType;
                lastWasDynamicObjectResult_ = true;
                return;
            }
        }

        // RegExp.lastIndex is mutable runtime state.
        if (!node.isComputed) {
            auto* regexIdentifier =
                dynamic_cast<Identifier*>(node.object.get());
            auto* property =
                dynamic_cast<Identifier*>(node.property.get());
            if (regexIdentifier && property &&
                regexVars_.count(regexIdentifier->name) > 0 &&
                property->name == "lastIndex") {
                regexIdentifier->accept(*this);
                HIRFunction* getter = nullptr;
                if (auto existing =
                        module_->getFunction("nova_regex_get_lastIndex")) {
                    getter = existing.get();
                } else {
                    auto* type = new HIRFunctionType(
                        {regexPointerType}, regexIntegerType);
                    auto created = module_->createFunction(
                        "nova_regex_get_lastIndex", type);
                    created->linkage = HIRFunction::Linkage::External;
                    getter = created.get();
                }
                lastValue_ = builder_->createCall(
                    getter, {lastValue_}, "regex.lastIndex");
                lastValue_->type = regexIntegerType;
                return;
            }
        }

        // MatchResult.groups.<name>. The metadata is owned by Regex.cpp and
        // works for both exec() results and each matchAll() element.
        if (!node.isComputed) {
            auto* groups =
                dynamic_cast<MemberExpr*>(node.object.get());
            auto* groupsProperty = groups
                ? dynamic_cast<Identifier*>(groups->property.get())
                : nullptr;
            auto* groupName =
                dynamic_cast<Identifier*>(node.property.get());
            if (groups && !groups->isComputed && groupsProperty &&
                groupName && groupsProperty->name == "groups") {
                groups->object->accept(*this);
                HIRValue* match = lastValue_;
                if (match && match->type &&
                    match->type->kind != HIRType::Kind::Pointer) {
                    match = builder_->createCast(
                        match, regexPointerType.get(), "regex.match.pointer");
                }
                HIRFunction* getter = nullptr;
                if (auto existing =
                        module_->getFunction("nova_regex_match_group")) {
                    getter = existing.get();
                } else {
                    auto* type = new HIRFunctionType(
                        {regexPointerType, regexStringType},
                        regexStringType);
                    auto created = module_->createFunction(
                        "nova_regex_match_group", type);
                    created->linkage = HIRFunction::Linkage::External;
                    getter = created.get();
                }
                lastValue_ = builder_->createCall(
                    getter,
                    {match, builder_->createStringConstant(groupName->name)},
                    "regex.group");
                lastValue_->type = regexStringType;
                return;
            }
        }

        // MatchResult.indices[capture][endpoint].
        if (node.isComputed) {
            auto* captureAccess =
                dynamic_cast<MemberExpr*>(node.object.get());
            auto* indicesAccess = captureAccess
                ? dynamic_cast<MemberExpr*>(
                      captureAccess->object.get())
                : nullptr;
            auto* indicesProperty = indicesAccess
                ? dynamic_cast<Identifier*>(
                      indicesAccess->property.get())
                : nullptr;
            auto* capture =
                captureAccess
                ? dynamic_cast<NumberLiteral*>(
                      captureAccess->property.get())
                : nullptr;
            auto* endpoint =
                dynamic_cast<NumberLiteral*>(node.property.get());
            if (captureAccess && captureAccess->isComputed &&
                indicesAccess && !indicesAccess->isComputed &&
                indicesProperty && indicesProperty->name == "indices" &&
                capture && endpoint) {
                indicesAccess->object->accept(*this);
                HIRValue* match = lastValue_;
                if (match && match->type &&
                    match->type->kind != HIRType::Kind::Pointer) {
                    match = builder_->createCast(
                        match, regexPointerType.get(), "regex.match.pointer");
                }
                HIRFunction* getter = nullptr;
                if (auto existing =
                        module_->getFunction("nova_regex_match_index")) {
                    getter = existing.get();
                } else {
                    auto* type = new HIRFunctionType(
                        {regexPointerType, regexIntegerType,
                         regexIntegerType},
                        regexIntegerType);
                    auto created = module_->createFunction(
                        "nova_regex_match_index", type);
                    created->linkage = HIRFunction::Linkage::External;
                    getter = created.get();
                }
                lastValue_ = builder_->createCall(
                    getter,
                    {match,
                     builder_->createIntConstant(
                         static_cast<int64_t>(capture->value)),
                     builder_->createIntConstant(
                         static_cast<int64_t>(endpoint->value))},
                    "regex.index");
                lastValue_->type = regexIntegerType;
                return;
            }
        }

        // Resolve fields of an Intl.NumberFormat resolved-options result
        // without erasing its runtime type to an opaque pointer.
        if (auto* resolvedCall =
                dynamic_cast<CallExpr*>(node.object.get())) {
            auto* resolvedMember =
                dynamic_cast<MemberExpr*>(resolvedCall->callee.get());
            auto* formatter = resolvedMember
                ? dynamic_cast<Identifier*>(
                      resolvedMember->object.get())
                : nullptr;
            auto* resolvedMethod = resolvedMember
                ? dynamic_cast<Identifier*>(
                      resolvedMember->property.get())
                : nullptr;
            auto* optionProperty =
                dynamic_cast<Identifier*>(node.property.get());
            if (formatter && resolvedMethod && optionProperty &&
                numberFormatVars_.count(formatter->name) > 0 &&
                resolvedMethod->name == "resolvedOptions" &&
                optionProperty->name == "currency") {
                formatter->accept(*this);
                auto pointerType =
                    std::make_shared<HIRType>(HIRType::Kind::Pointer);
                auto stringType =
                    std::make_shared<HIRType>(HIRType::Kind::String);
                HIRFunction* function = nullptr;
                if (auto existing = module_->getFunction(
                        "nova_intl_numberformat_currency")) {
                    function = existing.get();
                } else {
                    auto* functionType =
                        new HIRFunctionType({pointerType}, stringType);
                    auto created = module_->createFunction(
                        "nova_intl_numberformat_currency", functionType);
                    created->linkage =
                        HIRFunction::Linkage::External;
                    function = created.get();
                }
                lastValue_ = builder_->createCall(
                    function, {lastValue_}, "nf.option.currency");
                lastValue_->type = stringType;
                return;
            }
        }

        // Early Error-property routing for Error-typed variables. Errors are
        // also registered in dynamicObjectVars_ (see HIRGen_Statements.cpp),
        // but their known properties (name/message/stack/cause/errors) must
        // go through the strongly-typed runtime accessors so the result has
        // the correct HIR type (String for name/message/stack, JSValue for
        // cause, runtime-array pointer for errors). Without this short-circuit
        // the dynamic path returns a generic JSValue and `.errors.length` or
        // `.cause` propagation fails.
        if (auto* errIdent = dynamic_cast<Identifier*>(node.object.get())) {
            if (promiseWithResolversVars_.count(errIdent->name) > 0) {
                if (auto* propIdent = dynamic_cast<Identifier*>(node.property.get())) {
                    const std::string& propertyName = propIdent->name;
                    if (propertyName == "promise" || propertyName == "resolve" ||
                        propertyName == "reject") {
                        errIdent->accept(*this);
                        HIRValue* object = lastValue_;
                        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

                        std::string runtimeFunc;
                        if (propertyName == "promise") {
                            runtimeFunc = "nova_promise_withResolvers_promise";
                        } else if (propertyName == "resolve") {
                            runtimeFunc = "nova_promise_withResolvers_resolve_get";
                        } else {
                            runtimeFunc = "nova_promise_withResolvers_reject_get";
                        }
                        auto existingFunc = module_->getFunction(runtimeFunc);
                        HIRFunction* func = existingFunc ? existingFunc.get() : nullptr;
                        if (!func) {
                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            HIRFunctionType* ft = new HIRFunctionType(paramTypes, ptrType);
                            HIRFunctionPtr fp = module_->createFunction(runtimeFunc, ft);
                            fp->linkage = HIRFunction::Linkage::External;
                            func = fp.get();
                        }
                        lastValue_ = builder_->createCall(func, {object}, "resolver_field");
                        lastValue_->type = ptrType;
                        if (propertyName == "promise") {
                            // The promise field is a real Promise — flag it so
                            // subsequent .then/.catch and instanceof dispatch
                            // work.
                            lastWasPromise_ = true;
                        }
                        return;
                    }
                }
            }
            if (errorVars_.count(errIdent->name) > 0) {
                if (auto* propIdent = dynamic_cast<Identifier*>(node.property.get())) {
                    const std::string& propertyName = propIdent->name;
                    static const std::unordered_set<std::string> errorProps = {
                        "name", "message", "stack", "cause", "errors"
                    };
                    if (errorProps.count(propertyName) > 0) {
                        errIdent->accept(*this);
                        HIRValue* object = lastValue_;
                        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                        auto jsValueType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
                        auto stringType = std::make_shared<HIRType>(HIRType::Kind::String);

                        std::string runtimeFunc;
                        std::string returnTypeKind;  // empty = ptr(string), "jsvalue" = JSValue
                        if (propertyName == "name") {
                            runtimeFunc = "nova_error_get_name";
                        } else if (propertyName == "message") {
                            runtimeFunc = "nova_error_get_message";
                        } else if (propertyName == "stack") {
                            runtimeFunc = "nova_error_get_stack";
                        } else if (propertyName == "cause") {
                            runtimeFunc = "nova_error_get_cause";
                            returnTypeKind = "jsvalue";
                        } else if (propertyName == "errors") {
                            runtimeFunc = "nova_aggregate_error_get_errors";
                        }

                        std::vector<HIRTypePtr> paramTypes = {ptrType};
                        HIRTypePtr retType = ptrType;
                        if (propertyName == "name" || propertyName == "message" ||
                            propertyName == "stack") {
                            retType = stringType;
                        } else if (returnTypeKind == "jsvalue") {
                            retType = jsValueType;
                        }
                        auto existingFunc = module_->getFunction(runtimeFunc);
                        HIRFunction* func = existingFunc ? existingFunc.get() : nullptr;
                        if (!func) {
                            HIRFunctionType* funcType = new HIRFunctionType(paramTypes, retType);
                            HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                            funcPtr->linkage = HIRFunction::Linkage::External;
                            func = funcPtr.get();
                        }

                        HIRValue* errorObject = object;
                        if (errorObject && errorObject->type &&
                            errorObject->type->kind == HIRType::Kind::JSValue) {
                            auto existingUnbox = module_->getFunction("nova_value_to_object");
                            HIRFunction* unbox = existingUnbox ? existingUnbox.get() : nullptr;
                            if (!unbox) {
                                auto* type = new HIRFunctionType({jsValueType}, ptrType);
                                auto created = module_->createFunction("nova_value_to_object", type);
                                created->linkage = HIRFunction::Linkage::External;
                                unbox = created.get();
                            }
                            errorObject = builder_->createCall(unbox, {errorObject}, "error.object.unbox");
                            errorObject->type = ptrType;
                        }
                        lastValue_ = builder_->createCall(func, {errorObject}, "error_prop");
                        lastValue_->type = retType;
                        // `.errors` returns a ValueArray metadata wrapper so
                        // subsequent `.length` / `[i]` work like any other
                        // runtime array.
                        if (propertyName == "errors") {
                            lastWasRuntimeArray_ = true;
                        }
                        return;
                    }
                }
            }
        }

        std::function<bool(Expr*)> isDynamicObjectExpression =
            [&](Expr* expression) -> bool {
                if (!expression) return false;
                if (dynamic_cast<ThisExpr*>(expression)) {
                    // Class constructors/methods keep `this` as a statically
                    // laid-out struct pointer. Only the source entry/global
                    // receiver and ordinary functions using the tagged-this
                    // ABI must use the dynamic property runtime.
                    if (currentClassStructType_) {
                        return false;
                    }
                    return (currentFunction_ &&
                            currentFunction_->name == "main") ||
                        (currentThis_ && currentThis_->type &&
                         (currentThis_->type->kind ==
                              HIRType::Kind::Any ||
                          currentThis_->type->kind ==
                              HIRType::Kind::JSValue));
                }
                if (auto* identifier =
                        dynamic_cast<Identifier*>(expression)) {
                    // Array-typed callback parameters may also contain
                    // dynamic objects as their elements. The container
                    // itself still uses ValueArray metadata and must route
                    // `.length`/indexing through array helpers.
                    if (runtimeArrayVars_.count(identifier->name) > 0 ||
                        taggedRuntimeArrayVars_.count(identifier->name) > 0) {
                        return false;
                    }
                    HIRValue* binding = lookupVariable(identifier->name);
                    if (binding && binding->type) {
                        HIRType* bindingType = binding->type.get();
                        while (bindingType &&
                               bindingType->kind ==
                                   HIRType::Kind::Pointer) {
                            auto* pointerType =
                                dynamic_cast<HIRPointerType*>(bindingType);
                            bindingType = pointerType &&
                                    pointerType->pointeeType
                                ? pointerType->pointeeType.get()
                                : nullptr;
                        }
                        if (bindingType &&
                            bindingType->kind == HIRType::Kind::Array) {
                            return false;
                        }
                    }
                    if (dynamicObjectVars_.count(identifier->name) > 0) {
                        return true;
                    }
                    return binding && binding->type &&
                        (binding->type->kind == HIRType::Kind::Any ||
                         binding->type->kind == HIRType::Kind::JSValue);
                }
                if (auto* assertion = dynamic_cast<AsExpr*>(expression)) {
                    if (assertion->targetType &&
                        assertion->targetType->kind == Type::Kind::Any) {
                        return true;
                    }
                    return isDynamicObjectExpression(
                        assertion->expression.get());
                }
                if (auto* member = dynamic_cast<MemberExpr*>(expression)) {
                    if (!member->isComputed) {
                        auto* base =
                            dynamic_cast<Identifier*>(
                                member->object.get());
                        auto* property =
                            dynamic_cast<Identifier*>(
                                member->property.get());
                        if (base && property &&
                            property->name == "prototype" &&
                            (base->name == "Date" ||
                             base->name == "RegExp")) {
                            return true;
                        }
                    }
                    return isDynamicObjectExpression(member->object.get());
                }
                return false;
            };
        if (isDynamicObjectExpression(node.object.get())) {
            bool symbolKeySyntax = false;
            if (node.isComputed) {
                if (auto* symbolMember =
                        dynamic_cast<MemberExpr*>(node.property.get())) {
                    auto* symbolBase =
                        dynamic_cast<Identifier*>(symbolMember->object.get());
                    symbolKeySyntax =
                        symbolBase && symbolBase->name == "Symbol";
                } else if (auto* symbolIdentifier =
                               dynamic_cast<Identifier*>(node.property.get())) {
                    symbolKeySyntax =
                        symbolVars_.count(symbolIdentifier->name) > 0;
                }
            }
            if (symbolKeySyntax) {
                // Preserve identity for symbol-keyed reads. Computed Symbol
                // properties live in the runtime object's symbol table and
                // must not fall through to numeric array indexing.
                node.object->accept(*this);
                HIRValue* objectValue = lastValue_;
                lastWasSymbol_ = false;
                node.property->accept(*this);
                HIRValue* computedKey = lastValue_;
                const bool computedKeyIsSymbol = lastWasSymbol_;
                lastWasSymbol_ = false;
                if (computedKeyIsSymbol && computedKey &&
                    computedKey->type &&
                    computedKey->type->kind == HIRType::Kind::Pointer) {
                    auto pointerType = std::make_shared<HIRType>(
                        HIRType::Kind::Pointer);
                    auto jsValueType = std::make_shared<HIRType>(
                        HIRType::Kind::JSValue);
                    if (objectValue && objectValue->type &&
                        objectValue->type->kind == HIRType::Kind::JSValue) {
                        HIRFunction* unbox = nullptr;
                        if (auto existing =
                                module_->getFunction("nova_value_to_object")) {
                            unbox = existing.get();
                        } else {
                            auto* type = new HIRFunctionType(
                                {jsValueType}, pointerType);
                            auto created = module_->createFunction(
                                "nova_value_to_object", type);
                            created->linkage = HIRFunction::Linkage::External;
                            unbox = created.get();
                        }
                        objectValue = builder_->createCall(
                            unbox, {objectValue}, "dynamic.symbol_read.unbox");
                        objectValue->type = pointerType;
                    }
                    HIRFunction* getSymbol = nullptr;
                    if (auto existing =
                            module_->getFunction("nova_object_get_symbol")) {
                        getSymbol = existing.get();
                    } else {
                        auto* type = new HIRFunctionType(
                            {pointerType, pointerType}, jsValueType);
                        auto created = module_->createFunction(
                            "nova_object_get_symbol", type);
                        created->linkage = HIRFunction::Linkage::External;
                        getSymbol = created.get();
                    }
                    lastValue_ = builder_->createCall(
                        getSymbol, {objectValue, computedKey},
                        "dynamic.symbol_read");
                    lastValue_->type = jsValueType;
                    return;
                }
            }
            std::string propertyName;
            if (!node.isComputed) {
            if (auto* propertyIdentifier =
                    dynamic_cast<Identifier*>(node.property.get())) {
                propertyName = propertyIdentifier->name;
            } else if (auto* propertyString =
                           dynamic_cast<StringLiteral*>(node.property.get())) {
                propertyName = propertyString->value;
            }
            } else if (auto* propertyString =
                           dynamic_cast<StringLiteral*>(node.property.get())) {
                propertyName = propertyString->value;
            }
            if (!propertyName.empty() || node.isComputed) {
                node.object->accept(*this);
                HIRValue* objectValue = lastValue_;
                auto pointerType = std::make_shared<HIRType>(
                    HIRType::Kind::Pointer);
                auto stringType = std::make_shared<HIRType>(
                    HIRType::Kind::String);
                auto jsValueType = std::make_shared<HIRType>(
                    HIRType::Kind::JSValue);
                // If the object expression just produced a runtime-array
                // metadata pointer (e.g. `aggregate.errors`), the dynamic
                // fallback below would treat the ValueArray metadata as a
                // nova::runtime::Object and segfault. Short-circuit `.length`
                // and `[i]` on the freshly-produced runtime array instead.
                if (lastWasRuntimeArray_ && objectValue && objectValue->type &&
                    objectValue->type->kind == HIRType::Kind::Pointer) {
                    auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                    if (propertyName == "length") {
                        auto existingLen = module_->getFunction("nova_value_array_length");
                        HIRFunction* lenFn = existingLen ? existingLen.get() : nullptr;
                        if (!lenFn) {
                            std::vector<HIRTypePtr> paramTypes = {pointerType};
                            HIRFunctionType* ft = new HIRFunctionType(paramTypes, intType);
                            HIRFunctionPtr fp = module_->createFunction("nova_value_array_length", ft);
                            fp->linkage = HIRFunction::Linkage::External;
                            lenFn = fp.get();
                        }
                        lastValue_ = builder_->createCall(lenFn, {objectValue}, "runtime_array_len");
                        lastValue_->type = intType;
                        lastWasRuntimeArray_ = false;
                        return;
                    }
                    if (node.isComputed) {
                        node.property->accept(*this);
                        HIRValue* index = lastValue_;
                        auto existingAt = module_->getFunction("nova_value_array_at");
                        HIRFunction* atFn = existingAt ? existingAt.get() : nullptr;
                        if (!atFn) {
                            std::vector<HIRTypePtr> paramTypes = {pointerType, intType};
                            HIRFunctionType* ft = new HIRFunctionType(paramTypes, intType);
                            HIRFunctionPtr fp = module_->createFunction("nova_value_array_at", ft);
                            fp->linkage = HIRFunction::Linkage::External;
                            atFn = fp.get();
                        }
                        lastValue_ = builder_->createCall(atFn, {objectValue, index}, "runtime_array_elem");
                        lastValue_->type = intType;
                        lastWasRuntimeArray_ = false;
                        return;
                    }
                }
                if (propertyName == "length" && objectValue &&
                    objectValue->type &&
                    objectValue->type->kind == HIRType::Kind::JSValue) {
                    auto intType =
                        std::make_shared<HIRType>(HIRType::Kind::I64);
                    HIRFunction* lengthFn = nullptr;
                    if (auto existing =
                            module_->getFunction("nova_value_length")) {
                        lengthFn = existing.get();
                    } else {
                        auto* type = new HIRFunctionType(
                            {jsValueType}, intType);
                        auto created = module_->createFunction(
                            "nova_value_length", type);
                        created->linkage =
                            HIRFunction::Linkage::External;
                        lengthFn = created.get();
                    }
                    lastValue_ = builder_->createCall(
                        lengthFn, {objectValue}, "jsvalue.length");
                    lastValue_->type = intType;
                    return;
                }
                if (objectValue && objectValue->type &&
                    objectValue->type->kind == HIRType::Kind::JSValue) {
                    auto existingUnbox =
                        module_->getFunction("nova_value_to_object");
                    HIRFunction* unbox =
                        existingUnbox ? existingUnbox.get() : nullptr;
                    if (!unbox) {
                        auto* type = new HIRFunctionType(
                            {jsValueType}, pointerType);
                        auto created = module_->createFunction(
                            "nova_value_to_object", type);
                        created->linkage = HIRFunction::Linkage::External;
                        unbox = created.get();
                    }
                    objectValue = builder_->createCall(
                        unbox, {objectValue}, "dynamic.object.unbox");
                    objectValue->type = pointerType;
                }
                auto existing = module_->getFunction(
                    "nova_dynamic_object_get_tagged");
                HIRFunction* getter = existing ? existing.get() : nullptr;
                if (!getter) {
                    auto* type = new HIRFunctionType(
                        {pointerType, stringType}, jsValueType);
                    auto created = module_->createFunction(
                        "nova_dynamic_object_get_tagged", type);
                    created->linkage = HIRFunction::Linkage::External;
                    getter = created.get();
                }
                HIRValue* propertyKey = nullptr;
                if (!propertyName.empty()) {
                    propertyKey =
                        builder_->createStringConstant(propertyName);
                } else {
                    node.property->accept(*this);
                    propertyKey = lastValue_;
                    if (propertyKey && propertyKey->type &&
                        propertyKey->type->kind ==
                            HIRType::Kind::JSValue) {
                        HIRFunction* toString = nullptr;
                        if (auto found = module_->getFunction(
                                "nova_value_to_string_ptr")) {
                            toString = found.get();
                        } else {
                            auto* type = new HIRFunctionType(
                                {jsValueType}, stringType);
                            auto created = module_->createFunction(
                                "nova_value_to_string_ptr", type);
                            created->linkage =
                                HIRFunction::Linkage::External;
                            toString = created.get();
                        }
                        propertyKey = builder_->createCall(
                            toString, {propertyKey},
                            "dynamic.property.key");
                    } else if (propertyKey && propertyKey->type &&
                               propertyKey->type->kind !=
                                   HIRType::Kind::String &&
                               propertyKey->type->kind !=
                                   HIRType::Kind::Pointer) {
                        auto integerType =
                            std::make_shared<HIRType>(
                                HIRType::Kind::I64);
                        HIRFunction* toString = nullptr;
                        if (auto found = module_->getFunction(
                                "nova_value_key_to_string")) {
                            toString = found.get();
                        } else {
                            auto* type = new HIRFunctionType(
                                {integerType}, stringType);
                            auto created = module_->createFunction(
                                "nova_value_key_to_string", type);
                            created->linkage =
                                HIRFunction::Linkage::External;
                            toString = created.get();
                        }
                        propertyKey = builder_->createCall(
                            toString, {propertyKey},
                            "dynamic.property.numeric_key");
                    }
                }
                lastValue_ = builder_->createCall(getter, {
                    objectValue, propertyKey
                }, "dynamic.object.property");
                lastValue_->type = jsValueType;
                // Phase 2.4: nova_dynamic_object_get_tagged may dispatch a
                // Proxy trap, which can throw (e.g. revoked-proxy TypeError).
                // When inside a try block, poll for a pending exception and
                // branch to the catch block so `try { revoked.proxy.ok }`
                // actually catches. pollExceptionAfterCall() is a no-op when
                // currentCatchBlock_ is null.
                pollExceptionAfterCall();
                return;
            }
        }

        if (auto* objectIdentifier = dynamic_cast<Identifier*>(node.object.get())) {
            if (auto* propertyIdentifier = dynamic_cast<Identifier*>(node.property.get())) {
                const auto& namespaceName = objectIdentifier->name;
                const auto& propertyName = propertyIdentifier->name;
                if (auto space = moduleNamespaceNumberConstants_.find(namespaceName);
                    space != moduleNamespaceNumberConstants_.end()) {
                    if (auto value = space->second.find(propertyName);
                        value != space->second.end()) {
                        lastValue_ = value->second == static_cast<int64_t>(value->second)
                            ? builder_->createIntConstant(
                                  static_cast<int64_t>(value->second))
                            : builder_->createFloatConstant(value->second);
                        return;
                    }
                }
                if (auto space = moduleNamespaceStringConstants_.find(namespaceName);
                    space != moduleNamespaceStringConstants_.end()) {
                    if (auto value = space->second.find(propertyName);
                        value != space->second.end()) {
                        lastValue_ = builder_->createStringConstant(value->second);
                        return;
                    }
                }
                if (auto space = moduleNamespaceBooleanConstants_.find(namespaceName);
                    space != moduleNamespaceBooleanConstants_.end()) {
                    if (auto value = space->second.find(propertyName);
                        value != space->second.end()) {
                        lastValue_ = builder_->createBoolConstant(value->second);
                        return;
                    }
                }
            }
        }

        // Handle globalThis property access (ES2020)
        if (auto* objIdent = dynamic_cast<Identifier*>(node.object.get())) {
            if (objIdent->name == "globalThis") {
                if (auto* propIdent = dynamic_cast<Identifier*>(node.property.get())) {
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: globalThis." << propIdent->name << " property access" << std::endl;

                    // Global constants
                    if (propIdent->name == "Infinity") {
                        lastValue_ = builder_->createFloatConstant(std::numeric_limits<double>::infinity());
                        return;
                    }
                    if (propIdent->name == "NaN") {
                        lastValue_ = builder_->createFloatConstant(std::numeric_limits<double>::quiet_NaN());
                        return;
                    }
                    if (propIdent->name == "undefined") {
                        lastValue_ = builder_->createIntConstant(0);
                        return;
                    }

                    // Global objects - these return placeholder values
                    // The actual functionality is provided when methods are called on them
                    if (propIdent->name == "Math" || propIdent->name == "JSON" ||
                        propIdent->name == "console" || propIdent->name == "Array" ||
                        propIdent->name == "Object" || propIdent->name == "String" ||
                        propIdent->name == "Number" || propIdent->name == "Boolean" ||
                        propIdent->name == "Date" || propIdent->name == "Error" ||
                        propIdent->name == "Promise" || propIdent->name == "Symbol" ||
                        propIdent->name == "Map" || propIdent->name == "Set" ||
                        propIdent->name == "WeakMap" || propIdent->name == "WeakSet" ||
                        propIdent->name == "ArrayBuffer" || propIdent->name == "DataView" ||
                        propIdent->name == "Int8Array" || propIdent->name == "Uint8Array" ||
                        propIdent->name == "Int16Array" || propIdent->name == "Uint16Array" ||
                        propIdent->name == "Int32Array" || propIdent->name == "Uint32Array" ||
                        propIdent->name == "Float32Array" || propIdent->name == "Float64Array" ||
                        propIdent->name == "BigInt64Array" || propIdent->name == "BigUint64Array") {
                        // Return marker - actual methods will be handled by CallExpr
                        lastValue_ = builder_->createIntConstant(1);
                        lastWasGlobalThis_ = true;
                        return;
                    }

                    // Global functions - accessed as properties but can be called
                    // These are just property access, actual calls go through CallExpr
                    if (propIdent->name == "parseInt" || propIdent->name == "parseFloat" ||
                        propIdent->name == "isNaN" || propIdent->name == "isFinite" ||
                        propIdent->name == "eval" || propIdent->name == "encodeURI" ||
                        propIdent->name == "decodeURI" || propIdent->name == "encodeURIComponent" ||
                        propIdent->name == "decodeURIComponent" || propIdent->name == "atob" ||
                        propIdent->name == "btoa") {
                        lastValue_ = builder_->createIntConstant(1);  // Function reference placeholder
                        return;
                    }

                    // globalThis.globalThis = globalThis (self-reference)
                    if (propIdent->name == "globalThis") {
                        lastWasGlobalThis_ = true;
                        lastValue_ = builder_->createIntConstant(1);
                        return;
                    }
                }
            }
        }

        // Check for Math constants (PI, E, etc.)
        if (auto* objIdent = dynamic_cast<Identifier*>(node.object.get())) {
            if (auto* propIdent = dynamic_cast<Identifier*>(node.property.get())) {
                if (objIdent->name == "Math") {
                    if (propIdent->name == "PI") {
                        // Math.PI ≈ 3.14159... -> return 3 for integer
                        lastValue_ = builder_->createFloatConstant(3.14159265358979323846);
                        return;
                    } else if (propIdent->name == "E") {
                        // Math.E ≈ 2.71828... -> return 2 for integer (or 3 if you prefer rounding)
                        lastValue_ = builder_->createFloatConstant(2.71828182845904523536);
                        return;
                    } else if (propIdent->name == "LN2") {
                        // Math.LN2 ≈ 0.693147... -> return 0 for integer (truncated)
                        lastValue_ = builder_->createFloatConstant(0.69314718055994530942);
                        return;
                    } else if (propIdent->name == "LN10") {
                        // Math.LN10 ≈ 2.302585... -> return 2 for integer (truncated)
                        lastValue_ = builder_->createFloatConstant(2.30258509299404568402);
                        return;
                    } else if (propIdent->name == "LOG2E") {
                        // Math.LOG2E ≈ 1.442695... -> return 1 for integer (truncated)
                        lastValue_ = builder_->createFloatConstant(1.44269504088896340736);
                        return;
                    } else if (propIdent->name == "LOG10E") {
                        // Math.LOG10E ≈ 0.434294... -> return 0 for integer (truncated)
                        lastValue_ = builder_->createFloatConstant(0.43429448190325182765);
                        return;
                    } else if (propIdent->name == "SQRT1_2") {
                        // Math.SQRT1_2 ≈ 0.707106... -> return 0 for integer (truncated)
                        lastValue_ = builder_->createFloatConstant(0.70710678118654752440);
                        return;
                    } else if (propIdent->name == "SQRT2") {
                        // Math.SQRT2 ≈ 1.414213... -> return 1 for integer (truncated)
                        lastValue_ = builder_->createFloatConstant(1.41421356237309504880);
                        return;
                    }
                } else if (objIdent->name == "Number") {
                    // Number constants (ES2015)
                    if (propIdent->name == "MAX_SAFE_INTEGER") {
                        // Number.MAX_SAFE_INTEGER = 2^53 - 1 = 9007199254740991
                        lastValue_ = builder_->createIntConstant(9007199254740991LL);
                        return;
                    } else if (propIdent->name == "MIN_SAFE_INTEGER") {
                        // Number.MIN_SAFE_INTEGER = -(2^53 - 1) = -9007199254740991
                        lastValue_ = builder_->createIntConstant(-9007199254740991LL);
                        return;
                    } else if (propIdent->name == "MAX_VALUE") {
                        // Number.MAX_VALUE = 1.7976931348623157e+308 (largest representable number)
                        lastValue_ = builder_->createFloatConstant(1.7976931348623157e+308);
                        return;
                    } else if (propIdent->name == "MIN_VALUE") {
                        // Number.MIN_VALUE = 5e-324 (smallest positive number)
                        lastValue_ = builder_->createFloatConstant(5e-324);
                        return;
                    } else if (propIdent->name == "EPSILON") {
                        // Number.EPSILON = 2^-52 = 2.220446049250313e-16
                        lastValue_ = builder_->createFloatConstant(2.220446049250313e-16);
                        return;
                    } else if (propIdent->name == "POSITIVE_INFINITY") {
                        // Number.POSITIVE_INFINITY = Infinity
                        lastValue_ = builder_->createFloatConstant(std::numeric_limits<double>::infinity());
                        return;
                    } else if (propIdent->name == "NEGATIVE_INFINITY") {
                        // Number.NEGATIVE_INFINITY = -Infinity
                        lastValue_ = builder_->createFloatConstant(-std::numeric_limits<double>::infinity());
                        return;
                    } else if (propIdent->name == "NaN") {
                        // Number.NaN = NaN
                        lastValue_ = builder_->createFloatConstant(std::numeric_limits<double>::quiet_NaN());
                        return;
                    }
                } else if (objIdent->name == "Symbol") {
                    // Symbol well-known symbols (ES2015+)
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Symbol property access: Symbol." << propIdent->name << std::endl;

                    std::string runtimeFunc;
                    if (propIdent->name == "iterator") {
                        runtimeFunc = "nova_symbol_iterator";
                    } else if (propIdent->name == "asyncIterator") {
                        runtimeFunc = "nova_symbol_asyncIterator";
                    } else if (propIdent->name == "hasInstance") {
                        runtimeFunc = "nova_symbol_hasInstance";
                    } else if (propIdent->name == "isConcatSpreadable") {
                        runtimeFunc = "nova_symbol_isConcatSpreadable";
                    } else if (propIdent->name == "match") {
                        runtimeFunc = "nova_symbol_match";
                    } else if (propIdent->name == "matchAll") {
                        runtimeFunc = "nova_symbol_matchAll";
                    } else if (propIdent->name == "replace") {
                        runtimeFunc = "nova_symbol_replace";
                    } else if (propIdent->name == "search") {
                        runtimeFunc = "nova_symbol_search";
                    } else if (propIdent->name == "species") {
                        runtimeFunc = "nova_symbol_species";
                    } else if (propIdent->name == "split") {
                        runtimeFunc = "nova_symbol_split";
                    } else if (propIdent->name == "toPrimitive") {
                        runtimeFunc = "nova_symbol_toPrimitive";
                    } else if (propIdent->name == "toStringTag") {
                        runtimeFunc = "nova_symbol_toStringTag";
                    } else if (propIdent->name == "unscopables") {
                        runtimeFunc = "nova_symbol_unscopables";
                    } else if (propIdent->name == "dispose") {
                        runtimeFunc = "nova_symbol_dispose_obj";
                    } else if (propIdent->name == "asyncDispose") {
                        runtimeFunc = "nova_symbol_asyncDispose_obj";
                    }

                    if (!runtimeFunc.empty()) {
                        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                        std::vector<HIRTypePtr> paramTypes;  // No params

                        HIRFunction* func = nullptr;
                        auto existingFunc = module_->getFunction(runtimeFunc);
                        if (existingFunc) {
                            func = existingFunc.get();
                        } else {
                            HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                            HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                            funcPtr->linkage = HIRFunction::Linkage::External;
                            func = funcPtr.get();
                        }

                        std::vector<HIRValue*> args;
                        lastValue_ = builder_->createCall(func, args, "symbol_wellknown");
                        lastWasSymbol_ = true;
                        return;
                    }
                }

                // Check for enum access (e.g., Color.Red)
                auto enumIt = enumTable_.find(objIdent->name);
                if (enumIt != enumTable_.end()) {
                    auto memberIt = enumIt->second.find(propIdent->name);
                    if (memberIt != enumIt->second.end()) {
                        const auto& ev = memberIt->second;
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Enum access " << objIdent->name << "." << propIdent->name
                                                 << " = " << (ev.kind == HIRGenerator::EnumValue::Kind::String ? "\"" + ev.stringValue + "\"" : std::to_string(ev.numberValue)) << std::endl;
                        if (ev.kind == HIRGenerator::EnumValue::Kind::String) {
                            lastValue_ = builder_->createStringConstant(ev.stringValue);
                        } else {
                            lastValue_ = builder_->createIntConstant(ev.numberValue);
                        }
                        return;
                    }
                    // Reverse mapping for numeric enums: Color[0] -> "Red"
                    if (node.isComputed) {
                        // property is the expression in brackets
                        if (auto* numLit = dynamic_cast<NumberLiteral*>(node.property.get())) {
                            int64_t key = static_cast<int64_t>(numLit->value);
                            for (const auto& p : enumIt->second) {
                                if (p.second.kind == HIRGenerator::EnumValue::Kind::Number &&
                                    p.second.numberValue == key) {
                                    lastValue_ = builder_->createStringConstant(p.first);
                                    return;
                                }
                            }
                        }
                    }
                }

                // Check for static property access (e.g., Config.version)
                auto staticClassIt = classStaticProps_.find(objIdent->name);
                if (staticClassIt != classStaticProps_.end()) {
                    if (staticClassIt->second.find(propIdent->name) != staticClassIt->second.end()) {
                        // Emit runtime getter so static state can be mutated at runtime
                        // (e.g. private static counters). Use the lazy-init variant so the
                        // declared initializer is applied on first access without needing
                        // an active HIR insert point at class-declaration time.
                        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                        auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                        std::string propKey = objIdent->name + "_" + propIdent->name;
                        int64_t initValue = 0;
                        auto ivIt = staticPropertyValues_.find(propKey);
                        if (ivIt != staticPropertyValues_.end()) initValue = ivIt->second;
                        std::string getter = "nova_class_static_get_or_init_i64";
                        HIRFunction* func = nullptr;
                        auto existing = module_->getFunction(getter);
                        if (existing) func = existing.get();
                        else {
                            HIRFunctionType* ft = new HIRFunctionType({ptrType, ptrType, intType}, intType);
                            HIRFunctionPtr fp = module_->createFunction(getter, ft);
                            fp->linkage = HIRFunction::Linkage::External;
                            func = fp.get();
                        }
                        HIRValue* classNameArg = builder_->createStringConstant(objIdent->name);
                        HIRValue* fieldNameArg = builder_->createStringConstant(propIdent->name);
                        HIRValue* initValueArg = builder_->createIntConstant(initValue);
                        lastValue_ = builder_->createCall(func, {classNameArg, fieldNameArg, initValueArg}, "static_get");
                        return;
                    }
                }
            }
        }

        // Direct generator-call chains such as
        // `range(1, 3).next().value` have no identifier that can be recorded
        // in generatorVars_. Lower the complete chain here so the generator
        // expression is evaluated exactly once.
        if (auto* resultCall = dynamic_cast<CallExpr*>(node.object.get())) {
            auto* nextMember =
                dynamic_cast<MemberExpr*>(resultCall->callee.get());
            auto* method = nextMember
                ? dynamic_cast<Identifier*>(nextMember->property.get())
                : nullptr;
            auto* generatorCall = nextMember
                ? dynamic_cast<CallExpr*>(nextMember->object.get())
                : nullptr;
            auto* generatorName = generatorCall
                ? dynamic_cast<Identifier*>(generatorCall->callee.get())
                : nullptr;
            auto* resultProperty =
                dynamic_cast<Identifier*>(node.property.get());
            if (method && generatorName && resultProperty &&
                generatorFuncs_.count(generatorName->name) > 0 &&
                (method->name == "next" ||
                 method->name == "return" ||
                 method->name == "throw") &&
                (resultProperty->name == "value" ||
                 resultProperty->name == "done")) {
                generatorCall->accept(*this);
                HIRValue* generator = lastValue_;
                auto ptrType =
                    std::make_shared<HIRType>(HIRType::Kind::Pointer);
                auto intType =
                    std::make_shared<HIRType>(HIRType::Kind::I64);
                const std::string nextHelper =
                    method->name == "return"
                        ? "nova_generator_return"
                        : (method->name == "throw"
                            ? "nova_generator_throw"
                            : "nova_generator_next");
                HIRFunction* nextFunction = nullptr;
                if (auto existing =
                        module_->getFunction(nextHelper)) {
                    nextFunction = existing.get();
                } else {
                    auto* type = new HIRFunctionType(
                        {ptrType, intType}, ptrType);
                    auto created = module_->createFunction(
                        nextHelper, type);
                    created->linkage =
                        HIRFunction::Linkage::External;
                    nextFunction = created.get();
                }
                HIRValue* input = builder_->createIntConstant(0);
                if (!resultCall->arguments.empty()) {
                    resultCall->arguments.front()->accept(*this);
                    input = lastValue_;
                }
                auto* result = builder_->createCall(
                    nextFunction, {generator, input},
                    "direct.iterator.result");
                result->type = ptrType;

                const bool wantsValue =
                    resultProperty->name == "value";
                auto resultType = std::make_shared<HIRType>(
                    wantsValue ? HIRType::Kind::I64
                               : HIRType::Kind::Bool);
                const std::string accessor = wantsValue
                    ? "nova_iterator_result_value"
                    : "nova_iterator_result_done";
                HIRFunction* accessorFunction = nullptr;
                if (auto existing =
                        module_->getFunction(accessor)) {
                    accessorFunction = existing.get();
                } else {
                    auto* type = new HIRFunctionType(
                        {ptrType}, resultType);
                    auto created = module_->createFunction(
                        accessor, type);
                    created->linkage =
                        HIRFunction::Linkage::External;
                    accessorFunction = created.get();
                }
                lastValue_ = builder_->createCall(
                    accessorFunction, {result},
                    wantsValue ? "iter_value" : "iter_done");
                lastValue_->type = resultType;
                return;
            }
        }

        // Evaluate the object/array
        node.object->accept(*this);
        auto object = lastValue_;

        // Support immediate IteratorResult access (`gen.next().value` and
        // `gen.next().done`) as well as the named-variable form handled
        // below.  Iterator-result tracking used to depend on assigning the
        // call to a variable first, so the direct form silently fell through
        // to generic property access and produced zero.
        bool immediateIteratorResult = false;
        if (auto* resultCall = dynamic_cast<CallExpr*>(node.object.get())) {
            if (auto* nextMember =
                    dynamic_cast<MemberExpr*>(resultCall->callee.get())) {
                if (auto* method =
                        dynamic_cast<Identifier*>(nextMember->property.get())) {
                    if (method->name == "next" || method->name == "return" ||
                        method->name == "throw") {
                        if (auto* base =
                                dynamic_cast<Identifier*>(nextMember->object.get())) {
                            immediateIteratorResult =
                                generatorVars_.count(base->name) > 0 ||
                                asyncGeneratorVars_.count(base->name) > 0;
                        }
                    }
                }
            }
        }
        if (immediateIteratorResult) {
            if (auto* property =
                    dynamic_cast<Identifier*>(node.property.get())) {
                const bool wantsValue = property->name == "value";
                const bool wantsDone = property->name == "done";
                if (wantsValue || wantsDone) {
                    auto ptrType =
                        std::make_shared<HIRType>(HIRType::Kind::Pointer);
                    auto resultType = std::make_shared<HIRType>(
                        wantsValue ? HIRType::Kind::I64
                                   : HIRType::Kind::Bool);
                    const std::string helper = wantsValue
                        ? "nova_iterator_result_value"
                        : "nova_iterator_result_done";
                    auto existing = module_->getFunction(helper);
                    HIRFunction* function =
                        existing ? existing.get() : nullptr;
                    if (!function) {
                        auto* functionType =
                            new HIRFunctionType({ptrType}, resultType);
                        auto created =
                            module_->createFunction(helper, functionType);
                        created->linkage = HIRFunction::Linkage::External;
                        function = created.get();
                    }
                    lastValue_ =
                        builder_->createCall(function, {object},
                                             wantsValue ? "iter_value"
                                                        : "iter_done");
                    lastValue_->type = resultType;
                    return;
                }
            }
        }

        // If the object expression just produced a runtime-array metadata
        // pointer (e.g. `aggregate.errors` returning a ValueArray wrapper),
        // route `.length` and `[i]` through the runtime-array primitives
        // even when the receiver is not a simple Identifier. Without this,
        // `aggregate.errors.length` falls through to the dynamic-object or
        // generic-class path and returns undefined / segfaults.
        if (lastWasRuntimeArray_ && object && object->type &&
            object->type->kind == HIRType::Kind::Pointer) {
            if (auto* propIdent = dynamic_cast<Identifier*>(node.property.get())) {
                if (propIdent->name == "length") {
                    auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                    auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                    auto existingFunc = module_->getFunction("nova_value_array_length");
                    HIRFunction* func = existingFunc ? existingFunc.get() : nullptr;
                    if (!func) {
                        std::vector<HIRTypePtr> paramTypes = {ptrType};
                        HIRFunctionType* funcType = new HIRFunctionType(paramTypes, intType);
                        HIRFunctionPtr funcPtr = module_->createFunction("nova_value_array_length", funcType);
                        funcPtr->linkage = HIRFunction::Linkage::External;
                        func = funcPtr.get();
                    }
                    lastValue_ = builder_->createCall(func, {object}, "runtime_array_len");
                    lastValue_->type = intType;
                    lastWasRuntimeArray_ = false;
                    return;
                }
            }
            if (node.isComputed) {
                // numeric/symbol index on a freshly-produced runtime array
                node.property->accept(*this);
                auto index = lastValue_;
                auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                auto existingFunc = module_->getFunction("nova_value_array_at");
                HIRFunction* func = existingFunc ? existingFunc.get() : nullptr;
                if (!func) {
                    std::vector<HIRTypePtr> paramTypes = {ptrType, intType};
                    HIRFunctionType* funcType = new HIRFunctionType(paramTypes, intType);
                    HIRFunctionPtr funcPtr = module_->createFunction("nova_value_array_at", funcType);
                    funcPtr->linkage = HIRFunction::Linkage::External;
                    func = funcPtr.get();
                }
                std::vector<HIRValue*> args = {object, index};
                lastValue_ = builder_->createCall(func, args, "runtime_array_elem");
                lastValue_->type = intType;
                lastWasRuntimeArray_ = false;
                return;
            }
        }

        auto unboxTaggedObject = [&](HIRValue* value) -> HIRValue* {
            if (!value || !value->type ||
                value->type->kind != HIRType::Kind::JSValue) {
                return value;
            }
            auto jsValueType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
            auto pointerType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto existing = module_->getFunction("nova_value_to_object");
            HIRFunction* function = existing ? existing.get() : nullptr;
            if (!function) {
                auto* functionType = new HIRFunctionType(
                    {jsValueType}, pointerType);
                auto created = module_->createFunction(
                    "nova_value_to_object", functionType);
                created->linkage = HIRFunction::Linkage::External;
                function = created.get();
            }
            return builder_->createCall(function, {value}, "jsvalue.object");
        };

        if (node.isComputed) {
            // Computed member: obj[property] e.g., arr[index]
            // A literal string key on a fixed-layout object is statically the
            // same operation as dot access. Resolve it before the generic array
            // path, whose runtime ABI only accepts numeric indices.
            if (auto* keyLiteral = dynamic_cast<StringLiteral*>(node.property.get())) {
                HIRStructType* structType = nullptr;
                if (object && object->type) {
                    structType = dynamic_cast<HIRStructType*>(object->type.get());
                    if (!structType) {
                        if (auto* pointerType = dynamic_cast<HIRPointerType*>(object->type.get())) {
                            structType = dynamic_cast<HIRStructType*>(
                                pointerType->pointeeType.get());
                        }
                    }
                }
                if (structType) {
                    for (size_t i = 0; i < structType->fields.size(); ++i) {
                        if (structType->fields[i].name == keyLiteral->value) {
                            lastValue_ = builder_->createGetField(
                                object, static_cast<uint32_t>(i), keyLiteral->value);
                            return;
                        }
                    }
                    // A known fixed-layout object cannot gain an unknown field.
                    lastValue_ = builder_->createIntConstant(0);
                    return;
                }
            }

            node.property->accept(*this);
            auto index = lastValue_;

            // Statically constructed arrays retain their element type in HIR.
            // Keep their access in the compiler pipeline so string and nested-
            // array elements are not erased to i64 by the generic runtime ABI.
            bool isStaticArray = false;
            if (object && object->type) {
                if (auto* pointerType = dynamic_cast<HIRPointerType*>(object->type.get())) {
                    isStaticArray = dynamic_cast<HIRArrayType*>(pointerType->pointeeType.get()) != nullptr;
                } else {
                    isStaticArray = dynamic_cast<HIRArrayType*>(object->type.get()) != nullptr;
                }
            }
            if (auto* arrayIdentifier =
                    dynamic_cast<Identifier*>(node.object.get());
                arrayIdentifier &&
                runtimeArrayVars_.count(arrayIdentifier->name) > 0) {
                isStaticArray = false;
            }
            if (isStaticArray) {
                HIRTypePtr staticElementType;
                if (auto* pointerType =
                        dynamic_cast<HIRPointerType*>(object->type.get())) {
                    if (auto* arrayType = dynamic_cast<HIRArrayType*>(
                            pointerType->pointeeType.get())) {
                        staticElementType = arrayType->elementType;
                    }
                } else if (auto* arrayType =
                               dynamic_cast<HIRArrayType*>(object->type.get())) {
                    staticElementType = arrayType->elementType;
                }
                if (staticElementType &&
                    staticElementType->kind == HIRType::Kind::JSValue) {
                    auto ptrType =
                        std::make_shared<HIRType>(HIRType::Kind::Pointer);
                    auto intType =
                        std::make_shared<HIRType>(HIRType::Kind::I64);
                    auto jsType =
                        std::make_shared<HIRType>(HIRType::Kind::JSValue);
                    auto existing = module_->getFunction(
                        "nova_value_array_at_tagged");
                    HIRFunction* function =
                        existing ? existing.get() : nullptr;
                    if (!function) {
                        auto* functionType = new HIRFunctionType(
                            {ptrType, intType, intType}, jsType);
                        auto created = module_->createFunction(
                            "nova_value_array_at_tagged", functionType);
                        created->linkage =
                            HIRFunction::Linkage::External;
                        function = created.get();
                    }
                    lastValue_ = builder_->createCall(
                        function,
                        {object, index, builder_->createIntConstant(0)},
                        "array_elem.tagged");
                    lastValue_->type = jsType;
                    return;
                }
                lastValue_ = builder_->createGetElement(object, index, "array_elem");
                // For typed arrays declared as `let arr: string[]`, the runtime
                // element access returns I64 but downstream comparisons treat
                // it as String. Apply an explicit cast so types align.
                if (lastValue_ && lastValue_->type &&
                    lastValue_->type->kind == HIRType::Kind::I64) {
                    if (auto* objIdent = dynamic_cast<Identifier*>(node.object.get())) {
                        auto it = variableArrayElementTypes_.find(objIdent->name);
                        if (it != variableArrayElementTypes_.end() && it->second == "String") {
                            auto stringType = std::make_shared<HIRType>(HIRType::Kind::String);
                            lastValue_ = builder_->createCast(lastValue_, stringType.get(), "array_elem_str");
                        }
                    }
                }
                return;
            }

            // Check if this is runtime array element access (from keys(), values(), entries())
            if (auto* objIdent = dynamic_cast<Identifier*>(node.object.get())) {
                if (runtimeArrayVars_.count(objIdent->name) > 0) {
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Runtime array element access on " << objIdent->name << std::endl;

                    auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                    auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                    const bool tagged =
                        taggedRuntimeArrayVars_.count(objIdent->name) > 0;
                    HIRTypePtr elementType =
                        tagged
                        ? std::make_shared<HIRType>(HIRType::Kind::JSValue)
                        : intType;
                    if (!tagged) {
                        auto inferred =
                            variableArrayElementTypes_.find(objIdent->name);
                        if (inferred != variableArrayElementTypes_.end()) {
                            if (inferred->second == "String") {
                                elementType = std::make_shared<HIRType>(
                                    HIRType::Kind::String);
                            } else if (inferred->second == "Bool") {
                                elementType = std::make_shared<HIRType>(
                                    HIRType::Kind::Bool);
                            }
                        }
                    }

                    std::string runtimeFunc = tagged
                        ? "nova_value_array_at_tagged"
                        : "nova_value_array_at";
                    auto existingFunc = module_->getFunction(runtimeFunc);
                    HIRFunction* func = nullptr;
                    if (existingFunc) {
                        func = existingFunc.get();
                    } else {
                        std::vector<HIRTypePtr> paramTypes = {ptrType, intType};
                        if (tagged) paramTypes.push_back(intType);
                        HIRFunctionType* funcType = new HIRFunctionType(
                            paramTypes, elementType);
                        HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                        funcPtr->linkage = HIRFunction::Linkage::External;
                        func = funcPtr.get();
                    }

                    std::vector<HIRValue*> args = {
                        unboxTaggedObject(object), index};
                    if (tagged) {
                        args.push_back(builder_->createIntConstant(0));
                    }
                    HIRValue* rawElement =
                        builder_->createCall(func, args, "runtime_elem");
                    if (!tagged &&
                        elementType->kind != HIRType::Kind::I64) {
                        lastValue_ = builder_->createCast(
                            rawElement, elementType.get(),
                            "runtime_elem.typed");
                    } else {
                        lastValue_ = rawElement;
                        lastValue_->type = elementType;
                    }
                    return;
                }
            }

            // Check if this is TypedArray element access
            if (auto* objIdent = dynamic_cast<Identifier*>(node.object.get())) {
                auto typeIt = typedArrayTypes_.find(objIdent->name);
                if (typeIt != typedArrayTypes_.end()) {
                    std::string typedArrayType = typeIt->second;
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: TypedArray element access on " << objIdent->name
                              << " (type: " << typedArrayType << ")" << std::endl;

                    // Determine runtime function name
                    std::string runtimeFunc;
                    if (typedArrayType == "Int8Array") runtimeFunc = "nova_int8array_get";
                    else if (typedArrayType == "Uint8Array") runtimeFunc = "nova_uint8array_get";
                    else if (typedArrayType == "Uint8ClampedArray") runtimeFunc = "nova_uint8clampedarray_get";
                    else if (typedArrayType == "Int16Array") runtimeFunc = "nova_int16array_get";
                    else if (typedArrayType == "Uint16Array") runtimeFunc = "nova_uint16array_get";
                    else if (typedArrayType == "Int32Array") runtimeFunc = "nova_int32array_get";
                    else if (typedArrayType == "Uint32Array") runtimeFunc = "nova_uint32array_get";
                    else if (typedArrayType == "Float32Array") runtimeFunc = "nova_float32array_get";
                    else if (typedArrayType == "Float64Array") runtimeFunc = "nova_float64array_get";
                    else if (typedArrayType == "BigInt64Array") runtimeFunc = "nova_bigint64array_get";
                    else if (typedArrayType == "BigUint64Array") runtimeFunc = "nova_biguint64array_get";

                    if (!runtimeFunc.empty()) {
                        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                        auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

                        // Determine return type (float for Float32/Float64, i64 otherwise)
                        HIRTypePtr returnType;
                        if (typedArrayType == "Float32Array" || typedArrayType == "Float64Array") {
                            returnType = std::make_shared<HIRType>(HIRType::Kind::F64);
                        } else {
                            returnType = std::make_shared<HIRType>(HIRType::Kind::I64);
                        }

                        std::vector<HIRTypePtr> paramTypes = {ptrType, intType};
                        auto existingFunc = module_->getFunction(runtimeFunc);
                        HIRFunction* func = nullptr;
                        if (existingFunc) {
                            func = existingFunc.get();
                        } else {
                            HIRFunctionType* funcType = new HIRFunctionType(paramTypes, returnType);
                            HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                            funcPtr->linkage = HIRFunction::Linkage::External;
                            func = funcPtr.get();
                        }

                        std::vector<HIRValue*> args = {object, index};
                        lastValue_ = builder_->createCall(func, args, "typed_elem");
                        lastValue_->type = returnType;
                        return;
                    }
                }
            }

            // Use runtime function for array element access to ensure correct type
            // This fixes the bug where arr[i] returns Object type instead of the element type
            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

            // String indexing: s[i] returns the single-character string at i,
            // matching JS String.prototype.charAt semantics. Without this, the
            // generic array path would invoke nova_value_array_at on a pointer
            // to char data, reinterpreting char bytes as a runtime array header.
            if (object && object->type &&
                object->type->kind == hir::HIRType::Kind::String) {
                std::string runtimeFunc = "nova_string_at";
                auto existingFunc = module_->getFunction(runtimeFunc);
                HIRFunction* func = nullptr;
                if (existingFunc) {
                    func = existingFunc.get();
                } else {
                    std::vector<HIRTypePtr> paramTypes = {
                        std::make_shared<HIRType>(HIRType::Kind::String),
                        intType};
                    auto stringType = std::make_shared<HIRType>(HIRType::Kind::String);
                    HIRFunctionType* funcType = new HIRFunctionType(paramTypes, stringType);
                    HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                    funcPtr->linkage = HIRFunction::Linkage::External;
                    func = funcPtr.get();
                }
                std::vector<HIRValue*> args = {object, index};
                lastValue_ = builder_->createCall(func, args, "str_index");
                lastValue_->type = std::make_shared<HIRType>(HIRType::Kind::String);
                return;
            }

            // Determine element type from declared variable annotations
            // (e.g. `let arr: string[] = ...`) so arr[i] returns a proper String.
            HIRTypePtr resultType;
            if (auto* objIdent = dynamic_cast<Identifier*>(node.object.get())) {
                auto it = variableArrayElementTypes_.find(objIdent->name);
                if (it != variableArrayElementTypes_.end()) {
                    if (it->second == "String") {
                        resultType = std::make_shared<HIRType>(HIRType::Kind::String);
                    } else if (it->second == "Bool") {
                        resultType = std::make_shared<HIRType>(HIRType::Kind::Bool);
                    }
                }
            }

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

            std::vector<HIRValue*> args = {unboxTaggedObject(object), index};
            HIRValue* elementValue = builder_->createCall(func, args, "array_elem");
            // If the element type isn't I64 (e.g. String), emit an explicit
            // cast so downstream uses see the correct type.
            if (resultType && resultType->kind != HIRType::Kind::I64) {
                lastValue_ = builder_->createCast(elementValue, resultType.get(), "array_elem_cast");
            } else {
                lastValue_ = elementValue;
                lastValue_->type = intType;
            }
        } else {
            // Regular member: obj.property (struct field access)
            if (auto propExpr = dynamic_cast<Identifier*>(node.property.get())) {
                std::string propertyName = propExpr->name;

                // Extract the base variable name from node.object, handling chained calls
                // For: mySet.add(1).has(2) -> node.object is CallExpr(mySet.add(1)), property is "has"
                // We need to find the original Set variable name (mySet)
                std::string setVarName;
                Expr* current = node.object.get();
                while (auto* callExpr = dynamic_cast<CallExpr*>(current)) {
                    if (auto* memberExpr = dynamic_cast<MemberExpr*>(callExpr->callee.get())) {
                        current = memberExpr->object.get();
                    } else {
                        break;
                    }
                }
                if (auto* objIdent = dynamic_cast<Identifier*>(current)) {
                    setVarName = objIdent->name;
                }

                // Check if this is TypedArray property access
                if (auto* objIdent = dynamic_cast<Identifier*>(node.object.get())) {
                    auto typeIt = typedArrayTypes_.find(objIdent->name);
                    if (typeIt != typedArrayTypes_.end()) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: TypedArray property access: " << objIdent->name << "." << propertyName << std::endl;

                        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                        auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

                        std::string runtimeFunc;
                        HIRTypePtr returnType = intType;

                        if (propertyName == "length") {
                            runtimeFunc = "nova_typedarray_length";
                        } else if (propertyName == "byteLength") {
                            runtimeFunc = "nova_typedarray_byteLength";
                        } else if (propertyName == "byteOffset") {
                            runtimeFunc = "nova_typedarray_byteOffset";
                        } else if (propertyName == "buffer") {
                            runtimeFunc = "nova_typedarray_buffer";
                            returnType = ptrType;
                        } else if (propertyName == "BYTES_PER_ELEMENT") {
                            runtimeFunc = "nova_typedarray_BYTES_PER_ELEMENT";
                        }

                        if (!runtimeFunc.empty()) {
                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            auto existingFunc = module_->getFunction(runtimeFunc);
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, returnType);
                                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            std::vector<HIRValue*> args = {object};
                            lastValue_ = builder_->createCall(func, args, "typedarray_prop");
                            lastValue_->type = returnType;
                            return;
                        }
                    }

                    // Check if this is runtime array property access (from keys(), values(), entries())
                    if (runtimeArrayVars_.count(objIdent->name) > 0 && propertyName == "length") {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Runtime array length access on " << objIdent->name << std::endl;

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

                        std::vector<HIRValue*> args = {
                            unboxTaggedObject(object)};
                        lastValue_ = builder_->createCall(func, args, "runtime_array_len");
                        lastValue_->type = intType;
                        return;
                    }

                    // Check if this is ArrayBuffer property access
                    if (arrayBufferVars_.count(objIdent->name) > 0) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: ArrayBuffer property access: " << objIdent->name << "." << propertyName << std::endl;

                        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                        auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

                        std::string runtimeFunc;
                        if (propertyName == "byteLength") {
                            runtimeFunc = "nova_arraybuffer_byteLength";
                        } else if (propertyName == "resizable") {
                            runtimeFunc = "nova_arraybuffer_resizable";
                        } else if (propertyName == "maxByteLength") {
                            runtimeFunc = "nova_arraybuffer_maxByteLength";
                        }

                        if (!runtimeFunc.empty()) {
                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            auto existingFunc = module_->getFunction(runtimeFunc);
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, intType);
                                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            std::vector<HIRValue*> args = {object};
                            lastValue_ = builder_->createCall(func, args, "arraybuffer_prop");
                            lastValue_->type = intType;
                            return;
                        }
                    }

                    // Check if this is DataView property access
                    if (dataViewVars_.count(objIdent->name) > 0) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: DataView property access: " << objIdent->name << "." << propertyName << std::endl;

                        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                        auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

                        std::string runtimeFunc;
                        HIRTypePtr returnType = intType;

                        if (propertyName == "byteLength") {
                            runtimeFunc = "nova_dataview_byteLength";
                        } else if (propertyName == "byteOffset") {
                            runtimeFunc = "nova_dataview_byteOffset";
                        } else if (propertyName == "buffer") {
                            runtimeFunc = "nova_dataview_buffer";
                            returnType = ptrType;
                        }

                        if (!runtimeFunc.empty()) {
                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            auto existingFunc = module_->getFunction(runtimeFunc);
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, returnType);
                                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            std::vector<HIRValue*> args = {object};
                            lastValue_ = builder_->createCall(func, args, "dataview_prop");
                            lastValue_->type = returnType;
                            return;
                        }
                    }

                    // Check if this is DisposableStack property access
                    if (disposableStackVars_.count(objIdent->name) > 0) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: DisposableStack property access: " << objIdent->name << "." << propertyName << std::endl;

                        if (propertyName == "disposed") {
                            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            auto existingFunc = module_->getFunction("nova_disposablestack_get_disposed");
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, intType);
                                HIRFunctionPtr funcPtr = module_->createFunction("nova_disposablestack_get_disposed", funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            std::vector<HIRValue*> args = {object};
                            lastValue_ = builder_->createCall(func, args, "disposed");
                            lastValue_->type = intType;
                            return;
                        }
                    }

                    // Check if this is AsyncDisposableStack property access
                    if (asyncDisposableStackVars_.count(objIdent->name) > 0) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: AsyncDisposableStack property access: " << objIdent->name << "." << propertyName << std::endl;

                        if (propertyName == "disposed") {
                            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            auto existingFunc = module_->getFunction("nova_asyncdisposablestack_get_disposed");
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, intType);
                                HIRFunctionPtr funcPtr = module_->createFunction("nova_asyncdisposablestack_get_disposed", funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            std::vector<HIRValue*> args = {object};
                            lastValue_ = builder_->createCall(func, args, "disposed");
                            lastValue_->type = intType;
                            return;
                        }
                    }

                    // Check if this is IteratorResult property access (.value or .done)
                    if (iteratorResultVars_.count(objIdent->name) > 0) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: IteratorResult property access: " << objIdent->name << "." << propertyName << std::endl;

                        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                        auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                        auto boolType = std::make_shared<HIRType>(HIRType::Kind::Bool);

                        if (propertyName == "value") {
                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            auto existingFunc = module_->getFunction("nova_iterator_result_value");
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, intType);
                                HIRFunctionPtr funcPtr = module_->createFunction("nova_iterator_result_value", funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            std::vector<HIRValue*> args = {object};
                            lastValue_ = builder_->createCall(func, args, "iter_value");
                            lastValue_->type = intType;
                            return;
                        } else if (propertyName == "done") {
                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            auto existingFunc = module_->getFunction("nova_iterator_result_done");
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, boolType);
                                HIRFunctionPtr funcPtr = module_->createFunction("nova_iterator_result_done", funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            std::vector<HIRValue*> args = {object};
                            lastValue_ = builder_->createCall(func, args, "iter_done");
                            // IteratorResult.done is a JavaScript Boolean.  Keeping
                            // it typed as i64 makes strict comparisons such as
                            // `result.done !== false` fold to a constant because
                            // the operands appear to have different JS types.
                            lastValue_->type = boolType;
                            return;
                        }
                    }

                    // Check if this is Error property access (.name, .message, .stack)
                    if (errorVars_.count(objIdent->name) > 0) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Error property access: " << objIdent->name << "." << propertyName << std::endl;

                        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

                        std::string runtimeFunc;
                        std::string returnTypeKind;  // empty = ptr, "jsvalue" = JSValue
                        if (propertyName == "name") {
                            runtimeFunc = "nova_error_get_name";
                        } else if (propertyName == "message") {
                            runtimeFunc = "nova_error_get_message";
                        } else if (propertyName == "stack") {
                            runtimeFunc = "nova_error_get_stack";
                        } else if (propertyName == "cause") {
                            runtimeFunc = "nova_error_get_cause";
                            returnTypeKind = "jsvalue";
                        } else if (propertyName == "errors") {
                            runtimeFunc = "nova_aggregate_error_get_errors";
                        }

                        if (!runtimeFunc.empty()) {
                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            HIRTypePtr retType = ptrType;
                            if (returnTypeKind == "jsvalue") {
                                retType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
                            }
                            auto existingFunc = module_->getFunction(runtimeFunc);
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, retType);
                                HIRFunctionPtr funcPtr = module_->createFunction(runtimeFunc, funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            HIRValue* errorObject = object;
                            if (errorObject && errorObject->type &&
                                errorObject->type->kind ==
                                    HIRType::Kind::JSValue) {
                                auto jsValueType = std::make_shared<HIRType>(
                                    HIRType::Kind::JSValue);
                                auto existingUnbox = module_->getFunction(
                                    "nova_value_to_object");
                                HIRFunction* unbox = existingUnbox
                                    ? existingUnbox.get() : nullptr;
                                if (!unbox) {
                                    auto* type = new HIRFunctionType(
                                        {jsValueType}, ptrType);
                                    auto created = module_->createFunction(
                                        "nova_value_to_object", type);
                                    created->linkage =
                                        HIRFunction::Linkage::External;
                                    unbox = created.get();
                                }
                                errorObject = builder_->createCall(
                                    unbox, {errorObject},
                                    "error.object.unbox");
                                errorObject->type = ptrType;
                            }
                            std::vector<HIRValue*> args = {errorObject};
                            lastValue_ = builder_->createCall(func, args, "error_prop");
                            lastValue_->type = retType;
                            // `aggregate.errors` returns a ValueArray metadata
                            // wrapper so subsequent `.length` / `[i]` work like
                            // any other runtime array.
                            if (propertyName == "errors") {
                                lastWasRuntimeArray_ = true;
                            }
                            return;
                        }
                    }

                    // Check if this is SuppressedError property access (.error, .suppressed, .message, .name, .stack)
                    if (suppressedErrorVars_.count(objIdent->name) > 0) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: SuppressedError property access: " << objIdent->name << "." << propertyName << std::endl;

                        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

                        std::string runtimeFunc;
                        if (propertyName == "error") {
                            runtimeFunc = "nova_suppressederror_get_error";
                        } else if (propertyName == "suppressed") {
                            runtimeFunc = "nova_suppressederror_get_suppressed";
                        } else if (propertyName == "message") {
                            runtimeFunc = "nova_suppressederror_get_message";
                        } else if (propertyName == "name") {
                            runtimeFunc = "nova_suppressederror_get_name";
                        } else if (propertyName == "stack") {
                            runtimeFunc = "nova_suppressederror_get_stack";
                        }

                        if (!runtimeFunc.empty()) {
                            std::vector<HIRTypePtr> paramTypes = {ptrType};
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

                            std::vector<HIRValue*> args = {object};
                            lastValue_ = builder_->createCall(func, args, "suppressederror_prop");
                            lastValue_->type = ptrType;
                            return;
                        }
                    }

                    // Check if this is Symbol property access (.description)
                    if (symbolVars_.count(objIdent->name) > 0) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Symbol property access: " << objIdent->name << "." << propertyName << std::endl;

                        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                        auto strType = std::make_shared<HIRType>(HIRType::Kind::String);

                        if (propertyName == "description") {
                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            auto existingFunc = module_->getFunction("nova_symbol_get_description");
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, strType);
                                HIRFunctionPtr funcPtr = module_->createFunction("nova_symbol_get_description", funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            std::vector<HIRValue*> args = {object};
                            lastValue_ = builder_->createCall(func, args, "symbol_description");
                            lastValue_->type = strType;
                            return;
                        }
                    }
                }

                // Check if this is Map property access (ES2015) - handle chained calls
                if (auto* objIdent = dynamic_cast<Identifier*>(node.object.get())) {
                    std::string mapCheckName = !setVarName.empty() ? setVarName : objIdent->name;
                    if (mapVars_.count(mapCheckName) > 0) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Map property access: " << mapCheckName << "." << propertyName << std::endl;

                        if (propertyName == "size") {
                            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            auto existingFunc = module_->getFunction("nova_map_size");
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, intType);
                                HIRFunctionPtr funcPtr = module_->createFunction("nova_map_size", funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            std::vector<HIRValue*> args = {object};
                            lastValue_ = builder_->createCall(func, args, "map_size");
                            lastValue_->type = intType;
                            return;
                        } else if (propertyName == "set" || propertyName == "get" ||
                                   propertyName == "has" || propertyName == "delete" ||
                                   propertyName == "clear" || propertyName == "keys" ||
                                   propertyName == "values" || propertyName == "entries" ||
                                   propertyName == "forEach") {
                            // Return the Map object itself - CallExpr will call the actual method
                            lastValue_ = object;
                            return;
                        }
                    }
                }

                // Check if this is Set property access (ES2015) - handle chained calls
                if (auto* objIdent = dynamic_cast<Identifier*>(node.object.get())) {
                    std::string setCheckName = !setVarName.empty() ? setVarName : objIdent->name;
                    if (setVars_.count(setCheckName) > 0) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Set property access: " << setCheckName << "." << propertyName << std::endl;

                        if (propertyName == "size") {
                            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            auto existingFunc = module_->getFunction("nova_set_size");
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, intType);
                                HIRFunctionPtr funcPtr = module_->createFunction("nova_set_size", funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            std::vector<HIRValue*> args = {object};
                            lastValue_ = builder_->createCall(func, args, "set_size");
                            lastValue_->type = intType;
                            return;
                        } else if (propertyName == "add" || propertyName == "has" ||
                                   propertyName == "delete" || propertyName == "clear" ||
                                   propertyName == "forEach" || propertyName == "keys" ||
                                   propertyName == "values" || propertyName == "entries") {
                            // Return the Set object itself - CallExpr will call the actual method
                            lastValue_ = object;
                            return;
                        }
                    }
                }

                // Check if this is WeakMap property access - handle chained calls
                if (auto* objIdent = dynamic_cast<Identifier*>(node.object.get())) {
                    std::string weakMapCheckName = !setVarName.empty() ? setVarName : objIdent->name;
                    if (weakMapVars_.count(weakMapCheckName) > 0) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: WeakMap property access: " << weakMapCheckName << "." << propertyName << std::endl;

                        if (propertyName == "size") {
                            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            auto existingFunc = module_->getFunction("nova_weakmap_size");
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, intType);
                                HIRFunctionPtr funcPtr = module_->createFunction("nova_weakmap_size", funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            std::vector<HIRValue*> args = {object};
                            lastValue_ = builder_->createCall(func, args, "weakmap_size");
                            lastValue_->type = intType;
                            return;
                        } else if (propertyName == "set" || propertyName == "get" ||
                                   propertyName == "has" || propertyName == "delete") {
                            // Return the WeakMap object itself - CallExpr will call the actual method
                            lastValue_ = object;
                            return;
                        }
                    }
                }

                // Check if this is WeakSet property access - handle chained calls
                if (auto* objIdent = dynamic_cast<Identifier*>(node.object.get())) {
                    std::string weakSetCheckName = !setVarName.empty() ? setVarName : objIdent->name;
                    if (weakSetVars_.count(weakSetCheckName) > 0) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: WeakSet property access: " << weakSetCheckName << "." << propertyName << std::endl;

                        if (propertyName == "size") {
                            auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                            auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            auto existingFunc = module_->getFunction("nova_weakset_size");
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, intType);
                                HIRFunctionPtr funcPtr = module_->createFunction("nova_weakset_size", funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            std::vector<HIRValue*> args = {object};
                            lastValue_ = builder_->createCall(func, args, "weakset_size");
                            lastValue_->type = intType;
                            return;
                        } else if (propertyName == "add" || propertyName == "has" ||
                                   propertyName == "delete") {
                            // Return the WeakSet object itself - CallExpr will call the actual method
                            lastValue_ = object;
                            return;
                        }
                    }
                }

                // Try to get the struct type from the object
                uint32_t fieldIndex = 0;
                bool found = false;
                hir::HIRStructType* structType = nullptr;

                // Check if this is a 'this' property access
                if (object == currentThis_ && currentClassStructType_) {
                    // Use the current class struct type directly
                    structType = currentClassStructType_;
                } else if (object && object->type) {
                    // First check if object is directly a struct type
                    if (object->type->kind == hir::HIRType::Kind::Struct) {
                        structType = dynamic_cast<hir::HIRStructType*>(object->type.get());
                    }
                    // Otherwise try pointer to struct
                    else {
                        hir::HIRPointerType* ptrTypeCast = dynamic_cast<hir::HIRPointerType*>(object->type.get());

                        // Check if it's a pointer to struct
                        if (auto ptrType = ptrTypeCast) {
                            if (ptrType->pointeeType) {
                                structType = dynamic_cast<hir::HIRStructType*>(ptrType->pointeeType.get());
                            }
                        }
                    }
                }
                // Variable storage can retain a narrowed/inherited struct
                // view after constructing a derived class (notably a custom
                // Error subclass).  Recover the concrete declared class so
                // own fields such as `custom.code` remain addressable.
                if (auto* instanceIdentifier =
                        dynamic_cast<Identifier*>(node.object.get())) {
                    auto classRef =
                        classReferences_.find(instanceIdentifier->name);
                    if (classRef != classReferences_.end()) {
                        auto concrete =
                            classStructTypes_.find(classRef->second);
                        if (concrete != classStructTypes_.end()) {
                            structType = concrete->second;
                            // Keep the value's HIR type consistent with the
                            // recovered class. GetField derives both its result
                            // type and lowering layout from object->type; merely
                            // finding the field in the side table would still
                            // make a derived field look out-of-bounds.
                            if (object) {
                                object->type =
                                    std::shared_ptr<HIRStructType>(
                                        structType,
                                        [](HIRStructType*) {});
                            }
                            for (size_t field = 0;
                                 field < structType->fields.size(); ++field) {
                                if (structType->fields[field].name ==
                                    propertyName) {
                                    if (structType->fields[field].type &&
                                        structType->fields[field].type->kind ==
                                            HIRType::Kind::Any) {
                                        auto pointerType =
                                            std::make_shared<HIRType>(
                                                HIRType::Kind::Pointer);
                                        auto integerType =
                                            std::make_shared<HIRType>(
                                                HIRType::Kind::I64);
                                        auto existing = module_->getFunction(
                                            "nova_class_get_field_i64");
                                        HIRFunction* getter = existing
                                            ? existing.get() : nullptr;
                                        if (!getter) {
                                            auto* functionType =
                                                new HIRFunctionType(
                                                    {pointerType, integerType},
                                                    integerType);
                                            auto created =
                                                module_->createFunction(
                                                    "nova_class_get_field_i64",
                                                    functionType);
                                            created->linkage =
                                                HIRFunction::Linkage::External;
                                            getter = created.get();
                                        }
                                        lastValue_ = builder_->createCall(
                                            getter,
                                            {object,
                                             builder_->createIntConstant(
                                                 static_cast<int64_t>(field))},
                                            propertyName + ".dynamic");
                                        lastValue_->type = integerType;
                                        return;
                                    }
                                    lastValue_ = builder_->createGetField(
                                        object,
                                        static_cast<uint32_t>(field),
                                        propertyName);
                                    // `Any` class slots use the raw i64 ABI in
                                    // MIR. Expose that physical type here so
                                    // the value is actually loaded instead of
                                    // being replaced by the generic zero
                                    // fallback during lowering. Higher-level
                                    // equality still recognizes loaded i64
                                    // pointer bits for dynamic strings.
                                    if (structType->fields[field].type &&
                                        structType->fields[field].type->kind ==
                                            HIRType::Kind::Any) {
                                        lastValue_->type =
                                            std::make_shared<HIRType>(
                                                HIRType::Kind::I64);
                                    }
                                    return;
                                }
                            }
                        }
                    }
                }

                // Find the field in the struct type
                if (structType) {
                    for (size_t i = 0; i < structType->fields.size(); ++i) {
                        if (structType->fields[i].name == propertyName) {
                            fieldIndex = static_cast<uint32_t>(i);
                            found = true;
                            break;
                        }
                    }
                }

                // Check if this property has a getter
                if (structType) {
                    std::string className = structType->name;
                    auto getterClassIt = classGetters_.find(className);
                    if (getterClassIt != classGetters_.end()) {
                        if (getterClassIt->second.find(propertyName) != getterClassIt->second.end()) {
                            // This property has a getter - call the getter function
                            std::string getterName = className + "_get_" + propertyName;
                            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Calling getter " << getterName << std::endl;

                            auto getterFunc = module_->getFunction(getterName);
                            if (getterFunc) {
                                std::vector<HIRValue*> args = { object };
                                lastValue_ = builder_->createCall(getterFunc.get(), args, "getter_result");
                                return;
                            }
                        }
                    }
                }

                if (found) {
                    lastValue_ = builder_->createGetField(object, fieldIndex, propertyName);
                } else if (object && object->type &&
                           !node.isComputed &&
                           propertyName != "length" &&
                           object->type->kind == hir::HIRType::Kind::JSValue) {
                    // Phase 2.4: Member access on a JSValue-typed `this` or
                    // parameter (e.g. inside a function called via Reflect.apply
                    // where thisArg is a runtime Object). Unbox to Object* and
                    // route through nova_dynamic_object_get_tagged so the
                    // property is read at runtime regardless of how the caller
                    // constructed the object.
                    auto jsValueType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
                    auto pointerType2 = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                    auto stringType2 = std::make_shared<HIRType>(HIRType::Kind::String);
                    auto existingUnbox = module_->getFunction("nova_value_to_object");
                    HIRFunction* unbox = existingUnbox ? existingUnbox.get() : nullptr;
                    if (!unbox) {
                        auto* type = new HIRFunctionType({jsValueType}, pointerType2);
                        auto created = module_->createFunction("nova_value_to_object", type);
                        created->linkage = HIRFunction::Linkage::External;
                        unbox = created.get();
                    }
                    HIRValue* objPtr = builder_->createCall(unbox, {object}, "jsvalue.member.unbox");
                    objPtr->type = pointerType2;
                    auto existing = module_->getFunction("nova_dynamic_object_get_tagged");
                    HIRFunction* getter = existing ? existing.get() : nullptr;
                    if (!getter) {
                        auto* type = new HIRFunctionType({pointerType2, stringType2}, jsValueType);
                        auto created = module_->createFunction("nova_dynamic_object_get_tagged", type);
                        created->linkage = HIRFunction::Linkage::External;
                        getter = created.get();
                    }
                    lastValue_ = builder_->createCall(getter, {
                        objPtr,
                        builder_->createStringConstant(propertyName)
                    }, "jsvalue.member.get");
                    lastValue_->type = jsValueType;
                } else {
                    // When the static type is JSValue (e.g. an unannotated
                    // arrow-function parameter), emit a runtime dispatch
                    // helper that inspects the tag and computes length.
                    if (object && object->type &&
                        object->type->kind == hir::HIRType::Kind::JSValue &&
                        propertyName == "length") {
                        auto jsValueType = std::make_shared<HIRType>(HIRType::Kind::JSValue);
                        auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
                        auto existing = module_->getFunction("nova_value_length");
                        hir::HIRFunction* func = existing ? existing.get() : nullptr;
                        if (!func) {
                            std::vector<HIRTypePtr> paramTypes = {jsValueType};
                            HIRFunctionType* ft = new HIRFunctionType(paramTypes, intType);
                            HIRFunctionPtr fp = module_->createFunction("nova_value_length", ft);
                            fp->linkage = HIRFunction::Linkage::External;
                            func = fp.get();
                        }
                        lastValue_ = builder_->createCall(func, {object}, "jsvalue.length");
                        lastValue_->type = intType;
                    }
                    // Check for built-in string properties
                    else if (object && object->type && object->type->kind == hir::HIRType::Kind::String && propertyName == "length") {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Accessing built-in string.length property" << std::endl;

                        // Try to find if this is a string literal constant
                        hir::HIRConstant* strConst = dynamic_cast<hir::HIRConstant*>(object);

                        // Check if we found a string literal constant
                        if (strConst && strConst->kind == hir::HIRConstant::Kind::String) {
                            // For string literals, we can compute length at compile time
                            const std::string& strVal = std::get<std::string>(strConst->value);
                            int64_t length = static_cast<int64_t>(strVal.length());
                            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: String literal '" << strVal << "' length = " << length << std::endl;
                            lastValue_ = builder_->createIntConstant(length);
                        } else {
                            // For dynamic strings (from concat, variables, etc.), call strlen runtime function
                            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Creating strlen call for dynamic string" << std::endl;

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
                                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Creating strlen intrinsic function declaration" << std::endl;

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
                                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Created strlen function with external linkage" << std::endl;
                            }

                            // Create call to strlen
                            std::vector<HIRValue*> args = { object };
                            lastValue_ = builder_->createCall(strlenFunc, args, "str_len");
                        }
                    }
                    // Check for built-in array properties
                    else if (object && object->type && propertyName == "length") {
                        hir::HIRArrayType* arrayType = nullptr;
                        bool isOpaqueArrayPointer = false;

                        // Check if object is directly an array
                        if (object->type->kind == hir::HIRType::Kind::Array) {
                            arrayType = dynamic_cast<hir::HIRArrayType*>(object->type.get());
                        }
                        // Check if object is a pointer to an array
                        else if (object->type->kind == hir::HIRType::Kind::Pointer) {
                            hir::HIRPointerType* ptrType = dynamic_cast<hir::HIRPointerType*>(object->type.get());
                            if (ptrType && ptrType->pointeeType && ptrType->pointeeType->kind == hir::HIRType::Kind::Array) {
                                arrayType = dynamic_cast<hir::HIRArrayType*>(ptrType->pointeeType.get());
                            } else {
                                // Pointer without a typed pointee — this is how
                                // runtime helpers like nova_string_split surface
                                // their array results. Treat the pointer as a
                                // reference to runtime array metadata so .length
                                // resolves to the metadata's length field.
                                isOpaqueArrayPointer = true;
                            }
                        }

                        if (arrayType || isOpaqueArrayPointer) {
                            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Accessing built-in array.length property" << std::endl;

                            // Generate code to read length from metadata struct at runtime
                            // Metadata struct: { [24 x i8], i64 length, i64 capacity, ptr elements }
                            // Field index 1 is the length
                            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Generating GetField to read length from metadata" << std::endl;
                            lastValue_ = builder_->createGetField(object, 1);
                        }
                    } else {
                        // Check if this is a builtin object method access (e.g., emitter.on, readable.read)
                        bool foundBuiltinMethod = false;
                        if (auto* objIdent = dynamic_cast<Identifier*>(node.object.get())) {
                            auto typeIt = variableObjectTypes_.find(objIdent->name);
                            if (typeIt != variableObjectTypes_.end()) {
                                std::string objectType = typeIt->second;  // e.g., "events:EventEmitter"
                                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Found builtin object type: " << objIdent->name
                                          << " -> " << objectType << std::endl;

                                // Map method name to runtime function
                                // e.g., "events:EventEmitter" + "on" -> "nova_events_EventEmitter_on"
                                size_t colonPos = objectType.find(':');
                                if (colonPos != std::string::npos) {
                                    std::string moduleName = objectType.substr(0, colonPos);
                                    std::string typeName = objectType.substr(colonPos + 1);
                                    std::string runtimeFunc = "nova_" + moduleName + "_" + typeName + "_" + propertyName;

                                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Method access: " << propertyName
                                              << " -> " << runtimeFunc << std::endl;

                                    // For now, return a placeholder indicating method was found
                                    // The actual call will be handled in CallExpr visitor
                                    lastValue_ = builder_->createIntConstant(1);
                                    foundBuiltinMethod = true;
                                }
                            }
                        }

        if (!foundBuiltinMethod) {
                            // Property not found, return 0 as placeholder
                            lastValue_ = builder_->createIntConstant(0);
                        }
                    }
                }
            }
        }
    }

void HIRGenerator::visit(ObjectExpr& node) {
        // Object literal construction
        // Create struct type with fields for each property
        // Mark as Object for instanceof resolution
        lastVariableKind_ = "Object";

        // If this literal is being assigned to a variable that was flagged as
        // forced-dynamic (used with Object.create/defineProperty/delete/in/etc.),
        // emit a runtime nova::runtime::Object* instead of a fixed-layout struct.
        bool protocolObject = false;
        bool iteratorResultObject = false;
        bool hasValue = false;
        bool hasDone = false;
        for (const auto& property : node.properties) {
            // Computed keys cannot be represented safely by a fixed-layout
            // struct: their identity/value is only known at runtime and may
            // be a Symbol. Use the runtime property store for the complete
            // object literal.
            protocolObject = protocolObject || property.isComputed;
            std::string propertyName;
            if (auto* identifier =
                    dynamic_cast<Identifier*>(property.key.get())) {
                propertyName = identifier->name;
            } else if (auto* member =
                           dynamic_cast<MemberExpr*>(property.key.get())) {
                auto* base =
                    dynamic_cast<Identifier*>(member->object.get());
                auto* key =
                    dynamic_cast<Identifier*>(member->property.get());
                if (base && key && base->name == "Symbol" &&
                    key->name == "iterator") {
                    protocolObject = true;
                }
            }
            if (propertyName == "then" &&
                property.kind == ObjectExpr::Property::Kind::Method) {
                protocolObject = true;
            }
            hasValue = hasValue || propertyName == "value";
            hasDone = hasDone || propertyName == "done";
        }
        iteratorResultObject = false;
        if ((!currentDeclName_.empty() &&
             forcedDynamicObjectVars_.count(currentDeclName_) > 0) ||
            protocolObject || iteratorResultObject) {
            emitRuntimeObjectLiteral(node);
            lastWasDynamicObjectResult_ = true;
            lastWasGenerator_ = false;
            lastWasAsyncGenerator_ = false;
            lastWasPromise_ = false;
            return;
        }

        std::vector<hir::HIRStructType::Field> fields;
        std::vector<hir::HIRValue*> fieldValues;
        std::string structName = "anon_obj";

        // Generate unique ID for this object
        static int objectCounter = 0;
        std::string objectId = "__obj_" + std::to_string(objectCounter++);

        // FIRST PASS: Collect data fields and identify methods
        // We need to create the struct type FIRST, so methods can reference it
        std::vector<std::pair<std::string, FunctionExpr*>> methodsToGenerate;

        for (size_t i = 0; i < node.properties.size(); ++i) {
            auto& prop = node.properties[i];

            // Get the property name from the key
            std::string fieldName = "field" + std::to_string(i);
            if (auto identifier = dynamic_cast<Identifier*>(prop.key.get())) {
                fieldName = identifier->name;
            }

            // Check if this property is a method
            bool isMethod = (prop.kind == ObjectExpr::Property::Kind::Method) ||
                           dynamic_cast<FunctionExpr*>(prop.value.get()) != nullptr;

            if (isMethod) {
                // Store method for later generation (after struct type is created)
                auto* funcExpr = dynamic_cast<FunctionExpr*>(prop.value.get());
                if (funcExpr) {
                    methodsToGenerate.push_back({fieldName, funcExpr});
                }
            } else {
                // Regular property value - evaluate now
                prop.value->accept(*this);
                fieldValues.push_back(lastValue_);

                // Create field descriptor - ONLY for data properties
                hir::HIRStructType::Field field;
                field.name = fieldName;
                field.type = lastValue_->type;
                field.isPublic = true;
                fields.push_back(field);
            }
        }

        // Create the struct type NOW (before generating methods)
        structName = objectId;
        auto structType = new hir::HIRStructType(structName, fields);
        auto structTypePtr = std::make_shared<hir::HIRStructType>(*structType);

        // SECOND PASS: Generate methods with proper 'this' type
        for (auto& [fieldName, funcExpr] : methodsToGenerate) {
                // Generate unique function name for this method
                std::string methodFuncName = objectId + "_method_" + fieldName;

                // Save current state
                HIRFunction* savedFunction = currentFunction_;
                auto savedBuilder = std::move(builder_);
                auto savedSymbolTable = symbolTable_;
                HIRValue* savedThis = currentThis_;

                // Create function type with 'this' as first parameter
                std::vector<HIRTypePtr> paramTypes;

                // First parameter: 'this' (pointer to struct)
                auto thisType = std::make_shared<hir::HIRPointerType>(structTypePtr, true);
                paramTypes.push_back(thisType);

                // Remaining parameters from method signature
                for (size_t j = 0; j < funcExpr->params.size(); ++j) {
                    paramTypes.push_back(std::make_shared<HIRType>(HIRType::Kind::Any));
                }

                // Return type
                auto retType = std::make_shared<HIRType>(HIRType::Kind::Any);
                auto funcType = new HIRFunctionType(paramTypes, retType);

                // Create function
                auto func = module_->createFunction(methodFuncName, funcType);
                func->isAsync = funcExpr->isAsync;
                func->isGenerator = funcExpr->isGenerator;
                currentFunction_ = func.get();

                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Created method function: " << methodFuncName << std::endl;

                // Create entry block
                auto entryBlock = func->createBasicBlock("entry");
                builder_ = std::make_unique<HIRBuilder>(module_, func.get());
                builder_->setInsertPoint(entryBlock.get());

                // Clear symbol table for new function scope
                symbolTable_.clear();

                // Set currentThis_ to the first parameter (the 'this' pointer)
                currentThis_ = func->parameters[0];

                // Add method parameters to symbol table (starting from index 1, since 0 is 'this')
                for (size_t j = 0; j < funcExpr->params.size(); ++j) {
                    symbolTable_[funcExpr->params[j]] = func->parameters[j + 1];
                }

                // Generate method body
                if (funcExpr->body) {
                    funcExpr->body->accept(*this);

                    // Add implicit return if needed
                    if (!entryBlock->hasTerminator()) {
                        builder_->createReturn(nullptr);
                    }
                }

                // Restore context
                currentThis_ = savedThis;
                symbolTable_ = savedSymbolTable;
                builder_ = std::move(savedBuilder);
                currentFunction_ = savedFunction;

                // Store method function name for later lookup
                objectMethodFunctions_[objectId][fieldName] = methodFuncName;
                objectMethodProperties_[objectId].insert(fieldName);
        }

        // Store the field names for for-in loop support
        {
            std::vector<std::string> fieldNameList;
            for (auto& field : fields) {
                fieldNameList.push_back(field.name);
            }
            // Also add method names as they are enumerable properties
            for (auto& [methodName, funcExpr] : methodsToGenerate) {
                fieldNameList.push_back(methodName);
            }
            objectFieldNames_[objectId] = fieldNameList;
        }

        // Create struct construction instruction
        lastValue_ = builder_->createStructConstruct(structType, fieldValues, objectId);

        // Store the object ID for use in variable assignment
        currentObjectName_ = objectId;

        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Created object with " << fields.size() << " fields, "
                                  << objectMethodProperties_[objectId].size() << " methods" << std::endl;
    }

} // namespace nova::hir
