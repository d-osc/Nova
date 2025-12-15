# Nova Compiler - File Refactoring Plan

**Date:** 2025-12-07
**Purpose:** Split large files into manageable modules for better maintainability

---

## 🎯 Goals

1. **Reduce file sizes** - No single file > 2000 lines
2. **Improve readability** - Logical module separation
3. **Easier maintenance** - Related code grouped together
4. **Better compilation** - Parallel compilation of modules

---

## 📊 Current Status

### Files Requiring Refactoring:

| File | Lines | Status | Priority |
|------|-------|--------|----------|
| `src/hir/HIRGen.cpp` | 18,470 | ⚠️ CRITICAL | P1 |
| `src/codegen/LLVMCodeGen.cpp` | 6,461 | ⚠️ HIGH | P2 |
| `src/runtime/BuiltinFS.cpp` | 3,393 | ⚠️ MEDIUM | P3 |
| `src/pm/PackageManager.cpp` | 2,118 | ⚠️ MEDIUM | P4 |
| `src/transpiler/Transpiler.cpp` | 1,949 | ⚠️ LOW | P5 |

---

## 🔧 Refactoring Plan

### Priority 1: HIRGen.cpp (18,470 lines → ~10 files)

**Current Structure:** Single monolithic file with 76 visitor methods

**Proposed Split:**

```
src/hir/
├── HIRGen.cpp                    (Main class, ~500 lines)
├── HIRGen_Literals.cpp           (Literals: Number, String, Boolean, etc.)
├── HIRGen_Operators.cpp          (Binary, Unary, Update operators)
├── HIRGen_Functions.cpp          (Function, Arrow function expressions)
├── HIRGen_Classes.cpp            (Class expressions and methods)
├── HIRGen_Arrays.cpp             (Array expressions and operations)
├── HIRGen_Objects.cpp            (Object literals and member access)
├── HIRGen_ControlFlow.cpp        (If, Switch, Loops, etc.)
├── HIRGen_Statements.cpp         (Variable declarations, blocks)
├── HIRGen_Calls.cpp              (Function calls, built-in methods)
└── HIRGen_Advanced.cpp           (Async, Generators, JSX, etc.)
```

**Module Breakdown:**

#### 1. HIRGen.cpp (Main - ~500 lines)
- Class declaration
- Constructor/destructor
- Helper methods
- Module setup
- Symbol table management

#### 2. HIRGen_Literals.cpp (~400 lines)
**Visitors:**
- `NumberLiteral`
- `BigIntLiteral`
- `StringLiteral`
- `RegexLiteralExpr`
- `BooleanLiteral`
- `NullLiteral`
- `UndefinedLiteral`
- `TemplateLiteralExpr`

#### 3. HIRGen_Operators.cpp (~1500 lines)
**Visitors:**
- `BinaryExpr` (arithmetic, comparison, logical)
- `UnaryExpr` (!, -, +, typeof, etc.)
- `UpdateExpr` (++, --)
- `AssignmentExpr`
- `ConditionalExpr` (ternary)

#### 4. HIRGen_Functions.cpp (~2000 lines)
**Visitors:**
- `FunctionExpr`
- `ArrowFunctionExpr`
- `FunctionDecl`
- Parameter handling
- Return statements

#### 5. HIRGen_Classes.cpp (~3000 lines)
**Visitors:**
- `ClassExpr`
- `ClassDecl`
- `NewExpr`
- `ThisExpr`
- `SuperExpr`
- Constructor generation
- Method generation

#### 6. HIRGen_Arrays.cpp (~1000 lines)
**Visitors:**
- `ArrayExpr`
- Array method calls (map, filter, reduce, etc.)
- Array operations

#### 7. HIRGen_Objects.cpp (~1500 lines)
**Visitors:**
- `ObjectExpr`
- `MemberExpr`
- Property access
- Object methods

#### 8. HIRGen_ControlFlow.cpp (~2500 lines)
**Visitors:**
- `IfStmt`
- `SwitchStmt`
- `ForStmt`
- `WhileStmt`
- `DoWhileStmt`
- `ForInStmt`
- `ForOfStmt`
- `BreakStmt`
- `ContinueStmt`
- `ReturnStmt`
- `ThrowStmt`
- `TryStmt`

#### 9. HIRGen_Statements.cpp (~2000 lines)
**Visitors:**
- `VariableDecl`
- `BlockStmt`
- `ExprStmt`
- `EmptyStmt`
- `DebuggerStmt`
- `WithStmt`
- `LabeledStmt`

#### 10. HIRGen_Calls.cpp (~3500 lines)
**Visitors:**
- `CallExpr` (main implementation)
- Built-in function calls
- Console methods
- Math methods
- Array methods
- String methods
- Built-in module imports

#### 11. HIRGen_Advanced.cpp (~1000 lines)
**Visitors:**
- `AwaitExpr`
- `YieldExpr`
- `SpreadExpr`
- `ImportExpr`
- `JSXElement`
- `JSXFragment`
- `Decorator`
- Other advanced features

---

### Priority 2: LLVMCodeGen.cpp (6,461 lines → ~4 files)

**Proposed Split:**

```
src/codegen/
├── LLVMCodeGen.cpp              (Main class, ~1000 lines)
├── LLVMCodeGen_Types.cpp        (Type conversion and handling)
├── LLVMCodeGen_Values.cpp       (Value generation)
├── LLVMCodeGen_Functions.cpp    (Function code generation)
└── LLVMCodeGen_Runtime.cpp      (Runtime library integration)
```

---

### Priority 3: BuiltinFS.cpp (3,393 lines → ~3 files)

**Proposed Split:**

