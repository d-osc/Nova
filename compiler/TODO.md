# Nova Compiler - TODO List

> **Last Updated**: 2025-11-13
> **Current Version**: v0.6.0
> **Current Sprint**: Feature Enhancement Phase
> **Focus**: Arrays, Objects, and String Operations

---

## ✅ Recently Completed

### 1. ✅ Fix Loop Condition Generation Bug - COMPLETED! 🎉
**Priority**: P0 - Highest
**Completed**: 2025-11-12 (v0.50)

**Problem**: Loop conditions were hardcoded to `br i1 true` causing infinite loops

**Solution**:
- Fixed `src/mir/MIRGen.cpp:getOrCreatePlace()` to extract pointee type from pointers
- Added i1→i64 return type conversion in `src/codegen/LLVMCodeGen.cpp`
- Variables now have correct types throughout pipeline

**Result**: ✅ While loops work perfectly with runtime conditions!

### 2. ✅ Fix For Loop Void Type Bug - COMPLETED! 🎉
**Priority**: P0 - Highest
**Completed**: 2025-11-12 (v0.51)

**Problem**: For loops hung during compilation then segfaulted due to void type handling

**Solution**:
- Enhanced void type handling in `src/codegen/LLVMCodeGen.cpp`
- During alloca creation: replace void types with i64 placeholder
- During load: use alloca type when MIR type is void
- Proper type reconciliation between MIR and LLVM IR

**Result**: ✅ For loops compile and execute correctly with proper phi nodes!

### 3. ✅ Fix Logical Operations and Strict Equality - COMPLETED! 🎉
**Priority**: P0 - Highest
**Completed**: 2025-11-13 (v0.60)

**Problem**: Logical operators and strict equality (===, !==) not implemented

**Solution**:
- Added StrictEqual and StrictNotEqual cases in HIRGen.cpp
- Implemented short-circuit evaluation for && and ||
- All comparison operators now work correctly

**Result**: ✅ All 15 tests passing, logical operations work perfectly!

**Tests**:
- ✅ test_while_simple.ts - Returns 5 ✅
- ✅ test_for_simple.ts - Returns 10 ✅
- ✅ test_logical_ops.ts - Returns 42 ✅
- ✅ test_and_direct.ts - Returns 1 ✅
- ✅ test_or_direct.ts - Returns 3 ✅
- ✅ test_simple_if.ts - Returns 1 ✅
- ✅ test_assign_check.ts - Returns 42 ✅
- ✅ All 15/15 tests passing (100%)

### 4. ✅ Update Documentation - COMPLETED! 🎉
**Priority**: P1 - High
**Completed**: 2025-11-13

**Changes**:
- Updated README.md with accurate feature list
- Updated version to v0.6.0
- Updated test count to 15/15
- Added working examples for loops and conditionals
- Updated roadmap with realistic milestones

### 5. ✅ Array Literals and Indexing - COMPLETED! 🎉
**Priority**: P1 - High
**Completed**: 2025-11-13 (v0.7.0)

**What Works**:
```typescript
let arr = [1, 2, 3];  // ✅ Works
let x = arr[0];       // ✅ Works - Returns 1
```

**Implemented**:
- [x] Design array type in HIR (HIRArrayType)
- [x] Implement array literals with MIRAggregateRValue
- [x] Implement array indexing with MIRGetElementRValue
- [x] LLVM codegen with GEP instructions
- [x] Test with simple array access
- [x] Test suite passes

**Result**: ✅ Array literals and indexing work perfectly!

**Note**: Array assignment (`arr[0] = 5`) is not yet implemented

### 6. ✅ Object Property Reading - COMPLETED! 🎉
**Priority**: P1 - High
**Completed**: 2025-11-13 (v0.7.0)

**What Works**:
```typescript
let obj = {x: 10, y: 20};  // ✅ Works
let x = obj.x;             // ✅ Works - Returns 10
```

**Implemented**:
- [x] Implement object literals with HIRStructType
- [x] Implement property reading with GetField
- [x] Struct construction in MIR
- [x] Field access in MIR
- [x] LLVM codegen for structs with GEP
- [x] Fixed critical object slicing bug in type preservation
- [x] Test with simple property access

**Result**: ✅ Object literals and property reading work perfectly!

**Note**: Property assignment and nested objects not yet implemented

---

## 🔥 CRITICAL - Must Do Now (P0)

(No critical issues blocking development)

---

## 🎯 HIGH PRIORITY - Next Up (P1)

### 7. Object and Array Assignment ✅
**Priority**: P1 - High
**Completed**: 2025-11-14
**Status**: COMPLETED ✅ - All features working including nested objects!

