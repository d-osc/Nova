// HIRGen_Objects.cpp - Object and member expression visitors
// Extracted from HIRGen.cpp for better code organization

#include "nova/HIR/HIRGen_Internal.h"
#define NOVA_DEBUG 0

namespace nova::hir {

void HIRGenerator::visit(MemberExpr& node) {
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
                        lastValue_ = builder_->createIntConstant(3);
                        return;
                    } else if (propIdent->name == "E") {
                        // Math.E ≈ 2.71828... -> return 2 for integer (or 3 if you prefer rounding)
                        lastValue_ = builder_->createIntConstant(3);
                        return;
                    } else if (propIdent->name == "LN2") {
                        // Math.LN2 ≈ 0.693147... -> return 0 for integer (truncated)
                        lastValue_ = builder_->createIntConstant(0);
                        return;
                    } else if (propIdent->name == "LN10") {
                        // Math.LN10 ≈ 2.302585... -> return 2 for integer (truncated)
                        lastValue_ = builder_->createIntConstant(2);
                        return;
                    } else if (propIdent->name == "LOG2E") {
                        // Math.LOG2E ≈ 1.442695... -> return 1 for integer (truncated)
                        lastValue_ = builder_->createIntConstant(1);
                        return;
                    } else if (propIdent->name == "LOG10E") {
                        // Math.LOG10E ≈ 0.434294... -> return 0 for integer (truncated)
                        lastValue_ = builder_->createIntConstant(0);
                        return;
                    } else if (propIdent->name == "SQRT1_2") {
                        // Math.SQRT1_2 ≈ 0.707106... -> return 0 for integer (truncated)
                        lastValue_ = builder_->createIntConstant(0);
                        return;
                    } else if (propIdent->name == "SQRT2") {
                        // Math.SQRT2 ≈ 1.414213... -> return 1 for integer (truncated)
                        lastValue_ = builder_->createIntConstant(1);
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
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Enum access " << objIdent->name << "." << propIdent->name << " = " << memberIt->second << std::endl;
                        lastValue_ = builder_->createIntConstant(memberIt->second);
                        return;
                    }
                }

                // Check for static property access (e.g., Config.version)
                auto staticClassIt = classStaticProps_.find(objIdent->name);
                if (staticClassIt != classStaticProps_.end()) {
                    if (staticClassIt->second.find(propIdent->name) != staticClassIt->second.end()) {
                        std::string propKey = objIdent->name + "_" + propIdent->name;
                        auto valueIt = staticPropertyValues_.find(propKey);
                        if (valueIt != staticPropertyValues_.end()) {
                            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Static property access " << propKey << " = " << valueIt->second << std::endl;
                            lastValue_ = builder_->createIntConstant(valueIt->second);
                            return;
                        }
                    }
                }
            }
        }

        // Evaluate the object/array
        node.object->accept(*this);
        auto object = lastValue_;

        if (node.isComputed) {
            // Computed member: obj[property] e.g., arr[index]
            node.property->accept(*this);
            auto index = lastValue_;

            // Check if this is runtime array element access (from keys(), values(), entries())
            if (auto* objIdent = dynamic_cast<Identifier*>(node.object.get())) {
                if (runtimeArrayVars_.count(objIdent->name) > 0) {
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Runtime array element access on " << objIdent->name << std::endl;

                    auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                    auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

                    // Use nova_value_array_at for runtime arrays
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

                    std::vector<HIRValue*> args = {object, index};
                    lastValue_ = builder_->createCall(func, args, "runtime_elem");
                    lastValue_->type = intType;
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

            std::vector<HIRValue*> args = {object, index};
            lastValue_ = builder_->createCall(func, args, "array_elem");
            lastValue_->type = intType;  // Ensure element has correct type!
        } else {
            // Regular member: obj.property (struct field access)
            if (auto propExpr = dynamic_cast<Identifier*>(node.property.get())) {
                std::string propertyName = propExpr->name;

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

                        std::vector<HIRValue*> args = {object};
                        lastValue_ = builder_->createCall(func, args, "runtime_array_len");
                        lastValue_->type = intType;
                        return;
                    }

                    // Check if this is ArrayBuffer property access
                    if (arrayBufferVars_.count(objIdent->name) > 0) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: ArrayBuffer property access: " << objIdent->name << "." << propertyName << std::endl;

                        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);
                        auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);

                        if (propertyName == "byteLength") {
                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            auto existingFunc = module_->getFunction("nova_arraybuffer_byteLength");
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, intType);
                                HIRFunctionPtr funcPtr = module_->createFunction("nova_arraybuffer_byteLength", funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            std::vector<HIRValue*> args = {object};
                            lastValue_ = builder_->createCall(func, args, "arraybuffer_byteLength");
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

                    // Check if this is Map property access (ES2015)
                    if (mapVars_.count(objIdent->name) > 0) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Map property access: " << objIdent->name << "." << propertyName << std::endl;

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
                            lastValue_->type = intType;  // Use i64 for bool compatibility
                            return;
                        }
                    }

                    // Check if this is Error property access (.name, .message, .stack)
                    if (errorVars_.count(objIdent->name) > 0) {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Error property access: " << objIdent->name << "." << propertyName << std::endl;

                        auto ptrType = std::make_shared<HIRType>(HIRType::Kind::Pointer);

                        std::string runtimeFunc;
                        if (propertyName == "name") {
                            runtimeFunc = "nova_error_get_name";
                        } else if (propertyName == "message") {
                            runtimeFunc = "nova_error_get_message";
                        } else if (propertyName == "stack") {
                            runtimeFunc = "nova_error_get_stack";
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
                            lastValue_ = builder_->createCall(func, args, "error_prop");
                            lastValue_->type = ptrType;
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

                        if (propertyName == "description") {
                            std::vector<HIRTypePtr> paramTypes = {ptrType};
                            auto existingFunc = module_->getFunction("nova_symbol_get_description");
                            HIRFunction* func = nullptr;
                            if (existingFunc) {
                                func = existingFunc.get();
                            } else {
                                HIRFunctionType* funcType = new HIRFunctionType(paramTypes, ptrType);
                                HIRFunctionPtr funcPtr = module_->createFunction("nova_symbol_get_description", funcType);
                                funcPtr->linkage = HIRFunction::Linkage::External;
                                func = funcPtr.get();
                            }

                            std::vector<HIRValue*> args = {object};
                            lastValue_ = builder_->createCall(func, args, "symbol_description");
                            lastValue_->type = ptrType;
                            return;
                        }
                    }
                }

                // Try to get the struct type from the object
                uint32_t fieldIndex = 0;
                bool found = false;
                hir::HIRStructType* structType = nullptr;

                std::cerr << "\n=== TRACE: MemberExpr property access ===" << std::endl;
                std::cerr << "  Property name: '" << propertyName << "'" << std::endl;
                std::cerr << "  object pointer: " << object << std::endl;
                std::cerr << "  currentThis_ pointer: " << currentThis_ << std::endl;
                std::cerr << "  object == currentThis_: " << (object == currentThis_ ? "YES" : "NO") << std::endl;
                std::cerr << "  currentClassStructType_: " << currentClassStructType_ << std::endl;

                // Check if this is a 'this' property access
                if (object == currentThis_ && currentClassStructType_) {
                    // Use the current class struct type directly
                    structType = currentClassStructType_;
                    std::cerr << "  TRACE: Using currentClassStructType_ for 'this' property access" << std::endl;
                    std::cerr << "  TRACE: Struct has " << structType->fields.size() << " fields" << std::endl;
                } else if (object && object->type) {
                    std::cerr << "  TRACE: Extracting struct type from object->type" << std::endl;
                    std::cerr << "  TRACE: object->type pointer: " << object->type.get() << std::endl;
                    std::cerr << "  TRACE: object->type->kind = " << static_cast<int>(object->type->kind) << std::endl;

                    // First check if object is directly a struct type
                    if (object->type->kind == hir::HIRType::Kind::Struct) {
                        std::cerr << "  TRACE: Object type is directly a Struct" << std::endl;
                        structType = dynamic_cast<hir::HIRStructType*>(object->type.get());
                        if (structType) {
                            std::cerr << "  TRACE: Successfully cast to HIRStructType" << std::endl;
                            std::cerr << "  TRACE: Struct name: " << structType->name << std::endl;
                            std::cerr << "  TRACE: Struct has " << structType->fields.size() << " fields:" << std::endl;
                            for (size_t i = 0; i < structType->fields.size(); ++i) {
                                std::cerr << "    [" << i << "] " << structType->fields[i].name
                                          << " (kind=" << static_cast<int>(structType->fields[i].type->kind) << ")" << std::endl;
                            }
                        } else {
                            std::cerr << "  TRACE ERROR: Failed to cast to HIRStructType!" << std::endl;
                        }
                    }
                    // Otherwise try pointer to struct
                    else {
                        std::cerr << "  TRACE: Object type is NOT directly a Struct, trying Pointer" << std::endl;
                        // Try to cast to HIRPointerType
                        hir::HIRPointerType* ptrTypeCast = dynamic_cast<hir::HIRPointerType*>(object->type.get());
                        std::cerr << "  TRACE: HIRPointerType cast result: " << ptrTypeCast << std::endl;

                        // Check if it's a pointer to struct
                        if (auto ptrType = ptrTypeCast) {
                            std::cerr << "  TRACE: Successfully cast to HIRPointerType" << std::endl;
                            if (ptrType->pointeeType) {
                                std::cerr << "  TRACE: Pointee type exists, kind=" << static_cast<int>(ptrType->pointeeType->kind) << std::endl;

                                structType = dynamic_cast<hir::HIRStructType*>(ptrType->pointeeType.get());
                                if (structType) {
                                    std::cerr << "  TRACE: Pointee is a struct!" << std::endl;
                                    std::cerr << "  TRACE: Struct name: " << structType->name << std::endl;
                                    std::cerr << "  TRACE: Struct has " << structType->fields.size() << " fields:" << std::endl;
                                    for (size_t i = 0; i < structType->fields.size(); ++i) {
                                        std::cerr << "    [" << i << "] " << structType->fields[i].name
                                                  << " (kind=" << static_cast<int>(structType->fields[i].type->kind) << ")" << std::endl;
                                    }
                                } else {
                                    std::cerr << "  TRACE: Pointee is NOT a struct" << std::endl;
                                }
                            } else {
                                std::cerr << "  TRACE ERROR: Pointer has NULL pointeeType!" << std::endl;
                            }
                        } else {
                            std::cerr << "  TRACE ERROR: Failed to cast to HIRPointerType!" << std::endl;
                        }
                    }
                } else {
                    std::cerr << "  TRACE ERROR: object is NULL or object->type is NULL!" << std::endl;
                    std::cerr << "    object: " << object << std::endl;
                    if (object) {
                        std::cerr << "    object->type: " << object->type.get() << std::endl;
                    }
                }

                // Find the field in the struct type
                std::cerr << "\n  TRACE: Searching for field '" << propertyName << "' in struct..." << std::endl;
                if (structType) {
                    std::cerr << "  TRACE: structType is valid, searching " << structType->fields.size() << " fields" << std::endl;
                    for (size_t i = 0; i < structType->fields.size(); ++i) {
                        std::cerr << "    Checking field[" << i << "]: '" << structType->fields[i].name << "'" << std::endl;
                        if (structType->fields[i].name == propertyName) {
                            fieldIndex = static_cast<uint32_t>(i);
                            found = true;
                            std::cerr << "  TRACE SUCCESS: Found field '" << propertyName << "' at index " << fieldIndex << std::endl;
                            break;
                        }
                    }
                    if (!found) {
                        std::cerr << "  TRACE ERROR: Field '" << propertyName << "' NOT FOUND in struct!" << std::endl;
                    }
                } else {
                    std::cerr << "  TRACE ERROR: structType is NULL, cannot search for field!" << std::endl;
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
                    // Create GetField instruction with the correct field index
                    std::cerr << "\n  TRACE: Creating GetField operation" << std::endl;
                    std::cerr << "    object pointer: " << object << std::endl;
                    std::cerr << "    field index: " << fieldIndex << std::endl;
                    std::cerr << "    field name: '" << propertyName << "'" << std::endl;

                    lastValue_ = builder_->createGetField(object, fieldIndex, propertyName);

                    std::cerr << "    Result lastValue_: " << lastValue_ << std::endl;
                    if (lastValue_ && lastValue_->type) {
                        std::cerr << "    Result type kind: " << static_cast<int>(lastValue_->type->kind) << std::endl;
                    }
                    std::cerr << "=== END TRACE ===" << std::endl;
                } else {
                    std::cerr << "\n  TRACE: Field NOT found, entering fallback logic" << std::endl;
                    // Check for built-in string properties
                    if (object && object->type && object->type->kind == hir::HIRType::Kind::String && propertyName == "length") {
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

                        // Check if object is directly an array
                        if (object->type->kind == hir::HIRType::Kind::Array) {
                            arrayType = dynamic_cast<hir::HIRArrayType*>(object->type.get());
                        }
                        // Check if object is a pointer to an array
                        else if (object->type->kind == hir::HIRType::Kind::Pointer) {
                            hir::HIRPointerType* ptrType = dynamic_cast<hir::HIRPointerType*>(object->type.get());
                            if (ptrType && ptrType->pointeeType && ptrType->pointeeType->kind == hir::HIRType::Kind::Array) {
                                arrayType = dynamic_cast<hir::HIRArrayType*>(ptrType->pointeeType.get());
                            }
                        }

                        if (arrayType) {
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
                            std::cerr << "Warning: Property '" << propertyName << "' not found in struct" << std::endl;
                            if (object && object->type) {
                                std::cerr << "  Object type: kind=" << static_cast<int>(object->type->kind) << std::endl;
                            }
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
        std::vector<hir::HIRStructType::Field> fields;
        std::vector<hir::HIRValue*> fieldValues;
        // Use a placeholder - will be replaced with actual object ID
        std::string structName = "anon_obj";

        // Generate unique ID for this object
        static int objectCounter = 0;
        std::string objectId = "__obj_" + std::to_string(objectCounter++);

        std::cerr << "\n=== TRACE: ObjectExpr processing, ID=" << objectId << " ===" << std::endl;

        // FIRST PASS: Collect data fields and identify methods
        // We need to create the struct type FIRST, so methods can reference it
        std::vector<std::pair<std::string, FunctionExpr*>> methodsToGenerate;

        for (size_t i = 0; i < node.properties.size(); ++i) {
            auto& prop = node.properties[i];

            // Get the property name from the key
            std::string fieldName = "field" + std::to_string(i);  // Default name
            if (auto identifier = dynamic_cast<Identifier*>(prop.key.get())) {
                fieldName = identifier->name;
            }

            // Check if this property is a method
            bool isMethod = (prop.kind == ObjectExpr::Property::Kind::Method) ||
                           dynamic_cast<FunctionExpr*>(prop.value.get()) != nullptr;

            if (isMethod) {
                std::cerr << "  TRACE: Property '" << fieldName << "' is a method - deferring generation" << std::endl;
                // Store method for later generation (after struct type is created)
                auto* funcExpr = dynamic_cast<FunctionExpr*>(prop.value.get());
                if (funcExpr) {
                    methodsToGenerate.push_back({fieldName, funcExpr});
                }
            } else {
                // Regular property value - evaluate now
                std::cerr << "  TRACE: Property '" << fieldName << "' is a data field" << std::endl;
                prop.value->accept(*this);
                fieldValues.push_back(lastValue_);

                // Create field descriptor - ONLY for data properties
                hir::HIRStructType::Field field;
                field.name = fieldName;
                field.type = lastValue_->type;  // Use the value's type
                field.isPublic = true;
                fields.push_back(field);
            }
        }

        // Create the struct type NOW (before generating methods)
        // IMPORTANT: Use objectId as the struct name so LLVM codegen can find it!
        structName = objectId;  // e.g., "__obj_0"
        std::cerr << "  TRACE: Creating struct type '" << structName << "' with " << fields.size() << " data fields" << std::endl;
        auto structType = new hir::HIRStructType(structName, fields);
        auto structTypePtr = std::make_shared<hir::HIRStructType>(*structType);

        // SECOND PASS: Generate methods with proper 'this' type
        for (auto& [fieldName, funcExpr] : methodsToGenerate) {
                std::cerr << "  TRACE: Generating method '" << fieldName << "'" << std::endl;

                // Generate unique function name for this method
                std::string methodFuncName = objectId + "_method_" + fieldName;

                // Save current state
                HIRFunction* savedFunction = currentFunction_;
                auto savedBuilder = std::move(builder_);
                auto savedSymbolTable = symbolTable_;
                HIRValue* savedThis = currentThis_;

                // Create function type with 'this' as first parameter
                std::vector<HIRTypePtr> paramTypes;

                // First parameter: 'this' (pointer to struct) - USE PROPER HIRPointerType!
                auto thisType = std::make_shared<hir::HIRPointerType>(structTypePtr, true);
                std::cerr << "    TRACE: Created HIRPointerType for 'this' parameter" << std::endl;
                std::cerr << "    TRACE: thisType kind: " << static_cast<int>(thisType->kind) << std::endl;
                std::cerr << "    TRACE: thisType->pointeeType: " << thisType->pointeeType.get() << std::endl;
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

                std::cerr << "\n=== TRACE: Object method setup ===" << std::endl;
                std::cerr << "  Method: " << methodFuncName << std::endl;
                std::cerr << "  Set currentThis_ = func->parameters[0]" << std::endl;
                std::cerr << "  currentThis_ pointer: " << currentThis_ << std::endl;
                if (currentThis_->type) {
                    std::cerr << "  currentThis_ type kind: " << static_cast<int>(currentThis_->type->kind) << std::endl;
                    std::cerr << "  currentThis_ type pointer: " << currentThis_->type.get() << std::endl;
                } else {
                    std::cerr << "  ERROR: currentThis_->type is NULL!" << std::endl;
                }
                std::cerr << "=== END TRACE ===" << std::endl;

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

                std::cerr << "    TRACE: Method '" << fieldName << "' generation complete" << std::endl;
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
