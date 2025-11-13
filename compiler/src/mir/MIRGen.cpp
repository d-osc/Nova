#include "nova/MIR/MIRGen.h"
#include "nova/MIR/MIRBuilder.h"
#include <unordered_map>
#include <iostream>
#include <set>
#include <queue>

namespace nova::mir {

// ==================== MIR Generator Implementation ====================

// Loop context structure for tracking break/continue targets
struct LoopContext {
    MIRBasicBlock* breakTarget;    // Target for break statements
    MIRBasicBlock* continueTarget; // Target for continue statements
    std::shared_ptr<LoopContext> parent; // Parent loop context (for nested loops)
    
    LoopContext() : breakTarget(nullptr), continueTarget(nullptr), parent(nullptr) {}
};

class MIRGenerator {
private:
    hir::HIRModule* hirModule_;
    MIRModule* mirModule_;
    MIRBuilder* builder_;
    
    // Mapping from HIR to MIR
    std::unordered_map<hir::HIRValue*, MIRPlacePtr> valueMap_;
    std::unordered_map<hir::HIRBasicBlock*, MIRBasicBlock*> blockMap_;
    std::unordered_map<hir::HIRFunction*, MIRFunctionPtr> functionMap_;
    
    // Current function context
    MIRFunction* currentFunction_;
    uint32_t localCounter_;
    uint32_t blockCounter_;
    uint32_t totalBlocks_;
    
    // Current loop context for break/continue statements
    std::shared_ptr<LoopContext> currentLoopContext_;
    
public:
    MIRGenerator(hir::HIRModule* hirModule, MIRModule* mirModule)
        : hirModule_(hirModule), mirModule_(mirModule), builder_(nullptr),
          currentFunction_(nullptr), localCounter_(0), blockCounter_(0), totalBlocks_(0) {}
    
    ~MIRGenerator() {
        delete builder_;
    }
    
    void generate() {
        // Generate all functions
        for (const auto& hirFunc : hirModule_->functions) {
            generateFunction(hirFunc.get());
        }
    }
    
private:
    // ==================== Type Translation ====================
    
    MIRTypePtr translateType(hir::HIRType* hirType) {
        if (!hirType) {
            return std::make_shared<MIRType>(MIRType::Kind::Void);
        }
        
        switch (hirType->kind) {
            case hir::HIRType::Kind::Void:
            case hir::HIRType::Kind::Unit:
                return std::make_shared<MIRType>(MIRType::Kind::Void);
            
            case hir::HIRType::Kind::Bool:
                return std::make_shared<MIRType>(MIRType::Kind::I1);
            
            case hir::HIRType::Kind::I8:
                return std::make_shared<MIRType>(MIRType::Kind::I8);
            case hir::HIRType::Kind::I16:
                return std::make_shared<MIRType>(MIRType::Kind::I16);
            case hir::HIRType::Kind::I32:
                return std::make_shared<MIRType>(MIRType::Kind::I32);
            case hir::HIRType::Kind::I64:
                return std::make_shared<MIRType>(MIRType::Kind::I64);
            case hir::HIRType::Kind::ISize:
                return std::make_shared<MIRType>(MIRType::Kind::ISize);
            
            case hir::HIRType::Kind::U8:
                return std::make_shared<MIRType>(MIRType::Kind::U8);
            case hir::HIRType::Kind::U16:
                return std::make_shared<MIRType>(MIRType::Kind::U16);
            case hir::HIRType::Kind::U32:
                return std::make_shared<MIRType>(MIRType::Kind::U32);
            case hir::HIRType::Kind::U64:
                return std::make_shared<MIRType>(MIRType::Kind::U64);
            case hir::HIRType::Kind::USize:
                return std::make_shared<MIRType>(MIRType::Kind::USize);
            
            case hir::HIRType::Kind::F32:
                return std::make_shared<MIRType>(MIRType::Kind::F32);
            case hir::HIRType::Kind::F64:
                return std::make_shared<MIRType>(MIRType::Kind::F64);
            
            case hir::HIRType::Kind::Pointer:
            case hir::HIRType::Kind::Reference:
            case hir::HIRType::Kind::String:  // Strings are represented as pointers
                return std::make_shared<MIRType>(MIRType::Kind::Pointer);

            case hir::HIRType::Kind::Array:
                return std::make_shared<MIRType>(MIRType::Kind::Array);

            case hir::HIRType::Kind::Struct:
                return std::make_shared<MIRType>(MIRType::Kind::Struct);

            case hir::HIRType::Kind::Function:
                return std::make_shared<MIRType>(MIRType::Kind::Function);

            case hir::HIRType::Kind::Any:
                return std::make_shared<MIRType>(MIRType::Kind::I64);
            
            default:
                return std::make_shared<MIRType>(MIRType::Kind::Void);
        }
    }
    
