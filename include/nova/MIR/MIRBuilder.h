#pragma once

#include "nova/MIR/MIR.h"

namespace nova::mir {

// ==================== MIRBuilder - Helper Class for Building MIR ====================

class MIRBuilder {
private:
    MIRFunction* currentFunction_;
    MIRBasicBlock* currentBlock_;
    uint32_t tempCounter_;
    
public:
    explicit MIRBuilder(MIRFunction* func);
    
    void setInsertPoint(MIRBasicBlock* block);
    MIRBasicBlock* getInsertBlock() const;
    
    // ==================== Operand Creation ====================
    
    MIROperandPtr createCopyOperand(MIRPlacePtr place);
    MIROperandPtr createMoveOperand(MIRPlacePtr place);
    MIROperandPtr createIntConstant(int64_t value, MIRTypePtr type);
    MIROperandPtr createFloatConstant(double value, MIRTypePtr type);
    MIROperandPtr createBoolConstant(bool value, MIRTypePtr type);
    MIROperandPtr createStringConstant(const std::string& value, MIRTypePtr type);
    MIROperandPtr createNullConstant(MIRTypePtr type);
    MIROperandPtr createZeroInitConstant(MIRTypePtr type);
    
    // ==================== RValue Creation ====================
    
    MIRRValuePtr createUse(MIROperandPtr operand);
    MIRRValuePtr createBinaryOp(MIRBinaryOpRValue::BinOp op, MIROperandPtr lhs, MIROperandPtr rhs);
    
    MIRRValuePtr createAdd(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createSub(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createMul(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createDiv(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createRem(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createPow(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createBitAnd(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createBitOr(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createBitXor(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createShl(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createShr(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createUShr(MIROperandPtr lhs, MIROperandPtr rhs);

    MIRRValuePtr createEq(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createNe(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createLt(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createLe(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createGt(MIROperandPtr lhs, MIROperandPtr rhs);
    MIRRValuePtr createGe(MIROperandPtr lhs, MIROperandPtr rhs);
    
    MIRRValuePtr createUnaryOp(MIRUnaryOpRValue::UnOp op, MIROperandPtr operand);
    MIRRValuePtr createNot(MIROperandPtr operand);
    MIRRValuePtr createNeg(MIROperandPtr operand);
    
    MIRRValuePtr createCast(MIRCastRValue::CastKind kind, MIROperandPtr operand, MIRTypePtr targetType);
    
    // ==================== Statement Creation ====================
    
    void createAssign(MIRPlacePtr place, MIRRValuePtr rvalue);
    void createStorageLive(MIRPlacePtr place);
    void createStorageDead(MIRPlacePtr place);
    
    // ==================== Terminator Creation ====================
    
    void createReturn();
    void createGoto(MIRBasicBlock* target);
    void createSwitchInt(MIROperandPtr discriminant,
                        const std::vector<std::pair<int64_t, MIRBasicBlock*>>& targets,
                        MIRBasicBlock* otherwise);
    void createCall(MIROperandPtr func, const std::vector<MIROperandPtr>& args,
                   MIRPlacePtr destination, MIRBasicBlock* target,
                   MIRBasicBlock* unwind = nullptr);
    
    // ==================== Place and Type Creation ====================
    
    MIRPlacePtr createLocal(MIRTypePtr type, const std::string& name = "");
    MIRPlacePtr createTemp(MIRTypePtr type);
    MIRBasicBlockPtr createBasicBlock(const std::string& label);
    
    // ==================== Type Creation ====================
    
    static MIRTypePtr getVoidType();
    static MIRTypePtr getBoolType();
    static MIRTypePtr getI8Type();
    static MIRTypePtr getI16Type();
    static MIRTypePtr getI32Type();
    static MIRTypePtr getI64Type();
    static MIRTypePtr getISizeType();
    static MIRTypePtr getU8Type();
    static MIRTypePtr getU16Type();
    static MIRTypePtr getU32Type();
    static MIRTypePtr getU64Type();
    static MIRTypePtr getUSizeType();
    static MIRTypePtr getF32Type();
    static MIRTypePtr getF64Type();
    static MIRTypePtr getPointerType();
};

} // namespace nova::mir
