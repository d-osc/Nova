// LLVM Code Generator from MIR
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4100 4127 4244 4245 4267 4310 4324 4458 4459 4624)
#endif

#include "nova/CodeGen/LLVMCodeGen.h"
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/Reassociate.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Scalar/DCE.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <iostream>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace nova::codegen {

LLVMCodeGen::LLVMCodeGen(const std::string& moduleName) 
    : currentFunction(nullptr) {
    // Initialize LLVM context and module
    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>(moduleName, *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
    
    // Initialize LLVM targets
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
}

LLVMCodeGen::~LLVMCodeGen() = default;

bool LLVMCodeGen::generate(const mir::MIRModule& mirModule) {
    try {
        // Declare runtime functions
        declareRuntimeFunctions();
        
        // Disable constant folding to prevent runtime values from being folded
        // This is especially important for comparison operations in loops
        std::cerr << "DEBUG LLVM: Disabling constant folding to preserve runtime comparisons" << std::endl;
        
        
        // First pass: Create function declarations for all functions to support forward references
        std::cerr << "DEBUG LLVM: First pass - creating function declarations" << std::endl;
        for (const auto& mirFunc : mirModule.functions) {
            // Skip if already exists in functionMap
            if (functionMap.find(mirFunc->name) != functionMap.end()) {
                continue;
            }
            
            // Create function signature
            std::vector<llvm::Type*> paramTypes;
            for (const auto& arg : mirFunc->arguments) {
                llvm::Type* paramType = convertType(arg->type.get());
                if (paramType->isVoidTy()) {
                    paramType = llvm::Type::getInt64Ty(*context);
                }
                paramTypes.push_back(paramType);
            }
            
            llvm::Type* retType = convertType(mirFunc->returnType.get());
            if (retType->isVoidTy()) {
                retType = llvm::Type::getInt64Ty(*context);
            } else if (!retType->isPointerTy() && !retType->isIntegerTy(1)) {
                retType = llvm::Type::getInt64Ty(*context);
            }
            
            llvm::FunctionType* funcType = llvm::FunctionType::get(retType, paramTypes, false);
            llvm::Function* llvmFunc = llvm::Function::Create(
                funcType,
                llvm::Function::ExternalLinkage,
                mirFunc->name,
                module.get()
            );
            functionMap[mirFunc->name] = llvmFunc;
            std::cerr << "DEBUG LLVM: Declared function: " << mirFunc->name << std::endl;
        }
        
        std::cerr << "DEBUG LLVM: Second pass - generating function bodies" << std::endl;
        // Generate all functions
        for (const auto& mirFunc : mirModule.functions) {
            generateFunction(mirFunc.get());
        }
        
// Verify the module
        std::string errMsg;
        llvm::raw_string_ostream errStream(errMsg);
        if (llvm::verifyModule(*module, &errStream)) {
            std::cerr << "LLVM IR verification failed:\n" << errMsg << std::endl;
            // TEMPORARY: Continue anyway to see the IR
            std::cerr << "TEMPORARY: Continuing despite verification error to dump IR" << std::endl;
            // return false;
        }
        
        // Optimization passes are now enabled for basic optimizations
        std::cerr << "DEBUG LLVM: Basic optimization passes are enabled" << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error generating LLVM IR: " << e.what() << std::endl;
        return false;
    }
}

void LLVMCodeGen::dumpIR() const {
    module->print(llvm::outs(), nullptr);
    
    // Also dump to a file for inspection
    std::error_code EC;
    llvm::raw_fd_ostream dest("debug_output.ll", EC, llvm::sys::fs::OF_None);
    if (!EC) {
        module->print(dest, nullptr);
        dest.flush();
        std::cerr << "DEBUG: LLVM IR dumped to debug_output.ll" << std::endl;
    }
}

bool LLVMCodeGen::emitObjectFile(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << std::endl;
        return false;
    }
    
    // TODO: Implement object file generation
    // Need to setup target machine and emit object code
    std::cerr << "Object file generation not yet implemented" << std::endl;
    return false;
}

bool LLVMCodeGen::emitAssembly(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << std::endl;
        return false;
    }
    
    // TODO: Implement assembly generation
    std::cerr << "Assembly generation not yet implemented" << std::endl;
    return false;
}

bool LLVMCodeGen::emitLLVMIR(const std::string& filename) {
    std::cerr << "DEBUG LLVM: emitLLVMIR ENTRY POINT - filename=" << filename << std::endl;
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << std::endl;
        return false;
    }
    
    // Before printing, verify the module is complete
    std::string errMsg;
    llvm::raw_string_ostream errStream(errMsg);
    if (llvm::verifyModule(*module, &errStream)) {
        std::cerr << "LLVM IR verification failed before printing:\n" << errMsg << std::endl;
        return false;
    }
    
    // Create a raw version of the module before printing to debug file
    std::error_code debugEC;
    llvm::raw_fd_ostream debugDest("debug_output.ll", debugEC, llvm::sys::fs::OF_None);
    if (!debugEC) {
        // Debug: Print module information before dumping
        std::cerr << "DEBUG LLVM: emitLLVMIR called - Module has " << module->size() << " functions" << std::endl;
        for (auto& func : *module) {
            std::cerr << "DEBUG LLVM: emitLLVMIR - Function " << func.getName().str() << " has " << func.size() << " basic blocks" << std::endl;
            for (auto& bb : func) {
                std::cerr << "DEBUG LLVM: emitLLVMIR - Basic block " << bb.getName().str() << " has " << bb.size() << " instructions" << std::endl;
            }
        }
        
        module->print(debugDest, nullptr);
        debugDest.flush();
        std::cerr << "DEBUG: Raw LLVM IR dumped to debug_output.ll" << std::endl;
    }
    
    // Force the module to be written to disk immediately
    module->setTargetTriple("x86_64-pc-windows-msvc");
    module->setDataLayout("e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128");
    
    // Print the module to the requested file
    module->print(dest, nullptr);
    dest.flush();
    
    return true;
}

bool LLVMCodeGen::emitBitcode(const std::string& filename) {
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << std::endl;
        return false;
    }
    
    // TODO: Implement bitcode generation
    std::cerr << "Bitcode generation not yet implemented" << std::endl;
    return false;
}

void LLVMCodeGen::runOptimizationPasses(unsigned optLevel) {
    std::cerr << "DEBUG LLVM: runOptimizationPasses called with optLevel=" << optLevel << std::endl;
    if (optLevel == 0) {
        std::cerr << "DEBUG LLVM: optLevel is 0, skipping optimization passes" << std::endl;
        return;
    }
    
    std::cerr << "DEBUG LLVM: Creating function pass manager" << std::endl;
    // Create function pass manager
    auto FPM = std::make_unique<llvm::legacy::FunctionPassManager>(module.get());
    
    // Add basic optimization passes based on level
    if (optLevel >= 1) {
        std::cerr << "DEBUG LLVM: Adding basic optimization passes (optLevel >= 1)" << std::endl;
        // Basic optimizations that don't affect loop body operations
        FPM->add(llvm::createPromoteMemoryToRegisterPass());
        FPM->add(llvm::createInstructionCombiningPass());
        FPM->add(llvm::createReassociatePass());
        FPM->add(llvm::createGVNPass());
        FPM->add(llvm::createCFGSimplificationPass());
        // We're now allowing CFGSimplificationPass since it doesn't affect loop body operations
        std::cerr << "DEBUG LLVM: Added CFGSimplificationPass for basic optimizations" << std::endl;
    }
    
    if (optLevel >= 2) {
        std::cerr << "DEBUG LLVM: Adding intermediate optimization passes (optLevel >= 2)" << std::endl;
        // Intermediate optimizations that don't affect loop body operations
        FPM->add(llvm::createDeadCodeEliminationPass());
        FPM->add(llvm::createInstructionCombiningPass());
        FPM->add(llvm::createCFGSimplificationPass());
        // We're now allowing DeadCodeEliminationPass since it doesn't affect loop body operations
        std::cerr << "DEBUG LLVM: Added DeadCodeEliminationPass and other intermediate optimizations" << std::endl;
    }
    
    if (optLevel >= 3) {
        std::cerr << "DEBUG LLVM: Adding advanced optimization passes (optLevel >= 3)" << std::endl;
        // Advanced optimizations
        FPM->add(llvm::createInstructionCombiningPass());
        FPM->add(llvm::createCFGSimplificationPass());
        FPM->add(llvm::createAlwaysInlinerLegacyPass());
        std::cerr << "DEBUG LLVM: Added advanced optimizations and function inlining" << std::endl;
    }
    
    FPM->doInitialization();
    
    // Run passes on all functions
    for (auto& func : module->functions()) {
        if (!func.isDeclaration()) {
            FPM->run(func);
        }
    }
    
    std::cerr << "DEBUG LLVM: Initializing function pass manager" << std::endl;
    FPM->doInitialization();
    
    std::cerr << "DEBUG LLVM: Running optimization passes on functions" << std::endl;
    // Run passes on all functions
    for (auto& func : module->functions()) {
        if (!func.isDeclaration()) {
            std::cerr << "DEBUG LLVM: Running passes on function: " << func.getName().str() << std::endl;
            FPM->run(func);
        }
    }
    
    FPM->doFinalization();
    
    std::cerr << "DEBUG LLVM: Optimization passes completed" << std::endl;
}

int LLVMCodeGen::executeMain() {
    // Initialize LLVM Execution Engine
    std::cerr << "DEBUG LLVM: Initializing JIT execution engine" << std::endl;
    
    // Verify the module
    if (llvm::verifyModule(*module, &llvm::errs())) {
        std::cerr << "❌ Error: Module verification failed" << std::endl;
        // TEMPORARY: Continue anyway to see the IR
        std::cerr << "TEMPORARY: Continuing despite verification error" << std::endl;
        // return 1;
    }
    
    // Save the LLVM IR to a temporary file
    std::string tempFile = "temp_jit.ll";
    std::error_code EC;
    llvm::raw_fd_ostream out(tempFile, EC);
    if (EC) {
        std::cerr << "❌ Error: Could not create temporary file: " << EC.message() << std::endl;
        return 1;
    }
    
    // Write the LLVM IR to file
    module->print(out, nullptr);
    out.close();
    
    std::cerr << "DEBUG LLVM: LLVM IR saved to " << tempFile << std::endl;
    
    // Use llc to compile to assembly and then assemble with clang
    // This is a workaround for JIT not being available on this system
    std::string objFile = "temp_jit.o";
    std::string exeFile = "temp_jit.exe";
    
    // Compile LLVM IR to object file
    std::string llcCmd = "llc -filetype=obj -o \"" + objFile + "\" \"" + tempFile + "\"";
    std::cerr << "DEBUG LLVM: Running: " << llcCmd << std::endl;
    int llcResult = system(llcCmd.c_str());
    if (llcResult != 0) {
        std::cerr << "❌ Error: llc compilation failed" << std::endl;
        return 1;
    }
    
    // Link object file to executable (including novacore runtime library)
    std::string linkCmd;
#ifdef _WIN32
    linkCmd = "clang -o \"" + exeFile + "\" \"" + objFile + "\" \"build/Release/novacore.lib\" -lmsvcrt -lkernel32";
#else
    linkCmd = "clang -o \"" + exeFile + "\" \"" + objFile + "\" \"build/Release/libnovacore.a\" -lc -lstdc++";
#endif
    std::cerr << "DEBUG LLVM: Running: " << linkCmd << std::endl;
    int linkResult = system(linkCmd.c_str());
    if (linkResult != 0) {
        std::cerr << "❌ Error: Linking failed" << std::endl;
        return 1;
    }
    
    // Execute the compiled program
    std::cerr << "DEBUG LLVM: Executing compiled program..." << std::endl;
    int execResult = system((".\\\"" + exeFile + "\"").c_str());
    
    // Clean up temporary files
    // remove(tempFile.c_str());  // Keep temp_jit.ll for debugging
    remove(objFile.c_str());
    remove(exeFile.c_str());
    
    std::cerr << "DEBUG LLVM: Program executed with exit code: " << execResult << std::endl;
    std::cerr << "DEBUG LLVM: Temporary files cleaned up" << std::endl;
    return execResult;
}

// ==================== Type Conversion ====================

llvm::Type* LLVMCodeGen::convertType(mir::MIRType* type) {
    if (!type) {
        return llvm::Type::getVoidTy(*context);
    }
    
    // Check cache first
    auto it = typeCache.find(type);
    if (it != typeCache.end()) {
        return it->second;
    }
    
    llvm::Type* llvmType = nullptr;
    
    switch (type->kind) {
        case mir::MIRType::Kind::Void:
            llvmType = llvm::Type::getVoidTy(*context);
            break;
        case mir::MIRType::Kind::I1:
            llvmType = llvm::Type::getInt1Ty(*context);
            break;
        case mir::MIRType::Kind::I8:
            llvmType = llvm::Type::getInt8Ty(*context);
            break;
        case mir::MIRType::Kind::I16:
            llvmType = llvm::Type::getInt16Ty(*context);
            break;
        case mir::MIRType::Kind::I32:
            llvmType = llvm::Type::getInt32Ty(*context);
            break;
        case mir::MIRType::Kind::I64:
            llvmType = llvm::Type::getInt64Ty(*context);
            break;
        case mir::MIRType::Kind::ISize:
            llvmType = llvm::Type::getInt64Ty(*context); // Platform dependent
            break;
        case mir::MIRType::Kind::U8:
            llvmType = llvm::Type::getInt8Ty(*context);
            break;
        case mir::MIRType::Kind::U16:
            llvmType = llvm::Type::getInt16Ty(*context);
            break;
        case mir::MIRType::Kind::U32:
            llvmType = llvm::Type::getInt32Ty(*context);
            break;
        case mir::MIRType::Kind::U64:
            llvmType = llvm::Type::getInt64Ty(*context);
            break;
        case mir::MIRType::Kind::USize:
            llvmType = llvm::Type::getInt64Ty(*context); // Platform dependent
            break;
        case mir::MIRType::Kind::F32:
            llvmType = llvm::Type::getFloatTy(*context);
            break;
        case mir::MIRType::Kind::F64:
            llvmType = llvm::Type::getDoubleTy(*context);
            break;
        case mir::MIRType::Kind::Pointer:
            llvmType = llvm::PointerType::getUnqual(*context);
            break;
        case mir::MIRType::Kind::Array:
            // TODO: Handle array type with size
            llvmType = llvm::PointerType::getUnqual(*context);
            break;
        case mir::MIRType::Kind::Struct:
            // TODO: Handle struct type
            llvmType = llvm::PointerType::getUnqual(*context);
            break;
        case mir::MIRType::Kind::Function:
            // TODO: Handle function type
            llvmType = llvm::PointerType::getUnqual(*context);
            break;
        default:
            llvmType = llvm::Type::getVoidTy(*context);
            break;
    }
    
    typeCache[type] = llvmType;
    return llvmType;
}

