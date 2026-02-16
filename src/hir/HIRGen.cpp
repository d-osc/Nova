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
#include <fstream>

namespace nova::hir {

// Helper method implementation: Variable lookup with closure support
HIRValue* HIRGenerator::lookupVariable(const std::string& name) {
    std::ofstream logfile("lookup_log.txt", std::ios::app);
    logfile << "[LOOKUP] Looking up '" << name << "' in function '" << lastFunctionName_ << "'" << std::endl;
    logfile.close();

    // Check current scope first
    auto it = symbolTable_.find(name);
    if (it != symbolTable_.end()) {
        std::ofstream logfile2("lookup_log.txt", std::ios::app);
        logfile2 << "[LOOKUP] Found '" << name << "' in current scope" << std::endl;
        logfile2.close();
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
                }

                std::ofstream logfile3("capture_log.txt", std::ios::app);
                logfile3 << "[CAPTURE] Variable '" << name << "' captured by '" << lastFunctionName_ << "'" << std::endl;
                logfile3.close();
                std::cout << "[CAPTURE] Variable '" << name << "' captured by '" << lastFunctionName_ << "'" << std::endl;
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
    std::cout << "[CREATE-ENV] Called for function '" << funcName << "'" << std::endl;
    auto it = capturedVariables_.find(funcName);
    if (it == capturedVariables_.end() || it->second.empty()) {
        // No captured variables, no environment needed
        std::cout << "[CREATE-ENV] No captured variables found, returning nullptr" << std::endl;
        return nullptr;
    }
    std::cout << "[CREATE-ENV] Found " << it->second.size() << " captured variables" << std::endl;

    if(NOVA_DEBUG) std::cerr << "DEBUG HIRGen: Creating closure environment for " << funcName
                             << " with " << it->second.size() << " captured variables" << std::endl;

    // Collect fields for the environment struct
    std::vector<HIRStructType::Field> fields;
    std::vector<std::string> fieldNames;
    std::vector<HIRValue*> fieldValues;  // Store HIRValue pointers for MIRGen

    for (const auto& varName : it->second) {
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
        fieldNames.push_back(varName);
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

// Visitor implementation: Identifier (basic expression)
void HIRGenerator::visit(Identifier& node) {
    std::ofstream logfile("visit_id_log.txt", std::ios::app);
    logfile << "[VISIT-ID] Visiting identifier '" << node.name << "' in function '" << lastFunctionName_ << "'" << std::endl;
    logfile.close();

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
        lastValue_ = builder_->createIntConstant(0);  // undefined represented as 0
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

                                // Generate GetField to access the captured variable from environment
                                lastValue_ = builder_->createGetField(envParam, static_cast<uint32_t>(fieldIndex), node.name);
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
    if (value) {
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
                    lastValue_ = builder_->createGetField(envParam, static_cast<uint32_t>(fieldIndex), node.name);
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
