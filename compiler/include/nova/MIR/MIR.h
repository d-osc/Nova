#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <iostream>

namespace nova::mir {

// Forward declarations
class MIRType;
class MIRValue;
class MIRBasicBlock;
class MIRFunction;
class MIRModule;

using MIRTypePtr = std::shared_ptr<MIRType>;
using MIRValuePtr = std::shared_ptr<MIRValue>;
using MIRBasicBlockPtr = std::shared_ptr<MIRBasicBlock>;
using MIRFunctionPtr = std::shared_ptr<MIRFunction>;

// ==================== Types ====================

class MIRType {
public:
    enum class Kind {
        Void, I1, I8, I16, I32, I64, ISize,
        U8, U16, U32, U64, USize,
        F32, F64,
        Pointer, Struct, Array, Function
    };
    
    Kind kind;
    uint32_t sizeInBytes;
    uint32_t alignment;
    
    explicit MIRType(Kind k) : kind(k), sizeInBytes(0), alignment(0) {}
    virtual ~MIRType() = default;
    
    virtual std::string toString() const;
};

// ==================== Place (SSA Values) ====================

class MIRPlace {
public:
    enum class Kind {
        Local,           // _1, _2, etc.
        Static,          // @global
        Temp,            // temporary values
        Return,          // return value
        Argument         // function arguments
    };
    
    Kind kind;
    uint32_t index;
    MIRTypePtr type;
    std::string name;
    
    MIRPlace(Kind k, uint32_t idx, MIRTypePtr t, const std::string& n = "")
        : kind(k), index(idx), type(t), name(n) {}
    
    std::string toString() const;
};

using MIRPlacePtr = std::shared_ptr<MIRPlace>;

// ==================== Operands ====================

class MIROperand {
public:
    enum class Kind {
        Copy,      // copy _1
        Move,      // move _2
        Constant   // const 42
    };
    
    Kind kind;
    
    virtual ~MIROperand() = default;
    virtual std::string toString() const = 0;
};

class MIRCopyOperand : public MIROperand {
public:
    MIRPlacePtr place;
    
    explicit MIRCopyOperand(MIRPlacePtr p) : place(p) {
        kind = Kind::Copy;
    }
    
    std::string toString() const override;
};

class MIRMoveOperand : public MIROperand {
public:
    MIRPlacePtr place;
    
    explicit MIRMoveOperand(MIRPlacePtr p) : place(p) {
        kind = Kind::Move;
    }
    
    std::string toString() const override;
};

class MIRConstOperand : public MIROperand {
public:
    enum class ConstKind {
        Int, Float, Bool, String, Null, ZeroInit
    };
    
    ConstKind constKind;
    std::variant<int64_t, double, bool, std::string> value;
    MIRTypePtr type;
    
    template<typename T>
    MIRConstOperand(ConstKind ck, T v, MIRTypePtr t)
        : constKind(ck), value(v), type(t) {
        kind = Kind::Constant;
    }
    
    std::string toString() const override;
};

using MIROperandPtr = std::shared_ptr<MIROperand>;

// ==================== RValues ====================

class MIRRValue {
public:
    enum class Kind {
        Use,              // Use(operand)
        BinaryOp,         // BinaryOp(Add, op1, op2)
        UnaryOp,          // UnaryOp(Neg, op)
        CheckedBinaryOp,  // CheckedBinaryOp(Add, op1, op2)
        Ref,              // Ref(Shared, place)
        AddressOf,        // AddressOf(Mut, place)
        Cast,             // Cast(IntToInt, op, type)
        Aggregate,        // Aggregate(Array, [op1, op2])
        Len,              // Len(place)
        Discriminant      // Discriminant(place)
    };
    
    Kind kind;
    
    virtual ~MIRRValue() = default;
    virtual std::string toString() const = 0;
};

class MIRUseRValue : public MIRRValue {
public:
    MIROperandPtr operand;
    
    explicit MIRUseRValue(MIROperandPtr op) : operand(op) {
        kind = Kind::Use;
    }
    
    std::string toString() const override;
};

class MIRBinaryOpRValue : public MIRRValue {
public:
    enum class BinOp {
        Add, Sub, Mul, Div, Rem,
        BitAnd, BitOr, BitXor, Shl, Shr,
        Eq, Lt, Le, Ne, Ge, Gt,
        Offset
    };
    