    // ==================== Loop Analysis ====================
    
    void analyzeLoops(hir::HIRFunction* hirFunc) {
        // Reset loop context
        currentLoopContext_ = nullptr;
        
        std::cerr << "DEBUG: Analyzing loops in function with " << hirFunc->basicBlocks.size() << " basic blocks" << std::endl;
        
        // Find loop headers by looking for blocks with conditional branches
        // where one of the successors is a predecessor of the current block
        for (const auto& hirBlock : hirFunc->basicBlocks) {
            // Check if this block has a conditional branch terminator
            if (!hirBlock->instructions.empty()) {
                auto lastInst = hirBlock->instructions.back().get();
                if (lastInst->opcode == hir::HIRInstruction::Opcode::CondBr) {
                    std::cerr << "DEBUG: Found conditional branch in block" << std::endl;
                    // Check if this is a loop header (has a back edge)
                    if (isLoopHeader(hirBlock.get())) {
                        std::cerr << "DEBUG: Found loop header, setting up loop context" << std::endl;
                        // Set up loop context for this loop
                        setupLoopContext(hirBlock.get());
                    }
                }
            }
        }
    }
    
    bool isLoopHeader(hir::HIRBasicBlock* block) {
        if (block->successors.size() < 2) return false;
        
        // Get the successors of the conditional branch
        auto* trueSuccessor = block->successors[0].get();
        auto* falseSuccessor = block->successors[1].get();
        
        // Check if either successor can reach back to this block
        return canReachBlock(trueSuccessor, block) || canReachBlock(falseSuccessor, block);
    }
    
    bool canReachBlock(hir::HIRBasicBlock* from, hir::HIRBasicBlock* to) {
        if (from == to) return true;
        
        std::set<hir::HIRBasicBlock*> visited;
        std::queue<hir::HIRBasicBlock*> worklist;
        worklist.push(from);
        visited.insert(from);
        
        while (!worklist.empty()) {
            auto* current = worklist.front();
            worklist.pop();
            
            for (const auto& successor : current->successors) {
                if (successor.get() == to) {
                    return true;
                }
                if (visited.find(successor.get()) == visited.end()) {
                    visited.insert(successor.get());
                    worklist.push(successor.get());
                }
            }
        }
        
        return false;
    }
    
    void setupLoopContext(hir::HIRBasicBlock* loopHeader) {
        // Create a new loop context
        auto loopContext = std::make_shared<LoopContext>();
        
        std::cerr << "DEBUG: Setting up loop context for header with " << loopHeader->successors.size() << " successors" << std::endl;
        
        // For a while loop, the structure is:
        // - loopHeader (condition block) -> [bodyBlock, endBlock]
        // - bodyBlock -> loopHeader (back edge)
        // So:
        // - break target should be endBlock (the successor that doesn't lead back to the header)
        // - continue target should be the loopHeader (condition block)
        
        if (loopHeader->successors.size() >= 2) {
            auto* successor1 = loopHeader->successors[0].get();
            auto* successor2 = loopHeader->successors[1].get();
            
            std::cerr << "DEBUG: Checking successors: " << successor1 << " and " << successor2 << std::endl;
            
            // Check which successor can reach back to the loop header (this is the body block)
            // The other one is the exit block (break target)
            if (canReachBlock(successor1, loopHeader)) {
                // successor1 is the body block, successor2 is the exit block
                loopContext->breakTarget = blockMap_[successor2];
                loopContext->continueTarget = blockMap_[loopHeader];
                std::cerr << "DEBUG: Set break target to successor2 (exit block) and continue target to loop header" << std::endl;
            } else if (canReachBlock(successor2, loopHeader)) {
                // successor2 is the body block, successor1 is the exit block
                loopContext->breakTarget = blockMap_[successor1];
                loopContext->continueTarget = blockMap_[loopHeader];
                std::cerr << "DEBUG: Set break target to successor1 (exit block) and continue target to loop header" << std::endl;
            } else {
                // Default fallback - this shouldn't happen for well-formed loops
                std::cerr << "DEBUG: Neither successor leads back to loop header, using default assignment" << std::endl;
                loopContext->breakTarget = blockMap_[successor2];
                loopContext->continueTarget = blockMap_[successor1];
            }
        }
        
        // Set the parent context
        loopContext->parent = currentLoopContext_;
        
        // Set as current loop context
        currentLoopContext_ = loopContext;
        
        std::cerr << "DEBUG: Loop context set up with break target: " << loopContext->breakTarget 
                  << " and continue target: " << loopContext->continueTarget << std::endl;
    }
    
