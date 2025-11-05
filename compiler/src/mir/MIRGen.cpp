#include "nova/MIR/MIRGen.h"
#include "nova/MIR/MIRBuilder.h"
#include <unordered_map>
#include <iostream>

namespace nova::mir {

// ==================== MIR Generator Implementation ====================

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
    
public:
    MIRGenerator(hir::HIRModule* hirModule, MIRModule* mirModule)
        : hirModule_(hirModule), mirModule_(mirModule), builder_(nullptr),
          currentFunction_(nullptr), localCounter_(0), blockCounter_(0) {}
    
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
                return std::make_shared<MIRType>(MIRType::Kind::Pointer);
            
            case hir::HIRType::Kind::Array:
                return std::make_shared<MIRType>(MIRType::Kind::Array);
            
            case hir::HIRType::Kind::Struct:
                return std::make_shared<MIRType>(MIRType::Kind::Struct);
            
            case hir::HIRType::Kind::Function:
                return std::make_shared<MIRType>(MIRType::Kind::Function);
            
            default:
                return std::make_shared<MIRType>(MIRType::Kind::Void);
        }
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
        
        // Translate basic blocks
        blockMap_.clear();
        for (const auto& hirBlock : hirFunc->basicBlocks) {
            std::string label = "bb" + std::to_string(blockCounter_++);
            auto mirBlock = currentFunction_->createBasicBlock(label);
            blockMap_[hirBlock.get()] = mirBlock.get();
        }
        
        // Translate instructions
        for (const auto& hirBlock : hirFunc->basicBlocks) {
            auto mirBlock = blockMap_[hirBlock.get()];
            builder_->setInsertPoint(mirBlock);
            
            for (const auto& hirInst : hirBlock->instructions) {
                generateInstruction(hirInst.get(), mirBlock);
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
            
            case hir::HIRInstruction::Opcode::Br:
                generateBr(hirInst, mirBlock);
                break;
            
            case hir::HIRInstruction::Opcode::CondBr:
                generateCondBr(hirInst, mirBlock);
                break;
            
            case hir::HIRInstruction::Opcode::Cast:
                generateCast(hirInst, mirBlock);
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
        auto mirType = translateType(hirValue->type.get());
        auto place = currentFunction_->createLocal(mirType, hirValue->name);
        valueMap_[hirValue] = place;
        
        // Create StorageLive
        builder_->createStorageLive(place);
        
        return place;
    }
    
    MIROperandPtr translateOperand(hir::HIRValue* hirValue) {
        // Check if it's a constant
        auto* constant = dynamic_cast<hir::HIRConstant*>(hirValue);
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
        auto returnBlock = builder_->createBasicBlock("bb" + std::to_string(blockCounter_++));
        
        builder_->createCall(funcOperand, args, dest, returnBlock.get());
        builder_->setInsertPoint(returnBlock.get());
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
        // Find target block
        // In HIR, the successor should be in the basic block's successor list
        if (mirBlock->isCleanup) return;
        
        // For now, just create a simple goto to the next block
        if (blockCounter_ < currentFunction_->basicBlocks.size()) {
            auto targetBlock = currentFunction_->basicBlocks[blockCounter_].get();
            builder_->createGoto(targetBlock);
        } else {
            builder_->createReturn();
        }
    }
    
    void generateCondBr(hir::HIRInstruction* hirInst, MIRBasicBlock* mirBlock) {
        (void)mirBlock;
        if (hirInst->operands.empty()) return;
        
        auto condition = translateOperand(hirInst->operands[0].get());
        
        // Create true and false blocks
        auto trueBlock = builder_->createBasicBlock("bb" + std::to_string(blockCounter_++));
        auto falseBlock = builder_->createBasicBlock("bb" + std::to_string(blockCounter_++));
        
        // Create switch on boolean (1 = true, 0 = false)
        std::vector<std::pair<int64_t, MIRBasicBlock*>> targets;
        targets.push_back({1, trueBlock.get()});
        
        builder_->createSwitchInt(condition, targets, falseBlock.get());
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
