// LLVM Code Generator from MIR
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4100 4127 4244 4245 4267 4310 4324 4458 4459 4624)
#endif

#include "nova/CodeGen/LLVMCodeGen.h"
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>
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
        
        // Generate all functions
        for (const auto& mirFunc : mirModule.functions) {
            generateFunction(mirFunc.get());
        }
        
        // Verify the module
        std::string errMsg;
        llvm::raw_string_ostream errStream(errMsg);
        if (llvm::verifyModule(*module, &errStream)) {
            std::cerr << "LLVM IR verification failed:\n" << errMsg << std::endl;
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error generating LLVM IR: " << e.what() << std::endl;
        return false;
    }
}

void LLVMCodeGen::dumpIR() const {
    module->print(llvm::outs(), nullptr);
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
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    
    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << std::endl;
        return false;
    }
    
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
    if (optLevel == 0) return;
    
    // Create function pass manager
    auto FPM = std::make_unique<llvm::legacy::FunctionPassManager>(module.get());
    
    // Add basic optimization passes based on level
    if (optLevel >= 1) {
        FPM->add(llvm::createPromoteMemoryToRegisterPass());
        FPM->add(llvm::createInstructionCombiningPass());
        FPM->add(llvm::createReassociatePass());
        FPM->add(llvm::createGVNPass());
        FPM->add(llvm::createCFGSimplificationPass());
    }
    
    if (optLevel >= 2) {
        FPM->add(llvm::createDeadCodeEliminationPass());
        // Note: createAggressiveDCEPass was removed in LLVM 16
        // DeadCodeEliminationPass provides similar functionality
    }
    
    if (optLevel >= 3) {
        FPM->add(llvm::createAlwaysInlinerLegacyPass());
    }
    
    FPM->doInitialization();
    
    // Run passes on all functions
    for (auto& func : module->functions()) {
        if (!func.isDeclaration()) {
            FPM->run(func);
        }
    }
    
    FPM->doFinalization();
}

int LLVMCodeGen::executeMain() {
    // JIT execution is not implemented in this version
    // Would require LLVM JIT/ORC API which is complex
    std::cerr << "⚠️  JIT execution not yet implemented" << std::endl;
    std::cerr << "   Use '--emit-llvm' to generate LLVM IR and compile manually" << std::endl;
    return 0;
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
    if (!operand) return nullptr;
    
    if (operand->kind == mir::MIROperand::Kind::Copy) {
        auto* copyOp = static_cast<mir::MIRCopyOperand*>(operand);
        auto it = valueMap.find(copyOp->place.get());
        if (it != valueMap.end()) {
            // Check if this is an alloca (needs load) or direct value
            if (llvm::isa<llvm::AllocaInst>(it->second)) {
                llvm::Type* loadType = convertType(copyOp->place->type.get());
                return builder->CreateLoad(loadType, it->second, "load");
            } else {
                // Direct value (e.g., function argument or SSA value)
                return it->second;
            }
        }
        return nullptr;
    } else if (operand->kind == mir::MIROperand::Kind::Move) {
        // Load from place (move is same as copy for now)
        auto* moveOp = static_cast<mir::MIRMoveOperand*>(operand);
        auto it = valueMap.find(moveOp->place.get());
        if (it != valueMap.end()) {
            return builder->CreateLoad(convertType(moveOp->place->type.get()), 
                                     it->second, "load");
        }
    } else if (operand->kind == mir::MIROperand::Kind::Constant) {
        // Create constant
        auto* constOp = static_cast<mir::MIRConstOperand*>(operand);
        if (constOp->constKind == mir::MIRConstOperand::ConstKind::Int) {
            int64_t intVal = std::get<int64_t>(constOp->value);
            return llvm::ConstantInt::get(convertType(constOp->type.get()), 
                                         intVal, true);
        } else if (constOp->constKind == mir::MIRConstOperand::ConstKind::Float) {
            double floatVal = std::get<double>(constOp->value);
            return llvm::ConstantFP::get(convertType(constOp->type.get()), 
                                        floatVal);
        } else if (constOp->constKind == mir::MIRConstOperand::ConstKind::Bool) {
            bool boolVal = std::get<bool>(constOp->value);
            return llvm::ConstantInt::get(llvm::Type::getInt1Ty(*context), 
                                         boolVal ? 1 : 0);
        } else if (constOp->constKind == mir::MIRConstOperand::ConstKind::Null) {
            return llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*context));
        }
    }
    
    return nullptr;
}