**What Works**:
```typescript
let obj = {x: 10, y: 20};  // ✅ Works
let x = obj.x;             // ✅ Works - Returns 10
obj.x = 42;                // ✅ Works - Property assignment!
let arr = [10, 20, 30];    // ✅ Works
arr[0] = 42;               // ✅ Works - Array assignment!
```

**What Works**:
```typescript
let obj = {x: 10, child: {value: 42}};  // ✅ Works
let nested = obj.child.value;            // ✅ Works - Nested object access!
```

**Already Completed**:
- [x] Implement object literals with HIRStructType
- [x] Implement property reading in HIR (GetField)
- [x] Implement property assignment in HIR (SetField)
- [x] Implement struct construction in MIR
- [x] Implement field access in MIR
- [x] Implement LLVM codegen for structs with GEP
- [x] Fixed critical GEP bug (use loaded value not alloca)
- [x] Test with simple property access
- [x] Test with property assignment
- [x] Implement array literals
- [x] Implement array indexing (reading)
- [x] Implement array element assignment (SetElement)
- [x] Test with array assignment
- [x] Fixed object slicing bug in type preservation
- [x] Implement nested object access (obj.child.x) ✅
- [x] Added nestedStructTypeMap for tracking nested struct types
- [x] Propagate nested struct types when copying variables
- [x] Test with nested objects ✅

**Remaining Action Items**:
- [ ] Document in CHANGELOG.md

**Success Criteria**:
- ✅ Can read object properties: `obj.name`
- ✅ Can write object properties: `obj.name = "Jane"`
- ✅ Can read array elements: `arr[0]`
- ✅ Can write array elements: `arr[0] = 42`
- ✅ Nested objects work: `obj.child.value` ✅ COMPLETED!

---

### 8. Complete String Operations ✅
**Priority**: P1 - High
**Completed**: 2025-11-14
**Status**: FULLY COMPLETED! All string operations working! ✅

**What Works**:
```typescript
let s1 = "Hello";              // ✅ Works
let s2 = " World";             // ✅ Works
let s3 = s1 + s2;              // ✅ Works - String concatenation!
// s3 is now "Hello World"
```

**What's Working**:
```typescript
let len = "Hello".length;      // ✅ Works - Compile-time constant (5)
"Test".length                  // ✅ Works - Returns 4 at compile time
```

**What's Partially Working**:
```typescript
let s = "Hello";
let len2 = s.length;           // ⚠️  Optimized to constant 5 by LLVM
                               // (but needs proper type system for general case)
```

**What's Missing**:
```typescript
let s3 = `Hello ${name}`;      // ❌ Not implemented - Template literals
let sub = s.substring(0, 3);   // ❌ Not implemented - String methods
let idx = s.indexOf("ll");     // ❌ Not implemented - String methods
```

**Already Completed**:
- [x] Implement string concatenation ✅
  - [x] Compiler generates call to nova_string_concat_cstr
  - [x] Runtime function exists in String.cpp
  - [x] Memory allocation for result
  - [x] Copy strings into result
  - [x] Test string concatenation ✅
- [x] string.length property ✅ COMPLETED!
  - [x] Compile-time length for string literals
  - [x] HIRGen detects string.length access
  - [x] Create strlen() intrinsic function declaration
  - [x] LLVM CodeGen creates external strlen() declaration
  - [x] Skip external functions in MIR generation
  - [x] LLVM optimizer handles strlen() on constants
  - [x] Runtime strlen for variables with proper string type system ✅

**String Type System Fix - COMPLETED! ✅**
- [x] Fixed proper string type system
  - [x] Added paramTypes field to FunctionDecl AST
  - [x] Updated parser to save parameter type annotations
  - [x] Fixed HIRGen to use parameter type annotations
  - [x] String parameters now use pointer types (ptr) instead of i64
  - [x] Runtime strlen calls work correctly with string parameters
  - [x] Tested and verified with test_string_length_param.ts and test_string_ops.ts

**Template Literals - COMPLETED! ✅**
- [x] Implement template literal interpolation ✅
  - [x] Lexer already scans template literals
  - [x] Parser already parses template parts and expressions
  - [x] Implemented HIR generation for template literals
  - [x] Generate concatenation code using string concatenation
  - [x] Tested with multiple template literal patterns ✅
  - ⚠️  TODO: Convert non-string values to strings (numbers work, need toString())

**Tests Passing**:
- `` `Hello ${name}!` `` ✅
- `` `${a} and ${b}` `` ✅
- `` `Just a string` `` ✅
- Function returning template literal ✅

