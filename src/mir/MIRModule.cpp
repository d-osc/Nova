#include "nova/MIR/MIR.h"
#include <sstream>
#include <algorithm>

namespace nova::mir {

// ==================== MIRType Implementation ====================

std::string MIRType::toString() const {
    switch (kind) {
        case Kind::Void: return "()";
        case Kind::I1: return "bool";
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
        case Kind::Pointer: return "*const";
        case Kind::Struct: return "struct";
        case Kind::Array: return "array";
        case Kind::Function: return "fn";
        default: return "unknown";
    }
}

// ==================== MIRPlace Implementation ====================

std::string MIRPlace::toString() const {
    std::string prefix;
    switch (kind) {
        case Kind::Local: prefix = "_"; break;
        case Kind::Static: prefix = "@"; break;
        case Kind::Temp: prefix = "tmp_"; break;
        case Kind::Return: return "_0";
        case Kind::Argument: prefix = "arg_"; break;
    }
    
    if (!name.empty()) {
        return prefix + name;
    }
    return prefix + std::to_string(index);
}

// ==================== MIROperand Implementation ====================

std::string MIRCopyOperand::toString() const {
    return "copy " + place->toString();
}

std::string MIRMoveOperand::toString() const {
    return "move " + place->toString();
}

std::string MIRConstOperand::toString() const {
    std::ostringstream oss;
    oss << "const ";
    
    switch (constKind) {
        case ConstKind::Int:
            oss << std::get<int64_t>(value);
            break;
        case ConstKind::Float:
            oss << std::get<double>(value);
            break;
        case ConstKind::Bool:
            oss << (std::get<bool>(value) ? "true" : "false");
            break;
        case ConstKind::String:
            oss << "\"" << std::get<std::string>(value) << "\"";
            break;
        case ConstKind::Null:
            oss << "null";
            break;
        case ConstKind::ZeroInit:
            oss << "ZeroInit";
            break;
    }
    
    return oss.str();
}

// ==================== MIRRValue Implementation ====================

std::string MIRUseRValue::toString() const {
    return "Use(" + operand->toString() + ")";
}

std::string MIRBinaryOpRValue::toString() const {
    std::string opStr;
    switch (op) {
        case BinOp::Add: opStr = "Add"; break;
        case BinOp::Sub: opStr = "Sub"; break;
        case BinOp::Mul: opStr = "Mul"; break;
        case BinOp::Div: opStr = "Div"; break;
        case BinOp::Rem: opStr = "Rem"; break;
        case BinOp::BitAnd: opStr = "BitAnd"; break;
        case BinOp::BitOr: opStr = "BitOr"; break;
        case BinOp::BitXor: opStr = "BitXor"; break;
        case BinOp::Shl: opStr = "Shl"; break;
        case BinOp::Shr: opStr = "Shr"; break;
        case BinOp::UShr: opStr = "UShr"; break;
        case BinOp::Pow: opStr = "Pow"; break;
        case BinOp::Eq: opStr = "Eq"; break;
        case BinOp::Lt: opStr = "Lt"; break;
        case BinOp::Le: opStr = "Le"; break;
        case BinOp::Ne: opStr = "Ne"; break;
        case BinOp::Ge: opStr = "Ge"; break;
        case BinOp::Gt: opStr = "Gt"; break;
        case BinOp::Offset: opStr = "Offset"; break;
    }
    return "BinaryOp(" + opStr + ", " + lhs->toString() + ", " + rhs->toString() + ")";
}

std::string MIRUnaryOpRValue::toString() const {
    std::string opStr = (op == UnOp::Not) ? "Not" : "Neg";
    return "UnaryOp(" + opStr + ", " + operand->toString() + ")";
}

std::string MIRCastRValue::toString() const {
    std::string castStr;
    switch (castKind) {
        case CastKind::IntToInt: castStr = "IntToInt"; break;
        case CastKind::FloatToInt: castStr = "FloatToInt"; break;
        case CastKind::IntToFloat: castStr = "IntToFloat"; break;
        case CastKind::FloatToFloat: castStr = "FloatToFloat"; break;
        case CastKind::PtrToPtr: castStr = "PtrToPtr"; break;
        case CastKind::Bitcast: castStr = "Bitcast"; break;
        case CastKind::Unsize: castStr = "Unsize"; break;
    }
    return "Cast(" + castStr + ", " + operand->toString() + ", " + targetType->toString() + ")";
}

std::string MIRAggregateRValue::toString() const {
    std::string kindStr;
    switch (aggregateKind) {
        case AggregateKind::Array: kindStr = "Array"; break;
        case AggregateKind::Tuple: kindStr = "Tuple"; break;
        case AggregateKind::Struct: kindStr = "Struct"; break;
    }
    std::string result = "Aggregate(" + kindStr + ", [";
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i > 0) result += ", ";
        result += elements[i]->toString();
    }
    result += "])";
    return result;
}