    bool isInLoop(hir::HIRBasicBlock* block, hir::HIRBasicBlock* loopHeader) {
        // A block is in the loop if it can reach the loop header
        return canReachBlock(block, loopHeader);
    }

// ==================== Function Translation ====================
    
    void generateFunction(hir::HIRFunction* hirFunc) {
        if (!hirFunc) return;
        
        // Create MIR function
        auto mirFunc = mirModule_->createFunction(hirFunc->name);
        functionMap_[hirFunc] = mirFunc;
        currentFunction_ = mirFunc.get();
        localCounter_ = 0;
        blockCounter_ = 0;
        
        // Create builder
        if (builder_) delete builder_;
        builder_ = new MIRBuilder(currentFunction_);
        
        // Translate return type
        if (hirFunc->functionType) {
            mirFunc->returnType = translateType(hirFunc->functionType->returnType.get());
        } else {
            mirFunc->returnType = std::make_shared<MIRType>(MIRType::Kind::Void);
        }
        
        // Translate parameters
        for (size_t i = 0; i < hirFunc->parameters.size(); ++i) {
            auto hirParam = hirFunc->parameters[i];
            auto paramType = translateType(hirParam->type.get());
            auto mirParam = std::make_shared<MIRPlace>(
                MIRPlace::Kind::Argument, static_cast<uint32_t>(i + 1),
                paramType, hirParam->name);
            mirFunc->arguments.push_back(mirParam);
            valueMap_[hirParam] = mirParam;
        }
        
        // Create return place (_0)
        auto returnPlace = std::make_shared<MIRPlace>(
            MIRPlace::Kind::Return, 0, mirFunc->returnType, "");
        valueMap_[nullptr] = returnPlace;  // Use for return values
        
        // Initialize block counter
        blockCounter_ = 0;
        
        // Clear existing basic blocks in MIR function
        currentFunction_->basicBlocks.clear();
        
        // Translate basic blocks
        blockMap_.clear();
        blockCounter_ = 0;
        
        
        
        // First pass: create MIR blocks for all HIR blocks
        for (size_t i = 0; i < hirFunc->basicBlocks.size(); ++i) {
            const auto& hirBlock = hirFunc->basicBlocks[i];
            std::string label = "bb" + std::to_string(i);
            auto mirBlock = currentFunction_->createBasicBlock(label);
            blockMap_[hirBlock.get()] = mirBlock.get();
        }
        
        
        
        // Analyze control flow to identify loops and set up loop contexts
        analyzeLoops(hirFunc);
        
        // Second pass: translate instructions
        for (const auto& hirBlock : hirFunc->basicBlocks) {
            auto mirBlock = blockMap_[hirBlock.get()];
            if (!mirBlock) continue;
            
            builder_->setInsertPoint(mirBlock);
            
            // Still translate instructions for empty blocks (they might have a terminator)
            if (hirBlock->instructions.empty()) {
                
            }
            
            // Debug: Check if block has terminator after processing
            for (const auto& hirInst : hirBlock->instructions) {
                generateInstruction(hirInst.get(), mirBlock);
            }
            
            // Debug: Check if this block has a terminator
            if (!mirBlock->terminator) {
                
            }
        }
        
        
    }
    
    // ==================== Instruction Translation ====================
    