**String Methods - COMPLETED! ✅**
- [x] Implement basic string methods ✅
  - [x] substring method ✅
  - [x] indexOf method ✅
  - [x] charAt method ✅
- [x] Test string methods ✅

**Tests Passing**:
- `str.substring(0, 5)` returns "Hello" ✅
- `str.indexOf("World")` returns 6 ✅
- `str.charAt(6)` returns "W" ✅
- All methods work with string.length ✅
- Comprehensive test returns 12 (5+6+1) ✅

**Remaining Action Items**:
- [ ] Add more edge case tests
- [x] Document in CHANGELOG.md ✅ COMPLETED!

**Files Modified**:
- ✅ `tests/string_methods_runtime.c` (created)
- ✅ `src/hir/HIRGen.cpp`
- ✅ `src/codegen/LLVMCodeGen.cpp`
- ✅ Multiple test files created

**Success Criteria**:
- String concatenation works: `"a" + "b"`
- Template literals work: `` `Hello ${name}` ``
- String length works: `str.length`
- Test suite passes

---

## 📝 MEDIUM PRIORITY - After Quick Wins (P2)

### 9. Implement Arrow Functions 🟡
**Priority**: P2 - Medium
**Status**: Partially Implemented - Functions compile but not usable as values yet

**What Works**:
```typescript
// Arrow function compiles and generates correct LLVM IR
const add = (a, b) => a + b;  // ✅ Compiles to __arrow_0 function
```

**What Doesn't Work Yet**:
```typescript
const add = (a, b) => a + b;
let result = add(5, 3);  // ❌ Cannot call through variable (no function pointers)
```

**Already Completed**:
- [x] Parser fully supports arrow functions ✅
- [x] Added paramTypes to ArrowFunctionExpr AST ✅
- [x] Parser saves parameter type annotations ✅
- [x] Implemented HIR generation for arrow functions ✅
- [x] Handle implicit return (expression body) ✅
- [x] Handle block body with explicit return ✅
- [x] Arrow functions compile to LLVM IR ✅

**Remaining Action Items**:
- [ ] Implement first-class functions (function pointers/closures)
  - [ ] Add function pointer type to HIR/MIR
  - [ ] Store function references in variables
  - [ ] Enable calling functions through variables
- [ ] Implement lexical `this` binding (future)
- [ ] Add more unit tests
- [ ] Document in CHANGELOG.md

**Current Limitation**:
Arrow functions generate correct LLVM IR functions (e.g., `__arrow_0`) but cannot
be stored in variables or passed around as values. This requires implementing
first-class function support with function pointers.

---

### 8. Implement Classes 🟢
**Priority**: P2 - Medium
**Estimated Time**: 5-7 days

**What's Missing**:
```typescript
class Person {
    name: string;
    constructor(name: string) {
        this.name = name;
    }
    greet(): string {
        return "Hello, " + this.name;
    }
}
```

**Action Items**:
- [ ] Design class representation in HIR
  - [ ] Class declaration
  - [ ] Constructor
  - [ ] Methods
  - [ ] Properties
- [ ] Implement class codegen
  - [ ] Struct for class data
  - [ ] Constructor function
  - [ ] Method functions with `this` parameter
- [ ] Implement object instantiation
  - [ ] Memory allocation
  - [ ] Constructor call
  - [ ] Return instance
- [ ] Test with simple class
- [ ] Test with methods
- [ ] Test with properties
- [ ] Add unit tests
- [ ] Document in CHANGELOG.md

---

### 9. Implement Error Handling (Try/Catch) 🟢
**Priority**: P2 - Medium
**Estimated Time**: 3-5 days

**What's Missing**:
```typescript
try {
    riskyOperation();
} catch (error) {
    handleError(error);
} finally {
    cleanup();
}
```

**Action Items**:
- [ ] Research LLVM exception handling mechanisms
- [ ] Implement try/catch in HIR
- [ ] Implement throw statement
- [ ] Implement exception propagation
- [ ] Implement finally blocks
- [ ] Test with simple try/catch
- [ ] Test with nested try/catch
- [ ] Test with finally blocks
- [ ] Add unit tests
- [ ] Document in CHANGELOG.md

---

## 🔵 LOW PRIORITY - Future Work (P3)

### 10. Implement Async/Await
**Priority**: P3 - Low
**Estimated Time**: 7-10 days

- [ ] Research async runtime requirements
- [ ] Design async transformation
- [ ] Implement Promise types
- [ ] Implement await expressions
- [ ] Implement async functions

---

