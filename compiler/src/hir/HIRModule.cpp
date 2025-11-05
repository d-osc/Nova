#include "nova/HIR/HIR.h"
#include <sstream>

namespace nova::hir {

// HIRType implementations
bool HIRType::isInteger() const {
    return kind >= Kind::I8 && kind <= Kind::USize;
}

bool HIRType::isFloat() const {
    return kind == Kind::F32 || kind == Kind::F64;
}

bool HIRType::isNumeric() const {
    return isInteger() || isFloat();
}

bool HIRType::isPrimitive() const {
    return kind >= Kind::I8 && kind <= Kind::String;
}

bool HIRType::isAggregate() const {
    return kind == Kind::Array || kind == Kind::Tuple || kind == Kind::Struct;
}

bool HIRType::isPointer() const {
    return kind == Kind::Pointer || kind == Kind::Reference;
}

std::string HIRType::toString() const {
    switch (kind) {
        case Kind::Void: return "void";
        case Kind::Never: return "never";
        case Kind::Unit: return "unit";
        case Kind::I8: return "i8";
        case Kind::I16: return "i16";
        case Kind::I32: return "i32";
        case Kind::I64: return "i64";
        case Kind::ISize: return "isize";
        case Kind::U8: return "u8";
        case Kind::U16: return "u16";
        case Kind::U32: return "u32";
        case Kind::U64: return "u64";
        case Kind::USize: return "usize";
        case Kind::F32: return "f32";
        case Kind::F64: return "f64";
        case Kind::Bool: return "bool";
        case Kind::Char: return "char";
        case Kind::String: return "string";
        case Kind::Any: return "any";
        case Kind::Unknown: return "unknown";
        default: return "type";
    }
}

std::string HIRPointerType::toString() const {
    return std::string("*") + (isMutable ? "mut " : "") + pointeeType->toString();
}

std::string HIRArrayType::toString() const {
    return "[" + elementType->toString() + "; " + std::to_string(size) + "]";
}

std::string HIRTupleType::toString() const {
    std::ostringstream oss;
    oss << "(";
    for (size_t i = 0; i < elementTypes.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << elementTypes[i]->toString();
    }
    oss << ")";
    return oss.str();
}

std::string HIRStructType::toString() const {
    return "struct " + name;
}

std::string HIRFunctionType::toString() const {
    std::ostringstream oss;
    oss << "fn(";
    for (size_t i = 0; i < paramTypes.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << paramTypes[i]->toString();
    }
    if (isVariadic) oss << ", ...";
    oss << ") -> " << returnType->toString();
    return oss.str();
}

// HIRValue implementations
std::string HIRValue::toString() const {
    return "%" + name + ": " + type->toString();
}

std::string HIRConstant::toString() const {
    std::ostringstream oss;
    switch (kind) {
        case Kind::Integer:
            oss << std::get<int64_t>(value);
            break;
        case Kind::Float:
            oss << std::get<double>(value);
            break;
        case Kind::Boolean:
            oss << (std::get<bool>(value) ? "true" : "false");
            break;
        case Kind::String:
            oss << "\"" << std::get<std::string>(value) << "\"";
            break;
        case Kind::Null:
            oss << "null";
            break;
        case Kind::Undefined:
            oss << "undefined";
            break;
    }
    return oss.str();
}

// HIRInstruction implementations
std::string HIRInstruction::toString() const {
    std::ostringstream oss;
    if (!name.empty()) {
        oss << "%" << name << " = ";
    }
    
    switch (opcode) {
        case Opcode::Add: oss << "add"; break;
        case Opcode::Sub: oss << "sub"; break;
        case Opcode::Mul: oss << "mul"; break;
        case Opcode::Div: oss << "div"; break;
        case Opcode::Rem: oss << "rem"; break;
        case Opcode::Eq: oss << "eq"; break;
        case Opcode::Ne: oss << "ne"; break;
        case Opcode::Lt: oss << "lt"; break;
        case Opcode::Le: oss << "le"; break;
        case Opcode::Gt: oss << "gt"; break;
        case Opcode::Ge: oss << "ge"; break;
        case Opcode::Alloca: oss << "alloca"; break;
        case Opcode::Load: oss << "load"; break;
        case Opcode::Store: oss << "store"; break;
        case Opcode::Br: oss << "br"; break;
        case Opcode::CondBr: oss << "br_if"; break;
        case Opcode::Return: oss << "return"; break;
        case Opcode::Call: oss << "call"; break;
        case Opcode::Cast: oss << "cast"; break;
        default: oss << "unknown"; break;
    }
    
    for (const auto& op : operands) {
        oss << " " << op->toString();
    }
    
    return oss.str();
}

// HIRBasicBlock implementations
bool HIRBasicBlock::hasTerminator() const {
    if (instructions.empty()) return false;
    auto opcode = instructions.back()->opcode;
    return opcode == HIRInstruction::Opcode::Br ||
           opcode == HIRInstruction::Opcode::CondBr ||
           opcode == HIRInstruction::Opcode::Return ||
           opcode == HIRInstruction::Opcode::Unreachable;
}

std::string HIRBasicBlock::toString() const {
    std::ostringstream oss;
    oss << label << ":\n";
    for (const auto& inst : instructions) {
        oss << "  " << inst->toString() << "\n";
    }
    return oss.str();
}

// HIRFunction implementations
HIRBasicBlockPtr HIRFunction::createBasicBlock(const std::string& label) {
    auto block = std::make_shared<HIRBasicBlock>(label);
    block->parentFunction = std::shared_ptr<HIRFunction>(this, [](HIRFunction*){});
    basicBlocks.push_back(block);
    return block;
}

std::string HIRFunction::toString() const {
    std::ostringstream oss;
    oss << "fn " << name << "(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << parameters[i]->toString();
    }
    oss << ") -> " << functionType->returnType->toString();
    
    if (isAsync) oss << " async";
    if (isGenerator) oss << " generator";
    
    oss << " {\n";
    for (const auto& bb : basicBlocks) {
        oss << bb->toString();
    }
    oss << "}\n";
    
    return oss.str();
}

// HIRModule implementations
HIRFunctionPtr HIRModule::createFunction(const std::string& funcName, HIRFunctionType* type) {
    auto func = std::make_shared<HIRFunction>(funcName, type);
    
    // Create parameters
    for (size_t i = 0; i < type->paramTypes.size(); ++i) {
        auto param = new HIRParameter(
            type->paramTypes[i],
            "arg" + std::to_string(i),
            static_cast<uint32_t>(i));
        func->parameters.push_back(param);
    }
    
    functions.push_back(func);
    return func;
}

HIRStructType* HIRModule::createStructType(const std::string& typeName) {
    auto structType = new HIRStructType(typeName, {});
    types.push_back(structType);
    return structType;
}

HIRFunctionPtr HIRModule::getFunction(const std::string& funcName) const {
    for (const auto& func : functions) {
        if (func->name == funcName) {
            return func;
        }
    }
    return nullptr;
}

HIRStructType* HIRModule::getStructType(const std::string& typeName) const {
    for (auto* type : types) {
        if (type->name == typeName) {
            return type;
        }
    }
    return nullptr;
}

void HIRModule::dump() const {
    std::cout << toString();
}

std::string HIRModule::toString() const {
    std::ostringstream oss;
    oss << "; Module: " << name << "\n\n";
    
    for (const auto& type : types) {
        oss << type->toString() << "\n";
    }
    
    for (const auto& func : functions) {
        oss << func->toString() << "\n";
    }
    
    return oss.str();
}

} // namespace nova::hir
