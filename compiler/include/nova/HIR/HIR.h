#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <iostream>

namespace nova::hir {

// Forward declarations
class HIRType;
class HIRValue;
class HIRInstruction;
class HIRBasicBlock;
class HIRFunction;
class HIRModule;

using HIRTypePtr = std::shared_ptr<HIRType>;
using HIRValuePtr = std::shared_ptr<HIRValue>;
using HIRInstructionPtr = std::shared_ptr<HIRInstruction>;
using HIRBasicBlockPtr = std::shared_ptr<HIRBasicBlock>;
using HIRFunctionPtr = std::shared_ptr<HIRFunction>;

// ==================== Types ====================

class HIRType {
public:
    enum class Kind {
        Void, Never, Unit,
        I8, I16, I32, I64, ISize,
        U8, U16, U32, U64, USize,
        F32, F64,
        Bool, Char, String,
        Pointer, Reference,
        Array, Tuple, Struct,
        Function, Closure,
        Optional, Result,
        Any, Unknown
    };
    
    Kind kind;
    
    explicit HIRType(Kind k) : kind(k) {}
    virtual ~HIRType() = default;
    
    bool isInteger() const;
    bool isFloat() const;
    bool isNumeric() const;
    bool isPrimitive() const;
    bool isAggregate() const;
    bool isPointer() const;
    
    virtual std::string toString() const;
};

class HIRIntegerType : public HIRType {
public:
    uint32_t bitWidth;
    bool isSigned;
    
    HIRIntegerType(uint32_t width, bool sign)
        : HIRType(sign ? Kind::I64 : Kind::U64), bitWidth(width), isSigned(sign) {}
};

class HIRPointerType : public HIRType {
public:
    HIRTypePtr pointeeType;
    bool isMutable;
    
    HIRPointerType(HIRTypePtr pointee, bool mut)
        : HIRType(Kind::Pointer), pointeeType(pointee), isMutable(mut) {}
    
    std::string toString() const override;
};

class HIRArrayType : public HIRType {
public:
    HIRTypePtr elementType;
    uint64_t size;
    
    HIRArrayType(HIRTypePtr elem, uint64_t s)
        : HIRType(Kind::Array), elementType(elem), size(s) {}
    
    std::string toString() const override;
};

class HIRTupleType : public HIRType {
public:
    std::vector<HIRTypePtr> elementTypes;
    
    explicit HIRTupleType(std::vector<HIRTypePtr> elems)
        : HIRType(Kind::Tuple), elementTypes(std::move(elems)) {}
    
    std::string toString() const override;
};

class HIRStructType : public HIRType {
public:
    struct Field {
        std::string name;
        HIRTypePtr type;
        bool isPublic;
    };
    
    std::string name;
    std::vector<Field> fields;
    
    HIRStructType(const std::string& n, std::vector<Field> f)
        : HIRType(Kind::Struct), name(n), fields(std::move(f)) {}
    
    std::string toString() const override;
};

class HIRFunctionType : public HIRType {
public:
    std::vector<HIRTypePtr> paramTypes;
    HIRTypePtr returnType;
    bool isVariadic;
    
    HIRFunctionType(std::vector<HIRTypePtr> params, HIRTypePtr ret, bool variadic = false)
        : HIRType(Kind::Function), paramTypes(std::move(params)),
          returnType(ret), isVariadic(variadic) {}
    
    std::string toString() const override;
};

// ==================== Values ====================

class HIRValue {
public:
    HIRTypePtr type;
    std::string name;
    
    HIRValue(HIRTypePtr t, const std::string& n) : type(t), name(n) {}
    virtual ~HIRValue() = default;
    
    virtual std::string toString() const;
};

class HIRConstant : public HIRValue {
public:
    enum class Kind {
        Integer, Float, Boolean, String, Null, Undefined
    };
    
    Kind kind;
    std::variant<int64_t, double, bool, std::string> value;
    