    void generateInstruction(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        if (!hirInst) return;
        
        switch (hirInst->opcode) {
            case hir::HIRInstruction::Opcode::Add:
            case hir::HIRInstruction::Opcode::Sub:
            case hir::HIRInstruction::Opcode::Mul:
            case hir::HIRInstruction::Opcode::Div:
            case hir::HIRInstruction::Opcode::Rem:
                generateBinaryOp(hirInst, mirBlock);
                break;
            
            case hir::HIRInstruction::Opcode::And:
            case hir::HIRInstruction::Opcode::Or:
            case hir::HIRInstruction::Opcode::Xor:
            case hir::HIRInstruction::Opcode::Shl:
            case hir::HIRInstruction::Opcode::Shr:
                generateBinaryOp(hirInst, mirBlock);
                break;
            
            case hir::HIRInstruction::Opcode::Eq:
            case hir::HIRInstruction::Opcode::Ne:
            case hir::HIRInstruction::Opcode::Lt:
            case hir::HIRInstruction::Opcode::Le:
            case hir::HIRInstruction::Opcode::Gt:
            case hir::HIRInstruction::Opcode::Ge:
                generateComparison(hirInst, mirBlock);
                break;
            
            case hir::HIRInstruction::Opcode::Not:
            case hir::HIRInstruction::Opcode::Neg:
                generateUnaryOp(hirInst, mirBlock);
                break;
            
            case hir::HIRInstruction::Opcode::Alloca:
                generateAlloca(hirInst, mirBlock);
                break;
            
            case hir::HIRInstruction::Opcode::Load:
                generateLoad(hirInst, mirBlock);
                break;
            
            case hir::HIRInstruction::Opcode::Store:
                generateStore(hirInst, mirBlock);
                break;
            
            case hir::HIRInstruction::Opcode::Call:
                
                generateCall(hirInst, mirBlock);
                break;
            
            case hir::HIRInstruction::Opcode::Return:
                generateReturn(hirInst, mirBlock);
                break;
            
            case hir::HIRInstruction::Opcode::Break:
                std::cerr << "DEBUG: Processing Break instruction, currentLoopContext_=" << currentLoopContext_ << std::endl;
                generateBreak(hirInst, mirBlock);
                // Skip processing any remaining instructions in this block
                return;
            
            case hir::HIRInstruction::Opcode::Continue:
                std::cerr << "DEBUG: Processing Continue instruction, currentLoopContext_=" << currentLoopContext_ << std::endl;
                generateContinue(hirInst, mirBlock);
                // Skip processing any remaining instructions in this block
                return;
            
            case hir::HIRInstruction::Opcode::Br:
                generateBr(hirInst, mirBlock);
                break;
            
            case hir::HIRInstruction::Opcode::CondBr:
                generateCondBr(hirInst, mirBlock);
                break;
            
            case hir::HIRInstruction::Opcode::Cast:
                generateCast(hirInst, mirBlock);
                break;

            case hir::HIRInstruction::Opcode::ArrayConstruct:
                generateArrayConstruct(hirInst, mirBlock);
                break;

            case hir::HIRInstruction::Opcode::GetElement:
                generateGetElement(hirInst, mirBlock);
                break;

            case hir::HIRInstruction::Opcode::SetElement:
                generateSetElement(hirInst, mirBlock);
                break;

            case hir::HIRInstruction::Opcode::StructConstruct:
                generateStructConstruct(hirInst, mirBlock);
                break;

            case hir::HIRInstruction::Opcode::GetField:
                generateGetField(hirInst, mirBlock);
                break;

            case hir::HIRInstruction::Opcode::SetField:
                generateSetField(hirInst, mirBlock);
                break;

            default:
                // For unsupported instructions, create a nop or placeholder
                std::cerr << "Unsupported HIR instruction: " << hirInst->toString() << std::endl;
                break;
        }
    }
    
    MIRPlacePtr getOrCreatePlace(hir::HIRValue* hirValue) {
        if (!hirValue) {
            return valueMap_[nullptr];  // Return place
        }

        auto it = valueMap_.find(hirValue);
        if (it != valueMap_.end()) {
            return it->second;
        }

        // Create new local for this value
        // For pointer types (like alloca), use the pointee type instead
        hir::HIRType* typeToTranslate = hirValue->type.get();

        // Debug: Print the original type
        if (typeToTranslate) {
            std::cerr << "DEBUG MIR: Original HIR type for " << hirValue->name
                      << " is kind=" << static_cast<int>(typeToTranslate->kind) << std::endl;
        } else {
            std::cerr << "DEBUG MIR: HIR type is null for " << hirValue->name << std::endl;
        }

        try {
            if (typeToTranslate) {
                if (auto* ptrType = dynamic_cast<hir::HIRPointerType*>(typeToTranslate)) {
                    if (ptrType && ptrType->pointeeType) {
                        std::cerr << "DEBUG MIR: Pointer pointee type kind="
                                  << static_cast<int>(ptrType->pointeeType->kind) << std::endl;
                        // Don't extract pointee for array/struct pointers - keep them as pointers
                        if (ptrType->pointeeType->kind != hir::HIRType::Kind::Array &&
                            ptrType->pointeeType->kind != hir::HIRType::Kind::Struct) {
                            typeToTranslate = ptrType->pointeeType.get();
                            std::cerr << "DEBUG MIR: Using pointee type for pointer variable: "
                                      << hirValue->name << std::endl;
                        } else {
                            std::cerr << "DEBUG MIR: Keeping array/struct pointer as pointer type: "
                                      << hirValue->name << std::endl;
                        }
                    } else {
                        std::cerr << "DEBUG MIR: Warning - pointeeType is null for pointer variable: "
                                  << hirValue->name << std::endl;
                    }
                }
            }
        } catch (...) {
            // If cast fails, use the type as-is
            std::cerr << "DEBUG MIR: Exception during pointer type cast for variable: "
                      << hirValue->name << std::endl;
        }

        auto mirType = translateType(typeToTranslate);
        auto place = currentFunction_->createLocal(mirType, hirValue->name);
        valueMap_[hirValue] = place;

        // Create StorageLive
        builder_->createStorageLive(place);

        return place;
    }
    