    BinOp op;
    MIROperandPtr lhs;
    MIROperandPtr rhs;
    
    MIRBinaryOpRValue(BinOp o, MIROperandPtr l, MIROperandPtr r)
        : op(o), lhs(l), rhs(r) {
        kind = Kind::BinaryOp;
    }
    
    std::string toString() const override;
};

class MIRUnaryOpRValue : public MIRRValue {
public:
    enum class UnOp {
        Not, Neg
    };
    
    UnOp op;
    MIROperandPtr operand;
    
    MIRUnaryOpRValue(UnOp o, MIROperandPtr operand)
        : op(o), operand(operand) {
        kind = Kind::UnaryOp;
    }
    
    std::string toString() const override;
};

class MIRCastRValue : public MIRRValue {
public:
    enum class CastKind {
        IntToInt, FloatToInt, IntToFloat, FloatToFloat,
        PtrToPtr, Bitcast, Unsize
    };

    CastKind castKind;
    MIROperandPtr operand;
    MIRTypePtr targetType;

    MIRCastRValue(CastKind ck, MIROperandPtr op, MIRTypePtr type)
        : castKind(ck), operand(op), targetType(type) {
        kind = Kind::Cast;
    }

    std::string toString() const override;
};

class MIRAggregateRValue : public MIRRValue {
public:
    enum class AggregateKind {
        Array, Tuple, Struct
    };

    AggregateKind aggregateKind;
    std::vector<MIROperandPtr> elements;

    MIRAggregateRValue(AggregateKind ak, std::vector<MIROperandPtr> elems)
        : aggregateKind(ak), elements(std::move(elems)) {
        kind = Kind::Aggregate;
    }

    std::string toString() const override;
};

class MIRGetElementRValue : public MIRRValue {
public:
    MIROperandPtr array;   // The array operand
    MIROperandPtr index;   // The index operand

    MIRGetElementRValue(MIROperandPtr arr, MIROperandPtr idx)
        : array(arr), index(idx) {
        kind = Kind::Ref;  // Use Ref kind temporarily (will add GetElement kind later)
    }

    std::string toString() const override;
};

using MIRRValuePtr = std::shared_ptr<MIRRValue>;

// ==================== Statements ====================

class MIRStatement {
public:
    enum class Kind {
        Assign,        // _1 = rvalue
        StorageLive,   // StorageLive(_1)
        StorageDead,   // StorageDead(_1)
        SetDiscriminant,
        Deinit,
        Nop
    };
    
    Kind kind;
    
    virtual ~MIRStatement() = default;
    virtual std::string toString() const = 0;
};

class MIRAssignStatement : public MIRStatement {
public:
    MIRPlacePtr place;
    MIRRValuePtr rvalue;
    
    MIRAssignStatement(MIRPlacePtr p, MIRRValuePtr rv)
        : place(p), rvalue(rv) {
        kind = Kind::Assign;
    }
    
    std::string toString() const override;
};

class MIRStorageLiveStatement : public MIRStatement {
public:
    MIRPlacePtr place;
    
    explicit MIRStorageLiveStatement(MIRPlacePtr p) : place(p) {
        kind = Kind::StorageLive;
    }
    
    std::string toString() const override;
};

class MIRStorageDeadStatement : public MIRStatement {
public:
    MIRPlacePtr place;
    
    explicit MIRStorageDeadStatement(MIRPlacePtr p) : place(p) {
        kind = Kind::StorageDead;
    }
    
    std::string toString() const override;
};

using MIRStatementPtr = std::shared_ptr<MIRStatement>;

// ==================== Terminators ====================

class MIRTerminator {
public:
    enum class Kind {
        Return,
        Goto,
        SwitchInt,
        Call,
        Assert,
        Drop,
        Unreachable
    };
    
    Kind kind;
    
    virtual ~MIRTerminator() = default;
    virtual std::string toString() const = 0;
    virtual std::vector<MIRBasicBlock*> getSuccessors() const = 0;
};

class MIRReturnTerminator : public MIRTerminator {
public:
    MIRReturnTerminator() {
        kind = Kind::Return;
    }
    
    std::string toString() const override;
    std::vector<MIRBasicBlock*> getSuccessors() const override { return {}; }
};

class MIRGotoTerminator : public MIRTerminator {
public:
    MIRBasicBlock* target;
    
