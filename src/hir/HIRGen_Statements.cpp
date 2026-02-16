// HIRGen_Statements.cpp - Statement visitors
// Extracted from HIRGen.cpp for better code organization

#include "nova/HIR/HIRGen_Internal.h"
#define NOVA_DEBUG 0

namespace nova::hir {

void HIRGenerator::visit(BlockStmt& node) {
        for (auto& stmt : node.statements) {
            stmt->accept(*this);
        }
    }
    
void HIRGenerator::visit(ExprStmt& node) {
        if (node.expression) {
            node.expression->accept(*this);
        }
    }
    
void HIRGenerator::visit(VarDeclStmt& node) {
        for (auto& decl : node.declarations) {
            // Evaluate the initializer first to get its type
            HIRValue* initValue = nullptr;
            if (decl.init) {
                decl.init->accept(*this);
                initValue = lastValue_;
            }

            // Check if this is a destructuring pattern
            if (decl.pattern) {
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing destructuring pattern" << std::endl;

                // Handle array destructuring: let [a, b, c] = arr;
                if (auto* arrayPattern = dynamic_cast<ArrayPattern*>(decl.pattern.get())) {
                    if(NOVA_DEBUG) std::cerr << "  DEBUG: Array pattern with " << arrayPattern->elements.size() << " elements" << std::endl;

                    for (size_t i = 0; i < arrayPattern->elements.size(); ++i) {
                        auto& element = arrayPattern->elements[i];
                        if (!element) continue;

                        // Get the variable name from IdentifierPattern
                        if (auto* idPattern = dynamic_cast<IdentifierPattern*>(element.get())) {
                            std::string varName = idPattern->name;
                            if(NOVA_DEBUG) std::cerr << "    DEBUG: Element " << i << " -> " << varName << std::endl;

                            // Create index constant
                            auto indexVal = builder_->createIntConstant(static_cast<int64_t>(i));

                            // Get element from array: arr[i]
                            // Use runtime function to ensure correct type (same fix as array access)
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

                            std::vector<HIRValue*> args = {initValue, indexVal};
                            auto elementVal = builder_->createCall(func, args, "destructure_elem");
                            elementVal->type = intType;  // Ensure element has correct type!

                            // Allocate storage for this variable
                            auto alloca = builder_->createAlloca(intType.get(), varName);
                            symbolTable_[varName] = alloca;

                            // Store the element value
                            builder_->createStore(elementVal, alloca);
                        }
                    }
                }
                // Handle object destructuring: let {a, b} = obj;
                else if (auto* objPattern = dynamic_cast<ObjectPattern*>(decl.pattern.get())) {
                    if(NOVA_DEBUG) std::cerr << "  DEBUG: Object pattern with " << objPattern->properties.size() << " properties" << std::endl;

                    // Get the struct type from the initializer object
                    hir::HIRStructType* structType = nullptr;
                    if (initValue && initValue->type) {
                        if (initValue->type->kind == hir::HIRType::Kind::Struct) {
                            structType = dynamic_cast<hir::HIRStructType*>(initValue->type.get());
                        } else if (initValue->type->kind == hir::HIRType::Kind::Pointer) {
                            auto* ptrType = dynamic_cast<hir::HIRPointerType*>(initValue->type.get());
                            if (ptrType && ptrType->pointeeType && ptrType->pointeeType->kind == hir::HIRType::Kind::Struct) {
                                structType = dynamic_cast<hir::HIRStructType*>(ptrType->pointeeType.get());
                            }
                        }
                    }

                    if (!structType && NOVA_DEBUG) {
                        std::cerr << "  ERROR: Object destructuring failed - cannot determine struct type" << std::endl;
                    }

                    for (auto& prop : objPattern->properties) {
                        // Get the variable name
                        std::string varName = prop.key;
                        if (auto* idPattern = dynamic_cast<IdentifierPattern*>(prop.value.get())) {
                            varName = idPattern->name;
                        }
                        std::string propertyName = prop.key;
                        if(NOVA_DEBUG) std::cerr << "    DEBUG: Property " << propertyName << " -> " << varName << std::endl;

                        // Find the field index in the struct
                        uint32_t fieldIndex = 0;
                        bool found = false;
                        HIRTypePtr fieldType = std::make_shared<HIRType>(HIRType::Kind::I64);  // Default type

                        if (structType) {
                            for (size_t i = 0; i < structType->fields.size(); ++i) {
                                if (structType->fields[i].name == propertyName) {
                                    fieldIndex = static_cast<uint32_t>(i);
                                    fieldType = structType->fields[i].type;
                                    found = true;
                                    if(NOVA_DEBUG) std::cerr << "    DEBUG: Found field '" << propertyName << "' at index " << fieldIndex << std::endl;
                                    break;
                                }
                            }
                        }

                        HIRValue* propertyVal = nullptr;
                        if (found) {
                            // Extract property value using GetField
                            propertyVal = builder_->createGetField(initValue, fieldIndex, propertyName);
                            if(NOVA_DEBUG) std::cerr << "    DEBUG: Extracted property value using GetField" << std::endl;
                        } else {
                            // Property not found - use zero/null as default
                            if(NOVA_DEBUG) std::cerr << "    WARNING: Property '" << propertyName << "' not found in object, using zero" << std::endl;
                            propertyVal = builder_->createIntConstant(0);
                        }

                        // Allocate storage and store the property value
                        auto alloca = builder_->createAlloca(fieldType.get(), varName);
                        symbolTable_[varName] = alloca;
                        builder_->createStore(propertyVal, alloca);
                    }
                }
                continue;  // Don't process as normal variable
            }

            // Check if this is a function reference assignment
            if(NOVA_DEBUG) {
                std::cerr << "DEBUG HIRGen: Checking function reference for '" << decl.name
                          << "', lastFunctionName_ = '" << lastFunctionName_ << "'" << std::endl;
            }
            if (!lastFunctionName_.empty()) {
                // Register this variable as holding a function reference
                functionReferences_[decl.name] = lastFunctionName_;
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered function reference: " << decl.name
                          << " -> " << lastFunctionName_ << std::endl;
                lastFunctionName_.clear();  // Clear for next declaration
            }

            // Check if this is a class expression assignment
            if (!lastClassName_.empty()) {
                // Register this variable as holding a class reference
                classReferences_[decl.name] = lastClassName_;
                classNames_.insert(decl.name);  // Register the variable name as a class name
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered class reference: " << decl.name
                          << " -> " << lastClassName_ << std::endl;
                lastClassName_.clear();  // Clear for next declaration
            }

            // Check if this is an object with methods
            if (!currentObjectName_.empty()) {
                // Transfer method mappings from object ID to variable name
                if (objectMethodFunctions_.find(currentObjectName_) != objectMethodFunctions_.end()) {
                    objectMethodFunctions_[decl.name] = objectMethodFunctions_[currentObjectName_];
                    objectMethodProperties_[decl.name] = objectMethodProperties_[currentObjectName_];

                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Associated object methods with variable '"
                                              << decl.name << "'" << std::endl;
                }
                // Transfer field names for for-in loop support
                if (objectFieldNames_.find(currentObjectName_) != objectFieldNames_.end()) {
                    objectFieldNames_[decl.name] = objectFieldNames_[currentObjectName_];
                }
                currentObjectName_.clear();  // Clear for next declaration
            }

            // Check if this is a TypedArray assignment
            if (!lastTypedArrayType_.empty()) {
                typedArrayTypes_[decl.name] = lastTypedArrayType_;
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered TypedArray type: " << decl.name
                          << " -> " << lastTypedArrayType_ << std::endl;
                lastTypedArrayType_.clear();  // Clear for next declaration
            }

            // Check if this is an ArrayBuffer assignment
            if (lastWasArrayBuffer_) {
                arrayBufferVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered ArrayBuffer variable: " << decl.name << std::endl;
                lastWasArrayBuffer_ = false;  // Clear for next declaration
            }

            // Check if this is a SharedArrayBuffer assignment (ES2017)
            if (lastWasSharedArrayBuffer_) {
                sharedArrayBufferVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered SharedArrayBuffer variable: " << decl.name << std::endl;
                lastWasSharedArrayBuffer_ = false;  // Clear for next declaration
            }

            // Check if this is a BigInt assignment (ES2020)
            if (lastWasBigInt_) {
                bigIntVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered BigInt variable: " << decl.name << std::endl;
                lastWasBigInt_ = false;  // Clear for next declaration
            }

            // Check if this is a DataView assignment
            if (lastWasDataView_) {
                dataViewVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered DataView variable: " << decl.name << std::endl;
                lastWasDataView_ = false;  // Clear for next declaration
            }

            // Check if this is a Date assignment
            if (lastWasDate_) {
                dateVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Date variable: " << decl.name << std::endl;
                lastWasDate_ = false;  // Clear for next declaration
            }

            // Check if this is a DisposableStack assignment
            if (lastWasDisposableStack_) {
                disposableStackVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered DisposableStack variable: " << decl.name << std::endl;
                lastWasDisposableStack_ = false;  // Clear for next declaration
            }

            // Check if this is an AsyncDisposableStack assignment
            if (lastWasAsyncDisposableStack_) {
                asyncDisposableStackVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered AsyncDisposableStack variable: " << decl.name << std::endl;
                lastWasAsyncDisposableStack_ = false;  // Clear for next declaration
            }

            // Check if this is a FinalizationRegistry assignment
            if (lastWasFinalizationRegistry_) {
                finalizationRegistryVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered FinalizationRegistry variable: " << decl.name << std::endl;
                lastWasFinalizationRegistry_ = false;  // Clear for next declaration
            }

            // Check if this is a Promise assignment
            if (lastWasPromise_) {
                promiseVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Promise variable: " << decl.name << std::endl;
                lastWasPromise_ = false;  // Clear for next declaration
            }

            // Check if this is a Generator assignment
            if (lastWasGenerator_) {
                generatorVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Generator variable: " << decl.name << std::endl;
                lastWasGenerator_ = false;  // Clear for next declaration
            }

            // Check if this is an Error assignment
            if (lastWasError_) {
                errorVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Error variable: " << decl.name << std::endl;
                lastWasError_ = false;  // Clear for next declaration
            }

            // Check if this is a SuppressedError assignment
            if (lastWasSuppressedError_) {
                suppressedErrorVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered SuppressedError variable: " << decl.name << std::endl;
                lastWasSuppressedError_ = false;  // Clear for next declaration
            }

            // Check if this is a Symbol assignment
            if (lastWasSymbol_) {
                symbolVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Symbol variable: " << decl.name << std::endl;
                lastWasSymbol_ = false;  // Clear for next declaration
            }

            // Check if this is an AsyncGenerator assignment
            if (lastWasAsyncGenerator_) {
                asyncGeneratorVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered AsyncGenerator variable: " << decl.name << std::endl;
                lastWasAsyncGenerator_ = false;  // Clear for next declaration
            }

            // Check if this is an IteratorResult assignment (from gen.next())
            if (lastWasIteratorResult_) {
                iteratorResultVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered IteratorResult variable: " << decl.name << std::endl;
                lastWasIteratorResult_ = false;  // Clear for next declaration
            }

            // Check if this is a runtime array assignment (from keys(), values(), entries())
            if (lastWasRuntimeArray_) {
                runtimeArrayVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered runtime array variable: " << decl.name << std::endl;
                lastWasRuntimeArray_ = false;  // Clear for next declaration
            }

            // Check if this is an Intl.* assignment
            if (lastWasNumberFormat_) {
                numberFormatVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered NumberFormat variable: " << decl.name << std::endl;
                lastWasNumberFormat_ = false;
            }
            if (lastWasDateTimeFormat_) {
                dateTimeFormatVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered DateTimeFormat variable: " << decl.name << std::endl;
                lastWasDateTimeFormat_ = false;
            }
            if (lastWasCollator_) {
                collatorVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Collator variable: " << decl.name << std::endl;
                lastWasCollator_ = false;
            }
            if (lastWasPluralRules_) {
                pluralRulesVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered PluralRules variable: " << decl.name << std::endl;
                lastWasPluralRules_ = false;
            }
            if (lastWasRelativeTimeFormat_) {
                relativeTimeFormatVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered RelativeTimeFormat variable: " << decl.name << std::endl;
                lastWasRelativeTimeFormat_ = false;
            }
            if (lastWasListFormat_) {
                listFormatVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered ListFormat variable: " << decl.name << std::endl;
                lastWasListFormat_ = false;
            }
            if (lastWasDisplayNames_) {
                displayNamesVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered DisplayNames variable: " << decl.name << std::endl;
                lastWasDisplayNames_ = false;
            }
            if (lastWasLocale_) {
                localeVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Locale variable: " << decl.name << std::endl;
                lastWasLocale_ = false;
            }
            if (lastWasSegmenter_) {
                segmenterVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Segmenter variable: " << decl.name << std::endl;
                lastWasSegmenter_ = false;
            }
            if (lastWasIterator_) {
                iteratorVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Iterator variable: " << decl.name << std::endl;
                lastWasIterator_ = false;
            }
            if (lastWasMap_) {
                mapVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Map variable: " << decl.name << std::endl;
                lastWasMap_ = false;
            }
            if (lastWasSet_) {
                setVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Set variable: " << decl.name << std::endl;
                lastWasSet_ = false;
            }
            if (lastWasWeakMap_) {
                weakMapVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered WeakMap variable: " << decl.name << std::endl;
                lastWasWeakMap_ = false;
            }
            if (lastWasWeakRef_) {
                weakRefVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered WeakRef variable: " << decl.name << std::endl;
                lastWasWeakRef_ = false;
            }
            if (lastWasWeakSet_) {
                weakSetVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered WeakSet variable: " << decl.name << std::endl;
                lastWasWeakSet_ = false;
            }
            // Web API types tracking
            if (lastWasURL_) {
                urlVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered URL variable: " << decl.name << std::endl;
                lastWasURL_ = false;
            }
            if (lastWasURLSearchParams_) {
                urlSearchParamsVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered URLSearchParams variable: " << decl.name << std::endl;
                lastWasURLSearchParams_ = false;
            }
            if (lastWasTextEncoder_) {
                textEncoderVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered TextEncoder variable: " << decl.name << std::endl;
                lastWasTextEncoder_ = false;
            }
            if (lastWasTextDecoder_) {
                textDecoderVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered TextDecoder variable: " << decl.name << std::endl;
                lastWasTextDecoder_ = false;
            }
            if (lastWasHeaders_) {
                headersVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Headers variable: " << decl.name << std::endl;
                lastWasHeaders_ = false;
            }
            if (lastWasRequest_) {
                requestVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Request variable: " << decl.name << std::endl;
                lastWasRequest_ = false;
            }
            if (lastWasResponse_) {
                responseVars_.insert(decl.name);
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered Response variable: " << decl.name << std::endl;
                lastWasResponse_ = false;
            }

            // Check if this is a builtin object type assignment (stream, events, http, etc.)
            if (!lastBuiltinObjectType_.empty()) {
                variableObjectTypes_[decl.name] = lastBuiltinObjectType_;
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Registered builtin object type: " << decl.name
                          << " -> " << lastBuiltinObjectType_ << std::endl;
                lastBuiltinObjectType_.clear();  // Clear for next declaration
            }

            // Inside generators, use generator local storage for variables that may cross yield boundaries
            if (currentGeneratorPtr_ && generatorStoreLocalFunc_) {
                // Assign a slot index for this variable
                int slotIndex = generatorNextLocalSlot_++;
                generatorVarSlots_[decl.name] = slotIndex;
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Generator variable '" << decl.name << "' assigned to slot " << slotIndex << std::endl;

                // Store initial value to generator local slot
                if (initValue) {
                    auto* genPtr = builder_->createLoad(currentGeneratorPtr_);
                    auto* slotConst = builder_->createIntConstant(slotIndex);
                    std::vector<HIRValue*> storeArgs = {genPtr, slotConst, initValue};
                    builder_->createCall(generatorStoreLocalFunc_, storeArgs);
                }

                // Also create a normal alloca for within-block access (optimization)
                auto i64Type = std::make_shared<HIRType>(HIRType::Kind::I64);
                auto alloca = builder_->createAlloca(i64Type.get(), decl.name);
                symbolTable_[decl.name] = alloca;
                if (initValue) {
                    builder_->createStore(initValue, alloca);
                }
            } else {
                // Normal (non-generator) variable handling
                // Use the initializer's type for the alloca, or default to i64
                HIRType* allocaType = nullptr;
                if (initValue && initValue->type) {
                    allocaType = initValue->type.get();
                } else {
                    auto i64Type = std::make_shared<HIRType>(HIRType::Kind::I64);
                    allocaType = i64Type.get();
                }

                // Allocate storage with the correct type
                auto alloca = builder_->createAlloca(allocaType, decl.name);
                symbolTable_[decl.name] = alloca;

                // Store the initializer value if present
                if (initValue) {
                    builder_->createStore(initValue, alloca);
                }
            }
        }
    }
    
void HIRGenerator::visit(DeclStmt& node) {
        // Process the declaration within this statement
        if (node.declaration) {
            node.declaration->accept(*this);
        }
    }

void HIRGenerator::visit(LabeledStmt& node) {
        // Labeled statement - store label for potential break/continue targets
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Processing labeled statement: " << node.label << std::endl;

        // Track the label for potential labeled break/continue
        // The label applies to the next statement (usually a loop)
        std::string savedLabel = currentLabel_;
        currentLabel_ = node.label;

        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: About to visit labeled statement body" << std::endl;
        if (node.statement) {
            node.statement->accept(*this);
        } else {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: WARNING - labeled statement has null body" << std::endl;
        }

        currentLabel_ = savedLabel;
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Exiting labeled statement: " << node.label << std::endl;
    }

void HIRGenerator::visit(WithStmt& node) {
        // 'with' statement is deprecated in JavaScript and forbidden in strict mode
        std::cerr << "WARNING: 'with' statement is deprecated and not recommended" << std::endl;

        // Still evaluate the object expression (may have side effects)
        if (node.object) {
            node.object->accept(*this);
        }

        // Execute the body
        if (node.body) {
            node.body->accept(*this);
        }
    }
    
void HIRGenerator::visit(DebuggerStmt& node) {
        (void)node;
        // debugger statement - no-op in HIR
    }
    
void HIRGenerator::visit(EmptyStmt& node) {
        (void)node;
        // empty statement - no-op
    }

void HIRGenerator::visit(UsingStmt& node) {
        // ES2024 'using' statement for Explicit Resource Management
        // Creates a const binding that will be disposed when scope exits
        // For now, we implement it as a const binding - full dispose support needs runtime

        std::string name = node.name;

        // Evaluate the initializer first to get its type
        HIRValue* initValue = nullptr;
        if (node.init) {
            node.init->accept(*this);
            initValue = lastValue_;
        }

        // Use the initializer's type for the alloca, or default to Any
        HIRType* allocaType = nullptr;
        if (initValue && initValue->type) {
            allocaType = initValue->type.get();
        } else {
            auto anyType = std::make_shared<HIRType>(HIRType::Kind::Any);
            allocaType = anyType.get();
        }

        // Allocate storage with the correct type
        auto alloca = builder_->createAlloca(allocaType, name);
        symbolTable_[name] = alloca;

        // Store the initializer value if present
        if (initValue) {
            builder_->createStore(initValue, alloca);
        }

        // Note: Full implementation would track this for disposal at scope exit
        // This would require block-level resource tracking for [Symbol.dispose]()
        // For now, the resource is created but disposal must be done manually

        if (node.isAwait) {
            // await using - would use [Symbol.asyncDispose]() at scope exit
            // This requires async context and Promise handling
            (void)node.isAwait;  // Silence unused warning
        }
    }

} // namespace nova::hir