    MIROperandPtr translateOperand(hir::HIRValue* hirValue) {
        // Check if it's a constant
        hir::HIRConstant* constant = nullptr;
        try {
            if (hirValue) {
                constant = dynamic_cast<hir::HIRConstant*>(hirValue);
            }
        } catch (...) {
            // If cast fails, treat as non-constant
            constant = nullptr;
        }

        if (constant) {
            auto mirType = translateType(constant->type.get());

            switch (constant->kind) {
                case hir::HIRConstant::Kind::Integer:
                    return builder_->createIntConstant(
                        std::get<int64_t>(constant->value), mirType);

                case hir::HIRConstant::Kind::Float:
                    return builder_->createFloatConstant(
                        std::get<double>(constant->value), mirType);

                case hir::HIRConstant::Kind::Boolean:
                    return builder_->createBoolConstant(
                        std::get<bool>(constant->value), mirType);

                case hir::HIRConstant::Kind::String:
                    return builder_->createStringConstant(
                        std::get<std::string>(constant->value), mirType);

                case hir::HIRConstant::Kind::Null:
                    return builder_->createNullConstant(mirType);

                default:
                    return builder_->createZeroInitConstant(mirType);
            }
        }

        // Otherwise, it's a place reference
        auto place = getOrCreatePlace(hirValue);
        return builder_->createCopyOperand(place);
    }
    
    void generateBinaryOp(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;
        if (hirInst->operands.size() < 2) return;
        
        auto lhs = translateOperand(hirInst->operands[0].get());
        auto rhs = translateOperand(hirInst->operands[1].get());
        
        MIRRValuePtr rvalue;
        switch (hirInst->opcode) {
            case hir::HIRInstruction::Opcode::Add:
                rvalue = builder_->createAdd(lhs, rhs);
                break;
            case hir::HIRInstruction::Opcode::Sub:
                rvalue = builder_->createSub(lhs, rhs);
                break;
            case hir::HIRInstruction::Opcode::Mul:
                rvalue = builder_->createMul(lhs, rhs);
                break;
            case hir::HIRInstruction::Opcode::Div:
                rvalue = builder_->createDiv(lhs, rhs);
                break;
            case hir::HIRInstruction::Opcode::Rem:
                rvalue = builder_->createRem(lhs, rhs);
                break;
            case hir::HIRInstruction::Opcode::And:
                rvalue = builder_->createBitAnd(lhs, rhs);
                break;
            case hir::HIRInstruction::Opcode::Or:
                rvalue = builder_->createBitOr(lhs, rhs);
                break;
            case hir::HIRInstruction::Opcode::Xor:
                rvalue = builder_->createBitXor(lhs, rhs);
                break;
            case hir::HIRInstruction::Opcode::Shl:
                rvalue = builder_->createShl(lhs, rhs);
                break;
            case hir::HIRInstruction::Opcode::Shr:
                rvalue = builder_->createShr(lhs, rhs);
                break;
            default:
                return;
        }
        
        auto dest = getOrCreatePlace(hirInst);
        builder_->createAssign(dest, rvalue);
    }
    
