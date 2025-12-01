#include "nova/MIR/MIRBuilder.h"

namespace nova::mir {

// ==================== MIRBuilder Implementation ====================

MIRBuilder::MIRBuilder(MIRFunction* func)
    : currentFunction_(func), currentBlock_(nullptr), tempCounter_(0) {}

void MIRBuilder::setInsertPoint(MIRBasicBlock* block) {
    currentBlock_ = block;
}

MIRBasicBlock* MIRBuilder::getInsertBlock() const {
    return currentBlock_;
}
    
// ==================== Operand Creation ====================

MIROperandPtr MIRBuilder::createCopyOperand(MIRPlacePtr place) {
    return std::make_shared<MIRCopyOperand>(place);
}

MIROperandPtr MIRBuilder::createMoveOperand(MIRPlacePtr place) {
    return std::make_shared<MIRMoveOperand>(place);
}

MIROperandPtr MIRBuilder::createIntConstant(int64_t value, MIRTypePtr type) {
    return std::make_shared<MIRConstOperand>(
        MIRConstOperand::ConstKind::Int, value, type);
}

MIROperandPtr MIRBuilder::createFloatConstant(double value, MIRTypePtr type) {
    return std::make_shared<MIRConstOperand>(
        MIRConstOperand::ConstKind::Float, value, type);
}

MIROperandPtr MIRBuilder::createBoolConstant(bool value, MIRTypePtr type) {
    return std::make_shared<MIRConstOperand>(
        MIRConstOperand::ConstKind::Bool, value, type);
}

MIROperandPtr MIRBuilder::createStringConstant(const std::string& value, MIRTypePtr type) {
    return std::make_shared<MIRConstOperand>(
        MIRConstOperand::ConstKind::String, value, type);
}

MIROperandPtr MIRBuilder::createNullConstant(MIRTypePtr type) {
    return std::make_shared<MIRConstOperand>(
        MIRConstOperand::ConstKind::Null, 0, type);
}

MIROperandPtr MIRBuilder::createZeroInitConstant(MIRTypePtr type) {
    return std::make_shared<MIRConstOperand>(
        MIRConstOperand::ConstKind::ZeroInit, 0, type);
}

// ==================== RValue Creation ====================

MIRRValuePtr MIRBuilder::createUse(MIROperandPtr operand) {
    return std::make_shared<MIRUseRValue>(operand);
}

MIRRValuePtr MIRBuilder::createBinaryOp(MIRBinaryOpRValue::BinOp op,
                            MIROperandPtr lhs, MIROperandPtr rhs) {
    return std::make_shared<MIRBinaryOpRValue>(op, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createAdd(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::Add, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createSub(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::Sub, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createMul(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::Mul, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createDiv(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::Div, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createRem(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::Rem, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createPow(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::Pow, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createBitAnd(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::BitAnd, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createBitOr(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::BitOr, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createBitXor(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::BitXor, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createShl(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::Shl, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createShr(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::Shr, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createUShr(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::UShr, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createEq(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::Eq, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createNe(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::Ne, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createLt(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::Lt, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createLe(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::Le, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createGt(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::Gt, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createGe(MIROperandPtr lhs, MIROperandPtr rhs) {
    return createBinaryOp(MIRBinaryOpRValue::BinOp::Ge, lhs, rhs);
}

MIRRValuePtr MIRBuilder::createUnaryOp(MIRUnaryOpRValue::UnOp op, MIROperandPtr operand) {
    return std::make_shared<MIRUnaryOpRValue>(op, operand);
}

MIRRValuePtr MIRBuilder::createNot(MIROperandPtr operand) {
    return createUnaryOp(MIRUnaryOpRValue::UnOp::Not, operand);
}

MIRRValuePtr MIRBuilder::createNeg(MIROperandPtr operand) {
    return createUnaryOp(MIRUnaryOpRValue::UnOp::Neg, operand);
}

MIRRValuePtr MIRBuilder::createCast(MIRCastRValue::CastKind kind,
                       MIROperandPtr operand, MIRTypePtr targetType) {
    return std::make_shared<MIRCastRValue>(kind, operand, targetType);
}

// ==================== Statement Creation ====================

void MIRBuilder::createAssign(MIRPlacePtr place, MIRRValuePtr rvalue) {
    if (!currentBlock_) return;
    auto stmt = std::make_shared<MIRAssignStatement>(place, rvalue);
    currentBlock_->addStatement(stmt);
}

void MIRBuilder::createStorageLive(MIRPlacePtr place) {
    if (!currentBlock_) return;
    auto stmt = std::make_shared<MIRStorageLiveStatement>(place);
    currentBlock_->addStatement(stmt);
}

void MIRBuilder::createStorageDead(MIRPlacePtr place) {
    if (!currentBlock_) return;
    auto stmt = std::make_shared<MIRStorageDeadStatement>(place);
    currentBlock_->addStatement(stmt);
}

// ==================== Terminator Creation ====================

void MIRBuilder::createReturn() {
    if (!currentBlock_) return;
    auto term = std::make_shared<MIRReturnTerminator>();
    currentBlock_->setTerminator(term);
}

void MIRBuilder::createGoto(MIRBasicBlock* target) {
    if (!currentBlock_) return;
    auto term = std::make_shared<MIRGotoTerminator>(target);
    currentBlock_->setTerminator(term);
}



void MIRBuilder::createSwitchInt(MIROperandPtr discriminant,
                    const std::vector<std::pair<int64_t, MIRBasicBlock*>>& targets,
                    MIRBasicBlock* otherwise) {
    if (!currentBlock_) return;
    auto term = std::make_shared<MIRSwitchIntTerminator>(discriminant, otherwise);
    for (const auto& [value, target] : targets) {
        term->addTarget(value, target);
    }
    currentBlock_->setTerminator(term);
}

void MIRBuilder::createCall(MIROperandPtr func, const std::vector<MIROperandPtr>& args,
               MIRPlacePtr destination, MIRBasicBlock* target,
               MIRBasicBlock* unwind) {
    if (!currentBlock_) return;
    auto term = std::make_shared<MIRCallTerminator>(func, args, destination, target, unwind);
    currentBlock_->setTerminator(term);
}

// ==================== Place and Type Creation ====================

MIRPlacePtr MIRBuilder::createLocal(MIRTypePtr type, const std::string& name) {
    return currentFunction_->createLocal(type, name);
}

MIRPlacePtr MIRBuilder::createTemp(MIRTypePtr type) {
    return currentFunction_->createTemp(type);
}

MIRBasicBlockPtr MIRBuilder::createBasicBlock(const std::string& label) {
    return currentFunction_->createBasicBlock(label);
}

// ==================== Type Creation ====================

MIRTypePtr MIRBuilder::getVoidType() {
    static auto type = std::make_shared<MIRType>(MIRType::Kind::Void);
    return type;
}

MIRTypePtr MIRBuilder::getBoolType() {
    static auto type = std::make_shared<MIRType>(MIRType::Kind::I1);
    return type;
}

MIRTypePtr MIRBuilder::getI8Type() {
    static auto type = std::make_shared<MIRType>(MIRType::Kind::I8);
    return type;
}

MIRTypePtr MIRBuilder::getI16Type() {
    static auto type = std::make_shared<MIRType>(MIRType::Kind::I16);
    return type;
}

MIRTypePtr MIRBuilder::getI32Type() {
    static auto type = std::make_shared<MIRType>(MIRType::Kind::I32);
    return type;
}

MIRTypePtr MIRBuilder::getI64Type() {
    static auto type = std::make_shared<MIRType>(MIRType::Kind::I64);
    return type;
}

MIRTypePtr MIRBuilder::getISizeType() {
    static auto type = std::make_shared<MIRType>(MIRType::Kind::ISize);
    return type;
}

MIRTypePtr MIRBuilder::getU8Type() {
    static auto type = std::make_shared<MIRType>(MIRType::Kind::U8);
    return type;
}

MIRTypePtr MIRBuilder::getU16Type() {
    static auto type = std::make_shared<MIRType>(MIRType::Kind::U16);
    return type;
}

MIRTypePtr MIRBuilder::getU32Type() {
    static auto type = std::make_shared<MIRType>(MIRType::Kind::U32);
    return type;
}

MIRTypePtr MIRBuilder::getU64Type() {
    static auto type = std::make_shared<MIRType>(MIRType::Kind::U64);
    return type;
}

MIRTypePtr MIRBuilder::getUSizeType() {
    static auto type = std::make_shared<MIRType>(MIRType::Kind::USize);
    return type;
}

MIRTypePtr MIRBuilder::getF32Type() {
    static auto type = std::make_shared<MIRType>(MIRType::Kind::F32);
    return type;
}

MIRTypePtr MIRBuilder::getF64Type() {
    static auto type = std::make_shared<MIRType>(MIRType::Kind::F64);
    return type;
}

MIRTypePtr MIRBuilder::getPointerType() {
    static auto type = std::make_shared<MIRType>(MIRType::Kind::Pointer);
    return type;
}

} // namespace nova::mir