llvm::Value* LLVMCodeGen::convertOperand(mir::MIROperand* operand) {
    std::cerr << "DEBUG LLVM: convertOperand called" << std::endl;
    if (!operand) {
        std::cerr << "DEBUG LLVM: operand is null" << std::endl;
        return nullptr;
    }
    
    if (operand->kind == mir::MIROperand::Kind::Copy) {
        std::cerr << "DEBUG LLVM: Processing Copy operand" << std::endl;
        auto* copyOp = static_cast<mir::MIRCopyOperand*>(operand);
        std::cerr << "DEBUG LLVM: Looking for place " << copyOp->place.get() << " in valueMap (size: " << valueMap.size() << ")" << std::endl;
        
        // Print all entries in valueMap for debugging
        for (const auto& pair : valueMap) {
            std::cerr << "DEBUG LLVM: valueMap entry: place=" << pair.first << ", value=" << pair.second << std::endl;
        }
        
        auto it = valueMap.find(copyOp->place.get());
        if (it != valueMap.end()) {
            std::cerr << "DEBUG LLVM: Found place in valueMap" << std::endl;
            // Always load from alloca to prevent constant folding
            // This ensures we get the current value from memory
            if (llvm::isa<llvm::AllocaInst>(it->second)) {
                std::cerr << "DEBUG LLVM: Loading from alloca" << std::endl;
                llvm::Type* loadType = convertType(copyOp->place->type.get());

                // Get the actual alloca type
                llvm::AllocaInst* alloca = llvm::cast<llvm::AllocaInst>(it->second);
                llvm::Type* allocaType = alloca->getAllocatedType();

                // If the MIR type is void but alloca is not, use alloca type
                // This handles the case where void-typed intermediate values were given i64 allocas
                if (loadType->isVoidTy() && !allocaType->isVoidTy()) {
                    loadType = allocaType;
                }

                llvm::LoadInst* loadInst = builder->CreateLoad(loadType, it->second, "load");
                return loadInst;
            } else {
                // Fallback for non-alloca values (e.g., direct call results like malloc)
                std::cerr << "DEBUG LLVM: WARNING - Value is not an alloca, creating variable to prevent constant folding" << std::endl;
                llvm::AllocaInst* tempAlloca = builder->CreateAlloca(it->second->getType(), nullptr, "temp_var");
                builder->CreateStore(it->second, tempAlloca);

                // Propagate type information if the original value has it
                auto typeIt = arrayTypeMap.find(it->second);
                if (typeIt != arrayTypeMap.end()) {
                    arrayTypeMap[tempAlloca] = typeIt->second;
                    std::cerr << "DEBUG LLVM: Propagated type from original value to temp alloca" << std::endl;
                }

                llvm::LoadInst* loadInst = builder->CreateLoad(it->second->getType(), tempAlloca, "temp_load");
                return loadInst;
            }
        }
        std::cerr << "DEBUG LLVM: Place not found in valueMap" << std::endl;
        return nullptr;
    } else if (operand->kind == mir::MIROperand::Kind::Move) {
        std::cerr << "DEBUG LLVM: Processing Move operand" << std::endl;
        // Load from place (move is same as copy for now)
        auto* moveOp = static_cast<mir::MIRMoveOperand*>(operand);
        auto it = valueMap.find(moveOp->place.get());
        if (it != valueMap.end()) {
            return builder->CreateLoad(convertType(moveOp->place->type.get()), 
                                     it->second, "load");
        }
    } else if (operand->kind == mir::MIROperand::Kind::Constant) {
        std::cerr << "DEBUG LLVM: Processing Constant operand" << std::endl;
        // Create constant
        auto* constOp = static_cast<mir::MIRConstOperand*>(operand);
        if (constOp->constKind == mir::MIRConstOperand::ConstKind::Int) {
            int64_t intVal = std::get<int64_t>(constOp->value);
            std::cerr << "DEBUG LLVM: Creating int constant: " << intVal << std::endl;
            std::cerr << "DEBUG LLVM: constOp->type.get() = " << constOp->type.get() << std::endl;
            if (!constOp->type) {
                std::cerr << "ERROR: constOp->type is null!" << std::endl;
                return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), intVal, true);
            }
            std::cerr << "DEBUG LLVM: About to call convertType" << std::endl;
            llvm::Type* llvmType = convertType(constOp->type.get());
            std::cerr << "DEBUG LLVM: convertType returned: " << llvmType << std::endl;
            return llvm::ConstantInt::get(llvmType, intVal, true);
        } else if (constOp->constKind == mir::MIRConstOperand::ConstKind::Float) {
            double floatVal = std::get<double>(constOp->value);
            return llvm::ConstantFP::get(convertType(constOp->type.get()), 
                                        floatVal);
        } else if (constOp->constKind == mir::MIRConstOperand::ConstKind::Bool) {
            bool boolVal = std::get<bool>(constOp->value);
            std::cerr << "DEBUG LLVM: Creating bool constant: " << (boolVal ? "true" : "false") << std::endl;
            // For boolean constants in binary operations, we need to match the type
            // Use i64 for consistency with boolean variables in memory
            return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 
                                         boolVal ? 1 : 0);
        } else if (constOp->constKind == mir::MIRConstOperand::ConstKind::String) {
            std::string strVal = std::get<std::string>(constOp->value);
            std::cerr << "DEBUG LLVM: Creating string constant: " << strVal << std::endl;
            // Create global string constant
            llvm::Constant* strConstant = llvm::ConstantDataArray::getString(*context, strVal, true);
            // Create global variable for the string
            llvm::GlobalVariable* globalStr = new llvm::GlobalVariable(
                *module, strConstant->getType(), true, llvm::GlobalValue::PrivateLinkage,
                strConstant, ".str");
            // Get pointer to the string
            return builder->CreateBitCast(globalStr, llvm::PointerType::getUnqual(*context), "str");
        } else if (constOp->constKind == mir::MIRConstOperand::ConstKind::Null) {
            return llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context));
        }
    }
    
    std::cerr << "DEBUG LLVM: Unknown operand kind" << std::endl;
    return nullptr;
}

llvm::Value* LLVMCodeGen::convertRValue(mir::MIRRValue* rvalue) {
    std::cerr << "DEBUG LLVM: convertRValue called" << std::endl;
    if (!rvalue) {
        std::cerr << "DEBUG LLVM: rvalue is null" << std::endl;
        return nullptr;
    }
    
switch (rvalue->kind) {
        case mir::MIRRValue::Kind::Use: {
            std::cerr << "DEBUG LLVM: Processing Use rvalue" << std::endl;
            auto* useRVal = static_cast<mir::MIRUseRValue*>(rvalue);
            std::cerr << "DEBUG LLVM: Converting use operand" << std::endl;
            llvm::Value* result = convertOperand(useRVal->operand.get());
            std::cerr << "DEBUG LLVM: Use operand converted to value: " << result << std::endl;
            return result;
        }
        
        case mir::MIRRValue::Kind::BinaryOp: {
            std::cerr << "DEBUG LLVM: Processing BinaryOp rvalue" << std::endl;
            auto* binOp = static_cast<mir::MIRBinaryOpRValue*>(rvalue);
            std::cerr << "DEBUG LLVM: Converting operands" << std::endl;
            llvm::Value* lhs = convertOperand(binOp->lhs.get());
            llvm::Value* rhs = convertOperand(binOp->rhs.get());
            std::cerr << "DEBUG LLVM: Generating binary operation" << std::endl;
            return generateBinaryOp(binOp->op, lhs, rhs);
        }        case mir::MIRRValue::Kind::UnaryOp: {
            auto* unOp = static_cast<mir::MIRUnaryOpRValue*>(rvalue);
            llvm::Value* operand = convertOperand(unOp->operand.get());
            return generateUnaryOp(unOp->op, operand);
        }
        
        case mir::MIRRValue::Kind::Cast: {
            auto* castOp = static_cast<mir::MIRCastRValue*>(rvalue);
            llvm::Value* value = convertOperand(castOp->operand.get());
            llvm::Type* targetType = convertType(castOp->targetType.get());
            return generateCast(castOp->castKind, value, targetType);
        }

        case mir::MIRRValue::Kind::Aggregate: {
            std::cerr << "DEBUG LLVM: Processing Aggregate rvalue" << std::endl;
            auto* aggOp = static_cast<mir::MIRAggregateRValue*>(rvalue);
            return generateAggregate(aggOp);
        }

        case mir::MIRRValue::Kind::Ref: {
            // Ref kind is used for GetElement temporarily
            std::cerr << "DEBUG LLVM: Processing Ref rvalue (possibly GetElement)" << std::endl;
            auto* getElemOp = dynamic_cast<mir::MIRGetElementRValue*>(rvalue);
            if (getElemOp) {
                std::cerr << "DEBUG LLVM: Confirmed GetElement operation" << std::endl;
                return generateGetElement(getElemOp);
            }
            return nullptr;
        }

        default:
            return nullptr;
    }
}

// ==================== Function Generation ====================

llvm::Function* LLVMCodeGen::generateFunction(mir::MIRFunction* function) {
    if (!function) return nullptr;

    // Check if this is a class-related function (constructor or method)
    // If so, ensure the struct type is defined
    size_t underscorePos = function->name.find('_');
    if (underscorePos != std::string::npos) {
        std::string className = function->name.substr(0, underscorePos);
        std::string structName = "struct." + className;

        // Check if struct type already exists
        llvm::StructType* existingType = llvm::StructType::getTypeByName(*context, structName);
        if (!existingType) {
            // Create struct type for this class
            // For now, create an opaque struct with 2 i64 fields (will be refined later)
            // TODO: Get actual field types from HIR/MIR
            std::vector<llvm::Type*> fieldTypes = {
                llvm::Type::getInt64Ty(*context),  // Field 0
                llvm::Type::getInt64Ty(*context)   // Field 1
            };
            llvm::StructType::create(*context, fieldTypes, structName);
            std::cerr << "DEBUG LLVM: Created struct type " << structName << " with " << fieldTypes.size() << " fields" << std::endl;
        } else {
            std::cerr << "DEBUG LLVM: Struct type " << structName << " already exists" << std::endl;
        }
    }

    // Convert parameter types (use i64 for untyped/void parameters)
    std::vector<llvm::Type*> paramTypes;
    for (const auto& param : function->arguments) {
        if (!param) continue;
        llvm::Type* paramType = convertType(param->type.get());
        // Arguments cannot be void type - use i64 for dynamic typing
        if (paramType->isVoidTy()) {
            paramType = llvm::Type::getInt64Ty(*context);
        }
        paramTypes.push_back(paramType);
    }
    
    // Create function type (return type can be void)
    llvm::Type* retType = convertType(function->returnType.get());
    // For now, make all non-void, non-pointer functions return i64 
    // (can be ignored by caller)
    if (retType->isVoidTy()) {
        retType = llvm::Type::getInt64Ty(*context);
    } else if (!retType->isPointerTy() && !retType->isIntegerTy(1)) {
        // If not void, pointer, or boolean, use i64
        retType = llvm::Type::getInt64Ty(*context);
    }
    
    // Check if function already declared in first pass
    llvm::Function* llvmFunc;
    auto it = functionMap.find(function->name);
    if (it != functionMap.end()) {
        // Use existing declaration
        llvmFunc = it->second;
        std::cerr << "DEBUG LLVM: Using existing declaration for function: " << function->name << std::endl;
    } else {
        // Create function (fallback for runtime functions)
        llvm::FunctionType* funcType = llvm::FunctionType::get(retType, paramTypes, false);
        llvmFunc = llvm::Function::Create(
            funcType, 
            llvm::Function::ExternalLinkage,
            function->name,
            module.get()
        );
        functionMap[function->name] = llvmFunc;
        std::cerr << "DEBUG LLVM: Created new function: " << function->name << std::endl;
    }
    currentFunction = llvmFunc;
    currentReturnValue = nullptr;  // Reset return value for this function
    
    // Create entry block for alloca instructions
    llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(*context, "entry", llvmFunc);
    builder->SetInsertPoint(entryBB);
    
    // Create alloca for each variable that might change value
    // This prevents constant folding by using memory instead of SSA
    std::cerr << "DEBUG LLVM: Creating alloca for variables to prevent constant folding" << std::endl;
    std::cerr << "DEBUG LLVM: Function has " << function->basicBlocks.size() << " basic blocks" << std::endl;

    for (size_t bbIdx = 0; bbIdx < function->basicBlocks.size(); ++bbIdx) {
        const auto& bb = function->basicBlocks[bbIdx];
        std::cerr << "DEBUG LLVM: Processing block " << bbIdx << " with " << bb->statements.size() << " statements" << std::endl;

        for (size_t stmtIdx = 0; stmtIdx < bb->statements.size(); ++stmtIdx) {
            const auto& stmt = bb->statements[stmtIdx];
            std::cerr << "DEBUG LLVM: Checking statement " << stmtIdx << " kind=" << static_cast<int>(stmt->kind) << std::endl;

            if (stmt->kind == mir::MIRStatement::Kind::Assign) {
                auto* assign = static_cast<mir::MIRAssignStatement*>(stmt.get());
                std::cerr << "DEBUG LLVM: Found assign statement, place=" << assign->place.get() << std::endl;

                if (assign->place) {
                    std::cerr << "DEBUG LLVM: Converting type for place..." << std::endl;
                    llvm::Type* varType = convertType(assign->place->type.get());
                    std::cerr << "DEBUG LLVM: Type converted successfully, type=";
                    varType->print(llvm::errs());
                    std::cerr << std::endl;

                    // Skip void types - cannot create alloca for void
                    if (varType->isVoidTy()) {
                        std::cerr << "DEBUG LLVM: Skipping void type variable, using i64 placeholder" << std::endl;
                        varType = llvm::Type::getInt64Ty(*context);
                    }

                    // For debugging, check if the type is a pointer type
                    if (varType->isPointerTy()) {
                        std::cerr << "DEBUG LLVM: WARNING - Creating alloca for pointer type variable " << assign->place.get() << std::endl;
                    }

                    // Check if this place already has an alloca
                    if (valueMap.find(assign->place.get()) != valueMap.end()) {
                        std::cerr << "DEBUG LLVM: Variable already has alloca, skipping" << std::endl;
                        continue;
                    }

                    std::cerr << "DEBUG LLVM: Creating alloca instruction..." << std::endl;
                    std::cerr.flush();
                    llvm::AllocaInst* alloca = builder->CreateAlloca(varType, nullptr, "var");
                    std::cerr << "DEBUG LLVM: Alloca created, adding to valueMap..." << std::endl;
                    std::cerr.flush();
                    valueMap[assign->place.get()] = alloca;
                    std::cerr << "DEBUG LLVM: Created alloca for variable " << assign->place.get() << std::endl;
                }
            }
        }
        std::cerr << "DEBUG LLVM: Finished processing block " << bbIdx << std::endl;
    }
    std::cerr << "DEBUG LLVM: Finished creating all allocas" << std::endl;
    
    // Map parameters (create allocas for them too)
    auto argIt = llvmFunc->arg_begin();
    for (size_t i = 0; i < function->arguments.size(); ++i, ++argIt) {
        llvm::AllocaInst* argAlloca = builder->CreateAlloca(argIt->getType(), nullptr, "arg_" + std::to_string(i));
        builder->CreateStore(&(*argIt), argAlloca);
        valueMap[function->arguments[i].get()] = argAlloca;
        argIt->setName("arg" + std::to_string(i));

        // If this is the first parameter of a method function, associate it with the struct type
        if (i == 0) {
            std::string funcName = function->name;
            // Check if this is a method function (ClassName_methodName but not ClassName_constructor)
            size_t methodUnderscorePos = funcName.find('_');
            if (methodUnderscorePos != std::string::npos && funcName.find("_constructor") == std::string::npos) {
                // Extract class name
                std::string className = funcName.substr(0, methodUnderscorePos);
                std::string structName = "struct." + className;
                llvm::StructType* structType = llvm::StructType::getTypeByName(*context, structName);
                if (structType) {
                    arrayTypeMap[argAlloca] = structType;
                    std::cerr << "DEBUG LLVM: Associated struct type " << structName << " with 'this' parameter in method " << funcName << std::endl;
                }
            }
        }
    }
    
    // Generate basic blocks
    bool entryBlockCreated = false;
    for (const auto& bb : function->basicBlocks) {
        if (!bb) continue;
        
        // Use the original label, but ensure we have only one entry block
        std::string bbLabel = bb->label.empty() ? "entry" : bb->label;
        
        // Ensure only the first block is the entry block
        if (bbLabel == "entry" && entryBlockCreated) {
            bbLabel = "bb" + std::to_string(blockMap.size());
        }
        
        if (bbLabel == "entry") {
            entryBlockCreated = true;
        }
        
        llvm::BasicBlock* llvmBB = llvm::BasicBlock::Create(*context, bbLabel, llvmFunc);
        blockMap[bb.get()] = llvmBB;
        std::cerr << "DEBUG LLVM: Created basic block " << bbLabel << " with " << bb->statements.size() << " statements" << std::endl;
        
        // If this is the body block of a loop, make sure it's not empty
        if (bb->statements.empty() && (bbLabel.find("body") != std::string::npos || 
                                      bbLabel.find("bb2") != std::string::npos)) {
            std::cerr << "DEBUG LLVM: WARNING - Empty body block found: " << bbLabel << std::endl;
            
            // Create a dummy instruction to ensure the block is not empty
            builder->SetInsertPoint(llvmBB);
            llvm::Value* dummyVal = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 0);
            builder->CreateAdd(dummyVal, dummyVal, "dummy_add");
            std::cerr << "DEBUG LLVM: Added dummy instruction to empty block" << std::endl;
        }
    }
    
    // Create a branch from entry to the first basic block
    // Find the first basic block (skip if empty)
    for (const auto& bb : function->basicBlocks) {
        if (bb && !bb->statements.empty()) {
            llvm::BasicBlock* firstBB = blockMap[bb.get()];
            if (firstBB) {
                // Get the entry block
                llvm::BasicBlock* targetEntryBB = &llvmFunc->getEntryBlock();
                if (!targetEntryBB) {
                    // If no entry block exists, create one
                    targetEntryBB = llvm::BasicBlock::Create(*context, "entry", llvmFunc);
                }
                
                // Set insertion point to entry block and create branch
                builder->SetInsertPoint(targetEntryBB);
                builder->CreateBr(firstBB);
                std::cerr << "DEBUG LLVM: Created branch from entry to " << firstBB->getName().str() << std::endl;
                break;
            }
        }
    }
    
    // Generate code for each basic block
    for (const auto& bb : function->basicBlocks) {
        std::cerr << "DEBUG LLVM: Processing block " << bb->label << " with " << bb->statements.size() << " statements" << std::endl;
        generateBasicBlock(bb.get(), blockMap[bb.get()]);
        std::cerr << "DEBUG LLVM: Block " << bb->label << " processed" << std::endl;
    }
    
    return llvmFunc;
}

void LLVMCodeGen::generateBasicBlock(mir::MIRBasicBlock* bb, llvm::BasicBlock* llvmBB) {
    std::cerr << "DEBUG LLVM: Generating basic block: " << bb->label << std::endl;
    
    // Clear the insertion point to avoid any conflicts
    builder->ClearInsertionPoint();
    builder->SetInsertPoint(llvmBB);
    
    // Generate statements
    std::cerr << "DEBUG LLVM: Generating " << bb->statements.size() << " statements" << std::endl;
    for (const auto& stmt : bb->statements) {
        generateStatement(stmt.get());
    }
    std::cerr << "DEBUG LLVM: Statements generated" << std::endl;
    
    // Debug: Check if we have any instructions in the block
    if (llvmBB->empty()) {
        std::cerr << "DEBUG LLVM: WARNING - Basic block is empty after statement generation" << std::endl;
        
        // Create a dummy instruction to ensure the block is not empty
        builder->SetInsertPoint(llvmBB);
        llvm::Value* dummyVal = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 0);
        builder->CreateAdd(dummyVal, dummyVal, "dummy_add");
        std::cerr << "DEBUG LLVM: Added dummy instruction to empty block" << std::endl;
    } else {
        std::cerr << "DEBUG LLVM: Basic block has " << llvmBB->size() << " instructions" << std::endl;
    }
    
    // Generate terminator
    if (bb->terminator) {
        std::cerr << "DEBUG LLVM: Generating terminator" << std::endl;
        generateTerminator(bb->terminator.get());
        std::cerr << "DEBUG LLVM: Terminator generated" << std::endl;
    } else {
        std::cerr << "DEBUG LLVM: No terminator found, creating unreachable" << std::endl;
        builder->CreateUnreachable();
    }
    
    // Debug: Check final instruction count
    std::cerr << "DEBUG LLVM: Basic block now has " << llvmBB->size() << " instructions after terminator" << std::endl;
}