    template<typename T>
    HIRConstant(HIRTypePtr t, Kind k, T v)
        : HIRValue(t, ""), kind(k), value(v) {}
    
    std::string toString() const override;
};

class HIRParameter : public HIRValue {
public:
    uint32_t index;
    
    HIRParameter(HIRTypePtr t, const std::string& n, uint32_t idx)
        : HIRValue(t, n), index(idx) {}
};

// ==================== Instructions ====================

class HIRInstruction : public HIRValue {
public:
    enum class Opcode {
        // Arithmetic
        Add, Sub, Mul, Div, Rem, Neg,
        // Bitwise
        And, Or, Xor, Not, Shl, Shr, UShr,
        // Comparison
        Eq, Ne, Lt, Le, Gt, Ge,
        // Memory
        Alloca, Load, Store, GetField, SetField,
        GetElement, SetElement,
        // Control Flow
        Br, CondBr, Switch, Return, Unreachable,
        Break, Continue,
        // Function
        Call, InvokeDirectCall, InvokeVirtual,
        // Type Operations
        Cast, Bitcast, IntToPtr, PtrToInt,
        // Aggregate
        StructConstruct, ArrayConstruct, TupleConstruct,
        ExtractValue, InsertValue,
        // Closures
        CaptureClosure, InvokeClosure,
        // Async
        Await, Yield, AsyncCall,
        // Phi (SSA)
        Phi
    };
    
    Opcode opcode;
    std::vector<HIRValuePtr> operands;
    HIRBasicBlockPtr parentBlock;
    
    HIRInstruction(Opcode op, HIRTypePtr t, const std::string& n)
        : HIRValue(t, n), opcode(op) {}
    
    void addOperand(HIRValuePtr operand) {
        operands.push_back(operand);
    }
    
    std::string toString() const override;
};

// ==================== Basic Blocks ====================

class HIRBasicBlock : public std::enable_shared_from_this<HIRBasicBlock> {
public:
    std::string label;
    std::vector<HIRInstructionPtr> instructions;
    HIRFunctionPtr parentFunction;
    
    std::vector<HIRBasicBlockPtr> predecessors;
    std::vector<HIRBasicBlockPtr> successors;
    
    bool hasBreakOrContinue = false;
    
    explicit HIRBasicBlock(const std::string& lbl) : label(lbl) {}
    
    void addInstruction(HIRInstructionPtr inst) {
        inst->parentBlock = shared_from_this();
        instructions.push_back(inst);
    }
    
    HIRInstructionPtr getTerminator() const {
        if (instructions.empty()) return nullptr;
        return instructions.back();
    }
    
    bool hasTerminator() const;
    
    std::string toString() const;
};

// ==================== Functions ====================

class HIRFunction {
public:
    std::string name;
    HIRFunctionType* functionType;
    std::vector<HIRParameter*> parameters;
    std::vector<HIRBasicBlockPtr> basicBlocks;
    
    enum class Linkage {
        Internal, External, Public, Private
    };
    Linkage linkage;
    
    bool isAsync;
    bool isGenerator;
    
    struct Attribute {
        enum class Kind {
            Inline, NoInline, AlwaysInline,
            Pure, Const, ReadOnly, WriteOnly,
            NoReturn, NoUnwind, Cold, Hot
        };
        Kind kind;
    };
    std::vector<Attribute> attributes;
    
    HIRFunction(const std::string& n, HIRFunctionType* ft)
        : name(n), functionType(ft), linkage(Linkage::Public),
          isAsync(false), isGenerator(false) {}
    
    HIRBasicBlockPtr createBasicBlock(const std::string& label);
    HIRBasicBlockPtr getEntryBlock() const {
        return basicBlocks.empty() ? nullptr : basicBlocks[0];
    }
    
    void addAttribute(Attribute::Kind kind) {
        attributes.push_back({kind});
    }
    
    std::string toString() const;
};

// ==================== Module ====================

class HIRModule {
public:
    std::string name;
    std::vector<HIRFunctionPtr> functions;
    std::vector<HIRStructType*> types;
    std::unordered_map<std::string, HIRValuePtr> globals;
    