    void generateComparison(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;
        if (hirInst->operands.size() < 2) return;
        
        auto lhs = translateOperand(hirInst->operands[0].get());
        auto rhs = translateOperand(hirInst->operands[1].get());
        
        MIRRValuePtr rvalue;
        switch (hirInst->opcode) {
            case hir::HIRInstruction::Opcode::Eq:
                rvalue = builder_->createEq(lhs, rhs);
                break;
            case hir::HIRInstruction::Opcode::Ne:
                rvalue = builder_->createNe(lhs, rhs);
                break;
            case hir::HIRInstruction::Opcode::Lt:
                rvalue = builder_->createLt(lhs, rhs);
                break;
            case hir::HIRInstruction::Opcode::Le:
                rvalue = builder_->createLe(lhs, rhs);
                break;
            case hir::HIRInstruction::Opcode::Gt:
                rvalue = builder_->createGt(lhs, rhs);
                break;
            case hir::HIRInstruction::Opcode::Ge:
                rvalue = builder_->createGe(lhs, rhs);
                break;
            default:
                return;
        }
        
        auto dest = getOrCreatePlace(hirInst);
        builder_->createAssign(dest, rvalue);
    }
    
    void generateUnaryOp(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;
        if (hirInst->operands.empty()) return;
        
        auto operand = translateOperand(hirInst->operands[0].get());
        
        MIRRValuePtr rvalue;
        switch (hirInst->opcode) {
            case hir::HIRInstruction::Opcode::Not:
                rvalue = builder_->createNot(operand);
                break;
            case hir::HIRInstruction::Opcode::Neg:
                rvalue = builder_->createNeg(operand);
                break;
            default:
                return;
        }
        
        auto dest = getOrCreatePlace(hirInst);
        builder_->createAssign(dest, rvalue);
    }
    
    void generateAlloca(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;
        // Alloca just creates a local place
        auto place = getOrCreatePlace(hirInst);
        builder_->createStorageLive(place);
    }
    
    void generateLoad(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;
        if (hirInst->operands.empty()) return;
        
        auto ptr = translateOperand(hirInst->operands[0].get());
        auto dest = getOrCreatePlace(hirInst);
        
        // Load is just a Use(copy ptr) in MIR
        auto rvalue = builder_->createUse(ptr);
        builder_->createAssign(dest, rvalue);
    }
    
    void generateStore(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;
        if (hirInst->operands.size() < 2) return;
        
        auto value = translateOperand(hirInst->operands[0].get());
        auto ptr = getOrCreatePlace(hirInst->operands[1].get());
        
        // Store is an assignment in MIR
        auto rvalue = builder_->createUse(value);
        builder_->createAssign(ptr, rvalue);
    }
    
    void generateCall(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;
        if (hirInst->operands.empty()) return;
        
        
        
        // First operand is the function name (string constant)
        auto funcOperand = translateOperand(hirInst->operands[0].get());
        
        std::vector<MIROperandPtr> args;
        for (size_t i = 1; i < hirInst->operands.size(); ++i) {
            args.push_back(translateOperand(hirInst->operands[i].get()));
        }
        
        
        
        auto dest = getOrCreatePlace(hirInst);
        
        // Create a new block for the continuation after the call
        auto contBlock = builder_->createBasicBlock("call_cont");
        
        // Create the call terminator with destination and continuation block
        builder_->createCall(funcOperand, args, dest, contBlock.get(), nullptr);
        
        
        
        // Switch to the continuation block
        builder_->setInsertPoint(contBlock.get());
    }
    
    void generateReturn(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;
        if (!hirInst->operands.empty()) {
            auto returnValue = translateOperand(hirInst->operands[0].get());
            auto returnPlace = valueMap_[nullptr];
            auto rvalue = builder_->createUse(returnValue);
            builder_->createAssign(returnPlace, rvalue);
        }
        
        builder_->createReturn();
    }
    
void generateBr(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)hirInst;
        std::cerr << "DEBUG MIR: Entering generateBr" << std::endl;
        
        // Find target block from HIR
        if (hirInst->parentBlock && !hirInst->parentBlock->successors.empty()) {
            auto* hirTargetBlock = hirInst->parentBlock->successors[0].get();
            auto* mirTargetBlock = blockMap_[hirTargetBlock];
            std::cerr << "DEBUG MIR: Found target block: " << mirTargetBlock << std::endl;
            
            if (mirTargetBlock) {
                std::cerr << "DEBUG MIR: Creating goto to target block" << std::endl;
                builder_->createGoto(mirTargetBlock);
                std::cerr << "DEBUG MIR: Goto created" << std::endl;
                return;
            }
        }
        
