#include "nova/HIR/HIR.h"
#include <sstream>

namespace nova::hir {

std::string HIRBuilder::generateName(const std::string& hint) {
    std::ostringstream oss;
    if (!hint.empty()) {
        oss << hint << "." << nextValueId_++;
    } else {
        oss << "t" << nextValueId_++;
    }
    return oss.str();
}

// Arithmetic Operations
HIRInstruction* HIRBuilder::createAdd(HIRValue* lhs, HIRValue* rhs, const std::string& name) {
    // Debug: Check input types
    std::cerr << "DEBUG HIR: createAdd - lhs type kind="
              << (lhs && lhs->type ? static_cast<int>(lhs->type->kind) : -1)
              << ", rhs type kind="
              << (rhs && rhs->type ? static_cast<int>(rhs->type->kind) : -1) << std::endl;

    auto resultType = lhs->type;
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Add, resultType, generateName(name));

    // Debug: Check result type
    std::cerr << "DEBUG HIR: createAdd - result type kind="
              << (inst->type ? static_cast<int>(inst->type->kind) : -1) << std::endl;

    inst->addOperand(std::shared_ptr<HIRValue>(lhs, [](HIRValue*){}));
    inst->addOperand(std::shared_ptr<HIRValue>(rhs, [](HIRValue*){}));

    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createSub(HIRValue* lhs, HIRValue* rhs, const std::string& name) {
    auto resultType = lhs->type;
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Sub, resultType, generateName(name));
    inst->addOperand(std::shared_ptr<HIRValue>(lhs, [](HIRValue*){}));
    inst->addOperand(std::shared_ptr<HIRValue>(rhs, [](HIRValue*){}));
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createMul(HIRValue* lhs, HIRValue* rhs, const std::string& name) {
    auto resultType = lhs->type;
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Mul, resultType, generateName(name));
    inst->addOperand(std::shared_ptr<HIRValue>(lhs, [](HIRValue*){}));
    inst->addOperand(std::shared_ptr<HIRValue>(rhs, [](HIRValue*){}));
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createDiv(HIRValue* lhs, HIRValue* rhs, const std::string& name) {
    auto resultType = lhs->type;
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Div, resultType, generateName(name));
    inst->addOperand(std::shared_ptr<HIRValue>(lhs, [](HIRValue*){}));
    inst->addOperand(std::shared_ptr<HIRValue>(rhs, [](HIRValue*){}));
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

// Comparison Operations
HIRInstruction* HIRBuilder::createEq(HIRValue* lhs, HIRValue* rhs, const std::string& name) {
    auto boolType = std::make_shared<HIRType>(HIRType::Kind::Bool);
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Eq, boolType, generateName(name));
    inst->addOperand(std::shared_ptr<HIRValue>(lhs, [](HIRValue*){}));
    inst->addOperand(std::shared_ptr<HIRValue>(rhs, [](HIRValue*){}));
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createNe(HIRValue* lhs, HIRValue* rhs, const std::string& name) {
    auto boolType = std::make_shared<HIRType>(HIRType::Kind::Bool);
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Ne, boolType, generateName(name));
    inst->addOperand(std::shared_ptr<HIRValue>(lhs, [](HIRValue*){}));
    inst->addOperand(std::shared_ptr<HIRValue>(rhs, [](HIRValue*){}));
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createLt(HIRValue* lhs, HIRValue* rhs, const std::string& name) {
    auto boolType = std::make_shared<HIRType>(HIRType::Kind::Bool);
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Lt, boolType, generateName(name));
    inst->addOperand(std::shared_ptr<HIRValue>(lhs, [](HIRValue*){}));
    inst->addOperand(std::shared_ptr<HIRValue>(rhs, [](HIRValue*){}));
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createLe(HIRValue* lhs, HIRValue* rhs, const std::string& name) {
    auto boolType = std::make_shared<HIRType>(HIRType::Kind::Bool);
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Le, boolType, generateName(name));
    inst->addOperand(std::shared_ptr<HIRValue>(lhs, [](HIRValue*){}));
    inst->addOperand(std::shared_ptr<HIRValue>(rhs, [](HIRValue*){}));
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createGt(HIRValue* lhs, HIRValue* rhs, const std::string& name) {
    auto boolType = std::make_shared<HIRType>(HIRType::Kind::Bool);
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Gt, boolType, generateName(name));
    inst->addOperand(std::shared_ptr<HIRValue>(lhs, [](HIRValue*){}));
    inst->addOperand(std::shared_ptr<HIRValue>(rhs, [](HIRValue*){}));
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createGe(HIRValue* lhs, HIRValue* rhs, const std::string& name) {
    auto boolType = std::make_shared<HIRType>(HIRType::Kind::Bool);
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Ge, boolType, generateName(name));
    inst->addOperand(std::shared_ptr<HIRValue>(lhs, [](HIRValue*){}));
    inst->addOperand(std::shared_ptr<HIRValue>(rhs, [](HIRValue*){}));
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

// Memory Operations
HIRInstruction* HIRBuilder::createAlloca(HIRType* type, const std::string& name) {
    // Create a properly owned copy of the type to avoid dangling pointers
    HIRTypePtr ownedType;
    if (type) {
        ownedType = std::make_shared<HIRType>(*type);  // Copy the type
    } else {
        ownedType = std::make_shared<HIRType>(HIRType::Kind::Any);
    }

    auto ptrType = std::make_shared<HIRPointerType>(ownedType, true);
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Alloca, ptrType, generateName(name));

    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createLoad(HIRValue* ptr, const std::string& name) {
    // Extract pointee type from pointer
    HIRTypePtr resultType = std::make_shared<HIRType>(HIRType::Kind::Any);

    std::cerr << "DEBUG HIR: createLoad - ptr=" << ptr
              << ", ptr->type=" << (ptr ? ptr->type.get() : nullptr) << std::endl;

    try {
        if (ptr && ptr->type) {
            std::cerr << "DEBUG HIR: createLoad - ptr type kind="
                      << static_cast<int>(ptr->type->kind) << std::endl;
            if (auto* ptrType = dynamic_cast<HIRPointerType*>(ptr->type.get())) {
                if (ptrType && ptrType->pointeeType) {
                    resultType = ptrType->pointeeType;
                    std::cerr << "DEBUG HIR: createLoad - extracted pointee type kind="
                              << static_cast<int>(resultType->kind) << std::endl;
                } else {
                    std::cerr << "DEBUG HIR: createLoad - pointeeType is null" << std::endl;
                }
            } else {
                std::cerr << "DEBUG HIR: createLoad - not a pointer type, using default Any" << std::endl;
            }
        }
    } catch (...) {
        // If cast fails, use Any type
        std::cerr << "DEBUG HIR: createLoad - exception during type extraction" << std::endl;
    }

    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Load, resultType, generateName(name));

