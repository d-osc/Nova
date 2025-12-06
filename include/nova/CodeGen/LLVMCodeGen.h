#pragma once

#include "nova/MIR/MIR.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <unordered_map>
#include <map>
#include <memory>

namespace nova::codegen {

class LLVMCodeGen {
public:
    explicit LLVMCodeGen(const std::string& moduleName);
    ~LLVMCodeGen();
    
    // Generate LLVM IR from MIR
    bool generate(const mir::MIRModule& mirModule);
    
    // Output methods
    void dumpIR() const;
    bool emitObjectFile(const std::string& filename);
    bool emitAssembly(const std::string& filename);
    bool emitLLVMIR(const std::string& filename);
    bool emitBitcode(const std::string& filename);

    // Load pre-compiled bitcode (for cache)
    bool loadBitcode(const std::string& filename);

    // Compile to native executable
    bool emitExecutable(const std::string& filename);

    // Optimization
    void runOptimizationPasses(unsigned optLevel = 2);

    // JIT execution
    int executeMain();
    
    llvm::Module* getModule() const { return module.get(); }
    llvm::LLVMContext& getContext() { return *context; }
    
private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    
    // Type mappings
    std::unordered_map<mir::MIRType*, llvm::Type*> typeCache;
    
    // Value mappings (MIR places to LLVM values)
    std::unordered_map<mir::MIRPlace*, llvm::Value*> valueMap;
    
    // Function mappings
    std::unordered_map<std::string, llvm::Function*> functionMap;
    
    // Basic block mappings
    std::unordered_map<mir::MIRBasicBlock*, llvm::BasicBlock*> blockMap;

    // Array type tracking (maps array pointers to their array types)
    std::unordered_map<llvm::Value*, llvm::Type*> arrayTypeMap;

    // Function return struct type tracking (maps function name to struct type it returns)
    std::unordered_map<std::string, llvm::Type*> functionReturnStructTypes;

    // Nested struct type tracking (maps (parent value, field index) to nested struct type)
    // Used for tracking struct types of pointer fields in structs
    std::map<std::pair<llvm::Value*, unsigned>, llvm::Type*> nestedStructTypeMap;

    // Current function context
    llvm::Function* currentFunction;
    mir::MIRPlace* currentReturnPlace;  // The _0 place for return values
    llvm::Value* currentReturnValue;    // The actual return value
    
    // Helper methods
    llvm::Type* convertType(mir::MIRType* type);
    llvm::Value* convertOperand(mir::MIROperand* operand);
    llvm::Value* convertRValue(mir::MIRRValue* rvalue);
    
    llvm::Function* generateFunction(mir::MIRFunction* function);
    void generateBasicBlock(mir::MIRBasicBlock* bb, llvm::BasicBlock* llvmBB);
    void generateStatement(mir::MIRStatement* stmt);
    void generateTerminator(mir::MIRTerminator* terminator);
    
    // Binary operations
    llvm::Value* generateBinaryOp(mir::MIRBinaryOpRValue::BinOp op,
                                  llvm::Value* lhs, llvm::Value* rhs,
                                  mir::MIROperand* lhsOperand = nullptr,
                                  mir::MIROperand* rhsOperand = nullptr);
    
    // Unary operations
    llvm::Value* generateUnaryOp(mir::MIRUnaryOpRValue::UnOp op,
                                 llvm::Value* operand);
    
    // Cast operations
    llvm::Value* generateCast(mir::MIRCastRValue::CastKind kind,
                             llvm::Value* value, llvm::Type* targetType);

    // Aggregate operations (arrays, tuples, structs)
    llvm::Value* generateAggregate(mir::MIRAggregateRValue* aggOp);
    llvm::Value* generateGetElement(mir::MIRGetElementRValue* getElemOp);

    // Runtime library functions
    void declareRuntimeFunctions();
    llvm::Function* getRuntimeFunction(const std::string& name);
    
    // Intrinsics
    llvm::Function* getIntrinsic(unsigned id, llvm::ArrayRef<llvm::Type*> types);
};

} // namespace nova::codegen