        // Fallback: create a return if no valid target
        std::cerr << "DEBUG MIR: No valid target, creating return" << std::endl;
        builder_->createReturn();
    }
    
    void generateCondBr(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;
        if (hirInst->operands.empty()) return;
        
        std::cerr << "DEBUG MIR: Entering generateCondBr" << std::endl;
        auto condition = translateOperand(hirInst->operands[0].get());
        std::cerr << "DEBUG MIR: Condition translated" << std::endl;
        
        // Get the successor blocks from the current HIR block
        if (hirInst->parentBlock && hirInst->parentBlock->successors.size() >= 2) {
            auto* trueBlock = blockMap_[hirInst->parentBlock->successors[0].get()];
            auto* falseBlock = blockMap_[hirInst->parentBlock->successors[1].get()];
            
            std::cerr << "DEBUG MIR: Found successor blocks: true=" << trueBlock << ", false=" << falseBlock << std::endl;
            
            // Make sure the blocks exist
            if (trueBlock && falseBlock) {
                // Create switch on boolean (1 = true, 0 = false)
                std::vector<std::pair<int64_t, MIRBasicBlock*>> targets;
                targets.push_back(std::make_pair(1, trueBlock));  // If condition == 1, go to trueBlock
                std::cerr << "DEBUG MIR: Creating switch instruction" << std::endl;
                builder_->createSwitchInt(condition, targets, falseBlock);  // Otherwise go to falseBlock
                std::cerr << "DEBUG MIR: Switch instruction created" << std::endl;
            } else {
                // Fallback - create goto to next block
                if (blockCounter_ < currentFunction_->basicBlocks.size()) {
                    auto targetBlock = currentFunction_->basicBlocks[blockCounter_].get();
                    builder_->createGoto(targetBlock);
                } else {
                    builder_->createReturn();
                }
            }
        } else {
            // Fallback - create goto to next block
            if (blockCounter_ < totalBlocks_) {
                auto targetBlock = currentFunction_->basicBlocks[blockCounter_].get();
                builder_->createGoto(targetBlock);
            } else {
                builder_->createReturn();
            }
        }
    }
    
    void generateBreak(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)hirInst;
        std::cerr << "DEBUG: Generating break instruction" << std::endl;
        
        if (currentLoopContext_) {
            std::cerr << "DEBUG: Found loop context, break target: " << currentLoopContext_->breakTarget 
                      << ", creating goto" << std::endl;
            if (currentLoopContext_->breakTarget) {
                // Create a direct terminator for the block instead of adding a statement
                // This ensures the break is the last instruction in the block
                mirBlock->terminator = std::make_unique<MIRGotoTerminator>(currentLoopContext_->breakTarget);
                std::cerr << "DEBUG: Break terminator created directly to target: " << currentLoopContext_->breakTarget << std::endl;
                return;
            }
        }
        