std::string MIRGetElementRValue::toString() const {
    return "GetElement(" + array->toString() + ", " + index->toString() + ")";
}

// ==================== MIRStatement Implementation ====================

std::string MIRAssignStatement::toString() const {
    return place->toString() + " = " + rvalue->toString();
}

std::string MIRStorageLiveStatement::toString() const {
    return "StorageLive(" + place->toString() + ")";
}

std::string MIRStorageDeadStatement::toString() const {
    return "StorageDead(" + place->toString() + ")";
}

// ==================== MIRTerminator Implementation ====================

std::string MIRReturnTerminator::toString() const {
    return "return";
}

std::string MIRGotoTerminator::toString() const {
    return "goto -> " + target->label;
}

std::vector<MIRBasicBlock*> MIRGotoTerminator::getSuccessors() const {
    return {target};
}

std::string MIRSwitchIntTerminator::toString() const {
    std::ostringstream oss;
    oss << "switchInt(" << discriminant->toString() << ") {";
    for (const auto& [value, target] : targets) {
        oss << "\n        " << value << " => " << target->label << ",";
    }
    if (otherwise) {
        oss << "\n        otherwise => " << otherwise->label;
    }
    oss << "\n    }";
    return oss.str();
}

std::vector<MIRBasicBlock*> MIRSwitchIntTerminator::getSuccessors() const {
    std::vector<MIRBasicBlock*> succs;
    for (const auto& [value, target] : targets) {
        succs.push_back(target);
    }
    if (otherwise) {
        succs.push_back(otherwise);
    }
    return succs;
}

std::string MIRCallTerminator::toString() const {
    std::ostringstream oss;
    oss << destination->toString() << " = call " << func->toString() << "(";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << args[i]->toString();
    }
    oss << ") -> [return: " << target->label;
    if (unwind) {
        oss << ", unwind: " << unwind->label;
    }
    oss << "]";
    return oss.str();
}

std::vector<MIRBasicBlock*> MIRCallTerminator::getSuccessors() const {
    std::vector<MIRBasicBlock*> succs = {target};
    if (unwind) {
        succs.push_back(unwind);
    }
    return succs;
}

// ==================== MIRBasicBlock Implementation ====================

std::string MIRBasicBlock::toString() const {
    std::ostringstream oss;
    oss << label;
    if (isCleanup) {
        oss << " (cleanup)";
    }
    oss << ":\n";
    
    for (const auto& stmt : statements) {
        oss << "    " << stmt->toString() << ";\n";
    }
    
    if (terminator) {
        oss << "    " << terminator->toString() << ";\n";
    }
    
    return oss.str();
}

// ==================== MIRFunction Implementation ====================

MIRBasicBlockPtr MIRFunction::createBasicBlock(const std::string& label) {
    auto bb = std::make_shared<MIRBasicBlock>(label);
    basicBlocks.push_back(bb);
    return bb;
}

MIRPlacePtr MIRFunction::createLocal(MIRTypePtr type, const std::string& localName) {
    uint32_t index = static_cast<uint32_t>(locals.size()) + static_cast<uint32_t>(arguments.size()) + 1;
    auto place = std::make_shared<MIRPlace>(MIRPlace::Kind::Local, index, type, localName);
    locals.push_back(place);
    localDecls.push_back({place, true, localName});
    return place;
}