llvm::Value* LLVMCodeGen::convertRValue(mir::MIRRValue* rvalue) {
    if (!rvalue) return nullptr;
    
    switch (rvalue->kind) {
        case mir::MIRRValue::Kind::Use: {
            auto* useRVal = static_cast<mir::MIRUseRValue*>(rvalue);
            return convertOperand(useRVal->operand.get());
        }
        
        case mir::MIRRValue::Kind::BinaryOp: {
            auto* binOp = static_cast<mir::MIRBinaryOpRValue*>(rvalue);
            llvm::Value* lhs = convertOperand(binOp->lhs.get());
            llvm::Value* rhs = convertOperand(binOp->rhs.get());
            return generateBinaryOp(binOp->op, lhs, rhs);
        }
        
        case mir::MIRRValue::Kind::UnaryOp: {
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
        
        default:
            return nullptr;
    }
}

// ==================== Function Generation ====================

llvm::Function* LLVMCodeGen::generateFunction(mir::MIRFunction* function) {
    if (!function) return nullptr;
    
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
    // For now, make all functions return i64 (can be ignored by caller)
    if (retType->isVoidTy()) {
        retType = llvm::Type::getInt64Ty(*context);
    }
    llvm::FunctionType* funcType = llvm::FunctionType::get(retType, paramTypes, false);
    
    // Create function
    llvm::Function* llvmFunc = llvm::Function::Create(
        funcType, 
        llvm::Function::ExternalLinkage,
        function->name,
        module.get()
    );
    
    functionMap[function->name] = llvmFunc;
    currentFunction = llvmFunc;
    currentReturnValue = nullptr;  // Reset return value for this function
    
    // Map parameters
    auto argIt = llvmFunc->arg_begin();
    for (size_t i = 0; i < function->arguments.size(); ++i, ++argIt) {
        valueMap[function->arguments[i].get()] = &(*argIt);
        argIt->setName("arg" + std::to_string(i));
    }
    
    // Generate basic blocks
    for (const auto& bb : function->basicBlocks) {
        if (!bb) continue;
        std::string bbLabel = bb->label.empty() ? "entry" : bb->label;
        llvm::BasicBlock* llvmBB = llvm::BasicBlock::Create(*context, bbLabel, llvmFunc);
        blockMap[bb.get()] = llvmBB;
    }
    
    // Generate code for each basic block
    for (const auto& bb : function->basicBlocks) {
        generateBasicBlock(bb.get(), blockMap[bb.get()]);
    }
    
    return llvmFunc;
}

void LLVMCodeGen::generateBasicBlock(mir::MIRBasicBlock* bb, llvm::BasicBlock* llvmBB) {
    builder->SetInsertPoint(llvmBB);
    
    // Generate statements
    for (const auto& stmt : bb->statements) {
        generateStatement(stmt.get());
    }
    
    // Generate terminator
    if (bb->terminator) {
        generateTerminator(bb->terminator.get());
    }
}

void LLVMCodeGen::generateStatement(mir::MIRStatement* stmt) {
    if (!stmt) return;
    
    switch (stmt->kind) {
        case mir::MIRStatement::Kind::Assign: {
            auto* assign = static_cast<mir::MIRAssignStatement*>(stmt);
            llvm::Value* value = convertRValue(assign->rvalue.get());
            
            // Map the value directly (SSA style)
            if (value) {
                valueMap[assign->place.get()] = value;
                
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
    if (!terminator) return;
    
    switch (terminator->kind) {
        case mir::MIRTerminator::Kind::Return: {
            // Check if the function has a non-void return type
            llvm::Function* currentFunc = builder->GetInsertBlock()->getParent();
            llvm::Type* retType = currentFunc->getReturnType();
            
            if (retType->isVoidTy()) {
                builder->CreateRetVoid();
            } else {
                // Use currentReturnValue if available
                if (currentReturnValue) {
                    builder->CreateRet(currentReturnValue);
                } else {
                    // No return value found, return a default value (0 for i64)
                    if (retType->isIntegerTy()) {
                        builder->CreateRet(llvm::ConstantInt::get(retType, 0));
                    } else {
                        builder->CreateRetVoid(); // Fallback
                    }
                }
            }
            break;
        }
        
        case mir::MIRTerminator::Kind::Goto: {
            auto* gotoTerm = static_cast<mir::MIRGotoTerminator*>(terminator);
            llvm::BasicBlock* targetBB = blockMap[gotoTerm->target];
            builder->CreateBr(targetBB);
            break;
        }
        
        case mir::MIRTerminator::Kind::SwitchInt: {
            auto* switchTerm = static_cast<mir::MIRSwitchIntTerminator*>(terminator);
            llvm::Value* value = convertOperand(switchTerm->discriminant.get());
            llvm::BasicBlock* defaultBB = blockMap[switchTerm->otherwise];
            
            llvm::SwitchInst* switchInst = builder->CreateSwitch(value, defaultBB, 
                                                                static_cast<unsigned int>(switchTerm->targets.size()));
            
            for (const auto& caseItem : switchTerm->targets) {
                llvm::ConstantInt* caseVal = llvm::ConstantInt::get(
                    *context, llvm::APInt(32, caseItem.value));
                llvm::BasicBlock* caseBB = blockMap[caseItem.target];
                switchInst->addCase(caseVal, caseBB);
            }
            break;
        }
        
        case mir::MIRTerminator::Kind::Call: {
            auto* callTerm = static_cast<mir::MIRCallTerminator*>(terminator);
            
            // Get the function to call
            llvm::Function* callee = nullptr;
            
            // Check if func operand is a string constant (function name)
            if (auto* constOp = dynamic_cast<mir::MIRConstOperand*>(callTerm->func.get())) {
                if (constOp->constKind == mir::MIRConstOperand::ConstKind::String) {
                    std::string funcName = std::get<std::string>(constOp->value);
                    auto it = functionMap.find(funcName);
                    if (it != functionMap.end()) {
                        callee = it->second;
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
                // Convert arguments
                std::vector<llvm::Value*> args;
                for (const auto& arg : callTerm->args) {
                    llvm::Value* argValue = convertOperand(arg.get());
                    if (argValue) args.push_back(argValue);
                }
                
                // Create call
                llvm::Value* result = builder->CreateCall(callee, args);
                
                // Store result in destination
                if (callTerm->destination) {
                    valueMap[callTerm->destination.get()] = result;
                }
            }
            
            // Branch to target
            if (callTerm->target) {
                llvm::BasicBlock* targetBB = blockMap[callTerm->target];
                builder->CreateBr(targetBB);
            }
            break;
        }
        
        default:
            break;
    }
}

// ==================== Operation Generators ====================

llvm::Value* LLVMCodeGen::generateBinaryOp(mir::MIRBinaryOpRValue::BinOp op,
                                          llvm::Value* lhs, llvm::Value* rhs) {
    if (!lhs || !rhs) return nullptr;
    
    switch (op) {
        case mir::MIRBinaryOpRValue::BinOp::Add:
            return builder->CreateAdd(lhs, rhs, "add");
        case mir::MIRBinaryOpRValue::BinOp::Sub:
            return builder->CreateSub(lhs, rhs, "sub");
        case mir::MIRBinaryOpRValue::BinOp::Mul:
            return builder->CreateMul(lhs, rhs, "mul");
        case mir::MIRBinaryOpRValue::BinOp::Div:
            return builder->CreateSDiv(lhs, rhs, "div");
        case mir::MIRBinaryOpRValue::BinOp::Rem:
            return builder->CreateSRem(lhs, rhs, "rem");
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
        case mir::MIRBinaryOpRValue::BinOp::Eq:
            return builder->CreateICmpEQ(lhs, rhs, "eq");
        case mir::MIRBinaryOpRValue::BinOp::Ne:
            return builder->CreateICmpNE(lhs, rhs, "ne");
        case mir::MIRBinaryOpRValue::BinOp::Lt:
            return builder->CreateICmpSLT(lhs, rhs, "lt");
        case mir::MIRBinaryOpRValue::BinOp::Le:
            return builder->CreateICmpSLE(lhs, rhs, "le");
        case mir::MIRBinaryOpRValue::BinOp::Gt:
            return builder->CreateICmpSGT(lhs, rhs, "gt");
        case mir::MIRBinaryOpRValue::BinOp::Ge:
            return builder->CreateICmpSGE(lhs, rhs, "ge");
        default:
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
        case mir::MIRCastRValue::CastKind::IntToInt:
            return builder->CreateIntCast(value, targetType, true, "cast");
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