        std::cerr << "DEBUG: No loop context found, creating return" << std::endl;
        // Fallback: create a return statement
        mirBlock->terminator = std::make_unique<MIRReturnTerminator>();
    }
    
    void generateContinue(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)hirInst;
        std::cerr << "DEBUG: Generating continue instruction" << std::endl;
        
        if (currentLoopContext_) {
            std::cerr << "DEBUG: Found loop context, continue target: " << currentLoopContext_->continueTarget 
                      << ", creating goto" << std::endl;
            if (currentLoopContext_->continueTarget) {
                // Create a direct terminator for the block instead of adding a statement
                // This ensures the continue is the last instruction in the block
                mirBlock->terminator = std::make_unique<MIRGotoTerminator>(currentLoopContext_->continueTarget);
                std::cerr << "DEBUG: Continue terminator created directly to target: " << currentLoopContext_->continueTarget << std::endl;
                return;
            }
        }
        
        std::cerr << "DEBUG: No loop context found, creating return" << std::endl;
        // Fallback: create a return statement
        mirBlock->terminator = std::make_unique<MIRReturnTerminator>();
    }
    
    void generateCast(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;
        if (hirInst->operands.empty()) return;

        auto operand = translateOperand(hirInst->operands[0].get());
        auto targetType = translateType(hirInst->type.get());

        // Determine cast kind based on types
        MIRCastRValue::CastKind castKind = MIRCastRValue::CastKind::IntToInt;

        auto rvalue = builder_->createCast(castKind, operand, targetType);
        auto dest = getOrCreatePlace(hirInst);
        builder_->createAssign(dest, rvalue);
    }

    void generateArrayConstruct(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;

        auto dest = getOrCreatePlace(hirInst);

        // Convert HIR array elements to MIR operands
        std::vector<MIROperandPtr> mirElements;
        for (const auto& elem : hirInst->operands) {
            auto operand = translateOperand(elem.get());
            mirElements.push_back(operand);
        }

        // Create aggregate rvalue for array
        auto aggregateRValue = std::make_shared<MIRAggregateRValue>(
            MIRAggregateRValue::AggregateKind::Array,
            mirElements
        );

        builder_->createAssign(dest, aggregateRValue);
    }

    void generateGetElement(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;
        if (hirInst->operands.size() < 2) return;

        auto array = translateOperand(hirInst->operands[0].get());
        auto index = translateOperand(hirInst->operands[1].get());
        auto dest = getOrCreatePlace(hirInst);

        // Create GetElement rvalue
        auto getElementRValue = std::make_shared<MIRGetElementRValue>(array, index);
        builder_->createAssign(dest, getElementRValue);
    }

    void generateSetElement(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;
        if (hirInst->operands.size() < 3) return;

        // array, index, value
        auto array = translateOperand(hirInst->operands[0].get());
        auto index = translateOperand(hirInst->operands[1].get());
        auto value = translateOperand(hirInst->operands[2].get());

        // For now, this is a no-op placeholder
        // A real implementation would compute the address and store the value
        // TODO: Implement proper array element assignment
        (void)array;
        (void)index;
        (void)value;
    }

    void generateStructConstruct(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;

        auto dest = getOrCreatePlace(hirInst);

        // Convert HIR struct fields to MIR operands
        std::vector<MIROperandPtr> mirFields;
        for (const auto& field : hirInst->operands) {
            auto operand = translateOperand(field.get());
            mirFields.push_back(operand);
        }

        // Create aggregate rvalue for struct
        auto aggregateRValue = std::make_shared<MIRAggregateRValue>(
            MIRAggregateRValue::AggregateKind::Struct,
            mirFields
        );

        builder_->createAssign(dest, aggregateRValue);
    }

    void generateGetField(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;

        if (hirInst->operands.size() < 2) {
            std::cerr << "ERROR: GetField requires at least 2 operands (struct, index)" << std::endl;
            return;
        }

        // operands[0] = struct pointer, operands[1] = field index constant
        auto structPtr = translateOperand(hirInst->operands[0].get());
        auto fieldIndex = translateOperand(hirInst->operands[1].get());
        auto dest = getOrCreatePlace(hirInst);

        // Create GetElement rvalue for field access (similar to array indexing)
        auto getFieldRValue = std::make_shared<MIRGetElementRValue>(structPtr, fieldIndex);

        builder_->createAssign(dest, getFieldRValue);
    }

    void generateSetField(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        if (hirInst->operands.size() < 3) {
            std::cerr << "ERROR: SetField requires 3 operands (struct, index, value)" << std::endl;
            return;
        }

        // operands[0] = struct pointer, operands[1] = field index, operands[2] = value to store
        auto structPtr = translateOperand(hirInst->operands[0].get());
        auto fieldIndex = translateOperand(hirInst->operands[1].get());
        auto value = translateOperand(hirInst->operands[2].get());

        // Create a SetElement RValue that encodes the store operation
        // We create a special aggregate-like operation with the value to store
        std::vector<MIROperandPtr> elements;
        elements.push_back(structPtr);
        elements.push_back(fieldIndex);
        elements.push_back(value);

        // Use a special aggregate kind to signal this is a SetField
        // The LLVM backend will recognize this pattern
        auto setFieldRValue = std::make_shared<MIRAggregateRValue>(
            MIRAggregateRValue::AggregateKind::Struct,  // Reuse struct kind with 3 elements as marker
            elements
        );

        // Create a dummy place for the result (void type)
        auto resultPlace = getOrCreatePlace(hirInst);

        // This assignment signals "execute the SetField operation"
        builder_->createAssign(resultPlace, setFieldRValue);
    }
};

// ==================== Public API ====================

MIRModule* generateMIR(hir::HIRModule* hirModule, const std::string& moduleName) {
    if (!hirModule) return nullptr;
    
    auto mirModule = new MIRModule(moduleName);
    MIRGenerator generator(hirModule, mirModule);
    generator.generate();
    
    return mirModule;
}

} // namespace nova::mir