    std::cerr << "DEBUG HIR: createLoad - final result type kind="
              << (inst->type ? static_cast<int>(inst->type->kind) : -1) << std::endl;

    inst->addOperand(std::shared_ptr<HIRValue>(ptr, [](HIRValue*){}));

    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createStore(HIRValue* value, HIRValue* ptr) {
    auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Store, voidType, "");
    inst->addOperand(std::shared_ptr<HIRValue>(value, [](HIRValue*){}));
    inst->addOperand(std::shared_ptr<HIRValue>(ptr, [](HIRValue*){}));
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

// Control Flow
HIRInstruction* HIRBuilder::createBr(HIRBasicBlock* dest) {
    auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Br, voidType, "");
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
        currentBlock_->successors.push_back(
            std::shared_ptr<HIRBasicBlock>(dest, [](HIRBasicBlock*){}));
        dest->predecessors.push_back(
            std::shared_ptr<HIRBasicBlock>(currentBlock_, [](HIRBasicBlock*){}));
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createCondBr(HIRValue* cond, HIRBasicBlock* thenBlock, HIRBasicBlock* elseBlock) {
    auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::CondBr, voidType, "");
    inst->addOperand(std::shared_ptr<HIRValue>(cond, [](HIRValue*){}));
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
        currentBlock_->successors.push_back(
            std::shared_ptr<HIRBasicBlock>(thenBlock, [](HIRBasicBlock*){}));
        currentBlock_->successors.push_back(
            std::shared_ptr<HIRBasicBlock>(elseBlock, [](HIRBasicBlock*){}));
        thenBlock->predecessors.push_back(
            std::shared_ptr<HIRBasicBlock>(currentBlock_, [](HIRBasicBlock*){}));
        elseBlock->predecessors.push_back(
            std::shared_ptr<HIRBasicBlock>(currentBlock_, [](HIRBasicBlock*){}));
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createReturn(HIRValue* value) {
    auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Return, voidType, "");
    if (value) {
        inst->addOperand(std::shared_ptr<HIRValue>(value, [](HIRValue*){}));
    }
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

// Function Calls
HIRInstruction* HIRBuilder::createCall(HIRFunction* callee, const std::vector<HIRValue*>& args, const std::string& name) {
    auto resultType = callee->functionType->returnType;
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Call, resultType, generateName(name));
    