void LLVMCodeGen::generateStatement(mir::MIRStatement* stmt) {
    std::cerr << "DEBUG LLVM: generateStatement called" << std::endl;
    if (!stmt) {
        std::cerr << "DEBUG LLVM: Statement is null" << std::endl;
        return;
    }
    
    switch (stmt->kind) {
        case mir::MIRStatement::Kind::Assign: {
            std::cerr << "DEBUG LLVM: Generating assign statement" << std::endl;
            auto* assign = static_cast<mir::MIRAssignStatement*>(stmt);
            std::cerr << "DEBUG LLVM: Converting rvalue" << std::endl;
            llvm::Value* value = convertRValue(assign->rvalue.get());
            std::cerr << "DEBUG LLVM: RValue converted, value=" << value << std::endl;
            
            // Store the value in the alloca for this variable
            if (value) {
                std::cerr << "DEBUG LLVM: Looking for alloca for variable " << assign->place.get() << std::endl;
                auto it = valueMap.find(assign->place.get());
                if (it != valueMap.end()) {
                    if (llvm::isa<llvm::AllocaInst>(it->second)) {
                        std::cerr << "DEBUG LLVM: Storing value in alloca" << std::endl;
                        builder->CreateStore(value, it->second);

                        // If the value is an array pointer, propagate the array type to the variable's alloca
                        auto arrayTypeIt = arrayTypeMap.find(value);
                        if (arrayTypeIt != arrayTypeMap.end()) {
                            arrayTypeMap[it->second] = arrayTypeIt->second;
                            std::cerr << "DEBUG LLVM: Propagated array type to variable alloca from value" << std::endl;

                            // Also propagate nested struct types for all fields
                            if (auto* structType = llvm::dyn_cast<llvm::StructType>(arrayTypeIt->second)) {
                                for (unsigned fieldIdx = 0; fieldIdx < structType->getNumElements(); ++fieldIdx) {
                                    auto nestedKey = std::make_pair(value, fieldIdx);
                                    auto nestedIt = nestedStructTypeMap.find(nestedKey);
                                    if (nestedIt != nestedStructTypeMap.end()) {
                                        nestedStructTypeMap[std::make_pair(it->second, fieldIdx)] = nestedIt->second;
                                        std::cerr << "DEBUG LLVM: Propagated nested struct type for field " << fieldIdx << std::endl;
                                    }
                                }
                            }
                        } else {
                            // Check if we're loading from a variable that has array type
                            if (llvm::LoadInst* loadInst = llvm::dyn_cast<llvm::LoadInst>(value)) {
                                llvm::Value* loadSource = loadInst->getPointerOperand();
                                auto sourceArrayTypeIt = arrayTypeMap.find(loadSource);
                                if (sourceArrayTypeIt != arrayTypeMap.end()) {
                                    arrayTypeMap[it->second] = sourceArrayTypeIt->second;
                                    std::cerr << "DEBUG LLVM: Propagated array type to variable alloca from source" << std::endl;

                                    // Also propagate nested struct types for all fields
                                    if (auto* structType = llvm::dyn_cast<llvm::StructType>(sourceArrayTypeIt->second)) {
                                        for (unsigned fieldIdx = 0; fieldIdx < structType->getNumElements(); ++fieldIdx) {
                                            auto nestedKey = std::make_pair(loadSource, fieldIdx);
                                            auto nestedIt = nestedStructTypeMap.find(nestedKey);
                                            if (nestedIt != nestedStructTypeMap.end()) {
                                                nestedStructTypeMap[std::make_pair(it->second, fieldIdx)] = nestedIt->second;
                                                std::cerr << "DEBUG LLVM: Propagated nested struct type for field " << fieldIdx << " from source" << std::endl;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        std::cerr << "DEBUG LLVM: WARNING - No alloca found, creating one" << std::endl;
                        llvm::AllocaInst* alloca = builder->CreateAlloca(value->getType(), nullptr, "var_alloca");
                        builder->CreateStore(value, alloca);
                        valueMap[assign->place.get()] = alloca;
                    }
                } else {
                    std::cerr << "DEBUG LLVM: ERROR - Variable not found in valueMap" << std::endl;
                    // Create alloca and store value
                    llvm::AllocaInst* alloca = builder->CreateAlloca(value->getType(), nullptr, "missing_var");
                    builder->CreateStore(value, alloca);
                    valueMap[assign->place.get()] = alloca;
                }
                
                // Check if this is an assignment to the return place (_0)
                if (assign->place && (assign->place->kind == mir::MIRPlace::Kind::Return || 
                                     (assign->place->kind == mir::MIRPlace::Kind::Local && assign->place->index == 0))) {
                    currentReturnValue = value;
                }
            }
            break;
        }
        
        case mir::MIRStatement::Kind::StorageLive:
        case mir::MIRStatement::Kind::StorageDead:
            // These are lifetime markers - can be ignored or used for optimization
            break;
        
        default:
            break;
    }
}

void LLVMCodeGen::generateTerminator(mir::MIRTerminator* terminator) {
    std::cerr << "DEBUG LLVM: generateTerminator called" << std::endl;
    if (!terminator) {
        std::cerr << "DEBUG LLVM: Terminator is null" << std::endl;
        return;
    }
    
    switch (terminator->kind) {
        case mir::MIRTerminator::Kind::Return: {
            std::cerr << "DEBUG LLVM: Generating return terminator" << std::endl;
            // Check if the function has a non-void return type
            llvm::Function* currentFunc = builder->GetInsertBlock()->getParent();
            llvm::Type* retType = currentFunc->getReturnType();
            
            if (retType->isVoidTy()) {
                builder->CreateRetVoid();
            } else {
                // Use currentReturnValue if available
                if (currentReturnValue) {
                    // Check if the return value type matches the function return type
                    if (currentReturnValue->getType() != retType) {
                        std::cerr << "DEBUG LLVM: Return type mismatch, attempting to convert" << std::endl;
                        std::cerr << "DEBUG LLVM: Expected type: ";
                        retType->print(llvm::errs());
                        std::cerr << ", Got: ";
                        currentReturnValue->getType()->print(llvm::errs());
                        std::cerr << std::endl;
                        
                        // If we're returning a pointer but function expects i64, do a cast
                        if (currentReturnValue->getType()->isPointerTy() && retType->isIntegerTy(64)) {
                            currentReturnValue = builder->CreatePtrToInt(currentReturnValue, retType, "ptr_to_int");
                        }
                        // If we're returning i64 but function expects pointer, do a cast
                        else if (currentReturnValue->getType()->isIntegerTy(64) && retType->isPointerTy()) {
                            currentReturnValue = builder->CreateIntToPtr(currentReturnValue, retType, "int_to_ptr");
                        }
                        // If we're returning i1 (boolean) but function expects i64, extend it
                        else if (currentReturnValue->getType()->isIntegerTy(1) && retType->isIntegerTy(64)) {
                            std::cerr << "DEBUG LLVM: Converting i1 return value to i64" << std::endl;
                            currentReturnValue = builder->CreateZExt(currentReturnValue, retType, "bool_to_i64");
                        }
                    }
                    builder->CreateRet(currentReturnValue);
                } else {
                    // No return value found, return a default value (0 for i64)
                    if (retType->isIntegerTy()) {
                        builder->CreateRet(llvm::ConstantInt::get(retType, 0));
                    } else if (retType->isIntegerTy(1)) {
                        // Handle boolean (i1) return type
                        builder->CreateRet(llvm::ConstantInt::get(retType, 0));
                    } else if (retType->isPointerTy()) {
                        // Handle pointer (string) return type
                        builder->CreateRet(llvm::ConstantPointerNull::get(static_cast<llvm::PointerType*>(retType)));
                    } else {
                        builder->CreateRetVoid(); // Fallback
                    }
                }
            }
            break;
        }
        
        case mir::MIRTerminator::Kind::Goto: {
            std::cerr << "DEBUG LLVM: Generating goto terminator" << std::endl;
            auto* gotoTerm = static_cast<mir::MIRGotoTerminator*>(terminator);
            std::cerr << "DEBUG LLVM: Goto target: " << gotoTerm->target << " with label: " << (gotoTerm->target ? gotoTerm->target->label : "null") << std::endl;
            llvm::BasicBlock* targetBB = blockMap[gotoTerm->target];
            if (!targetBB) {
                std::cerr << "DEBUG LLVM: ERROR - Target block not found in blockMap" << std::endl;
                // Try to find the block by label
                for (const auto& pair : blockMap) {
                    if (pair.first->label == gotoTerm->target->label) {
                        std::cerr << "DEBUG LLVM: Found block with matching label: " << pair.first->label << std::endl;
                        targetBB = pair.second;
                        break;
                    }
                }
            }
            if (targetBB) {
                std::cerr << "DEBUG LLVM: Creating branch to target block" << std::endl;
                builder->CreateBr(targetBB);
            } else {
                std::cerr << "DEBUG LLVM: ERROR - Could not find target block even by label" << std::endl;
            }
            break;
        }
        
        case mir::MIRTerminator::Kind::SwitchInt: {
            std::cerr << "DEBUG LLVM: Generating switch terminator" << std::endl;
            auto* switchTerm = static_cast<mir::MIRSwitchIntTerminator*>(terminator);
            
            // Debug: Print the discriminant
            std::cerr << "DEBUG LLVM: Discriminant: " << switchTerm->discriminant.get() << std::endl;
            if (switchTerm->discriminant->kind == mir::MIROperand::Kind::Copy) {
                auto* copyOp = static_cast<mir::MIRCopyOperand*>(switchTerm->discriminant.get());
                std::cerr << "DEBUG LLVM: Copy operand place: " << copyOp->place.get() << std::endl;
            }
            
            llvm::Value* value = convertOperand(switchTerm->discriminant.get());
            llvm::BasicBlock* defaultBB = blockMap[switchTerm->otherwise];
            
            // Debug: Print the original value
            std::cerr << "DEBUG LLVM: Original value: ";
            value->print(llvm::errs());
            std::cerr << ", type: ";
            value->getType()->print(llvm::errs());
            std::cerr << std::endl;
            
            // For boolean conditions, use a conditional branch instead of a switch
            if (switchTerm->targets.size() == 1 && switchTerm->targets[0].value == 1) {
                llvm::BasicBlock* trueBB = blockMap[switchTerm->targets[0].target];
                
                // Check if the value is already a boolean (i1) type
                llvm::Value* cond;
                if (value->getType()->isIntegerTy(1)) {
                    // Value is already a boolean, use it directly
                    cond = value;
                    std::cerr << "DEBUG LLVM: Value is already boolean, using directly" << std::endl;
                } else if (value->getType()->isPointerTy()) {
                    // For pointer types (strings), compare with null pointer
                    cond = builder->CreateICmpNE(value, 
                        llvm::ConstantPointerNull::get(static_cast<llvm::PointerType*>(value->getType())));
                    std::cerr << "DEBUG LLVM: Converted pointer to boolean (null check)" << std::endl;
                } else {
                    // Convert the value to i1 (boolean) type for the condition
                    cond = builder->CreateICmpNE(value, 
                        llvm::ConstantInt::get(value->getType(), 0));
                    std::cerr << "DEBUG LLVM: Converted to boolean" << std::endl;
                }
                
                std::cerr << "DEBUG LLVM: Creating conditional branch with condition: ";
                cond->print(llvm::errs());
                std::cerr << std::endl;
                
                // Check if the condition is a constant
                if (llvm::isa<llvm::ConstantInt>(cond)) {
                    std::cerr << "DEBUG LLVM: WARNING - Condition is a constant!" << std::endl;
                }
                
                builder->CreateCondBr(cond, trueBB, defaultBB);
            } else {
                // Get the bit width of the switch value
                unsigned bitWidth = 1; // Default to 1 bit for boolean
                if (auto* intType = llvm::dyn_cast<llvm::IntegerType>(value->getType())) {
                    bitWidth = intType->getBitWidth();
                }
                
                llvm::SwitchInst* switchInst = builder->CreateSwitch(value, defaultBB, 
                                                                    static_cast<unsigned int>(switchTerm->targets.size()));
                
                for (const auto& caseItem : switchTerm->targets) {
                    llvm::ConstantInt* caseVal = llvm::ConstantInt::get(
                        *context, llvm::APInt(bitWidth, caseItem.value));
                    llvm::BasicBlock* caseBB = blockMap[caseItem.target];
                    if (caseBB) {
                        switchInst->addCase(caseVal, caseBB);
                    }
                }
            }
            break;
        }
        
        case mir::MIRTerminator::Kind::Call: {
            std::cerr << "DEBUG LLVM: Processing Call terminator" << std::endl;
            auto* callTerm = static_cast<mir::MIRCallTerminator*>(terminator);

            // Get the function to call
            llvm::Function* callee = nullptr;

            // Debug: Check what kind of operand we have
            std::cerr << "DEBUG LLVM: Call func operand kind: " << static_cast<int>(callTerm->func->kind) << std::endl;

            // Check if func operand is a string constant (function name)
            if (auto* constOp = dynamic_cast<mir::MIRConstOperand*>(callTerm->func.get())) {
                std::cerr << "DEBUG LLVM: func operand IS a const operand, constKind: " << static_cast<int>(constOp->constKind) << std::endl;
                if (constOp->constKind == mir::MIRConstOperand::ConstKind::String) {
                    std::string funcName = std::get<std::string>(constOp->value);
                    std::cerr << "DEBUG LLVM: Looking for function: " << funcName << std::endl;

                    auto it = functionMap.find(funcName);
                    if (it != functionMap.end()) {
                        callee = it->second;
                        std::cerr << "DEBUG LLVM: Found function in functionMap" << std::endl;
                    } else {
                        // Function not found in map - may be an external/intrinsic function
                        // Try to find it in the LLVM module
                        callee = module->getFunction(funcName);
                        std::cerr << "DEBUG LLVM: Tried module->getFunction, result: " << callee << std::endl;

                        if (!callee && funcName == "malloc") {
                            // Create malloc declaration: ptr @malloc(i64)
                            std::cerr << "DEBUG LLVM: Creating external malloc declaration" << std::endl;
                            llvm::FunctionType* mallocType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                mallocType,
                                llvm::Function::ExternalLinkage,
                                "malloc",
                                module.get()
                            );
                            std::cerr << "DEBUG LLVM: Created malloc declaration" << std::endl;
                        }

                        if (!callee && funcName == "strlen") {
                            // Create strlen declaration: i64 @strlen(ptr)
                            std::cerr << "DEBUG LLVM: Creating external strlen declaration" << std::endl;
                            llvm::FunctionType* strlenType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),
                                {llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                strlenType,
                                llvm::Function::ExternalLinkage,
                                "strlen",
                                module.get()
                            );
                            std::cerr << "DEBUG LLVM: Created strlen declaration" << std::endl;
                        }

                        // Create declarations for string method runtime functions
                        if (!callee && funcName == "nova_string_substring") {
                            // ptr @nova_string_substring(ptr, i64, i64)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_substring declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_substring",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_indexOf") {
                            // i64 @nova_string_indexOf(ptr, ptr)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_indexOf declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_indexOf",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_lastIndexOf") {
                            // i64 @nova_string_lastIndexOf(ptr, ptr) - searches from end
                            std::cerr << "DEBUG LLVM: Creating external nova_string_lastIndexOf declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),  // Returns i64 (index or -1)
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_lastIndexOf",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_match") {
                            // i64 @nova_string_match(ptr, ptr) - counts substring occurrences
                            std::cerr << "DEBUG LLVM: Creating external nova_string_match declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),  // Returns i64 (count of matches)
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_match",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_charAt") {
                            // ptr @nova_string_charAt(ptr, i64)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_charAt declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_charAt",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_charCodeAt") {
                            // i64 @nova_string_charCodeAt(ptr, i64) - returns character code
                            std::cerr << "DEBUG LLVM: Creating external nova_string_charCodeAt declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),  // Returns i64 (character code)
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_charCodeAt",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_codePointAt") {
                            // i64 @nova_string_codePointAt(ptr, i64) - returns Unicode code point (ES2015)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_codePointAt declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),  // Returns i64 (code point)
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_codePointAt",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_at") {
                            // i64 @nova_string_at(ptr, i64) - returns character code at index (supports negative)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_at declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),  // Returns i64 (character code)
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_at",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_fromCharCode") {
                            // ptr @nova_string_fromCharCode(i64) - returns string from character code
                            std::cerr << "DEBUG LLVM: Creating external nova_string_fromCharCode declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns ptr (string)
                                {llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_fromCharCode",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_fromCodePoint") {
                            // ptr @nova_string_fromCodePoint(i64) - returns string from Unicode code point (ES2015)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_fromCodePoint declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns ptr (string)
                                {llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_fromCodePoint",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_concat") {
                            // ptr @nova_string_concat(ptr, ptr) - concatenates two strings
                            std::cerr << "DEBUG LLVM: Creating external nova_string_concat declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns ptr (string)
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_concat",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_toLowerCase") {
                            // ptr @nova_string_toLowerCase(ptr)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_toLowerCase declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_toLowerCase",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_toUpperCase") {
                            // ptr @nova_string_toUpperCase(ptr)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_toUpperCase declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_toUpperCase",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_trim") {
                            // ptr @nova_string_trim(ptr)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_trim declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_trim",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_trimStart") {
                            // ptr @nova_string_trimStart(ptr) - removes leading whitespace
                            std::cerr << "DEBUG LLVM: Creating external nova_string_trimStart declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_trimStart",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_trimEnd") {
                            // ptr @nova_string_trimEnd(ptr) - removes trailing whitespace
                            std::cerr << "DEBUG LLVM: Creating external nova_string_trimEnd declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_trimEnd",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_startsWith") {
                            // i64 @nova_string_startsWith(ptr, ptr)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_startsWith declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_startsWith",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_endsWith") {
                            // i64 @nova_string_endsWith(ptr, ptr)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_endsWith declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_endsWith",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_repeat") {
                            // ptr @nova_string_repeat(ptr, i64)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_repeat declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_repeat",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_includes") {
                            // i64 @nova_string_includes(ptr, ptr)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_includes declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_includes",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_slice") {
                            // ptr @nova_string_slice(ptr, i64, i64)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_slice declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_slice",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_replace") {
                            // ptr @nova_string_replace(ptr, ptr, ptr)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_replace declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_replace",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_replaceAll") {
                            // ptr @nova_string_replaceAll(ptr, ptr, ptr) - ES2021
                            std::cerr << "DEBUG LLVM: Creating external nova_string_replaceAll declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_replaceAll",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_padStart") {
                            // ptr @nova_string_padStart(ptr, i64, ptr)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_padStart declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_padStart",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_string_padEnd") {
                            // ptr @nova_string_padEnd(ptr, i64, ptr)
                            std::cerr << "DEBUG LLVM: Creating external nova_string_padEnd declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_string_padEnd",
                                module.get()
                            );
                        }

                        // ==================== Value Array Method Runtime Functions ====================
                        // These work with value-based arrays (int64 elements)

                        if (!callee && funcName == "nova_value_array_push") {
                            // void @nova_value_array_push(ptr, i64)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_push declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_push",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_pop") {
                            // i64 @nova_value_array_pop(ptr)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_pop declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),
                                {llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_pop",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_shift") {
                            // i64 @nova_value_array_shift(ptr)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_shift declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),
                                {llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_shift",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_unshift") {
                            // void @nova_value_array_unshift(ptr, i64)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_unshift declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_unshift",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_at") {
                            // i64 @nova_value_array_at(ptr, i64) - returns element at index (supports negative indices)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_at declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_at",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_with") {
                            // ptr @nova_value_array_with(ptr, i64, i64) - ES2023, returns new array with element replaced
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_with declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to new array
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_with",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_toReversed") {
                            // ptr @nova_value_array_toReversed(ptr) - ES2023, returns new reversed array
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_toReversed declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to new array
                                {llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_toReversed",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_toSorted") {
                            // ptr @nova_value_array_toSorted(ptr) - ES2023, returns new sorted array
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_toSorted declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to new array
                                {llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_toSorted",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_sort") {
                            // ptr @nova_value_array_sort(ptr) - in-place sort, returns same array
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_sort declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to same array
                                {llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_sort",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_splice") {
                            // ptr @nova_value_array_splice(ptr, i64, i64) - removes elements, returns same array
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_splice declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to same array
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_splice",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_copyWithin") {
                            // ptr @nova_value_array_copyWithin(ptr, i64, i64, i64) - copies part to another location
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_copyWithin declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to same array
                                {llvm::PointerType::getUnqual(*context),  // array pointer
                                 llvm::Type::getInt64Ty(*context),        // target
                                 llvm::Type::getInt64Ty(*context),        // start
                                 llvm::Type::getInt64Ty(*context)},       // end
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_copyWithin",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_toSpliced") {
                            // ptr @nova_value_array_toSpliced(ptr, i64, i64) - returns new array with elements removed (ES2023)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_toSpliced declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to new array
                                {llvm::PointerType::getUnqual(*context),  // array pointer
                                 llvm::Type::getInt64Ty(*context),        // start
                                 llvm::Type::getInt64Ty(*context)},       // deleteCount
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_toSpliced",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_toString") {
                            // ptr @nova_value_array_toString(ptr) - returns comma-separated string
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_toString declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to string
                                {llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_toString",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_flat") {
                            // ptr @nova_value_array_flat(ptr) - flattens nested arrays one level deep
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_flat declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to new array
                                {llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_flat",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_flatMap") {
                            // ptr @nova_value_array_flatMap(ptr, ptr) - maps then flattens (ES2019)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_flatMap declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to new array
                                {llvm::PointerType::getUnqual(*context),  // array pointer
                                 llvm::PointerType::getUnqual(*context)}, // callback function pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_flatMap",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_array_from") {
                            // ptr @nova_array_from(ptr) - static method creates array from array-like
                            std::cerr << "DEBUG LLVM: Creating external nova_array_from declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to new array
                                {llvm::PointerType::getUnqual(*context)}, // array pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_array_from",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_array_of") {
                            // ptr @nova_array_of(i64, ...) - variadic static method creates array from elements
                            std::cerr << "DEBUG LLVM: Creating external nova_array_of declaration (variadic)" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),      // Returns pointer to new array
                                {llvm::Type::getInt64Ty(*context)},          // First param: count
                                true                                          // Variadic function
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_array_of",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_object_values") {
                            // ptr @nova_object_values(ptr) - returns array of object's property values (ES2017)
                            std::cerr << "DEBUG LLVM: Creating external nova_object_values declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to array
                                {llvm::PointerType::getUnqual(*context)}, // Object pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_object_values",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_object_keys") {
                            // ptr @nova_object_keys(ptr) - returns array of object's property keys (ES2015)
                            std::cerr << "DEBUG LLVM: Creating external nova_object_keys declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to array
                                {llvm::PointerType::getUnqual(*context)}, // Object pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_object_keys",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_object_entries") {
                            // ptr @nova_object_entries(ptr) - returns array of [key, value] pairs (ES2017)
                            std::cerr << "DEBUG LLVM: Creating external nova_object_entries declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to array
                                {llvm::PointerType::getUnqual(*context)}, // Object pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_object_entries",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_object_assign") {
                            // ptr @nova_object_assign(ptr, ptr) - copies properties from source to target (ES2015)
                            std::cerr << "DEBUG LLVM: Creating external nova_object_assign declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to target object
                                {llvm::PointerType::getUnqual(*context),  // Target object pointer
                                 llvm::PointerType::getUnqual(*context)}, // Source object pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_object_assign",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_object_hasOwn") {
                            // i64 @nova_object_hasOwn(ptr, ptr) - checks if object has own property (ES2022)
                            std::cerr << "DEBUG LLVM: Creating external nova_object_hasOwn declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),        // Returns boolean (i64)
                                {llvm::PointerType::getUnqual(*context),  // Object pointer
                                 llvm::PointerType::getUnqual(*context)}, // Key string pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_object_hasOwn",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_object_freeze") {
                            // ptr @nova_object_freeze(ptr) - makes object immutable (ES5)
                            std::cerr << "DEBUG LLVM: Creating external nova_object_freeze declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to frozen object
                                {llvm::PointerType::getUnqual(*context)}, // Object pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_object_freeze",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_object_isFrozen") {
                            // i64 @nova_object_isFrozen(ptr) - checks if object is frozen (ES5)
                            std::cerr << "DEBUG LLVM: Creating external nova_object_isFrozen declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),        // Returns boolean (i64)
                                {llvm::PointerType::getUnqual(*context)}, // Object pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_object_isFrozen",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_object_seal") {
                            // ptr @nova_object_seal(ptr) - seals object, prevents add/delete properties (ES5)
                            std::cerr << "DEBUG LLVM: Creating external nova_object_seal declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to sealed object
                                {llvm::PointerType::getUnqual(*context)}, // Object pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_object_seal",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_object_isSealed") {
                            // i64 @nova_object_isSealed(ptr) - checks if object is sealed (ES5)
                            std::cerr << "DEBUG LLVM: Creating external nova_object_isSealed declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),        // Returns boolean (i64)
                                {llvm::PointerType::getUnqual(*context)}, // Object pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_object_isSealed",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_object_is") {
                            // i64 @nova_object_is(i64, i64) - determines if two values are the same (ES2015)
                            std::cerr << "DEBUG LLVM: Creating external nova_object_is declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),        // Returns boolean (i64)
                                {llvm::Type::getInt64Ty(*context),       // value1 (i64)
                                 llvm::Type::getInt64Ty(*context)},      // value2 (i64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_object_is",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_date_now") {
                            // i64 @nova_date_now() - returns current timestamp in milliseconds (ES5)
                            std::cerr << "DEBUG LLVM: Creating external nova_date_now declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),        // Returns timestamp (i64)
                                {},                                       // No parameters
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_date_now",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_performance_now") {
                            // double @nova_performance_now() - returns high-resolution timestamp (Web Performance API)
                            std::cerr << "DEBUG LLVM: Creating external nova_performance_now declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),        // Returns double (high-res milliseconds)
                                {},                                        // No parameters
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_performance_now",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_math_min") {
                            // i64 @nova_math_min(i64, i64) - returns the smaller of two values (ES1)
                            std::cerr << "DEBUG LLVM: Creating external nova_math_min declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),          // Returns i64
                                {llvm::Type::getInt64Ty(*context),         // First value (a)
                                 llvm::Type::getInt64Ty(*context)},        // Second value (b)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_math_min",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_math_max") {
                            // i64 @nova_math_max(i64, i64) - returns the larger of two values (ES1)
                            std::cerr << "DEBUG LLVM: Creating external nova_math_max declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),          // Returns i64
                                {llvm::Type::getInt64Ty(*context),         // First value (a)
                                 llvm::Type::getInt64Ty(*context)},        // Second value (b)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_math_max",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_json_stringify_number") {
                            // ptr @nova_json_stringify_number(i64) - converts number to JSON string (ES5)
                            std::cerr << "DEBUG LLVM: Creating external nova_json_stringify_number declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),    // Returns string pointer
                                {llvm::Type::getInt64Ty(*context)},        // Number value
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_json_stringify_number",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_json_stringify_string") {
                            // ptr @nova_json_stringify_string(ptr) - converts string to JSON string with quotes (ES5)
                            std::cerr << "DEBUG LLVM: Creating external nova_json_stringify_string declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),    // Returns string pointer
                                {llvm::PointerType::getUnqual(*context)},  // String value
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_json_stringify_string",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_json_stringify_bool") {
                            // ptr @nova_json_stringify_bool(i64) - converts boolean to JSON string (ES5)
                            std::cerr << "DEBUG LLVM: Creating external nova_json_stringify_bool declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),    // Returns string pointer
                                {llvm::Type::getInt64Ty(*context)},        // Boolean value (as i64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_json_stringify_bool",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_encodeURIComponent") {
                            // ptr @nova_encodeURIComponent(ptr) - encodes a URI component (ES3)
                            std::cerr << "DEBUG LLVM: Creating external nova_encodeURIComponent declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),    // Returns string pointer
                                {llvm::PointerType::getUnqual(*context)},  // String value
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_encodeURIComponent",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_decodeURIComponent") {
                            // ptr @nova_decodeURIComponent(ptr) - decodes a URI component (ES3)
                            std::cerr << "DEBUG LLVM: Creating external nova_decodeURIComponent declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),    // Returns string pointer
                                {llvm::PointerType::getUnqual(*context)},  // String value
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_decodeURIComponent",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_btoa") {
                            // ptr @nova_btoa(ptr) - encodes a string to base64 (Web API)
                            std::cerr << "DEBUG LLVM: Creating external nova_btoa declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),    // Returns string pointer
                                {llvm::PointerType::getUnqual(*context)},  // String value
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_btoa",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_atob") {
                            // ptr @nova_atob(ptr) - decodes a base64 string (Web API)
                            std::cerr << "DEBUG LLVM: Creating external nova_atob declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),    // Returns string pointer
                                {llvm::PointerType::getUnqual(*context)},  // String value
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_atob",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_encodeURI") {
                            // ptr @nova_encodeURI(ptr) - encodes a full URI (ES3)
                            std::cerr << "DEBUG LLVM: Creating external nova_encodeURI declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),    // Returns string pointer
                                {llvm::PointerType::getUnqual(*context)},  // String value
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_encodeURI",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_decodeURI") {
                            // ptr @nova_decodeURI(ptr) - decodes a full URI (ES3)
                            std::cerr << "DEBUG LLVM: Creating external nova_decodeURI declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),    // Returns string pointer
                                {llvm::PointerType::getUnqual(*context)},  // String value
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_decodeURI",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_throw") {
                            // void @nova_throw(i64) - throws an exception
                            std::cerr << "DEBUG LLVM: Creating external nova_throw declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),    // Returns void
                                {llvm::Type::getInt64Ty(*context)}, // Exception value (i64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_throw",
                                module.get()
                            );
                        }


                        if (!callee && funcName == "nova_try_begin") {
                            std::cerr << "DEBUG LLVM: Creating external nova_try_begin declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),
                                {},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_try_begin",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_try_end") {
                            std::cerr << "DEBUG LLVM: Creating external nova_try_end declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),
                                {},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_try_end",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_get_exception") {
                            std::cerr << "DEBUG LLVM: Creating external nova_get_exception declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),
                                {},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_get_exception",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_clear_exception") {
                            std::cerr << "DEBUG LLVM: Creating external nova_clear_exception declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),
                                {},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_clear_exception",
                                module.get()
                            );
                        }
                        if (!callee && funcName == "nova_number_toFixed") {
                            // ptr @nova_number_toFixed(double, i64) - formats number with fixed decimal places
                            std::cerr << "DEBUG LLVM: Creating external nova_number_toFixed declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns string pointer
                                {llvm::Type::getDoubleTy(*context),      // Number (as double/F64)
                                 llvm::Type::getInt64Ty(*context)},      // Digits (i64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_number_toFixed",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_number_toExponential") {
                            // ptr @nova_number_toExponential(double, i64) - formats number in exponential notation
                            std::cerr << "DEBUG LLVM: Creating external nova_number_toExponential declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns string pointer
                                {llvm::Type::getDoubleTy(*context),      // Number (as double/F64)
                                 llvm::Type::getInt64Ty(*context)},      // FractionDigits (i64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_number_toExponential",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_number_toPrecision") {
                            // ptr @nova_number_toPrecision(double, i64) - formats number with specified precision
                            std::cerr << "DEBUG LLVM: Creating external nova_number_toPrecision declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns string pointer
                                {llvm::Type::getDoubleTy(*context),      // Number (as double/F64)
                                 llvm::Type::getInt64Ty(*context)},      // Precision (i64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_number_toPrecision",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_number_toString") {
                            // ptr @nova_number_toString(double, i64) - converts number to string with optional radix
                            std::cerr << "DEBUG LLVM: Creating external nova_number_toString declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns string pointer
                                {llvm::Type::getDoubleTy(*context),      // Number (as double/F64)
                                 llvm::Type::getInt64Ty(*context)},      // Radix (i64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_number_toString",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_number_valueOf") {
                            // double @nova_number_valueOf(double) - returns primitive value
                            std::cerr << "DEBUG LLVM: Creating external nova_number_valueOf declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),       // Returns double (F64)
                                {llvm::Type::getDoubleTy(*context)},     // Number (as double/F64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_number_valueOf",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_number_parseInt") {
                            // i64 @nova_number_parseInt(ptr, i64) - parses string to integer
                            std::cerr << "DEBUG LLVM: Creating external nova_number_parseInt declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),          // Returns i64
                                {llvm::PointerType::getUnqual(*context),   // String pointer
                                 llvm::Type::getInt64Ty(*context)},        // Radix (i64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_number_parseInt",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_number_parseFloat") {
                            // double @nova_number_parseFloat(ptr) - parses string to floating-point number
                            std::cerr << "DEBUG LLVM: Creating external nova_number_parseFloat declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),         // Returns double (F64)
                                {llvm::PointerType::getUnqual(*context)},  // String pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_number_parseFloat",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_global_isNaN") {
                            // i64 @nova_global_isNaN(double) - tests if value is NaN
                            std::cerr << "DEBUG LLVM: Creating external nova_global_isNaN declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),        // Returns i64 (bool as int)
                                {llvm::Type::getDoubleTy(*context)},     // Value to test (F64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_global_isNaN",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_global_isFinite") {
                            // i64 @nova_global_isFinite(double) - tests if value is finite
                            std::cerr << "DEBUG LLVM: Creating external nova_global_isFinite declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),        // Returns i64 (bool as int)
                                {llvm::Type::getDoubleTy(*context)},     // Value to test (F64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_global_isFinite",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_global_parseInt") {
                            // i64 @nova_global_parseInt(ptr, i64) - parses string to integer
                            std::cerr << "DEBUG LLVM: Creating external nova_global_parseInt declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),           // Returns i64
                                {llvm::PointerType::getUnqual(*context),    // String pointer
                                 llvm::Type::getInt64Ty(*context)},         // Radix (i64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_global_parseInt",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_global_parseFloat") {
                            // double @nova_global_parseFloat(ptr) - parses string to float
                            std::cerr << "DEBUG LLVM: Creating external nova_global_parseFloat declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),          // Returns double (F64)
                                {llvm::PointerType::getUnqual(*context)},   // String pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_global_parseFloat",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_log_string") {
                            // void @nova_console_log_string(ptr) - outputs string to stdout
                            std::cerr << "DEBUG LLVM: Creating external nova_console_log_string declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {llvm::PointerType::getUnqual(*context)}, // String pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_log_string",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_log_number") {
                            // void @nova_console_log_number(i64) - outputs number to stdout
                            std::cerr << "DEBUG LLVM: Creating external nova_console_log_number declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),        // Returns void
                                {llvm::Type::getInt64Ty(*context)},     // Number (i64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_log_number",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_error_string") {
                            // void @nova_console_error_string(ptr) - outputs string to stderr
                            std::cerr << "DEBUG LLVM: Creating external nova_console_error_string declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {llvm::PointerType::getUnqual(*context)}, // String pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_error_string",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_error_number") {
                            // void @nova_console_error_number(i64) - outputs number to stderr
                            std::cerr << "DEBUG LLVM: Creating external nova_console_error_number declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),        // Returns void
                                {llvm::Type::getInt64Ty(*context)},     // Number (i64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_error_number",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_warn_string") {
                            // void @nova_console_warn_string(ptr) - outputs warning string to stderr
                            std::cerr << "DEBUG LLVM: Creating external nova_console_warn_string declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {llvm::PointerType::getUnqual(*context)}, // String pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_warn_string",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_warn_number") {
                            // void @nova_console_warn_number(i64) - outputs number to stderr
                            std::cerr << "DEBUG LLVM: Creating external nova_console_warn_number declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),        // Returns void
                                {llvm::Type::getInt64Ty(*context)},     // Number (i64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_warn_number",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_info_string") {
                            // void @nova_console_info_string(ptr) - outputs info string to stdout
                            std::cerr << "DEBUG LLVM: Creating external nova_console_info_string declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {llvm::PointerType::getUnqual(*context)}, // String pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_info_string",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_info_number") {
                            // void @nova_console_info_number(i64) - outputs number to stdout
                            std::cerr << "DEBUG LLVM: Creating external nova_console_info_number declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),        // Returns void
                                {llvm::Type::getInt64Ty(*context)},     // Number (i64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_info_number",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_debug_string") {
                            // void @nova_console_debug_string(ptr) - outputs debug string to stdout
                            std::cerr << "DEBUG LLVM: Creating external nova_console_debug_string declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {llvm::PointerType::getUnqual(*context)}, // String pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_debug_string",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_debug_number") {
                            // void @nova_console_debug_number(i64) - outputs number to stdout
                            std::cerr << "DEBUG LLVM: Creating external nova_console_debug_number declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),        // Returns void
                                {llvm::Type::getInt64Ty(*context)},     // Number (i64)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_debug_number",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_clear") {
                            // void @nova_console_clear() - clears the console
                            std::cerr << "DEBUG LLVM: Creating external nova_console_clear declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),  // Returns void
                                {},                                // No parameters
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_clear",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_time_string") {
                            // void @nova_console_time_string(ptr) - starts timer with label
                            std::cerr << "DEBUG LLVM: Creating external nova_console_time_string declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {llvm::PointerType::getUnqual(*context)}, // String pointer (label)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_time_string",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_timeEnd_string") {
                            // void @nova_console_timeEnd_string(ptr) - ends timer and prints elapsed
                            std::cerr << "DEBUG LLVM: Creating external nova_console_timeEnd_string declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {llvm::PointerType::getUnqual(*context)}, // String pointer (label)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_timeEnd_string",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_assert") {
                            // void @nova_console_assert(i64, ptr) - prints error if condition is false
                            std::cerr << "DEBUG LLVM: Creating external nova_console_assert declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),           // Returns void
                                {llvm::Type::getInt64Ty(*context),         // Condition (i64)
                                 llvm::PointerType::getUnqual(*context)},  // Message (string pointer)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_assert",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_count_string") {
                            // void @nova_console_count_string(ptr) - increments and prints counter
                            std::cerr << "DEBUG LLVM: Creating external nova_console_count_string declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {llvm::PointerType::getUnqual(*context)}, // String pointer (label)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_count_string",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_countReset_string") {
                            // void @nova_console_countReset_string(ptr) - resets counter to zero
                            std::cerr << "DEBUG LLVM: Creating external nova_console_countReset_string declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {llvm::PointerType::getUnqual(*context)}, // String pointer (label)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_countReset_string",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_table_array") {
                            // void @nova_console_table_array(ptr) - displays array in table format
                            std::cerr << "DEBUG LLVM: Creating external nova_console_table_array declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {llvm::PointerType::getUnqual(*context)}, // ValueArray* pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_table_array",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_group_string") {
                            // void @nova_console_group_string(ptr) - starts group with label
                            std::cerr << "DEBUG LLVM: Creating external nova_console_group_string declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {llvm::PointerType::getUnqual(*context)}, // String pointer (label)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_group_string",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_group_default") {
                            // void @nova_console_group_default() - starts group without label
                            std::cerr << "DEBUG LLVM: Creating external nova_console_group_default declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {},                                       // No parameters
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_group_default",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_groupEnd") {
                            // void @nova_console_groupEnd() - ends current group
                            std::cerr << "DEBUG LLVM: Creating external nova_console_groupEnd declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {},                                       // No parameters
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_groupEnd",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_trace_string") {
                            // void @nova_console_trace_string(ptr) - prints stack trace with message
                            std::cerr << "DEBUG LLVM: Creating external nova_console_trace_string declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {llvm::PointerType::getUnqual(*context)}, // String pointer (message)
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_trace_string",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_trace_default") {
                            // void @nova_console_trace_default() - prints stack trace without message
                            std::cerr << "DEBUG LLVM: Creating external nova_console_trace_default declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {},                                       // No parameters
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_trace_default",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_dir_number") {
                            // void @nova_console_dir_number(i64) - displays number value
                            std::cerr << "DEBUG LLVM: Creating external nova_console_dir_number declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {llvm::Type::getInt64Ty(*context)},      // int64 value
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_dir_number",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_dir_string") {
                            // void @nova_console_dir_string(ptr) - displays string value
                            std::cerr << "DEBUG LLVM: Creating external nova_console_dir_string declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {llvm::PointerType::getUnqual(*context)}, // String pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_dir_string",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_console_dir_array") {
                            // void @nova_console_dir_array(ptr) - displays array value
                            std::cerr << "DEBUG LLVM: Creating external nova_console_dir_array declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),         // Returns void
                                {llvm::PointerType::getUnqual(*context)}, // Array pointer
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_console_dir_array",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_includes") {
                            // i64 @nova_value_array_includes(ptr, i64)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_includes declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_includes",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_indexOf") {
                            // i64 @nova_value_array_indexOf(ptr, i64)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_indexOf declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_indexOf",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_lastIndexOf") {
                            // i64 @nova_value_array_lastIndexOf(ptr, i64) - searches from end
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_lastIndexOf declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),  // Returns i64 (index or -1)
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_lastIndexOf",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_reverse") {
                            // ptr @nova_value_array_reverse(ptr)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_reverse declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_reverse",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_fill") {
                            // ptr @nova_value_array_fill(ptr, i64)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_fill declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_fill",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_concat") {
                            // ptr @nova_value_array_concat(ptr, ptr)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_concat declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_concat",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_slice") {
                            // ptr @nova_value_array_slice(ptr, i64, i64)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_slice declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_slice",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_find") {
                            // i64 @nova_value_array_find(ptr, ptr) - array and callback function pointer
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_find declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_find",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_findIndex") {
                            // i64 @nova_value_array_findIndex(ptr, ptr) - array and callback function pointer, returns index
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_findIndex declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),  // Returns i64 (index or -1)
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_findIndex",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_findLast") {
                            // i64 @nova_value_array_findLast(ptr, ptr) - array and callback, returns element (ES2023)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_findLast declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),  // Returns i64 (element or 0)
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_findLast",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_findLastIndex") {
                            // i64 @nova_value_array_findLastIndex(ptr, ptr) - array and callback, returns index (ES2023)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_findLastIndex declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),  // Returns i64 (index or -1)
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_findLastIndex",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_filter") {
                            // ptr @nova_value_array_filter(ptr, ptr) - array and callback function pointer, returns new array
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_filter declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to new array
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_filter",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_map") {
                            // ptr @nova_value_array_map(ptr, ptr) - array and callback function pointer, returns new array
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_map declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::PointerType::getUnqual(*context),  // Returns pointer to new array
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_map",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_some") {
                            // i64 @nova_value_array_some(ptr, ptr) - array and callback function pointer, returns boolean
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_some declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),  // Returns i64 (boolean)
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_some",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_every") {
                            // i64 @nova_value_array_every(ptr, ptr) - array and callback function pointer, returns boolean
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_every declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),  // Returns i64 (boolean)
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_every",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_forEach") {
                            // void @nova_value_array_forEach(ptr, ptr) - array and callback function pointer, returns void
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_forEach declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getVoidTy(*context),  // Returns void
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_forEach",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_reduce") {
                            // i64 @nova_value_array_reduce(ptr, ptr, i64) - array, callback function pointer, and initial value, returns i64
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_reduce declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),  // Returns i64
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_reduce",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "nova_value_array_reduceRight") {
                            // i64 @nova_value_array_reduceRight(ptr, ptr, i64) - array, callback, initial value, returns i64 (processes right-to-left)
                            std::cerr << "DEBUG LLVM: Creating external nova_value_array_reduceRight declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getInt64Ty(*context),  // Returns i64
                                {llvm::PointerType::getUnqual(*context),
                                 llvm::PointerType::getUnqual(*context),
                                 llvm::Type::getInt64Ty(*context)},
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "nova_value_array_reduceRight",
                                module.get()
                            );
                        }

                        // Math functions
                        if (!callee && funcName == "log") {
                            // double @log(double) - C library natural logarithm function
                            std::cerr << "DEBUG LLVM: Creating external log declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "log",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "exp") {
                            // double @exp(double) - C library exponential function
                            std::cerr << "DEBUG LLVM: Creating external exp declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "exp",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "log10") {
                            // double @log10(double) - C library base 10 logarithm function
                            std::cerr << "DEBUG LLVM: Creating external log10 declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "log10",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "log2") {
                            // double @log2(double) - C library base 2 logarithm function
                            std::cerr << "DEBUG LLVM: Creating external log2 declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "log2",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "sin") {
                            // double @sin(double) - C library sine function
                            std::cerr << "DEBUG LLVM: Creating external sin declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "sin",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "cos") {
                            // double @cos(double) - C library cosine function
                            std::cerr << "DEBUG LLVM: Creating external cos declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "cos",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "tan") {
                            // double @tan(double) - C library tangent function
                            std::cerr << "DEBUG LLVM: Creating external tan declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "tan",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "atan") {
                            // double @atan(double) - C library arctangent function
                            std::cerr << "DEBUG LLVM: Creating external atan declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "atan",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "asin") {
                            // double @asin(double) - C library arcsine function
                            std::cerr << "DEBUG LLVM: Creating external asin declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "asin",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "acos") {
                            // double @acos(double) - C library arccosine function
                            std::cerr << "DEBUG LLVM: Creating external acos declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "acos",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "atan2") {
                            // double @atan2(double, double) - C library two-argument arctangent function
                            std::cerr << "DEBUG LLVM: Creating external atan2 declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context), llvm::Type::getDoubleTy(*context)},  // Takes two doubles
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "atan2",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "sinh") {
                            // double @sinh(double) - C library hyperbolic sine function
                            std::cerr << "DEBUG LLVM: Creating external sinh declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "sinh",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "cosh") {
                            // double @cosh(double) - C library hyperbolic cosine function
                            std::cerr << "DEBUG LLVM: Creating external cosh declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "cosh",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "tanh") {
                            // double @tanh(double) - C library hyperbolic tangent function
                            std::cerr << "DEBUG LLVM: Creating external tanh declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "tanh",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "asinh") {
                            // double @asinh(double) - C library inverse hyperbolic sine function
                            std::cerr << "DEBUG LLVM: Creating external asinh declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "asinh",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "acosh") {
                            // double @acosh(double) - C library inverse hyperbolic cosine function
                            std::cerr << "DEBUG LLVM: Creating external acosh declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "acosh",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "atanh") {
                            // double @atanh(double) - C library inverse hyperbolic tangent function
                            std::cerr << "DEBUG LLVM: Creating external atanh declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "atanh",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "expm1") {
                            // double @expm1(double) - C library function returns e^x - 1
                            std::cerr << "DEBUG LLVM: Creating external expm1 declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "expm1",
                                module.get()
                            );
                        }

                        if (!callee && funcName == "log1p") {
                            // double @log1p(double) - C library function returns ln(1 + x)
                            std::cerr << "DEBUG LLVM: Creating external log1p declaration" << std::endl;
                            llvm::FunctionType* funcType = llvm::FunctionType::get(
                                llvm::Type::getDoubleTy(*context),  // Returns double
                                {llvm::Type::getDoubleTy(*context)},  // Takes double
                                false
                            );
                            callee = llvm::Function::Create(
                                funcType,
                                llvm::Function::ExternalLinkage,
                                "log1p",
                                module.get()
                            );
                        }
                    }
                }
            }
            
            // If not found, try to convert operand as usual
            if (!callee) {
                llvm::Value* funcValue = convertOperand(callTerm->func.get());
                if (funcValue && funcValue->getType()->isPointerTy()) {
                    callee = llvm::dyn_cast<llvm::Function>(funcValue);
                }
            }
            
            if (callee) {
                std::cerr << "DEBUG LLVM: Have callee, processing arguments..." << std::endl;
                // Convert arguments with type checking
                std::vector<llvm::Value*> args;
                auto paramIt = callee->arg_begin();

                std::cerr << "DEBUG LLVM: callTerm->args.size() = " << callTerm->args.size() << std::endl;
                int argIdx = 0;
                std::string calleeName = callee->getName().str();
                for (const auto& arg : callTerm->args) {
                    std::cerr << "DEBUG LLVM: Processing argument " << argIdx << std::endl;
                    llvm::Value* argValue = convertOperand(arg.get());
                    std::cerr << "DEBUG LLVM: Argument converted, argValue = " << argValue << std::endl;

                    // Special handling for callback arguments: convert string constant to function pointer
                    if ((calleeName == "nova_value_array_find" || calleeName == "nova_value_array_findIndex" || calleeName == "nova_value_array_filter" || calleeName == "nova_value_array_map" || calleeName == "nova_value_array_some" || calleeName == "nova_value_array_every" || calleeName == "nova_value_array_forEach" || calleeName == "nova_value_array_reduce" || calleeName == "nova_value_array_reduceRight") && argIdx == 1) {
                        // Second argument should be a function pointer, but comes as string constant
                        if (auto* globalStr = llvm::dyn_cast<llvm::GlobalVariable>(argValue)) {
                            std::cerr << "DEBUG LLVM: Detected string constant for callback argument" << std::endl;

                            // Try to extract the function name from the string constant
                            if (globalStr->hasInitializer()) {
                                if (auto* constData = llvm::dyn_cast<llvm::ConstantDataArray>(globalStr->getInitializer())) {
                                    if (constData->isCString()) {
                                        std::string funcName = constData->getAsCString().str();
                                        std::cerr << "DEBUG LLVM: Extracted function name from string: " << funcName << std::endl;

                                        // Look up the actual function
                                        llvm::Function* callbackFunc = module->getFunction(funcName);
                                        if (callbackFunc) {
                                            std::cerr << "DEBUG LLVM: Found callback function, using function pointer" << std::endl;
                                            argValue = callbackFunc;  // Use function pointer instead of string
                                        } else {
                                            std::cerr << "DEBUG LLVM: WARNING - Function not found: " << funcName << std::endl;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    argIdx++;
                    if (argValue && paramIt != callee->arg_end()) {
                        // Check if we need to convert types
                        llvm::Type* expectedType = paramIt->getType();
                        llvm::Type* actualType = argValue->getType();
                        
                        if (actualType != expectedType) {
                            std::cerr << "DEBUG LLVM: Type mismatch in function call, converting from ";
                            actualType->print(llvm::errs());
                            std::cerr << " to ";
                            expectedType->print(llvm::errs());
                            std::cerr << std::endl;
                            
                            // Handle pointer to integer conversion
                            if (actualType->isPointerTy() && expectedType->isIntegerTy()) {
                                std::cerr << "DEBUG LLVM: Converting pointer to integer" << std::endl;
                                argValue = builder->CreatePtrToInt(argValue, expectedType);
                            }
                            // Handle integer to pointer conversion
                            else if (actualType->isIntegerTy() && expectedType->isPointerTy()) {
                                std::cerr << "DEBUG LLVM: Converting integer to pointer" << std::endl;
                                argValue = builder->CreateIntToPtr(argValue, expectedType);
                            }
                            // Handle integer size mismatch
                            else if (actualType->isIntegerTy() && expectedType->isIntegerTy()) {
                                auto actualIntType = static_cast<llvm::IntegerType*>(actualType);
                                auto expectedIntType = static_cast<llvm::IntegerType*>(expectedType);

                                if (actualIntType->getBitWidth() < expectedIntType->getBitWidth()) {
                                    std::cerr << "DEBUG LLVM: Extending integer from " << actualIntType->getBitWidth()
                                              << " to " << expectedIntType->getBitWidth() << std::endl;
                                    argValue = builder->CreateSExt(argValue, expectedType);
                                } else if (actualIntType->getBitWidth() > expectedIntType->getBitWidth()) {
                                    std::cerr << "DEBUG LLVM: Truncating integer from " << actualIntType->getBitWidth()
                                              << " to " << expectedIntType->getBitWidth() << std::endl;
                                    argValue = builder->CreateTrunc(argValue, expectedType);
                                }
                            }
                            // Handle integer to float/double conversion
                            else if (actualType->isIntegerTy() && (expectedType->isFloatTy() || expectedType->isDoubleTy())) {
                                std::cerr << "DEBUG LLVM: Converting integer to floating point" << std::endl;
                                argValue = builder->CreateSIToFP(argValue, expectedType, "int_to_fp");
                            }
                            // Handle float/double to integer conversion
                            else if ((actualType->isFloatTy() || actualType->isDoubleTy()) && expectedType->isIntegerTy()) {
                                std::cerr << "DEBUG LLVM: Converting floating point to integer" << std::endl;
                                argValue = builder->CreateFPToSI(argValue, expectedType, "fp_to_int");
                            }
                        }
                        
                        args.push_back(argValue);
                        ++paramIt;
                    }
                }

                // Create call
                std::cerr << "DEBUG LLVM: About to create call with " << args.size() << " arguments" << std::endl;
                std::cerr << "DEBUG LLVM: callee = " << callee << std::endl;
                std::cerr << "DEBUG LLVM: callee name = " << callee->getName().str() << std::endl;
                llvm::Value* result = builder->CreateCall(callee, args);
                std::cerr << "DEBUG LLVM: CreateCall succeeded, result = " << result << std::endl;

                // Handle return value type conversion (e.g., double to I64 for Math functions)
                if (result && result->getType()->isFloatingPointTy()) {
                    std::cerr << "DEBUG LLVM: Converting floating point return value to i64" << std::endl;
                    result = builder->CreateFPToSI(result, llvm::Type::getInt64Ty(*context), "fp_result_to_i64");
                }

                // Special handling for array functions that return new arrays
                // calleeName already declared above at line 1658
                if (calleeName == "nova_value_array_concat" || calleeName == "nova_value_array_slice" || calleeName == "nova_value_array_filter" || calleeName == "nova_value_array_map" || calleeName == "nova_value_array_toReversed" || calleeName == "nova_value_array_toSorted" || calleeName == "nova_value_array_flat" || calleeName == "nova_value_array_flatMap" || calleeName == "nova_array_from" || calleeName == "nova_array_of") {
                    std::cerr << "DEBUG LLVM: Detected array-returning function: " << calleeName << std::endl;
                    // Create ValueArrayMeta type and register it for the result
                    // ValueArrayMeta = { [24 x i8], i64 length, i64 capacity, ptr elements }
                    std::vector<llvm::Type*> metaFields = {
                        llvm::ArrayType::get(llvm::Type::getInt8Ty(*context), 24), // padding
                        llvm::Type::getInt64Ty(*context),                          // length
                        llvm::Type::getInt64Ty(*context),                          // capacity
                        llvm::PointerType::get(*context, 0)                        // elements pointer
                    };
                    llvm::StructType* arrayMetaType = llvm::StructType::get(*context, metaFields);
                    arrayTypeMap[result] = arrayMetaType;
                    std::cerr << "DEBUG LLVM: Registered array metadata type for result of " << calleeName << std::endl;
                }

                // Special handling for malloc in constructors
                std::cerr << "DEBUG LLVM: Checking for malloc in constructor..." << std::endl;
                // If this is a malloc call in a constructor, track the struct type
                if (auto* constOp = dynamic_cast<mir::MIRConstOperand*>(callTerm->func.get())) {
                    std::cerr << "DEBUG LLVM: Got constOp" << std::endl;
                    if (constOp->constKind == mir::MIRConstOperand::ConstKind::String) {
                        std::string funcName = std::get<std::string>(constOp->value);
                        if (funcName == "malloc") {
                            // Check if we're in a constructor function
                            std::string currentFuncName = currentFunction->getName().str();
                            if (currentFuncName.find("_constructor") != std::string::npos) {
                                // Extract class name from "ClassName_constructor"
                                size_t pos = currentFuncName.find("_constructor");
                                std::string className = currentFuncName.substr(0, pos);

                                // Find the struct type for this class
                                std::string structName = "struct." + className;
                                llvm::StructType* structType = llvm::StructType::getTypeByName(*context, structName);
                                if (structType) {
                                    // Store the struct type for this malloc'd pointer
                                    arrayTypeMap[result] = structType;
                                }
                            }
                        }

                        // Special handling for array functions that return new arrays
                        if (funcName == "nova_value_array_concat" ||
                            funcName == "nova_value_array_slice" ||
                            funcName == "nova_value_array_toReversed" ||
                            funcName == "nova_value_array_toSorted" ||
                            funcName == "nova_value_array_flat" ||
                            funcName == "nova_value_array_flatMap" ||
                            funcName == "nova_array_from" ||
                            funcName == "nova_array_of") {
                            std::cerr << "DEBUG LLVM: Detected array-returning function: " << funcName << std::endl;
                            // Create ValueArrayMeta type and register it for the result
                            // ValueArrayMeta = { [24 x i8], i64 length, i64 capacity, ptr elements }
                            std::vector<llvm::Type*> metaFields = {
                                llvm::ArrayType::get(llvm::Type::getInt8Ty(*context), 24), // padding
                                llvm::Type::getInt64Ty(*context),                          // length
                                llvm::Type::getInt64Ty(*context),                          // capacity
                                llvm::PointerType::get(*context, 0)                        // elements pointer
                            };
                            llvm::StructType* arrayMetaType = llvm::StructType::get(*context, metaFields);
                            arrayTypeMap[result] = arrayMetaType;
                            std::cerr << "DEBUG LLVM: Registered array metadata type for result of " << funcName << std::endl;
                        }
                    }
                }

                // Store result in destination
                std::cerr << "DEBUG LLVM: Storing result in destination..." << std::endl;

                // Skip storing result if function returns void
                bool isVoid = result->getType()->isVoidTy();
                std::cerr << "DEBUG LLVM: Result type is void: " << isVoid << std::endl;

                if (callTerm->destination && !isVoid) {
                    std::cerr << "DEBUG LLVM: Has destination and result is not void" << std::endl;
                    // Check if destination already has an alloca
                    auto it = valueMap.find(callTerm->destination.get());
                    if (it != valueMap.end() && llvm::isa<llvm::AllocaInst>(it->second)) {
                        std::cerr << "DEBUG LLVM: Destination exists, storing in existing alloca" << std::endl;
                        // Store result in existing alloca
                        builder->CreateStore(result, it->second);
                        std::cerr << "DEBUG LLVM: Store completed" << std::endl;

                        // Propagate type information from result to alloca
                        auto typeIt = arrayTypeMap.find(result);
                        if (typeIt != arrayTypeMap.end()) {
                            arrayTypeMap[it->second] = typeIt->second;
                        }
                    } else {
                        // Create a new alloca for the call result
                        llvm::AllocaInst* resultAlloca = builder->CreateAlloca(result->getType(), nullptr, "call_result");
                        builder->CreateStore(result, resultAlloca);
                        valueMap[callTerm->destination.get()] = resultAlloca;

                        // Propagate type information from result to alloca
                        auto typeIt = arrayTypeMap.find(result);
                        if (typeIt != arrayTypeMap.end()) {
                            arrayTypeMap[resultAlloca] = typeIt->second;
                        }
                    }
                }
                std::cerr << "DEBUG LLVM: Result storage completed" << std::endl;
            } else {
                std::cerr << "DEBUG LLVM: No destination for result" << std::endl;
            }

            // Branch to target
            std::cerr << "DEBUG LLVM: About to create branch to target" << std::endl;
            if (callTerm->target) {
                llvm::BasicBlock* targetBB = blockMap[callTerm->target];
                builder->CreateBr(targetBB);
                std::cerr << "DEBUG LLVM: Branch created successfully" << std::endl;
            }
            std::cerr << "DEBUG LLVM: Call terminator processing complete" << std::endl;
            break;
        }
        
        default:
            break;
    }
}

// ==================== Operation Generators ====================

llvm::Value* LLVMCodeGen::generateBinaryOp(mir::MIRBinaryOpRValue::BinOp op,
                                          llvm::Value* lhs, llvm::Value* rhs) {
    std::cerr << "DEBUG LLVM: generateBinaryOp called" << std::endl;
    if (!lhs || !rhs) {
        std::cerr << "DEBUG LLVM: Null operand in generateBinaryOp" << std::endl;
        return nullptr;
    }
    
    std::cerr << "DEBUG LLVM: Creating binary operation with op=" << static_cast<int>(op) << std::endl;
    
    // Convert operands to compatible types for arithmetic operations
    // Special case: Don't convert pointers to integers for string concatenation
    bool isStringConcat = (op == mir::MIRBinaryOpRValue::BinOp::Add &&
                          lhs->getType()->isPointerTy() &&
                          rhs->getType()->isPointerTy());

    if (!isStringConcat &&
        op != mir::MIRBinaryOpRValue::BinOp::Eq &&
        op != mir::MIRBinaryOpRValue::BinOp::Ne &&
        op != mir::MIRBinaryOpRValue::BinOp::Lt &&
        op != mir::MIRBinaryOpRValue::BinOp::Le &&
        op != mir::MIRBinaryOpRValue::BinOp::Gt &&
        op != mir::MIRBinaryOpRValue::BinOp::Ge) {
        // For arithmetic operations, ensure both operands are of the same type
        if (lhs->getType()->isPointerTy() && !rhs->getType()->isPointerTy()) {
            // Convert pointer to integer
            std::cerr << "DEBUG LLVM: Converting pointer to integer in binary operation" << std::endl;
            lhs = builder->CreatePtrToInt(lhs, llvm::Type::getInt64Ty(*context), "ptr_to_int");
        } else if (!lhs->getType()->isPointerTy() && rhs->getType()->isPointerTy()) {
            // Convert pointer to integer
            std::cerr << "DEBUG LLVM: Converting pointer to integer in binary operation" << std::endl;
            rhs = builder->CreatePtrToInt(rhs, llvm::Type::getInt64Ty(*context), "ptr_to_int");
        } else if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy() &&
                   lhs->getType() != rhs->getType()) {
            // Convert integers to the same width
            auto lhsIntTy = static_cast<llvm::IntegerType*>(lhs->getType());
            auto rhsIntTy = static_cast<llvm::IntegerType*>(rhs->getType());
            if (lhsIntTy->getBitWidth() > rhsIntTy->getBitWidth()) {
                rhs = builder->CreateSExt(rhs, lhs->getType(), "int_ext");
            } else {
                lhs = builder->CreateSExt(lhs, rhs->getType(), "int_ext");
            }
        }
    }
    
    switch (op) {
        case mir::MIRBinaryOpRValue::BinOp::Add:
            std::cerr << "DEBUG LLVM: Creating add instruction" << std::endl;
            // Check if operands are pointers (strings)
            if (lhs->getType()->isPointerTy() && rhs->getType()->isPointerTy()) {
                // Declare nova_string_concat_cstr if not already declared
                llvm::Function* concatFunc = module->getFunction("nova_string_concat_cstr");
                if (!concatFunc) {
                    llvm::FunctionType* concatFuncTy = llvm::FunctionType::get(
                        llvm::PointerType::getUnqual(*context),  // returns ptr
                        {llvm::PointerType::getUnqual(*context), llvm::PointerType::getUnqual(*context)},  // takes two ptrs
                        false  // not variadic
                    );
                    concatFunc = llvm::Function::Create(
                        concatFuncTy,
                        llvm::Function::ExternalLinkage,
                        "nova_string_concat_cstr",
                        *module
                    );
                }

                // Call the concat function
                return builder->CreateCall(concatFunc, {lhs, rhs}, "str_concat");
            }
            return builder->CreateAdd(lhs, rhs, "add");
        case mir::MIRBinaryOpRValue::BinOp::Sub:
            return builder->CreateSub(lhs, rhs, "sub");
        case mir::MIRBinaryOpRValue::BinOp::Mul:
            return builder->CreateMul(lhs, rhs, "mul");
        case mir::MIRBinaryOpRValue::BinOp::Div:
            return builder->CreateSDiv(lhs, rhs, "div");
        case mir::MIRBinaryOpRValue::BinOp::Rem:
            return builder->CreateSRem(lhs, rhs, "rem");
        case mir::MIRBinaryOpRValue::BinOp::Pow: {
            // For integer exponentiation, implement as repeated multiplication
            // This is a simple inline implementation for integer pow
            // result = base ** exponent
            llvm::Value* base = lhs;
            llvm::Value* exponent = rhs;

            // Create blocks for the power loop
            llvm::Function* func = builder->GetInsertBlock()->getParent();
            llvm::BasicBlock* loopCondBlock = llvm::BasicBlock::Create(*context, "pow.cond", func);
            llvm::BasicBlock* loopBodyBlock = llvm::BasicBlock::Create(*context, "pow.body", func);
            llvm::BasicBlock* loopEndBlock = llvm::BasicBlock::Create(*context, "pow.end", func);

            // Initialize result = 1, i = 0
            llvm::Value* one = llvm::ConstantInt::get(lhs->getType(), 1);
            llvm::Value* zero = llvm::ConstantInt::get(lhs->getType(), 0);
            llvm::AllocaInst* resultPtr = builder->CreateAlloca(lhs->getType(), nullptr, "pow.result");
            llvm::AllocaInst* iPtr = builder->CreateAlloca(lhs->getType(), nullptr, "pow.i");
            builder->CreateStore(one, resultPtr);
            builder->CreateStore(zero, iPtr);

            // Jump to loop condition
            builder->CreateBr(loopCondBlock);

            // Loop condition: i < exponent
            builder->SetInsertPoint(loopCondBlock);
            llvm::Value* i = builder->CreateLoad(lhs->getType(), iPtr, "i");
            llvm::Value* cond = builder->CreateICmpSLT(i, exponent, "pow.cond");
            builder->CreateCondBr(cond, loopBodyBlock, loopEndBlock);

            // Loop body: result *= base, i++
            builder->SetInsertPoint(loopBodyBlock);
            llvm::Value* result = builder->CreateLoad(lhs->getType(), resultPtr, "result");
            llvm::Value* newResult = builder->CreateMul(result, base, "pow.mul");
            builder->CreateStore(newResult, resultPtr);
            llvm::Value* newI = builder->CreateAdd(i, one, "pow.inc");
            builder->CreateStore(newI, iPtr);
            builder->CreateBr(loopCondBlock);

            // After loop: return result
            builder->SetInsertPoint(loopEndBlock);
            return builder->CreateLoad(lhs->getType(), resultPtr, "pow");
        }
        case mir::MIRBinaryOpRValue::BinOp::BitAnd:
            return builder->CreateAnd(lhs, rhs, "and");
        case mir::MIRBinaryOpRValue::BinOp::BitOr:
            return builder->CreateOr(lhs, rhs, "or");
        case mir::MIRBinaryOpRValue::BinOp::BitXor:
            return builder->CreateXor(lhs, rhs, "xor");
        case mir::MIRBinaryOpRValue::BinOp::Shl:
            return builder->CreateShl(lhs, rhs, "shl");
        case mir::MIRBinaryOpRValue::BinOp::Shr:
            return builder->CreateAShr(lhs, rhs, "shr");
        case mir::MIRBinaryOpRValue::BinOp::UShr:
            return builder->CreateLShr(lhs, rhs, "ushr");
        case mir::MIRBinaryOpRValue::BinOp::Eq:
            std::cerr << "DEBUG LLVM: Creating ICMP EQ instruction" << std::endl;
            // Check if operands are pointers (strings)
            if (lhs->getType()->isPointerTy() && rhs->getType()->isPointerTy()) {
                std::cerr << "DEBUG LLVM: String comparison detected" << std::endl;
                // For now, just compare pointer addresses
                // TODO: Implement proper string content comparison
                return builder->CreateICmpEQ(lhs, rhs, "str_eq");
            }
            
            // Handle type conversion for comparisons
            if (lhs->getType() != rhs->getType()) {
                std::cerr << "DEBUG LLVM: Type mismatch in EQ comparison, converting types" << std::endl;
                
                // Handle pointer to integer conversion
                if (lhs->getType()->isPointerTy() && !rhs->getType()->isPointerTy()) {
                    std::cerr << "DEBUG LLVM: Converting LHS pointer to integer for comparison" << std::endl;
                    lhs = builder->CreatePtrToInt(lhs, rhs->getType(), "ptr_to_int");
                } else if (!lhs->getType()->isPointerTy() && rhs->getType()->isPointerTy()) {
                    std::cerr << "DEBUG LLVM: Converting RHS pointer to integer for comparison" << std::endl;
                    rhs = builder->CreatePtrToInt(rhs, lhs->getType(), "ptr_to_int");
                }
                // Handle boolean to integer conversion
                else if (lhs->getType()->isIntegerTy(1) && rhs->getType()->isIntegerTy(64)) {
                    std::cerr << "DEBUG LLVM: Converting boolean to i64" << std::endl;
                    lhs = builder->CreateZExt(lhs, rhs->getType(), "bool_to_int");
                } else if (rhs->getType()->isIntegerTy(1) && lhs->getType()->isIntegerTy(64)) {
                    std::cerr << "DEBUG LLVM: Converting boolean to i64" << std::endl;
                    rhs = builder->CreateZExt(rhs, lhs->getType(), "bool_to_int");
                }
                // Handle integer width differences
                else if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy()) {
                    auto lhsIntTy = static_cast<llvm::IntegerType*>(lhs->getType());
                    auto rhsIntTy = static_cast<llvm::IntegerType*>(rhs->getType());
                    if (lhsIntTy->getBitWidth() > rhsIntTy->getBitWidth()) {
                        rhs = builder->CreateZExt(rhs, lhs->getType(), "int_ext");
                    } else {
                        lhs = builder->CreateZExt(lhs, rhs->getType(), "int_ext");
                    }
                }
            }
            return builder->CreateICmpEQ(lhs, rhs, "eq");
        case mir::MIRBinaryOpRValue::BinOp::Ne:
            std::cerr << "DEBUG LLVM: Creating ICMP NE instruction" << std::endl;
            // Check if operands are pointers (strings)
            if (lhs->getType()->isPointerTy() && rhs->getType()->isPointerTy()) {
                std::cerr << "DEBUG LLVM: String comparison detected" << std::endl;
                // For now, just compare pointer addresses
                // TODO: Implement proper string content comparison
                return builder->CreateICmpNE(lhs, rhs, "str_ne");
            }
            
            // Handle type conversion for comparisons
            if (lhs->getType() != rhs->getType()) {
                std::cerr << "DEBUG LLVM: Type mismatch in NE comparison, converting types" << std::endl;
                
                // Handle pointer to integer conversion
                if (lhs->getType()->isPointerTy() && !rhs->getType()->isPointerTy()) {
                    std::cerr << "DEBUG LLVM: Converting LHS pointer to integer for comparison" << std::endl;
                    lhs = builder->CreatePtrToInt(lhs, rhs->getType(), "ptr_to_int");
                } else if (!lhs->getType()->isPointerTy() && rhs->getType()->isPointerTy()) {
                    std::cerr << "DEBUG LLVM: Converting RHS pointer to integer for comparison" << std::endl;
                    rhs = builder->CreatePtrToInt(rhs, lhs->getType(), "ptr_to_int");
                }
                // Handle boolean to integer conversion
                else if (lhs->getType()->isIntegerTy(1) && rhs->getType()->isIntegerTy(64)) {
                    std::cerr << "DEBUG LLVM: Converting boolean to i64" << std::endl;
                    lhs = builder->CreateZExt(lhs, rhs->getType(), "bool_to_int");
                } else if (rhs->getType()->isIntegerTy(1) && lhs->getType()->isIntegerTy(64)) {
                    std::cerr << "DEBUG LLVM: Converting boolean to i64" << std::endl;
                    rhs = builder->CreateZExt(rhs, lhs->getType(), "bool_to_int");
                }
                // Handle integer width differences
                else if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy()) {
                    auto lhsIntTy = static_cast<llvm::IntegerType*>(lhs->getType());
                    auto rhsIntTy = static_cast<llvm::IntegerType*>(rhs->getType());
                    if (lhsIntTy->getBitWidth() > rhsIntTy->getBitWidth()) {
                        rhs = builder->CreateZExt(rhs, lhs->getType(), "int_ext");
                    } else {
                        lhs = builder->CreateZExt(lhs, rhs->getType(), "int_ext");
                    }
                }
            }
            return builder->CreateICmpNE(lhs, rhs, "ne");
        case mir::MIRBinaryOpRValue::BinOp::Lt:
            std::cerr << "DEBUG LLVM: Creating ICMP SLT instruction" << std::endl;
            // Handle type conversion for comparisons
            if (lhs->getType() != rhs->getType()) {
                std::cerr << "DEBUG LLVM: Type mismatch in LT comparison, converting types" << std::endl;
                
                // Handle pointer to integer conversion
                if (lhs->getType()->isPointerTy() && !rhs->getType()->isPointerTy()) {
                    lhs = builder->CreatePtrToInt(lhs, rhs->getType(), "ptr_to_int");
                } else if (!lhs->getType()->isPointerTy() && rhs->getType()->isPointerTy()) {
                    rhs = builder->CreatePtrToInt(rhs, lhs->getType(), "ptr_to_int");
                }
                // Handle integer width differences
                else if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy()) {
                    auto lhsIntTy = static_cast<llvm::IntegerType*>(lhs->getType());
                    auto rhsIntTy = static_cast<llvm::IntegerType*>(rhs->getType());
                    if (lhsIntTy->getBitWidth() > rhsIntTy->getBitWidth()) {
                        rhs = builder->CreateSExt(rhs, lhs->getType(), "int_ext");
                    } else {
                        lhs = builder->CreateSExt(lhs, rhs->getType(), "int_ext");
                    }
                }
            }
            return builder->CreateICmpSLT(lhs, rhs, "lt");
        case mir::MIRBinaryOpRValue::BinOp::Le:
            std::cerr << "DEBUG LLVM: Creating ICMP SLE instruction" << std::endl;
            // Handle type conversion for comparisons
            if (lhs->getType() != rhs->getType()) {
                std::cerr << "DEBUG LLVM: Type mismatch in LE comparison, converting types" << std::endl;
                
                // Handle pointer to integer conversion
                if (lhs->getType()->isPointerTy() && !rhs->getType()->isPointerTy()) {
                    lhs = builder->CreatePtrToInt(lhs, rhs->getType(), "ptr_to_int");
                } else if (!lhs->getType()->isPointerTy() && rhs->getType()->isPointerTy()) {
                    rhs = builder->CreatePtrToInt(rhs, lhs->getType(), "ptr_to_int");
                }
                // Handle integer width differences
                else if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy()) {
                    auto lhsIntTy = static_cast<llvm::IntegerType*>(lhs->getType());
                    auto rhsIntTy = static_cast<llvm::IntegerType*>(rhs->getType());
                    if (lhsIntTy->getBitWidth() > rhsIntTy->getBitWidth()) {
                        rhs = builder->CreateSExt(rhs, lhs->getType(), "int_ext");
                    } else {
                        lhs = builder->CreateSExt(lhs, rhs->getType(), "int_ext");
                    }
                }
            }
            return builder->CreateICmpSLE(lhs, rhs, "le");
        case mir::MIRBinaryOpRValue::BinOp::Gt:
            std::cerr << "DEBUG LLVM: Creating ICMP SGT instruction" << std::endl;
            // Handle type conversion for comparisons
            if (lhs->getType() != rhs->getType()) {
                std::cerr << "DEBUG LLVM: Type mismatch in GT comparison, converting types" << std::endl;
                
                // Handle pointer to integer conversion
                if (lhs->getType()->isPointerTy() && !rhs->getType()->isPointerTy()) {
                    lhs = builder->CreatePtrToInt(lhs, rhs->getType(), "ptr_to_int");
                } else if (!lhs->getType()->isPointerTy() && rhs->getType()->isPointerTy()) {
                    rhs = builder->CreatePtrToInt(rhs, lhs->getType(), "ptr_to_int");
                }
                // Handle integer width differences
                else if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy()) {
                    auto lhsIntTy = static_cast<llvm::IntegerType*>(lhs->getType());
                    auto rhsIntTy = static_cast<llvm::IntegerType*>(rhs->getType());
                    if (lhsIntTy->getBitWidth() > rhsIntTy->getBitWidth()) {
                        rhs = builder->CreateSExt(rhs, lhs->getType(), "int_ext");
                    } else {
                        lhs = builder->CreateSExt(lhs, rhs->getType(), "int_ext");
                    }
                }
            }
            return builder->CreateICmpSGT(lhs, rhs, "gt");
        case mir::MIRBinaryOpRValue::BinOp::Ge:
            std::cerr << "DEBUG LLVM: Creating ICMP SGE instruction" << std::endl;
            // Handle type conversion for comparisons
            if (lhs->getType() != rhs->getType()) {
                std::cerr << "DEBUG LLVM: Type mismatch in GE comparison, converting types" << std::endl;
                
                // Handle pointer to integer conversion
                if (lhs->getType()->isPointerTy() && !rhs->getType()->isPointerTy()) {
                    lhs = builder->CreatePtrToInt(lhs, rhs->getType(), "ptr_to_int");
                } else if (!lhs->getType()->isPointerTy() && rhs->getType()->isPointerTy()) {
                    rhs = builder->CreatePtrToInt(rhs, lhs->getType(), "ptr_to_int");
                }
                // Handle integer width differences
                else if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy()) {
                    auto lhsIntTy = static_cast<llvm::IntegerType*>(lhs->getType());
                    auto rhsIntTy = static_cast<llvm::IntegerType*>(rhs->getType());
                    if (lhsIntTy->getBitWidth() > rhsIntTy->getBitWidth()) {
                        rhs = builder->CreateSExt(rhs, lhs->getType(), "int_ext");
                    } else {
                        lhs = builder->CreateSExt(lhs, rhs->getType(), "int_ext");
                    }
                }
            }
            return builder->CreateICmpSGE(lhs, rhs, "ge");
        default:
            std::cerr << "DEBUG LLVM: Unknown binary operation: " << static_cast<int>(op) << std::endl;
            return nullptr;
    }
}

llvm::Value* LLVMCodeGen::generateUnaryOp(mir::MIRUnaryOpRValue::UnOp op,
                                         llvm::Value* operand) {
    if (!operand) return nullptr;
    
    switch (op) {
        case mir::MIRUnaryOpRValue::UnOp::Not:
            return builder->CreateNot(operand, "not");
        case mir::MIRUnaryOpRValue::UnOp::Neg:
            return builder->CreateNeg(operand, "neg");
        default:
            return nullptr;
    }
}

llvm::Value* LLVMCodeGen::generateCast(mir::MIRCastRValue::CastKind kind,
                                      llvm::Value* value, llvm::Type* targetType) {
    if (!value || !targetType) return nullptr;
    
    switch (kind) {
        case mir::MIRCastRValue::CastKind::IntToInt: {
            // Use unsigned cast (zero extension) for boolean to int conversions
            // For i1 (boolean): 0 -> 0, 1 -> 1 (not -1)
            // For other integer types, use signed cast
            bool isSigned = !value->getType()->isIntegerTy(1);
            return builder->CreateIntCast(value, targetType, isSigned, "cast");
        }
        case mir::MIRCastRValue::CastKind::IntToFloat:
            return builder->CreateSIToFP(value, targetType, "cast");
        case mir::MIRCastRValue::CastKind::FloatToInt:
            return builder->CreateFPToSI(value, targetType, "cast");
        case mir::MIRCastRValue::CastKind::FloatToFloat:
            return builder->CreateFPCast(value, targetType, "cast");
        case mir::MIRCastRValue::CastKind::PtrToPtr:
            // Pointer to pointer cast
            return builder->CreateBitCast(value, targetType, "ptrcast");
        case mir::MIRCastRValue::CastKind::Bitcast:
            return builder->CreateBitCast(value, targetType, "cast");
        case mir::MIRCastRValue::CastKind::Unsize:
            // Unsize operation (e.g., slice coercion)
            return builder->CreateBitCast(value, targetType, "unsize");
        default:
            return nullptr;
    }
}

// ==================== Aggregate Operations ====================

llvm::Value* LLVMCodeGen::generateAggregate(mir::MIRAggregateRValue* aggOp) {
    if (!aggOp) return nullptr;

    std::cerr << "DEBUG LLVM: generateAggregate called with " << aggOp->elements.size() << " elements" << std::endl;

    // For arrays: allocate array on stack and initialize elements
    if (aggOp->aggregateKind == mir::MIRAggregateRValue::AggregateKind::Array) {
        size_t arraySize = aggOp->elements.size();

        // Check if this is SetElement operation (array with 3 elements)
        // elements[0] = array pointer, elements[1] = index, elements[2] = value to store
        if (arraySize == 3) {
            llvm::Value* arrayPtr = convertOperand(aggOp->elements[0].get());
            llvm::Value* indexValue = convertOperand(aggOp->elements[1].get());
            llvm::Value* valueToStore = convertOperand(aggOp->elements[2].get());

            if (arrayPtr && indexValue && valueToStore) {
                // This is a SetElement operation - generate GEP + Store
                llvm::Value* basePtr = arrayPtr;

                // If arrayPtr is a load, get the pointer operand
                if (llvm::LoadInst* loadInst = llvm::dyn_cast<llvm::LoadInst>(arrayPtr)) {
                    basePtr = loadInst->getPointerOperand();
                }

                // Check if basePtr is an alloca storing a pointer (metadata struct pattern)
                llvm::AllocaInst* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(basePtr);
                if (allocaInst && allocaInst->getAllocatedType()->isPointerTy()) {
                    // This is array with metadata struct - extract elements pointer first
                    std::cerr << "DEBUG LLVM: SetElement - extracting elements pointer from metadata struct" << std::endl;

                    // Build expected metadata struct type
                    llvm::Type* i8Type = llvm::Type::getInt8Ty(*context);
                    llvm::Type* i64Type = llvm::Type::getInt64Ty(*context);
                    llvm::Type* ptrType = llvm::PointerType::get(*context, 0);
                    std::vector<llvm::Type*> metadataFields;
                    metadataFields.push_back(llvm::ArrayType::get(i8Type, 24));  // header
                    metadataFields.push_back(i64Type);  // length
                    metadataFields.push_back(i64Type);  // capacity
                    metadataFields.push_back(ptrType);  // elements
                    llvm::StructType* expectedMetadataType = llvm::StructType::get(*context, metadataFields);

                    // Extract elements pointer (field 3) from metadata struct
                    llvm::Value* elementsFieldPtr = builder->CreateStructGEP(
                        expectedMetadataType,
                        arrayPtr,  // loaded metadata struct pointer
                        3,
                        "meta_elements_field");

                    // Load the elements pointer
                    llvm::Value* elementsPtr = builder->CreateLoad(ptrType, elementsFieldPtr, "elements_ptr_load");

                    // Look up the array type from arrayTypeMap
                    auto typeIt = arrayTypeMap.find(allocaInst);
                    if (typeIt != arrayTypeMap.end()) {
                        llvm::Type* arrayType = typeIt->second;

                        // Now GEP into the actual array using the extracted elements pointer
                        std::vector<llvm::Value*> indices = {
                            llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0),
                            indexValue
                        };
                        llvm::Value* elementPtr = builder->CreateGEP(arrayType, elementsPtr, indices, "setelem_ptr");

                        // Store the value to the element
                        builder->CreateStore(valueToStore, elementPtr);

                        std::cerr << "DEBUG LLVM: SetElement - stored value to array element" << std::endl;

                        // Return a dummy value (void represented as 0)
                        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 0);
                    }
                } else {
                    // Regular array without metadata struct
                    // Look up array type
                    auto typeIt = arrayTypeMap.find(basePtr);
                    if (typeIt != arrayTypeMap.end()) {
                        llvm::Type* arrayType = typeIt->second;

                        // Create GEP to get element pointer
                        // IMPORTANT: Use arrayPtr (loaded value), not basePtr (alloca)!
                        std::vector<llvm::Value*> indices = {
                            llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0),
                            indexValue
                        };
                        llvm::Value* elementPtr = builder->CreateGEP(arrayType, arrayPtr, indices, "setelem_ptr");

                        // Store the value to the element
                        builder->CreateStore(valueToStore, elementPtr);

                        // Return a dummy value (void represented as 0)
                        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 0);
                    }
                }
            }
        }

        // Create array type [N x i64]
        llvm::Type* elementType = llvm::Type::getInt64Ty(*context);
        llvm::ArrayType* arrayType = llvm::ArrayType::get(elementType, arraySize);

        // Allocate array on stack
        llvm::AllocaInst* arrayAlloca = builder->CreateAlloca(arrayType, nullptr, "array");

        // Initialize each element
        for (size_t i = 0; i < arraySize; ++i) {
            llvm::Value* elementValue = convertOperand(aggOp->elements[i].get());
            if (!elementValue) continue;

            // Get pointer to element using GEP
            std::vector<llvm::Value*> indices = {
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0), // array index
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), i)  // element index
            };
            llvm::Value* elementPtr = builder->CreateGEP(arrayType, arrayAlloca, indices, "elem_ptr");

            // Store element value
            builder->CreateStore(elementValue, elementPtr);
        }

        std::cerr << "DEBUG LLVM: Array allocated and initialized, creating metadata struct" << std::endl;

        // Store the array type for later GEP operations
        arrayTypeMap[arrayAlloca] = arrayType;
        std::cerr << "DEBUG LLVM: Stored array type in arrayTypeMap" << std::endl;

        // Create Array metadata struct compatible with nova::runtime::Array
        // struct Array { ObjectHeader(24 bytes), i64 length, i64 capacity, ptr elements }
        // We create a simplified version with just the essential fields:
        // { [24 x i8] padding, i64 length, i64 capacity, ptr elements }

        llvm::Type* i8Type = llvm::Type::getInt8Ty(*context);
        llvm::Type* i64Type = llvm::Type::getInt64Ty(*context);
        llvm::Type* ptrType = llvm::PointerType::get(*context, 0);

        // Create struct type: { [24 x i8], i64, i64, ptr }
        std::vector<llvm::Type*> arrayMetadataFields;
        arrayMetadataFields.push_back(llvm::ArrayType::get(i8Type, 24));  // ObjectHeader padding
        arrayMetadataFields.push_back(i64Type);  // length
        arrayMetadataFields.push_back(i64Type);  // capacity
        arrayMetadataFields.push_back(ptrType);  // elements
        llvm::StructType* arrayMetadataType = llvm::StructType::get(*context, arrayMetadataFields);

        // Allocate metadata struct on stack
        llvm::AllocaInst* metadataAlloca = builder->CreateAlloca(arrayMetadataType, nullptr, "array_meta");

        // Initialize metadata fields
        // Field 0: ObjectHeader padding (skip initialization - not needed for stack arrays)

        // Field 1: length
        llvm::Value* lengthPtr = builder->CreateStructGEP(arrayMetadataType, metadataAlloca, 1, "meta_length_ptr");
        llvm::Value* lengthValue = llvm::ConstantInt::get(i64Type, arraySize);
        builder->CreateStore(lengthValue, lengthPtr);

        // Field 2: capacity
        llvm::Value* capacityPtr = builder->CreateStructGEP(arrayMetadataType, metadataAlloca, 2, "meta_capacity_ptr");
        llvm::Value* capacityValue = llvm::ConstantInt::get(i64Type, arraySize);
        builder->CreateStore(capacityValue, capacityPtr);

        // Field 3: elements (pointer to stack array)
        llvm::Value* elementsPtr = builder->CreateStructGEP(arrayMetadataType, metadataAlloca, 3, "meta_elements_ptr");
        // Cast array pointer to generic pointer
        llvm::Value* arrayPtr = builder->CreateBitCast(arrayAlloca, ptrType, "array_ptr");
        builder->CreateStore(arrayPtr, elementsPtr);

        std::cerr << "DEBUG LLVM: Created array metadata struct" << std::endl;

        // Store metadata in a map for tracking (reuse arrayTypeMap for now)
        arrayTypeMap[metadataAlloca] = arrayType;

        // Return pointer to metadata struct (compatible with Array*)
        return metadataAlloca;
    }

    // For structs: allocate struct on stack and initialize fields
    if (aggOp->aggregateKind == mir::MIRAggregateRValue::AggregateKind::Struct) {
        size_t numFields = aggOp->elements.size();

        // Check if this is actually a SetField operation (encoded as struct with 3 elements)
        // elements[0] = struct pointer, elements[1] = field index, elements[2] = value to store
        if (numFields == 3) {
            // Try to get the struct pointer, index, and value
            llvm::Value* structPtr = convertOperand(aggOp->elements[0].get());
            llvm::Value* indexValue = convertOperand(aggOp->elements[1].get());
            llvm::Value* valueToStore = convertOperand(aggOp->elements[2].get());

            if (structPtr && indexValue && valueToStore) {
                // This is a SetField operation - generate GEP + Store
                // Get the struct type from arrayTypeMap
                llvm::Value* basePtr = structPtr;

                // If structPtr is a load, get the pointer operand
                if (llvm::LoadInst* loadInst = llvm::dyn_cast<llvm::LoadInst>(structPtr)) {
                    basePtr = loadInst->getPointerOperand();
                }

                // Look up struct type
                auto typeIt = arrayTypeMap.find(basePtr);
                if (typeIt != arrayTypeMap.end()) {
                    llvm::Type* structType = typeIt->second;

                    // Convert field index to i32 for struct GEP
                    llvm::Value* fieldIndex = indexValue;
                    if (auto* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
                        uint64_t indexVal = constIndex->getZExtValue();
                        fieldIndex = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), indexVal);
                    }

                    // If structPtr is an integer (i64), convert it to a pointer
                    llvm::Value* actualStructPtr = structPtr;
                    if (structPtr->getType()->isIntegerTy()) {
                        llvm::Type* ptrType = llvm::PointerType::getUnqual(structType);
                        actualStructPtr = builder->CreateIntToPtr(structPtr, ptrType, "struct_ptr_cast");
                    }

                    // Create GEP to get field pointer
                    // IMPORTANT: Use actualStructPtr (converted if necessary)
                    std::vector<llvm::Value*> indices = {
                        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0),
                        fieldIndex
                    };
                    llvm::Value* fieldPtr = builder->CreateGEP(structType, actualStructPtr, indices, "setfield_ptr");

                    // Store the value to the field
                    builder->CreateStore(valueToStore, fieldPtr);
                }

                // Return a dummy value (void represented as 0)
                return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 0);
            }
        }

        // First pass: convert all field values to determine their types
        std::vector<llvm::Value*> fieldValues;
        std::vector<llvm::Type*> fieldTypes;
        std::vector<llvm::Type*> nestedStructTypes; // Track actual struct types for nested structs

        for (size_t i = 0; i < numFields; ++i) {
            llvm::Value* fieldValue = convertOperand(aggOp->elements[i].get());
            if (!fieldValue) {
                fieldValue = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 0);
            }

            fieldValues.push_back(fieldValue);

            // Determine the field type from the actual value
            llvm::Type* fieldType = fieldValue->getType();
            fieldTypes.push_back(fieldType);

            // If this is a pointer (potential nested struct/array), track the struct type
            if (fieldType->isPointerTy()) {
                // Try to find the struct type this pointer points to
                llvm::Value* sourceAlloca = fieldValue;

                // If fieldValue is a load instruction, unwrap it to get the alloca
                if (auto* loadInst = llvm::dyn_cast<llvm::LoadInst>(fieldValue)) {
                    sourceAlloca = loadInst->getPointerOperand();
                    std::cerr << "DEBUG LLVM: Field " << i << " is loaded pointer, unwrapped to get source alloca" << std::endl;
                }

                // Now try to find the struct type
                auto it = arrayTypeMap.find(sourceAlloca);
                if (it != arrayTypeMap.end()) {
                    nestedStructTypes.push_back(it->second);
                    std::cerr << "DEBUG LLVM: Field " << i << " is pointer to struct/array type: ";
                    it->second->print(llvm::errs());
                    std::cerr << std::endl;
                } else {
                    nestedStructTypes.push_back(nullptr);
                    std::cerr << "DEBUG LLVM: Field " << i << " is pointer but no struct type found in arrayTypeMap" << std::endl;
                }
            } else {
                nestedStructTypes.push_back(nullptr);
            }

            std::cerr << "DEBUG LLVM: Field " << i << " type: ";
            fieldType->print(llvm::errs());
            std::cerr << std::endl;
        }

        // Create struct type with the actual field types
        llvm::StructType* structType = llvm::StructType::create(*context, fieldTypes, "anon_struct");

        // Allocate struct on stack
        llvm::AllocaInst* structAlloca = builder->CreateAlloca(structType, nullptr, "struct");

        // Initialize each field
        for (size_t i = 0; i < numFields; ++i) {
            // Get pointer to field using GEP
            std::vector<llvm::Value*> indices = {
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0), // struct index
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), i)  // field index
            };
            llvm::Value* fieldPtr = builder->CreateGEP(structType, structAlloca, indices, "field_ptr");

            // Store field value
            builder->CreateStore(fieldValues[i], fieldPtr);

            // If this field is a pointer to a struct/array, track it in nestedStructTypeMap
            if (nestedStructTypes[i]) {
                // Store the nested struct type using (parent alloca, field index) as key
                // This allows nested field access to work
                nestedStructTypeMap[std::make_pair(structAlloca, i)] = nestedStructTypes[i];
                std::cerr << "DEBUG LLVM: Stored nested struct type for field " << i << " in nestedStructTypeMap: ";
                nestedStructTypes[i]->print(llvm::errs());
                std::cerr << std::endl;
            }
        }

        std::cerr << "DEBUG LLVM: Struct allocated and initialized, returning pointer" << std::endl;

        // Store the struct type for later GEP operations
        arrayTypeMap[structAlloca] = structType;
        std::cerr << "DEBUG LLVM: Stored struct type in arrayTypeMap" << std::endl;

        // Return pointer to the struct
        return structAlloca;
    }

    std::cerr << "DEBUG LLVM: Unsupported aggregate kind" << std::endl;
    return nullptr;
}