    explicit HIRModule(const std::string& n) : name(n) {}
    
    HIRFunctionPtr createFunction(const std::string& name, HIRFunctionType* type);
    HIRStructType* createStructType(const std::string& name);
    
    HIRFunctionPtr getFunction(const std::string& name) const;
    HIRStructType* getStructType(const std::string& name) const;
    
    void dump() const;
    std::string toString() const;
};

// ==================== Builder ====================

class HIRBuilder {
public:
    HIRBuilder(HIRModule* mod, HIRFunction* func)
        : module_(mod), function_(func), currentBlock_(nullptr) {}
    
    void setInsertPoint(HIRBasicBlock* block) {
        currentBlock_ = block;
    }
    
    HIRBasicBlock* getInsertBlock() const { return currentBlock_; }
    
    // Arithmetic
    HIRInstruction* createAdd(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");
    HIRInstruction* createSub(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");
    HIRInstruction* createMul(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");
    HIRInstruction* createDiv(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");
    HIRInstruction* createRem(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");

    // Bitwise Operations
    HIRInstruction* createAnd(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");
    HIRInstruction* createOr(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");
    HIRInstruction* createXor(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");
    HIRInstruction* createShl(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");
    HIRInstruction* createShr(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");
    HIRInstruction* createNot(HIRValue* operand, const std::string& name = "");

    // Comparison
    HIRInstruction* createEq(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");
    HIRInstruction* createNe(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");
    HIRInstruction* createLt(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");
    HIRInstruction* createLe(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");
    HIRInstruction* createGt(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");
    HIRInstruction* createGe(HIRValue* lhs, HIRValue* rhs, const std::string& name = "");
    
    // Memory
    HIRInstruction* createAlloca(HIRType* type, const std::string& name = "");
    HIRInstruction* createLoad(HIRValue* ptr, const std::string& name = "");
    HIRInstruction* createStore(HIRValue* value, HIRValue* ptr);
    
    // Control Flow
    HIRInstruction* createBr(HIRBasicBlock* dest);
    HIRInstruction* createCondBr(HIRValue* cond, HIRBasicBlock* thenBlock, HIRBasicBlock* elseBlock);
    HIRInstruction* createReturn(HIRValue* value = nullptr);
    
    // Function Calls
    HIRInstruction* createCall(HIRFunction* callee, const std::vector<HIRValue*>& args,
                               const std::string& name = "");
    
    // Type Conversions
    HIRInstruction* createCast(HIRValue* value, HIRType* destType, const std::string& name = "");
    
    // Aggregate Operations
    HIRInstruction* createGetField(HIRValue* struct_, uint32_t fieldIndex, const std::string& name = "");
    HIRInstruction* createSetField(HIRValue* struct_, uint32_t fieldIndex, HIRValue* value, const std::string& name = "");
    HIRInstruction* createGetElement(HIRValue* array, HIRValue* index, const std::string& name = "");
    HIRInstruction* createSetElement(HIRValue* array, HIRValue* index, HIRValue* value);
    HIRInstruction* createArrayConstruct(const std::vector<HIRValue*>& elements, const std::string& name = "");
    HIRInstruction* createStructConstruct(HIRStructType* structType, const std::vector<HIRValue*>& fieldValues, const std::string& name = "");
    
    // Constants
    HIRConstant* createIntConstant(int64_t value, uint32_t bitWidth = 64);
    HIRConstant* createFloatConstant(double value);
    HIRConstant* createBoolConstant(bool value);
    HIRConstant* createStringConstant(const std::string& value);
    HIRConstant* createNullConstant(HIRType* type);
    
private:
    HIRModule* module_;
    HIRFunction* function_;
    HIRBasicBlock* currentBlock_;
    
    uint32_t nextValueId_ = 0;
    std::string generateName(const std::string& hint);
};

} // namespace nova::hir