    // Add callee function name as first operand (string constant)
    // Create a temporary string constant for the function name
    auto funcNameType = std::make_shared<HIRType>(HIRType::Kind::I64); // Placeholder type
    auto funcNameConst = std::make_shared<HIRConstant>(
        funcNameType, HIRConstant::Kind::String, callee->name);
    inst->addOperand(funcNameConst);
    
    // Add arguments as remaining operands
    for (auto* arg : args) {
        inst->addOperand(std::shared_ptr<HIRValue>(arg, [](HIRValue*){}));
    }
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

// Type Conversions
HIRInstruction* HIRBuilder::createCast(HIRValue* value, HIRType* destType, const std::string& name) {
    // Create a properly owned copy of the type
    HIRTypePtr ownedDestType;
    if (destType) {
        ownedDestType = std::make_shared<HIRType>(*destType);
    } else {
        ownedDestType = std::make_shared<HIRType>(HIRType::Kind::Any);
    }

    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Cast, ownedDestType, generateName(name));
    inst->addOperand(std::shared_ptr<HIRValue>(value, [](HIRValue*){}));

    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

// Aggregate Operations
HIRInstruction* HIRBuilder::createGetField(HIRValue* struct_, uint32_t fieldIndex, const std::string& name) {
    (void)fieldIndex;
    auto anyType = std::make_shared<HIRType>(HIRType::Kind::Any);
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::GetField, anyType, generateName(name));
    inst->addOperand(std::shared_ptr<HIRValue>(struct_, [](HIRValue*){}));
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createGetElement(HIRValue* array, HIRValue* index, const std::string& name) {
    auto anyType = std::make_shared<HIRType>(HIRType::Kind::Any);
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::GetElement, anyType, generateName(name));
    inst->addOperand(std::shared_ptr<HIRValue>(array, [](HIRValue*){}));
    inst->addOperand(std::shared_ptr<HIRValue>(index, [](HIRValue*){}));
    
    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

// Constants
HIRConstant* HIRBuilder::createIntConstant(int64_t value, uint32_t bitWidth) {
    (void)bitWidth;
    auto intType = std::make_shared<HIRType>(HIRType::Kind::I64);
    return new HIRConstant(intType, HIRConstant::Kind::Integer, value);
}

HIRConstant* HIRBuilder::createFloatConstant(double value) {
    auto floatType = std::make_shared<HIRType>(HIRType::Kind::F64);
    return new HIRConstant(floatType, HIRConstant::Kind::Float, value);
}

HIRConstant* HIRBuilder::createBoolConstant(bool value) {
    auto boolType = std::make_shared<HIRType>(HIRType::Kind::Bool);
    return new HIRConstant(boolType, HIRConstant::Kind::Boolean, value);
}

HIRConstant* HIRBuilder::createStringConstant(const std::string& value) {
    auto strType = std::make_shared<HIRType>(HIRType::Kind::String);
    return new HIRConstant(strType, HIRConstant::Kind::String, value);
}

HIRConstant* HIRBuilder::createNullConstant(HIRType* type) {
    // Create a properly owned copy of the type
    HIRTypePtr ownedType;
    if (type) {
        ownedType = std::make_shared<HIRType>(*type);
    } else {
        ownedType = std::make_shared<HIRType>(HIRType::Kind::Any);
    }

    return new HIRConstant(ownedType, HIRConstant::Kind::Null, 0);
}

} // namespace nova::hir