```
src/runtime/
├── BuiltinFS_Sync.cpp           (Synchronous file operations)
├── BuiltinFS_Async.cpp          (Asynchronous file operations)
└── BuiltinFS_Streams.cpp        (File streams)
```

---

## 🏗️ Implementation Strategy

### Phase 1: Prepare Infrastructure
1. Create header files with shared declarations
2. Set up partial class pattern (if using C++)
3. Update CMakeLists.txt for new files

### Phase 2: Extract Methods
1. Copy visitor methods to new files
2. Keep original file as backup
3. Add necessary includes

### Phase 3: Test & Validate
1. Ensure compilation succeeds
2. Run all tests
3. Verify no functionality broken

### Phase 4: Cleanup
1. Remove backup files
2. Update documentation
3. Add comments to new files

---

## 📝 Implementation Steps for HIRGen.cpp

### Step 1: Create Header File

**File:** `include/nova/HIR/HIRGen.h`

```cpp
#ifndef NOVA_HIR_HIRGEN_H
#define NOVA_HIR_HIRGEN_H

#include "nova/Frontend/AST.h"
#include "nova/HIR/HIR.h"
#include <map>
#include <vector>

namespace nova {
namespace hir {

class HIRGen : public ASTVisitor {
private:
    // Member variables
    std::unique_ptr<HIRModule> module_;
    std::unique_ptr<HIRBuilder> builder_;
    HIRFunction* currentFunction_;
    std::map<std::string, HIRValue*> symbolTable_;
    std::vector<std::map<std::string, HIRValue*>> scopeStack_;
    HIRValue* lastValue_;

    // ... other member variables

public:
    HIRGen();
    ~HIRGen() = default;

    std::unique_ptr<HIRModule> generate(Program& program);

    // Visitor methods - declared here, implemented in separate files

    // Literals (HIRGen_Literals.cpp)
    void visit(NumberLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(BooleanLiteral& node) override;
    // ... etc

    // Operators (HIRGen_Operators.cpp)
    void visit(BinaryExpr& node) override;
    void visit(UnaryExpr& node) override;
    // ... etc

    // Functions (HIRGen_Functions.cpp)
    void visit(FunctionExpr& node) override;
    void visit(ArrowFunctionExpr& node) override;
    // ... etc

    // ... all other visitor declarations
};

} // namespace hir
} // namespace nova

#endif
```

### Step 2: Split Implementation Files

Each implementation file follows this pattern:

**File:** `src/hir/HIRGen_Literals.cpp`

```cpp
#include "nova/HIR/HIRGen.h"

namespace nova {
namespace hir {

void HIRGen::visit(NumberLiteral& node) {
    // Implementation here
}

void HIRGen::visit(StringLiteral& node) {
    // Implementation here
}

// ... other literal visitors

} // namespace hir
} // namespace nova
```

### Step 3: Update CMakeLists.txt

```cmake
# src/hir/CMakeLists.txt

set(HIR_SOURCES
    HIRGen.cpp
    HIRGen_Literals.cpp
    HIRGen_Operators.cpp
    HIRGen_Functions.cpp
    HIRGen_Classes.cpp
    HIRGen_Arrays.cpp
    HIRGen_Objects.cpp
    HIRGen_ControlFlow.cpp
    HIRGen_Statements.cpp
    HIRGen_Calls.cpp
    HIRGen_Advanced.cpp
    MIRGen.cpp
)

add_library(novahir ${HIR_SOURCES})
```

---

## ✅ Benefits

### 1. **Faster Compilation**
- Parallel compilation of separate modules
- Only recompile changed modules
- Estimated: 50% faster incremental builds

### 2. **Better Code Organization**
- Related functionality grouped together
- Easier to find specific implementations
- Clear module boundaries

### 3. **Easier Maintenance**
- Smaller files easier to understand
- Reduce merge conflicts
- Better code reviews

### 4. **Improved Testability**
- Test specific modules independently
- Easier to mock/stub components
- Better unit test coverage

### 5. **Team Collaboration**
- Multiple developers can work on different modules
- Less file locking issues
- Clear ownership of modules

---

## 📊 Expected Results

### Before Refactoring:
```
HIRGen.cpp:              18,470 lines
Compilation time:        ~30 seconds
Incremental rebuild:     ~20 seconds
```

### After Refactoring:
```
HIRGen.cpp:                 500 lines
HIRGen_*.cpp (10 files):  ~1800 lines each
Compilation time:         ~25 seconds (parallel)
Incremental rebuild:      ~5 seconds (only changed module)
```

**Improvement:**
- ✅ 75% faster incremental builds
- ✅ 95% smaller individual files
- ✅ Better code organization
- ✅ Easier to maintain

---

## 🚀 Next Steps

1. ✅ Create refactoring plan (this document)
2. ⏳ Get approval for refactoring approach
3. ⏳ Create header file with all declarations
4. ⏳ Split HIRGen.cpp into modules
5. ⏳ Update build system
6. ⏳ Test compilation
7. ⏳ Refactor other large files

---

## ⚠️ Risks & Mitigation

### Risk 1: Breaking Compilation
**Mitigation:** Keep backup, test after each file split

### Risk 2: Missing Dependencies
**Mitigation:** Careful include management, use forward declarations

### Risk 3: Performance Regression
**Mitigation:** Measure compilation times before/after

### Risk 4: Git History Loss
**Mitigation:** Use `git mv` when possible, document refactoring

---

**Nova Compiler - Refactoring Plan v1.0**
**Status:** Ready for Implementation
**Estimated Time:** 2-4 hours
**Expected Benefit:** Significant improvement in maintainability