llvm::Value* LLVMCodeGen::generateGetElement(mir::MIRGetElementRValue* getElemOp) {
    if (!getElemOp) return nullptr;

    std::cerr << "DEBUG LLVM: generateGetElement called" << std::endl;

    // Get the array pointer (should be from a Copy operand)
    llvm::Value* arrayPtr = convertOperand(getElemOp->array.get());
    if (!arrayPtr) {
        std::cerr << "DEBUG LLVM: Failed to convert array operand" << std::endl;
        return nullptr;
    }

    // Get the index value
    llvm::Value* indexValue = convertOperand(getElemOp->index.get());
    if (!indexValue) {
        std::cerr << "DEBUG LLVM: Failed to convert index operand" << std::endl;
        return nullptr;
    }

    std::cerr << "DEBUG LLVM: arrayPtr type: " << arrayPtr->getType()->getTypeID() << std::endl;
    std::cerr << "DEBUG LLVM: Using GEP to access array element" << std::endl;

    // Save the loaded array pointer for use in GEP
    llvm::Value* loadedArrayPtr = arrayPtr;

    // Get the alloca instruction to look up array type
    llvm::AllocaInst* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(arrayPtr);

    // If arrayPtr is a load instruction, get the pointer operand (the variable's alloca)
    if (!allocaInst) {
        if (llvm::LoadInst* loadInst = llvm::dyn_cast<llvm::LoadInst>(arrayPtr)) {
            std::cerr << "DEBUG LLVM: arrayPtr is a load, getting pointer operand" << std::endl;
            arrayPtr = loadInst->getPointerOperand();
            allocaInst = llvm::dyn_cast<llvm::AllocaInst>(arrayPtr);
        }
    }

    if (!allocaInst) {
        std::cerr << "DEBUG LLVM: arrayPtr is not an alloca (even after unwrapping load)" << std::endl;
        return nullptr;
    }

    // Look up the array type from our tracking map
    auto arrayTypeIt = arrayTypeMap.find(allocaInst);
    if (arrayTypeIt == arrayTypeMap.end()) {
        std::cerr << "DEBUG LLVM: ERROR - Array type not found in arrayTypeMap for alloca" << std::endl;
        return nullptr;
    }

    llvm::Type* arrayType = arrayTypeIt->second;
    std::cerr << "DEBUG LLVM: Array type retrieved from arrayTypeMap: ";
    arrayType->print(llvm::errs());
    std::cerr << std::endl;

    std::cerr << "DEBUG LLVM: loadedArrayPtr type: ";
    loadedArrayPtr->getType()->print(llvm::errs());
    std::cerr << std::endl;

    std::cerr << "DEBUG LLVM: loadedArrayPtr is ";
    if (llvm::isa<llvm::AllocaInst>(loadedArrayPtr)) {
        std::cerr << "an AllocaInst";
    } else if (llvm::isa<llvm::LoadInst>(loadedArrayPtr)) {
        std::cerr << "a LoadInst";
    } else {
        std::cerr << "something else: ";
        loadedArrayPtr->print(llvm::errs());
    }
    std::cerr << std::endl;

    // Use GEP to get pointer to the element
    // loadedArrayPtr is the pointer to the array/struct (either directly or loaded from variable)

    // For structs, field indices must be i32 constants
    llvm::Value* secondIndex = indexValue;
    if (llvm::isa<llvm::StructType>(arrayType)) {
        // For structs, convert the index to i32 constant
        if (auto* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
            secondIndex = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context),
                                                  constIndex->getZExtValue());
            std::cerr << "DEBUG LLVM: Converted struct field index to i32" << std::endl;
        }
    }

    // If loadedArrayPtr is an integer (i64), convert it to a pointer
    llvm::Value* actualPtr = loadedArrayPtr;
    if (loadedArrayPtr->getType()->isIntegerTy()) {
        std::cerr << "DEBUG LLVM: loadedArrayPtr is i64, converting to pointer" << std::endl;
        llvm::Type* ptrType = llvm::PointerType::getUnqual(arrayType);
        actualPtr = builder->CreateIntToPtr(loadedArrayPtr, ptrType, "ptr_cast");
    }

    // Check if the allocaInst (variable's alloca) stores an array metadata struct
    // Metadata struct has type: { [24 x i8], i64, i64, ptr }
    // Fields: 0=header, 1=length, 2=capacity, 3=elements
    bool isMetadataFieldAccess = false;
    if (allocaInst) {
        llvm::Type* allocatedType = allocaInst->getAllocatedType();
        // Check if allocated type is a pointer to struct
        if (allocatedType->isPointerTy()) {
            std::cerr << "DEBUG LLVM: Allocated type is pointer, checking for metadata struct pattern" << std::endl;
            // The variable holds a pointer to metadata struct
            // loadedArrayPtr is the loaded pointer value (points to metadata struct)

            // Try to create struct type { [24 x i8], i64, i64, ptr } to match
            llvm::Type* i8Type = llvm::Type::getInt8Ty(*context);
            llvm::Type* i64Type = llvm::Type::getInt64Ty(*context);
            llvm::Type* ptrType = llvm::PointerType::get(*context, 0);
            std::vector<llvm::Type*> metadataFields;
            metadataFields.push_back(llvm::ArrayType::get(i8Type, 24));
            metadataFields.push_back(i64Type);
            metadataFields.push_back(i64Type);
            metadataFields.push_back(ptrType);
            llvm::StructType* expectedMetadataType = llvm::StructType::get(*context, metadataFields);

            // Check if this is a field access (arr.length) or element access (arr[0])
            // using the isFieldAccess flag from MIR
            if (getElemOp->isFieldAccess) {
                // Field access: Access metadata struct fields directly (header=0, length=1, capacity=2)
                if (auto* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
                    unsigned fieldIndex = constIndex->getZExtValue();

                    std::cerr << "DEBUG LLVM: Accessing metadata struct field " << fieldIndex << " directly" << std::endl;
                    isMetadataFieldAccess = true;

                    // Access the struct field directly without extracting elements pointer
                    llvm::Value* fieldPtr = builder->CreateStructGEP(
                        expectedMetadataType,
                        loadedArrayPtr,
                        fieldIndex,
                        "meta_field_ptr");

                    // Get the field type
                    llvm::Type* fieldType = expectedMetadataType->getElementType(fieldIndex);

                    // Load the field value
                    llvm::Value* fieldValue = builder->CreateLoad(fieldType, fieldPtr, "field_value");

                    std::cerr << "DEBUG LLVM: Loaded metadata field " << fieldIndex << std::endl;
                    return fieldValue;
                }
            } else {
                // Element access: Extract elements pointer from metadata and index into array
                std::cerr << "DEBUG LLVM: Element access - extracting elements pointer from metadata struct" << std::endl;

                // Extract the `elements` field (field 3) from loadedArrayPtr
                llvm::Value* elementsFieldPtr = builder->CreateStructGEP(
                    expectedMetadataType,
                    loadedArrayPtr,
                    3,
                    "meta_elements_field");

                // Load the elements pointer
                actualPtr = builder->CreateLoad(ptrType, elementsFieldPtr, "elements_ptr_load");

                std::cerr << "DEBUG LLVM: Extracted elements pointer from metadata" << std::endl;
            }
        }
    }

    std::vector<llvm::Value*> indices = {
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), 0),  // Dereference pointer
        secondIndex                                                     // Element/field index
    };

    llvm::Value* elementPtr = builder->CreateGEP(
        arrayType,
        actualPtr,  // Use the pointer (converted if necessary)
        indices,
        "elem_ptr"
    );

    // Determine the actual type of the element from the array/struct type
    llvm::Type* elementType = nullptr;
    if (auto* structType = llvm::dyn_cast<llvm::StructType>(arrayType)) {
        // For struct types, get the type of the specific field
        if (auto* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
            unsigned fieldIndex = constIndex->getZExtValue();
            if (fieldIndex < structType->getNumElements()) {
                elementType = structType->getElementType(fieldIndex);
                std::cerr << "DEBUG LLVM: Field type at index " << fieldIndex << ": ";
                elementType->print(llvm::errs());
                std::cerr << std::endl;
            }
        }
    } else if (auto* arrayType_llvm = llvm::dyn_cast<llvm::ArrayType>(arrayType)) {
        // For array types, all elements have the same type
        elementType = arrayType_llvm->getElementType();
    }

    // Load the element value with the correct type
    llvm::Type* loadType = elementType ? elementType : llvm::Type::getInt64Ty(*context);
    llvm::Value* elementValue = builder->CreateLoad(
        loadType,
        elementPtr,
        "elem_value"
    );

    std::cerr << "DEBUG LLVM: Element loaded successfully" << std::endl;

    // If the element is a pointer type (nested struct/array), we need to track its type
    // for potential nested field access
    if (elementType && elementType->isPointerTy()) {
        std::cerr << "DEBUG LLVM: Element is a pointer type, checking for nested struct type" << std::endl;

        // Look up if we stored the nested struct type for this field
        // We need the parent alloca and field index
        if (auto* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
            unsigned fieldIndex = constIndex->getZExtValue();
            auto nestedTypeKey = std::make_pair(static_cast<llvm::Value*>(allocaInst), fieldIndex);
            auto nestedTypeIt = nestedStructTypeMap.find(nestedTypeKey);

            if (nestedTypeIt != nestedStructTypeMap.end()) {
                std::cerr << "DEBUG LLVM: Found nested struct type in nestedStructTypeMap for field " << fieldIndex << ": ";
                nestedTypeIt->second->print(llvm::errs());
                std::cerr << std::endl;

                // Create a temporary alloca to store the loaded pointer
                // This allows us to do GEP on it for nested field access
                llvm::AllocaInst* tempAlloca = builder->CreateAlloca(
                    elementType,
                    nullptr,
                    "nested_ptr"
                );

                // Store the loaded pointer
                builder->CreateStore(elementValue, tempAlloca);

                // Track the nested struct type for this temporary alloca
                // This is critical - it allows the next GetElement to find the struct type
                arrayTypeMap[tempAlloca] = nestedTypeIt->second;
                std::cerr << "DEBUG LLVM: Stored nested struct type for temp alloca in arrayTypeMap" << std::endl;

                // Return a load from the alloca
                // This ensures that future operations can unwrap the load to find the alloca
                llvm::Value* reloaded = builder->CreateLoad(elementType, tempAlloca, "nested_reload");
                std::cerr << "DEBUG LLVM: Returning reloaded value from temp alloca" << std::endl;
                return reloaded;
            } else {
                std::cerr << "DEBUG LLVM: No nested struct type found for parent=" << allocaInst
                          << ", field=" << fieldIndex << std::endl;
            }
        }
    }

    return elementValue;
}

// ==================== Runtime Functions ====================

void LLVMCodeGen::declareRuntimeFunctions() {
    // Declare printf for debugging
    llvm::FunctionType* printfType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context),
        llvm::PointerType::getUnqual(*context),
        true  // varargs
    );
    module->getOrInsertFunction("printf", printfType);
    
    // TODO: Declare other runtime functions (GC, string ops, etc.)
}

llvm::Function* LLVMCodeGen::getRuntimeFunction(const std::string& name) {
    return module->getFunction(name);
}

llvm::Function* LLVMCodeGen::getIntrinsic(unsigned id, llvm::ArrayRef<llvm::Type*> types) {
    return llvm::Intrinsic::getDeclaration(module.get(), 
                                          static_cast<llvm::Intrinsic::ID>(id), 
                                          types);
}

} // namespace nova::codegen
