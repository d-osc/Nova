// HIRGen.cpp - HIR Generator infrastructure and public API
// This file contains only the core infrastructure.
// Visitor implementations are split across multiple files:
//   - HIRGen_Literals.cpp
//   - HIRGen_Operators.cpp
//   - HIRGen_Functions.cpp
//   - HIRGen_Classes.cpp
//   - HIRGen_Arrays.cpp
//   - HIRGen_Objects.cpp
//   - HIRGen_ControlFlow.cpp
//   - HIRGen_Statements.cpp
//   - HIRGen_Calls.cpp
//   - HIRGen_Advanced.cpp

#define NOVA_DEBUG 0

#include "nova/HIR/HIRGen_Internal.h"

namespace nova::hir {

// Helper method implementation: Variable lookup with closure support
HIRValue* HIRGenerator::lookupVariable(const std::string& name) {
    // Check current scope first
    auto it = symbolTable_.find(name);
    if (it != symbolTable_.end()) {
        return it->second;
    }

    // Check parent scopes (for closure support)
    for (auto scopeIt = scopeStack_.rbegin(); scopeIt != scopeStack_.rend(); ++scopeIt) {
        auto varIt = scopeIt->find(name);
        if (varIt != scopeIt->end()) {
            // Variable found in parent scope - this is a captured variable!
            if (currentFunction_ && !lastFunctionName_.empty()) {
                // Track this variable as captured by the current function
                capturedVariables_[lastFunctionName_].insert(name);

                // Store the parent scope value for later use
                if (environmentFieldNames_.find(lastFunctionName_) == environmentFieldNames_.end()) {
                    environmentFieldNames_[lastFunctionName_] = std::vector<std::string>();
                    environmentFieldValues_[lastFunctionName_] = std::vector<HIRValue*>();
                }
                // Add to field names if not already there
                auto& fieldNames = environmentFieldNames_[lastFunctionName_];
                if (std::find(fieldNames.begin(), fieldNames.end(), name) == fieldNames.end()) {
                    // Debug: Check the type of the value we're about to store
                    if(NOVA_DEBUG && varIt->second) {
                        std::cerr << "DEBUG HIRGen: lookupVariable - About to store '" << name
                                  << "' for function '" << lastFunctionName_ << "'"
                                  << ", HIRValue ptr: " << varIt->second
                                  << ", has type: " << (varIt->second->type ? "YES" : "NO");
                        if (varIt->second->type) {
                            std::cerr << ", type kind: " << static_cast<int>(varIt->second->type->kind);
                        }
                        std::cerr << std::endl;
                    }

                    fieldNames.push_back(name);
                    environmentFieldValues_[lastFunctionName_].push_back(varIt->second);
                    if (currentFunction_ && !currentFunction_->parameters.empty()) {
                        auto* environment = currentFunction_->parameters.back();
                        if (environment && environment->name == "__env") {
                            if (auto* temporary = dynamic_cast<HIRStructType*>(
                                    environment->type.get())) {
                                HIRTypePtr cellType = varIt->second->type;
                                auto* sourceInstruction =
                                    dynamic_cast<HIRInstruction*>(varIt->second);
                                const bool sourceIsStorage =
                                    heapClosureCells_.count(varIt->second) != 0 ||
                                    (sourceInstruction &&
                                     (sourceInstruction->opcode ==
                                          HIRInstruction::Opcode::Alloca ||
                                      (sourceInstruction->opcode ==
                                           HIRInstruction::Opcode::GetField &&
                                       sourceInstruction->name.find(".cell") !=
                                           std::string::npos)));
                                if (name == "this" && !sourceIsStorage) {
                                    cellType = std::make_shared<HIRPointerType>(
                                        varIt->second->type, true);
                                }
                                temporary->fields.push_back({
                                    name, cellType, true});
                            }
                        }
                    }
                }

                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Variable '" << name << "' captured by function '" << lastFunctionName_ << "'" << std::endl;

                // Return nullptr to signal that this is a captured variable
                // The Identifier visitor will handle creating GetField access
                return nullptr;
            }
            return varIt->second;
        }
    }

    return nullptr;
}

// Helper method: Create closure environment struct for captured variables
hir::HIRStructType* HIRGenerator::createClosureEnvironment(const std::string& funcName) {
    auto it = capturedVariables_.find(funcName);
    if (it == capturedVariables_.end() || it->second.empty()) {
        // No captured variables, no environment needed
        return nullptr;
    }

    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Creating closure environment for " << funcName
                             << " with " << it->second.size() << " captured variables" << std::endl;

    // Collect fields for the environment struct
    std::vector<HIRStructType::Field> fields;
    std::vector<std::string> fieldNames = environmentFieldNames_[funcName];
    std::vector<HIRValue*> fieldValues;  // Store HIRValue pointers for MIRGen

    // Preserve the encounter order used by GetField instructions emitted while
    // the nested function body was generated. unordered_set iteration order is
    // not stable and can otherwise connect a captured name to the wrong cell.
    for (const auto& varName : it->second) {
        if (std::find(fieldNames.begin(), fieldNames.end(), varName) ==
            fieldNames.end()) {
            fieldNames.push_back(varName);
        }
    }

    for (const auto& varName : fieldNames) {
        // Look up the variable to get its type
        HIRValue* varValue = nullptr;

        // Check in parent scopes (since we're creating environment for nested function)
        for (auto scopeIt = scopeStack_.rbegin(); scopeIt != scopeStack_.rend(); ++scopeIt) {
            auto varIt = scopeIt->find(varName);
            if (varIt != scopeIt->end()) {
                varValue = varIt->second;
                break;
            }
        }

        HIRStructType::Field field;
        field.name = varName;
        field.isPublic = true;  // Environment fields are accessible

        if (varValue && varValue->type) {
            // Capture the binding cell itself. Reads load through this pointer
            // and writes store through it, preserving JavaScript closure
            // mutation semantics across every invocation.
            field.type = varValue->type;
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Environment field: " << varName
                                     << " (type kind: " << static_cast<int>(varValue->type->kind) << ")" << std::endl;
        } else {
            // Default to i64 if we can't determine type
            field.type = std::make_shared<HIRType>(HIRType::Kind::I64);
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Environment field: " << varName
                                     << " (default i64)" << std::endl;
        }

        fields.push_back(field);
        fieldValues.push_back(varValue);  // Store HIRValue for MIRGen to use
    }

    // Create the environment struct type with a unique name
    std::string envStructName = "__closure_env_" + funcName;
    auto envStruct = new HIRStructType(envStructName, fields);

    // Store the field names and values for later use
    environmentFieldNames_[funcName] = fieldNames;
    environmentFieldValues_[funcName] = fieldValues;

    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Created environment struct '" << envStructName
                             << "' with " << fields.size() << " fields" << std::endl;

    return envStruct;
}

HIRValue* HIRGenerator::getCapturedVariableStorage(const std::string& name) {
    if (!currentFunction_ || lastFunctionName_.empty() ||
        capturedVariables_[lastFunctionName_].count(name) == 0 ||
        currentFunction_->parameters.empty()) {
        return nullptr;
    }
    auto* environment = currentFunction_->parameters.back();
    if (!environment || environment->name != "__env") return nullptr;
    auto names = environmentFieldNames_.find(lastFunctionName_);
    if (names == environmentFieldNames_.end()) return nullptr;
    auto found = std::find(names->second.begin(), names->second.end(), name);
    if (found == names->second.end()) return nullptr;
    return builder_->createGetField(
        environment, static_cast<uint32_t>(
            std::distance(names->second.begin(), found)), name + ".cell");
}

HIRValue* HIRGenerator::materializeClosureEnvironment(
        const std::string& funcName) {
    auto names = environmentFieldNames_.find(funcName);
    if (names == environmentFieldNames_.end() || names->second.empty()) {
        return nullptr;
    }
    auto* envStruct = createClosureEnvironment(funcName);
    if (!envStruct) return nullptr;

    auto* environment = builder_->createAlloca(
        envStruct, "__env_struct." + funcName);
    for (size_t i = 0; i < names->second.size(); ++i) {
        const std::string& variableName = names->second[i];
        HIRValue* storage = lookupVariable(variableName);
        if (!storage) continue;
        if (auto* inherited = getCapturedVariableStorage(variableName)) {
            storage = inherited;
        }

        auto* instruction = dynamic_cast<HIRInstruction*>(storage);
        const bool alreadyCell = heapClosureCells_.count(storage) != 0 ||
            (instruction && instruction->opcode ==
                 HIRInstruction::Opcode::GetField &&
             instruction->name.find(".cell") != std::string::npos);
        if (!alreadyCell) {
            HIRValue* initialValue = storage;
            if (instruction && instruction->opcode ==
                    HIRInstruction::Opcode::Alloca) {
                initialValue = builder_->createLoad(
                    storage, variableName + ".closure.initial");
            }
            auto sizeType = std::make_shared<HIRType>(HIRType::Kind::I64);
            auto opaqueType = std::make_shared<HIRType>(HIRType::Kind::Any);
            auto opaquePointer = std::make_shared<HIRPointerType>(
                opaqueType, true);
            auto allocator = module_->getFunction("nova_alloc_closure_env");
            HIRFunction* allocatorFunction = allocator
                ? allocator.get() : nullptr;
            if (!allocatorFunction) {
                auto allocatorType = new HIRFunctionType(
                    {sizeType}, opaquePointer);
                auto created = module_->createFunction(
                    "nova_alloc_closure_env", allocatorType);
                created->linkage = HIRFunction::Linkage::External;
                allocatorFunction = created.get();
            }
            auto* cell = builder_->createCall(
                allocatorFunction, {builder_->createIntConstant(8)},
                variableName + ".closure.cell");
            cell->type = std::make_shared<HIRPointerType>(
                initialValue->type, true);
            builder_->createStore(initialValue, cell);
            heapClosureCells_.insert(cell);
            symbolTable_[variableName] = cell;
            storage = cell;
        }
        auto* field = builder_->createGetField(
            environment, static_cast<uint32_t>(i), variableName);
        builder_->createStore(storage, field);
    }
    return environment;
}

void HIRGenerator::propagateTransitiveCaptures(
    const std::string& enclosingFunction,
    const std::unordered_map<std::string, HIRValue*>& enclosingLocals,
    const std::string& childFunction) {
    if (enclosingFunction.empty()) return;
    auto child = capturedVariables_.find(childFunction);
    if (child == capturedVariables_.end()) return;

    for (const auto& name : child->second) {
        // A value declared by the immediate parent can be boxed when the child
        // closure is created. Values outside that parent must first become a
        // capture of the parent so the same cell can cross every environment.
        if (enclosingLocals.count(name) != 0) continue;

        capturedVariables_[enclosingFunction].insert(name);
        auto& names = environmentFieldNames_[enclosingFunction];
        if (std::find(names.begin(), names.end(), name) != names.end()) continue;

        HIRValue* storage = nullptr;
        for (auto scope = scopeStack_.rbegin(); scope != scopeStack_.rend();
             ++scope) {
            auto found = scope->find(name);
            if (found != scope->end()) {
                storage = found->second;
                break;
            }
        }
        names.push_back(name);
        environmentFieldValues_[enclosingFunction].push_back(storage);
    }
}

// Visitor implementation: Identifier (basic expression)
void HIRGenerator::visit(Identifier& node) {
    lastWasBigInt_ = bigIntVars_.count(node.name) > 0;
    lastWasSymbol_ = symbolVars_.count(node.name) > 0;
    lastWasPromise_ = promiseVars_.count(node.name) > 0;
    // Handle globalThis (ES2020) - the global object
    if (node.name == "globalThis") {
        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Detected globalThis identifier" << std::endl;
        // Return a special marker value for globalThis
        // When used in MemberExpr, we'll handle the property access
        lastWasGlobalThis_ = true;
        lastValue_ = builder_->createIntConstant(1);  // Placeholder for globalThis object
        return;
    }

    // Handle global constants accessed directly
    if (node.name == "Infinity") {
        lastValue_ = builder_->createFloatConstant(std::numeric_limits<double>::infinity());
        return;
    }
    if (node.name == "NaN") {
        lastValue_ = builder_->createFloatConstant(std::numeric_limits<double>::quiet_NaN());
        return;
    }
    if (node.name == "undefined") {
        auto undefType = std::make_shared<HIRType>(HIRType::Kind::Unknown);
        lastValue_ = builder_->createUndefinedConstant(undefType.get());
        return;
    }

    if (auto imported = importedNumberConstants_.find(node.name);
        imported != importedNumberConstants_.end()) {
        const double value = imported->second;
        lastValue_ = value == static_cast<int64_t>(value)
            ? builder_->createIntConstant(static_cast<int64_t>(value))
            : builder_->createFloatConstant(value);
        return;
    }
    if (auto imported = importedStringConstants_.find(node.name);
        imported != importedStringConstants_.end()) {
        lastValue_ = builder_->createStringConstant(imported->second);
        return;
    }
    if (auto imported = importedBooleanConstants_.find(node.name);
        imported != importedBooleanConstants_.end()) {
        lastValue_ = builder_->createBoolConstant(imported->second);
        return;
    }

    if (auto nullish = staticNullishVariables_.find(node.name);
        nullish != staticNullishVariables_.end()) {
        auto valueType = std::make_shared<HIRType>(HIRType::Kind::Unknown);
        lastValue_ = nullish->second == HIRConstant::Kind::Null
            ? builder_->createNullConstant(valueType.get())
            : builder_->createUndefinedConstant(valueType.get());
        return;
    }

    // Inside generators, check if this variable is stored in generator local slots
    // and load from there to ensure cross-yield persistence
    if (currentGeneratorPtr_ && generatorLoadLocalFunc_) {
        auto slotIt = generatorVarSlots_.find(node.name);
        if (slotIt != generatorVarSlots_.end()) {
            // Load from generator local storage
            auto* genPtr = builder_->createLoad(currentGeneratorPtr_);
            auto* slotConst = builder_->createIntConstant(slotIt->second);
            std::vector<HIRValue*> loadArgs = {genPtr, slotConst};
            lastValue_ = builder_->createCall(generatorLoadLocalFunc_, loadArgs, node.name);
            return;
        }
    }

    // Check if we're inside a closure and this variable is captured
    if (currentFunction_ && !lastFunctionName_.empty()) {
        auto capturedIt = capturedVariables_.find(lastFunctionName_);
        if (capturedIt != capturedVariables_.end()) {
            const auto& capturedVars = capturedIt->second;

            // Check if this variable is in the captured list
            if (capturedVars.count(node.name) > 0) {
                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Accessing captured variable '" << node.name
                                          << "' in closure '" << lastFunctionName_ << "'" << std::endl;

                // Find the __env parameter (should be the last parameter)
                HIRParameter* envParam = nullptr;
                if (currentFunction_ && !currentFunction_->parameters.empty()) {
                    // Environment is the last parameter
                    envParam = currentFunction_->parameters.back();
                    if (envParam->name == "__env") {
                        if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Found __env parameter" << std::endl;

                        // Find field index for this variable
                        auto fieldNamesIt = environmentFieldNames_.find(lastFunctionName_);
                        if (fieldNamesIt != environmentFieldNames_.end()) {
                            const auto& fieldNames = fieldNamesIt->second;
                            auto fieldIt = std::find(fieldNames.begin(), fieldNames.end(), node.name);
                            if (fieldIt != fieldNames.end()) {
                                int fieldIndex = std::distance(fieldNames.begin(), fieldIt);
                                if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Variable '" << node.name
                                                          << "' is at field index " << fieldIndex << std::endl;

                                // Environment fields are binding-cell pointers.
                                auto* storage = builder_->createGetField(
                                    envParam, static_cast<uint32_t>(fieldIndex),
                                    node.name + ".cell");
                                lastValue_ = builder_->createLoad(storage, node.name);
                                return;
                            }
                        }
                    }
                }
            }
        }
    }

    // Look up variable in symbol table and parent scopes
    HIRValue* value = lookupVariable(node.name);

    // If not in symbol table, check whether this name refers to a known
    // module function. When a function name is used as a value (e.g. the
    // target argument of Reflect.apply), return the function name as a
    // string constant — LLVM codegen auto-resolves string constants to
    // function pointers when the callee's parameter type is `ptr` (see
    // LLVMCodeGen.cpp:5353). This is the same trick used for array/Promise
    // callbacks.
    if (!value) {
        auto existingFunc = module_->getFunction(node.name);
        if (existingFunc) {
            lastValue_ = builder_->createStringConstant(node.name);
            return;
        }
    }
    if (value) {
        if (heapClosureCells_.count(value) != 0) {
            lastValue_ = builder_->createLoad(value, node.name);
            return;
        }
        // Check if this is an alloca (memory location)
        // Try to cast to HIRInstruction to check the opcode
        try {
            if (auto* inst = dynamic_cast<hir::HIRInstruction*>(value)) {
                if (inst && inst->opcode == hir::HIRInstruction::Opcode::Alloca) {
                    // For allocas, we need to load the value
                    lastValue_ = builder_->createLoad(value, node.name);
                    return;
                }
            }
        } catch (...) {
            // If cast fails, just use the value directly
        }
        // For other values (like function parameters), use directly
        lastValue_ = value;
    } else {
        // lookupVariable returned nullptr - this might be a captured variable
        // Try to create GetField instruction to access from __env parameter
        if (capturedVariables_.count(lastFunctionName_) &&
            capturedVariables_[lastFunctionName_].count(node.name)) {
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Accessing captured variable '"
                                      << node.name << "' in function '" << lastFunctionName_ << "'" << std::endl;

            // Find the __env parameter (should be the last parameter after body generation adds it)
            // But during body generation, __env hasn't been added yet
            // So we need to find it from currentFunction_
            HIRParameter* envParam = nullptr;
            if (currentFunction_ && !currentFunction_->parameters.empty()) {
                // Check if last parameter is __env
                envParam = currentFunction_->parameters.back();
                if (envParam->name == "__env") {
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Found __env parameter in current function" << std::endl;
                } else {
                    envParam = nullptr;
                }
            }

            if (envParam) {
                // __env parameter exists, create GetField instruction
                auto& fieldNames = environmentFieldNames_[lastFunctionName_];
                auto it = std::find(fieldNames.begin(), fieldNames.end(), node.name);
                if (it != fieldNames.end()) {
                    int fieldIndex = std::distance(fieldNames.begin(), it);
                    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Creating GetField for '" << node.name
                                              << "' at field index " << fieldIndex << std::endl;
                    auto* storage = builder_->createGetField(
                        envParam, static_cast<uint32_t>(fieldIndex),
                        node.name + ".cell");
                    lastValue_ = builder_->createLoad(storage, node.name);
                    return;
                }
            }

            // __env not available yet (during body generation), use placeholder
            if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: __env not available yet, using constant 0 placeholder for '"
                                      << node.name << "'" << std::endl;
            lastValue_ = builder_->createIntConstant(0);
        }
    }
}

// Public API to generate HIR from AST
HIRModule* generateHIR(Program& program, const std::string& moduleName, const std::string& filePath) {
    auto* module = new HIRModule(moduleName);
    HIRGenerator generator(module);
    generator.setFilePath(filePath);
    program.accept(generator);
    return module;
}

} // namespace nova::hir
