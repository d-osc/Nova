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

HIRInstruction* HIRBuilder::createRem(HIRValue* lhs, HIRValue* rhs, const std::string& name) {
    auto resultType = lhs->type;
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Rem, resultType, generateName(name));
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
    // Don't copy the type - it causes object slicing for derived types
    // Instead, wrap the existing type pointer in a non-owning shared_ptr
    HIRTypePtr typePtr;
    if (type) {
        // Create non-owning shared_ptr (types are managed elsewhere)
        typePtr = std::shared_ptr<HIRType>(type, [](HIRType*){});
    } else {
        typePtr = std::make_shared<HIRType>(HIRType::Kind::Any);
    }

    auto ptrType = std::make_shared<HIRPointerType>(typePtr, true);
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
                    // Check if the pointee is a pointer to array/struct
                    bool keepPointerType = false;
                    if (auto* innerPtr = dynamic_cast<HIRPointerType*>(ptrType->pointeeType.get())) {
                        if (innerPtr->pointeeType &&
                            (innerPtr->pointeeType->kind == HIRType::Kind::Array ||
                             innerPtr->pointeeType->kind == HIRType::Kind::Struct)) {
                            keepPointerType = true;
                        }
                    }

                    if (keepPointerType) {
                        resultType = ptrType->pointeeType;  // Extract one level but keep ptr-to-array/struct
                        std::cerr << "DEBUG HIR: createLoad - keeping pointer-to-array/struct type" << std::endl;
                    } else {
                        resultType = ptrType->pointeeType;
                        std::cerr << "DEBUG HIR: createLoad - extracted pointee type kind="
                                  << static_cast<int>(resultType->kind) << std::endl;
                    }
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

    std::cerr << "DEBUG HIR: createLoad - before creating instruction, resultType.get()=" << resultType.get() << std::endl;
    HIRPointerType* checkResultType = dynamic_cast<HIRPointerType*>(resultType.get());
    std::cerr << "DEBUG HIR: createLoad - resultType dynamic_cast to HIRPointerType=" << checkResultType << std::endl;

    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::Load, resultType, generateName(name));

    std::cerr << "DEBUG HIR: createLoad - final result type kind="
              << (inst->type ? static_cast<int>(inst->type->kind) : -1) << std::endl;
    std::cerr << "DEBUG HIR: createLoad - inst->type.get()=" << inst->type.get() << std::endl;

    // Check if it's actually an HIRPointerType
    HIRPointerType* checkPtr = dynamic_cast<HIRPointerType*>(inst->type.get());
    std::cerr << "DEBUG HIR: createLoad - dynamic_cast to HIRPointerType=" << checkPtr << std::endl;

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
    // Determine the result type from the struct type
    HIRTypePtr resultType = std::make_shared<HIRType>(HIRType::Kind::Any);

    if (struct_ && struct_->type) {
        // Check if it's a pointer to struct
        if (auto ptrType = dynamic_cast<HIRPointerType*>(struct_->type.get())) {
            if (auto structType = dynamic_cast<HIRStructType*>(ptrType->pointeeType.get())) {
                if (fieldIndex < structType->fields.size()) {
                    resultType = structType->fields[fieldIndex].type;
                }
            }
        }
    }

    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::GetField, resultType, generateName(name));
    inst->addOperand(std::shared_ptr<HIRValue>(struct_, [](HIRValue*){}));

    // Store the field index as a constant operand
    auto indexConstant = createIntConstant(fieldIndex);
    inst->addOperand(std::shared_ptr<HIRValue>(indexConstant, [](HIRValue*){}));

    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createSetField(HIRValue* struct_, uint32_t fieldIndex, HIRValue* value, const std::string& name) {
    auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::SetField, voidType, generateName(name));
    inst->addOperand(std::shared_ptr<HIRValue>(struct_, [](HIRValue*){}));

    // Add field index as a constant
    auto indexConstant = createIntConstant(fieldIndex);
    inst->addOperand(std::shared_ptr<HIRValue>(indexConstant, [](HIRValue*){}));

    // Add value to store
    inst->addOperand(std::shared_ptr<HIRValue>(value, [](HIRValue*){}));

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

HIRInstruction* HIRBuilder::createSetElement(HIRValue* array, HIRValue* index, HIRValue* value) {
    auto voidType = std::make_shared<HIRType>(HIRType::Kind::Void);
    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::SetElement, voidType, "");
    inst->addOperand(std::shared_ptr<HIRValue>(array, [](HIRValue*){}));
    inst->addOperand(std::shared_ptr<HIRValue>(index, [](HIRValue*){}));
    inst->addOperand(std::shared_ptr<HIRValue>(value, [](HIRValue*){}));

    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createArrayConstruct(const std::vector<HIRValue*>& elements, const std::string& name) {
    // Create array type based on elements
    auto elementType = std::make_shared<HIRType>(HIRType::Kind::I64);  // For now, assume i64 arrays
    auto arrayType = std::make_shared<HIRArrayType>(elementType, elements.size());

    // ArrayConstruct returns a pointer to the array, not the array itself
    auto ptrToArray = std::make_shared<HIRPointerType>(arrayType, true);

    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::ArrayConstruct, ptrToArray, generateName(name));

    // Add all elements as operands
    for (auto* elem : elements) {
        inst->addOperand(std::shared_ptr<HIRValue>(elem, [](HIRValue*){}));
    }

    if (currentBlock_) {
        currentBlock_->addInstruction(inst);
    }
    return inst.get();
}

HIRInstruction* HIRBuilder::createStructConstruct(HIRStructType* structType, const std::vector<HIRValue*>& fieldValues, const std::string& name) {
    // Create a pointer-to-struct type (struct construction returns a pointer)
    auto ptrToStruct = std::make_shared<HIRPointerType>(
        std::shared_ptr<HIRStructType>(structType, [](HIRStructType*){}),
        true
    );

    auto inst = std::make_shared<HIRInstruction>(
        HIRInstruction::Opcode::StructConstruct, ptrToStruct, generateName(name));

    // Add all field values as operands
    for (auto* fieldValue : fieldValues) {
        inst->addOperand(std::shared_ptr<HIRValue>(fieldValue, [](HIRValue*){}));
    }

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
