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

    // Map from loop header HIR block to its loop context
    std::unordered_map<hir::HIRBasicBlock*, std::shared_ptr<LoopContext>> loopContextMap_;

    // Map from HIR block to its containing loop context
    std::unordered_map<hir::HIRBasicBlock*, std::shared_ptr<LoopContext>> blockToLoopMap_;

    // Current HIR block being processed
    hir::HIRBasicBlock* currentHIRBlock_;
    
public:
    MIRGenerator(hir::HIRModule* hirModule, MIRModule* mirModule)
        : hirModule_(hirModule), mirModule_(mirModule), builder_(nullptr),
          currentFunction_(nullptr), localCounter_(0), blockCounter_(0), totalBlocks_(0),
          currentHIRBlock_(nullptr) {}
    
    ~MIRGenerator() {
        delete builder_;
    }
    
    void generate() {
        // Generate all functions
        for (const auto& hirFunc : hirModule_->functions) {
            // Skip external functions (declarations only, no body to generate)
            if (hirFunc->linkage == hir::HIRFunction::Linkage::External) {
                continue;
            }
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
    //
    // IMPLEMENTATION: This implementation uses DOMINANCE ANALYSIS for correct loop membership detection.
    //
    // Algorithm:
    // 1. Compute dominators for all blocks using iterative data-flow analysis
    // 2. Identify loop headers using back-edge detection with UPDATE block filtering
    // 3. Sort loops by nesting depth using dominance (not reachability)
    // 4. Map blocks to loops using dominance criterion:
    //    - A block belongs to a loop if the loop header DOMINATES it
    //    - AND the loop header can REACH it
    //    - AND it's not the exit block
    //
    // WORKING CASES:
    // ✅ Single loops with break/continue (test_break_simple.ts)
    // ✅ Simple nested loops (test_nested_simple.ts)
    // ✅ Sequential loops with break/continue (test_break_continue.ts) - FIXED by dominance!
    // ✅ Nested loops with continue inside conditionals (test_nested_break_continue.ts) - FIXED!
    //
    // All major break/continue patterns now work correctly!
    //

    void analyzeLoops(hir::HIRFunction* hirFunc) {
        // Reset loop context and maps
        currentLoopContext_ = nullptr;
        loopContextMap_.clear();
        blockToLoopMap_.clear();

        // Compute dominators for all blocks
        auto dominators = computeDominators(hirFunc);

        // Find loop headers by looking for blocks with conditional branches
        // where one of the successors is a predecessor of the current block
        // Collect all loop headers first
        std::vector<hir::HIRBasicBlock*> loopHeaders;
        for (const auto& hirBlock : hirFunc->basicBlocks) {
            // Check if this block has a conditional branch terminator
            if (!hirBlock->instructions.empty()) {
                auto lastInst = hirBlock->instructions.back().get();
                if (lastInst->opcode == hir::HIRInstruction::Opcode::CondBr) {
                    // Check if this is a loop header (has a back edge)
                    if (isLoopHeader(hirBlock.get())) {
                        loopHeaders.push_back(hirBlock.get());
                    }
                }
            }
        }

        // Sort loops by nesting using dominance
        // A loop A is outer to loop B if A's header dominates B's header
        std::vector<std::pair<int, hir::HIRBasicBlock*>> sortedLoops; // (depth, header)
        for (auto* header : loopHeaders) {
            // Count how many other loop headers dominate this header (nesting depth)
            int depth = 0;
            for (auto* otherHeader : loopHeaders) {
                if (header != otherHeader && dominates(otherHeader, header, dominators)) {
                    depth++;
                }
            }
            sortedLoops.push_back({depth, header});
        }

        // Sort by depth (ascending): outer loops first (depth 0), inner loops last
        std::sort(sortedLoops.begin(), sortedLoops.end());

        // Process loops in order: outer first, then inner (so inner can overwrite)
        for (const auto& pair : sortedLoops) {
            setupLoopContext(pair.second, hirFunc, dominators);
        }
    }
    
    bool isLoopHeader(hir::HIRBasicBlock* block) {
        if (block->successors.size() < 2) return false;

        // Get the successors of the conditional branch
        auto* trueSuccessor = block->successors[0].get();
        auto* falseSuccessor = block->successors[1].get();

        // Check if either successor can reach back to this block
        bool hasBackEdge = canReachBlock(trueSuccessor, block) || canReachBlock(falseSuccessor, block);

        if (!hasBackEdge) return false;

        // Additional filtering to distinguish real loop headers from if-conditionals inside loops:
        // A real loop header typically has an UPDATE block that unconditionally jumps back to it.
        // An update block has only ONE successor (the loop header).
        // Check if there's a predecessor with only one successor that can be reached from this block.

        bool hasUpdateBlock = false;
        for (const auto& pred : block->predecessors) {
            // Check if this predecessor is an update block:
            // 1. It has only one successor (unconditional jump)
            // 2. That successor is this block
            // 3. This block can reach the predecessor (forming the loop cycle)
            if (pred->successors.size() == 1 &&
                pred->successors[0].get() == block &&
                canReachBlock(block, pred.get())) {
                hasUpdateBlock = true;
                break;
            }
        }

        return hasUpdateBlock;
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

    // ==================== Dominance Analysis ====================

    // Compute dominators for all blocks in the function
    // Returns a map: block -> set of blocks that dominate it
    std::unordered_map<hir::HIRBasicBlock*, std::set<hir::HIRBasicBlock*>>
    computeDominators(hir::HIRFunction* hirFunc) {
        std::unordered_map<hir::HIRBasicBlock*, std::set<hir::HIRBasicBlock*>> dominators;

        if (hirFunc->basicBlocks.empty()) {
            return dominators;
        }

        // Entry block is the first block
        auto* entry = hirFunc->basicBlocks[0].get();

        // Initialize: entry dominates only itself
        dominators[entry].insert(entry);

        // All other blocks are initially dominated by all blocks
        std::set<hir::HIRBasicBlock*> allBlocks;
        for (const auto& block : hirFunc->basicBlocks) {
            allBlocks.insert(block.get());
            if (block.get() != entry) {
                dominators[block.get()] = allBlocks;
            }
        }

        // Iterate until fixed point
        bool changed = true;
        while (changed) {
            changed = false;

            // For each block except entry
            for (const auto& blockPtr : hirFunc->basicBlocks) {
                auto* block = blockPtr.get();
                if (block == entry) continue;

                // If block has no predecessors, skip it
                if (block->predecessors.empty()) {
                    continue;
                }

                // New dominators = {block} ∪ (∩ dominators of all predecessors)
                std::set<hir::HIRBasicBlock*> newDom;

                // Start with dominators of first predecessor
                bool firstPred = true;
                for (const auto& predPtr : block->predecessors) {
                    auto* pred = predPtr.get();

                    if (firstPred) {
                        newDom = dominators[pred];
                        firstPred = false;
                    } else {
                        // Intersect with dominators of this predecessor
                        std::set<hir::HIRBasicBlock*> intersection;
                        std::set_intersection(
                            newDom.begin(), newDom.end(),
                            dominators[pred].begin(), dominators[pred].end(),
                            std::inserter(intersection, intersection.begin())
                        );
                        newDom = intersection;
                    }
                }

                // Add the block itself
                newDom.insert(block);

                // Check if dominators changed
                if (newDom != dominators[block]) {
                    dominators[block] = newDom;
                    changed = true;
                }
            }
        }

        return dominators;
    }

    // Check if block A dominates block B
    bool dominates(hir::HIRBasicBlock* a, hir::HIRBasicBlock* b,
                   const std::unordered_map<hir::HIRBasicBlock*, std::set<hir::HIRBasicBlock*>>& dominators) {
        auto it = dominators.find(b);
        if (it == dominators.end()) {
            return false;
        }
        return it->second.find(a) != it->second.end();
    }

    // Find which loop (if any) the given HIR block belongs to
    // Returns the loop header's context, or nullptr if not in a loop
    std::shared_ptr<LoopContext> findContainingLoop(hir::HIRBasicBlock* block) {
        // Look up the block directly in the blockToLoopMap
        auto it = blockToLoopMap_.find(block);
        if (it != blockToLoopMap_.end()) {
            return it->second;
        }
        return nullptr;
    }

    void setupLoopContext(hir::HIRBasicBlock* loopHeader, hir::HIRFunction* hirFunc,
                         const std::unordered_map<hir::HIRBasicBlock*, std::set<hir::HIRBasicBlock*>>& dominators) {
        // Create a new loop context
        auto loopContext = std::make_shared<LoopContext>();

        // For a while loop, the structure is:
        // - loopHeader (condition block) -> [bodyBlock, endBlock]
        // - bodyBlock -> loopHeader (back edge)
        // For a for loop, the structure is:
        // - loopHeader (condition block) -> [bodyBlock, endBlock]
        // - bodyBlock -> ... -> updateBlock -> loopHeader (back edge)
        //
        // Break target: endBlock (the successor that doesn't lead back to the header)
        // Continue target: updateBlock for for-loops, loopHeader for while-loops

        if (loopHeader->successors.size() >= 2) {
            auto* successor1 = loopHeader->successors[0].get();
            auto* successor2 = loopHeader->successors[1].get();

            // Check which successor can reach back to the loop header (this is the body block)
            // The other one is the exit block (break target)
            hir::HIRBasicBlock* bodyBlock = nullptr;
            hir::HIRBasicBlock* exitBlock = nullptr;

            if (canReachBlock(successor1, loopHeader)) {
                bodyBlock = successor1;
                exitBlock = successor2;
            } else if (canReachBlock(successor2, loopHeader)) {
                bodyBlock = successor2;
                exitBlock = successor1;
            }

            if (bodyBlock && exitBlock) {
                loopContext->breakTarget = blockMap_[exitBlock];

                // Check if this is a for-loop structure by finding the update block
                // The update block is the block that directly branches to the loop header
                // and is reachable from the body block
                hir::HIRBasicBlock* updateBlock = nullptr;

                // Find all blocks that directly branch to the loop header
                for (const auto& hirBlock : hirFunc->basicBlocks) {
                    if (hirBlock.get() != loopHeader &&
                        !hirBlock->successors.empty() &&
                        hirBlock->successors.size() == 1 &&
                        hirBlock->successors[0].get() == loopHeader) {
                        // This block has a single successor: the loop header
                        // CRITICAL: Check that the loop header DOMINATES this block
                        // This distinguishes true update blocks (inside the loop) from
                        // initialization blocks (before the loop) which have the same CFG pattern
                        if (dominates(loopHeader, hirBlock.get(), dominators) &&
                            canReachBlock(bodyBlock, hirBlock.get())) {
                            updateBlock = hirBlock.get();
                            break;
                        }
                    }
                }

                // Set continue target
                if (updateBlock) {
                    // For-loop style: continue goes to update block
                    loopContext->continueTarget = blockMap_[updateBlock];
                } else {
                    // While-loop style: continue goes to loop header
                    loopContext->continueTarget = blockMap_[loopHeader];
                }
            } else {
                // Default fallback
                loopContext->breakTarget = blockMap_[successor2];
                loopContext->continueTarget = blockMap_[loopHeader];
            }
        }

        // Set the parent context
        loopContext->parent = currentLoopContext_;

        // Store in map for lookup by loop header
        loopContextMap_[loopHeader] = loopContext;

        // Find all blocks that belong to this loop and map them to this context
        // Using DOMINANCE ANALYSIS: A block belongs to a loop if:
        // 1. The loop header DOMINATES it (header is on all paths from entry to block)
        // 2. The loop header can REACH it (block is reachable in control flow)
        // 3. It's not the exit block (blocks after the loop)

        // Get the exit block (the successor that doesn't loop back)
        hir::HIRBasicBlock* exitBlock = nullptr;
        if (loopHeader->successors.size() >= 2) {
            auto* succ1 = loopHeader->successors[0].get();
            auto* succ2 = loopHeader->successors[1].get();
            if (canReachBlock(succ1, loopHeader)) {
                exitBlock = succ2;  // succ1 loops back, so succ2 is the exit
            } else {
                exitBlock = succ1;  // succ2 loops back (or neither), so succ1 is the exit
            }
        }

        for (const auto& hirBlock : hirFunc->basicBlocks) {
            hir::HIRBasicBlock* block = hirBlock.get();

            // A block is in the loop if:
            // 1. The loop header dominates it (NEW: using dominance analysis)
            // 2. The loop header can reach it
            // 3. It's not the exit block
            // 4. It's not another loop's header (nested loops should be separate)
            if (block != exitBlock &&
                dominates(loopHeader, block, dominators) &&
                canReachBlock(loopHeader, block)) {

                // Check if this block is a loop header itself
                bool isOtherLoopHeader = (block != loopHeader) &&
                                        (loopContextMap_.find(block) != loopContextMap_.end());

                // For nested loops, allow later-processed loops to overwrite earlier mappings
                // This ensures blocks are mapped to their innermost containing loop when
                // loops are processed in an order where inner loops come later
                if (!isOtherLoopHeader) {
                    blockToLoopMap_[block] = loopContext;
                }
            }
        }

        // Set as current loop context
        currentLoopContext_ = loopContext;
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

            // Track current HIR block for loop context lookup
            currentHIRBlock_ = hirBlock.get();

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
            case hir::HIRInstruction::Opcode::Pow:
                generateBinaryOp(hirInst, mirBlock);
                break;

            case hir::HIRInstruction::Opcode::And:
            case hir::HIRInstruction::Opcode::Or:
            case hir::HIRInstruction::Opcode::Xor:
            case hir::HIRInstruction::Opcode::Shl:
            case hir::HIRInstruction::Opcode::Shr:
            case hir::HIRInstruction::Opcode::UShr:
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
                generateBreak(hirInst, mirBlock);
                // Skip processing any remaining instructions in this block
                return;

            case hir::HIRInstruction::Opcode::Continue:
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

        try {
            if (typeToTranslate) {
                if (auto* ptrType = dynamic_cast<hir::HIRPointerType*>(typeToTranslate)) {
                    if (ptrType && ptrType->pointeeType) {
                        // Don't extract pointee for array/struct pointers - keep them as pointers
                        if (ptrType->pointeeType->kind != hir::HIRType::Kind::Array &&
                            ptrType->pointeeType->kind != hir::HIRType::Kind::Struct) {
                            typeToTranslate = ptrType->pointeeType.get();
                        }
                    }
                }
            }
        } catch (...) {
            // If cast fails, use the type as-is
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
            case hir::HIRInstruction::Opcode::Pow:
                rvalue = builder_->createPow(lhs, rhs);
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
            case hir::HIRInstruction::Opcode::UShr:
                rvalue = builder_->createUShr(lhs, rhs);
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

        // Find target block from HIR
        if (hirInst->parentBlock && !hirInst->parentBlock->successors.empty()) {
            auto* hirTargetBlock = hirInst->parentBlock->successors[0].get();
            auto* mirTargetBlock = blockMap_[hirTargetBlock];

            if (mirTargetBlock) {
                builder_->createGoto(mirTargetBlock);
                return;
            }
        }

        // Fallback: create a return if no valid target
        builder_->createReturn();
    }
    
    void generateCondBr(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;
        if (hirInst->operands.empty()) return;

        auto condition = translateOperand(hirInst->operands[0].get());

        // Get the successor blocks from the current HIR block
        if (hirInst->parentBlock && hirInst->parentBlock->successors.size() >= 2) {
            auto* trueBlock = blockMap_[hirInst->parentBlock->successors[0].get()];
            auto* falseBlock = blockMap_[hirInst->parentBlock->successors[1].get()];

            // Make sure the blocks exist
            if (trueBlock && falseBlock) {
                // Create switch on boolean (1 = true, 0 = false)
                std::vector<std::pair<int64_t, MIRBasicBlock*>> targets;
                targets.push_back(std::make_pair(1, trueBlock));  // If condition == 1, go to trueBlock
                builder_->createSwitchInt(condition, targets, falseBlock);  // Otherwise go to falseBlock
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

        // Find the containing loop for the current HIR block
        auto loopContext = findContainingLoop(currentHIRBlock_);

        if (loopContext && loopContext->breakTarget) {
            // Create a direct terminator for the block
            mirBlock->terminator = std::make_unique<MIRGotoTerminator>(loopContext->breakTarget);
            return;
        }

        // Fallback: create a return statement
        mirBlock->terminator = std::make_unique<MIRReturnTerminator>();
    }

    void generateContinue(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)hirInst;

        // Find the containing loop for the current HIR block
        auto loopContext = findContainingLoop(currentHIRBlock_);

        if (loopContext && loopContext->continueTarget) {
            // Create a direct terminator for the block
            mirBlock->terminator = std::make_unique<MIRGotoTerminator>(loopContext->continueTarget);
            return;
        }

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

        // operands[0] = array pointer, operands[1] = index, operands[2] = value to store
        auto arrayPtr = translateOperand(hirInst->operands[0].get());
        auto index = translateOperand(hirInst->operands[1].get());
        auto value = translateOperand(hirInst->operands[2].get());

        // Encode as special 3-element aggregate (same pattern as SetField)
        std::vector<MIROperandPtr> elements;
        elements.push_back(arrayPtr);
        elements.push_back(index);
        elements.push_back(value);

        // Use Array aggregate kind to differentiate from SetField
        auto setElemRValue = std::make_shared<MIRAggregateRValue>(
            MIRAggregateRValue::AggregateKind::Array,  // Use Array kind for SetElement
            elements
        );

        // Create a dummy place for the result
        auto resultPlace = getOrCreatePlace(hirInst);

        // This assignment signals "execute the SetElement operation"
        builder_->createAssign(resultPlace, setElemRValue);
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