    explicit MIRGotoTerminator(MIRBasicBlock* t) : target(t) {
        kind = Kind::Goto;
    }
    
    std::string toString() const override;
    std::vector<MIRBasicBlock*> getSuccessors() const override;
};

class MIRSwitchIntTerminator : public MIRTerminator {
public:
    struct SwitchTarget {
        int64_t value;
        MIRBasicBlock* target;
    };
    
    MIROperandPtr discriminant;
    std::vector<SwitchTarget> targets;
    MIRBasicBlock* otherwise;
    
    MIRSwitchIntTerminator(MIROperandPtr disc, MIRBasicBlock* other)
        : discriminant(disc), otherwise(other) {
        kind = Kind::SwitchInt;
    }
    
    void addTarget(int64_t value, MIRBasicBlock* target) {
        targets.push_back({value, target});
    }
    
    std::string toString() const override;
    std::vector<MIRBasicBlock*> getSuccessors() const override;
};

class MIRCallTerminator : public MIRTerminator {
public:
    MIROperandPtr func;
    std::vector<MIROperandPtr> args;
    MIRPlacePtr destination;
    MIRBasicBlock* target;
    MIRBasicBlock* unwind;  // nullptr if no unwind
    
    MIRCallTerminator(MIROperandPtr f, std::vector<MIROperandPtr> a,
                      MIRPlacePtr dest, MIRBasicBlock* t, MIRBasicBlock* u = nullptr)
        : func(f), args(std::move(a)), destination(dest), target(t), unwind(u) {
        kind = Kind::Call;
    }
    
    std::string toString() const override;
    std::vector<MIRBasicBlock*> getSuccessors() const override;
};

using MIRTerminatorPtr = std::shared_ptr<MIRTerminator>;

// ==================== Basic Blocks ====================

class MIRBasicBlock {
public:
    std::string label;
    std::vector<MIRStatementPtr> statements;
    MIRTerminatorPtr terminator;
    
    bool isCleanup;
    
    explicit MIRBasicBlock(const std::string& lbl)
        : label(lbl), isCleanup(false) {}
    
    void addStatement(MIRStatementPtr stmt) {
        statements.push_back(stmt);
    }
    
    void setTerminator(MIRTerminatorPtr term) {
        terminator = term;
    }
    
    std::vector<MIRBasicBlock*> getSuccessors() const {
        if (terminator) {
            return terminator->getSuccessors();
        }
        return {};
    }
    
    std::string toString() const;
};

// ==================== Functions ====================

class MIRFunction {
public:
    std::string name;
    MIRTypePtr returnType;
    std::vector<MIRPlacePtr> arguments;
    std::vector<MIRPlacePtr> locals;
    std::vector<MIRBasicBlockPtr> basicBlocks;
    
    struct LocalDecl {
        MIRPlacePtr place;
        bool isMutable;
        std::string debugName;
    };
    std::vector<LocalDecl> localDecls;
    
    explicit MIRFunction(const std::string& n) : name(n) {}
    
    MIRBasicBlockPtr createBasicBlock(const std::string& label);
    MIRBasicBlockPtr getEntryBlock() const {
        return basicBlocks.empty() ? nullptr : basicBlocks[0];
    }
    
    MIRPlacePtr createLocal(MIRTypePtr type, const std::string& name = "");
    MIRPlacePtr createTemp(MIRTypePtr type);
    
    std::string toString() const;
    
    // CFG Analysis
    std::unordered_map<MIRBasicBlock*, std::unordered_set<MIRBasicBlock*>> computeCFG() const;
    std::unordered_map<MIRBasicBlock*, std::unordered_set<MIRBasicBlock*>> computeDominators() const;
};

// ==================== Module ====================

class MIRModule {
public:
    std::string name;
    std::vector<MIRFunctionPtr> functions;
    std::unordered_map<std::string, MIRTypePtr> types;
    std::unordered_map<std::string, MIRPlacePtr> statics;
    
    explicit MIRModule(const std::string& n) : name(n) {}
    
    MIRFunctionPtr createFunction(const std::string& name);
    MIRFunctionPtr getFunction(const std::string& name) const;
    
    void dump() const;
    std::string toString() const;
};

} // namespace nova::mir