MIRPlacePtr MIRFunction::createTemp(MIRTypePtr type) {
    uint32_t index = static_cast<uint32_t>(locals.size()) + static_cast<uint32_t>(arguments.size()) + 1;
    auto place = std::make_shared<MIRPlace>(MIRPlace::Kind::Temp, index, type);
    locals.push_back(place);
    return place;
}

std::string MIRFunction::toString() const {
    std::ostringstream oss;
    oss << "fn " << name << "(";
    
    // Print arguments
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << arguments[i]->toString() << ": " << arguments[i]->type->toString();
    }
    
    oss << ") -> " << returnType->toString() << " {\n";
    
    // Print local declarations
    if (!localDecls.empty()) {
        oss << "    // Local declarations\n";
        for (const auto& [place, isMutable, debugName] : localDecls) {
            oss << "    let ";
            if (isMutable) oss << "mut ";
            oss << place->toString() << ": " << place->type->toString();
            if (!debugName.empty()) {
                oss << " // " << debugName;
            }
            oss << ";\n";
        }
        oss << "\n";
    }
    
    // Print basic blocks
    for (const auto& bb : basicBlocks) {
        oss << "    " << bb->toString() << "\n";
    }
    
    oss << "}\n";
    return oss.str();
}

std::unordered_map<MIRBasicBlock*, std::unordered_set<MIRBasicBlock*>> 
MIRFunction::computeCFG() const {
    std::unordered_map<MIRBasicBlock*, std::unordered_set<MIRBasicBlock*>> cfg;
    
    for (const auto& bb : basicBlocks) {
        auto succs = bb->getSuccessors();
        cfg[bb.get()] = std::unordered_set<MIRBasicBlock*>(succs.begin(), succs.end());
    }
    
    return cfg;
}

std::unordered_map<MIRBasicBlock*, std::unordered_set<MIRBasicBlock*>> 
MIRFunction::computeDominators() const {
    std::unordered_map<MIRBasicBlock*, std::unordered_set<MIRBasicBlock*>> dominators;
    
    // Initialize: entry block dominates itself, all other blocks dominated by all blocks
    if (basicBlocks.empty()) return dominators;
    
    auto entry = basicBlocks[0].get();
    dominators[entry] = {entry};
    
    for (size_t i = 1; i < basicBlocks.size(); ++i) {
        auto bb = basicBlocks[i].get();
        for (const auto& other : basicBlocks) {
            dominators[bb].insert(other.get());
        }
    }
    
    // Iterative algorithm
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 1; i < basicBlocks.size(); ++i) {
            auto bb = basicBlocks[i].get();
            std::unordered_set<MIRBasicBlock*> newDom = {bb};
            
            // Find predecessors
            bool first = true;
            for (const auto& pred : basicBlocks) {
                auto succs = pred->getSuccessors();
                if (std::find(succs.begin(), succs.end(), bb) != succs.end()) {
                    if (first) {
                        newDom = dominators[pred.get()];
                        newDom.insert(bb);
                        first = false;
                    } else {
                        std::unordered_set<MIRBasicBlock*> intersection;
                        for (auto* dom : dominators[pred.get()]) {
                            if (newDom.count(dom)) {
                                intersection.insert(dom);
                            }
                        }
                        newDom = intersection;
                        newDom.insert(bb);
                    }
                }
            }
            
            if (newDom != dominators[bb]) {
                dominators[bb] = newDom;
                changed = true;
            }
        }
    }
    
    return dominators;
}

// ==================== MIRModule Implementation ====================

MIRFunctionPtr MIRModule::createFunction(const std::string& funcName) {
    auto func = std::make_shared<MIRFunction>(funcName);
    functions.push_back(func);
    return func;
}

MIRFunctionPtr MIRModule::getFunction(const std::string& funcName) const {
    for (const auto& func : functions) {
        if (func->name == funcName) {
            return func;
        }
    }
    return nullptr;
}

void MIRModule::dump() const {
    std::cout << toString();
}

std::string MIRModule::toString() const {
    std::ostringstream oss;
    oss << "// MIR Module: " << name << "\n\n";
    
    for (const auto& func : functions) {
        oss << func->toString() << "\n";
    }
    
    return oss.str();
}

} // namespace nova::mir