### 11. Implement Modules (Import/Export)
**Priority**: P3 - Low
**Estimated Time**: 5-7 days

- [ ] Design module system
- [ ] Implement import statements
- [ ] Implement export statements
- [ ] Implement module resolution
- [ ] Link multiple modules

---

### 12. Implement Destructuring
**Priority**: P3 - Low
**Estimated Time**: 4-5 days

- [ ] Array destructuring
- [ ] Object destructuring
- [ ] Nested destructuring
- [ ] Default values in destructuring

---

### 13. Implement Generators
**Priority**: P3 - Low
**Estimated Time**: 5-7 days

- [ ] Generator functions (function*)
- [ ] Yield expressions
- [ ] Iterator protocol
- [ ] Generator state machine

---

## 🧪 Testing & Quality

### Improve Test Coverage
**Ongoing**

- [ ] Add unit tests for all features
- [ ] Create integration tests
- [ ] Add regression tests for fixed bugs
- [ ] Test edge cases
- [ ] Test error conditions
- [ ] Aim for 80%+ code coverage

**Files**:
- `tests/` directory
- Create test files as needed

---

### Improve Error Messages
**Ongoing**

- [ ] Better compiler error messages
- [ ] Source location in errors
- [ ] Helpful suggestions in errors
- [ ] Color-coded error output
- [ ] Warning messages

---

### Add Debug Logging
**Ongoing**

- [ ] Configurable debug levels
- [ ] Per-phase logging
- [ ] Performance profiling
- [ ] Memory usage tracking

---

## 📖 Documentation

### Update Documentation
**Ongoing**

- [ ] Update README.md with current features
- [ ] Keep DEVELOPMENT_STATUS.md updated
- [ ] Update CHANGELOG.md after every change
- [ ] Add code comments and documentation
- [ ] Create API reference
- [ ] Add more examples

---

### Create New Documentation
**As Needed**

- [ ] CONTRIBUTING.md - Contribution guidelines
- [ ] ARCHITECTURE.md - Detailed architecture docs
- [ ] API_REFERENCE.md - Public API documentation
- [ ] EXAMPLES.md - More code examples
- [ ] TROUBLESHOOTING.md - Common issues and solutions

---

## 🛠️ Infrastructure

### Build System Improvements
**Low Priority**

- [ ] Add Linux build support
- [ ] Add macOS build support
- [ ] Improve build speed
- [ ] Add CI/CD pipeline
- [ ] Automated testing on push

---

### Development Tools
**Low Priority**

- [ ] Add formatter configuration
- [ ] Add linter configuration
- [ ] Create development container
- [ ] Add VS Code debug configurations

---

## 📊 Progress Tracking

### Current Sprint: Feature Enhancement (Week 3-4) - v0.7.0
- [ ] Issue #4: Implement array indexing 🟡
- [ ] Issue #5: Complete object property access 🟡
- [ ] Issue #6: Complete string operations 🟡

**Goal**: Reach 70% completion
**Status**: Ready to start - all blockers resolved

---

### Completed Sprint: Bug Fixing (Week 1-2) - v0.6.0 ✅
- [x] Issue #1: Fix loop condition generation ✅
- [x] Issue #2: Fix loop variable scoping ✅
- [x] Issue #3: Fix logical operations ✅
- [x] Update documentation ✅

**Result**: 100% completion - All 15 tests passing!

---

### Future Sprint: Major Features (Week 5-8) - v0.8.0+
- [ ] Issue #7: Implement arrow functions 🟢
- [ ] Issue #8: Implement classes 🟢
- [ ] Issue #9: Implement error handling 🟢

**Goal**: Reach 80% completion

---

## 📝 Notes

### Development Workflow
1. Pick a task from this TODO list
2. Create a feature branch (optional)
3. Work on the task with tests
4. Update CHANGELOG.md
5. Mark task as complete in TODO.md
6. Commit changes with descriptive message
7. Run all tests to verify

### Priority Levels
- 🔴 P0: Critical - Must fix ASAP (NONE currently!)
- 🟡 P1: High - Current sprint
- 🟢 P2: Medium - Next sprint
- 🔵 P3: Low - Future work

### Time Estimates
- Based on complexity and previous experience
- May need adjustment based on actual progress
- Includes time for testing and documentation

### Current Status
- **Version**: v0.6.0
- **Tests Passing**: 15/15 (100%)
- **Performance**: Excellent (~10ms per file)
- **Blockers**: None
- **Ready For**: P1 features (arrays, objects, strings)

---

**Last Updated**: 2025-11-13
**Next Review**: After completing P1 features
**Current Focus**: Arrays, Objects, and String Operations
