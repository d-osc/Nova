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

---

## 🔥 CRITICAL - Must Do Now (P0)

(No critical issues blocking development)

---

## 🎯 HIGH PRIORITY - Next Up (P1)

### 4. Implement Array Indexing 🟡
**Priority**: P1 - High
**Estimated Time**: 3-5 days
**Status**: Ready to start

**What's Missing**:
```typescript
let arr = [1, 2, 3];  // ✅ Works
let x = arr[0];       // ❌ Not implemented
arr[1] = 10;          // ❌ Not implemented
```

**Action Items**:
- [ ] Design array type in HIR
  - [ ] Add ArrayType class
  - [ ] Add array allocation operations
- [ ] Implement array type in MIR
  - [ ] Array type representation
  - [ ] Array access operations
- [ ] Implement array indexing in LLVM codegen
  - [ ] GetElementPtr instructions
  - [ ] Load from array element
  - [ ] Store to array element
- [ ] Add array bounds checking (optional)
- [ ] Test with simple array access
- [ ] Test with array assignment
- [ ] Test with multi-dimensional arrays
- [ ] Add unit tests for arrays
- [ ] Document in CHANGELOG.md

**Files to Create/Modify**:
- `include/nova/HIR/HIR.h` - Add ArrayType
- `src/hir/HIRGen.cpp` - Array operations
- `src/mir/MIRGen.cpp` - Array MIR generation
- `src/codegen/LLVMCodeGen.cpp` - Array LLVM codegen
- `tests/test_arrays.ts` - Array tests

**Success Criteria**:
- Can read array elements: `arr[0]`
- Can write array elements: `arr[0] = 5`
- Bounds checking works (optional)
- Test suite passes

---

### 5. Complete Object Property Access 🟡
**Priority**: P1 - High
**Estimated Time**: 3-5 days
**Status**: Ready to start

**What's Missing**:
```typescript
let obj = {name: "John", age: 30};  // ✅ Works
let name = obj.name;                // ⚠️ Limited
obj.age = 31;                       // ❌ Not implemented
```

**Action Items**:
- [ ] Review current object literal implementation
- [ ] Implement property access in HIR
  - [ ] PropertyAccess operation
  - [ ] Track property names/indices
- [ ] Implement property access in MIR
  - [ ] Struct field access
  - [ ] Property offset calculation
- [ ] Implement property access in LLVM codegen
  - [ ] GetElementPtr for struct fields
  - [ ] Load from property
  - [ ] Store to property
- [ ] Test with simple property access
- [ ] Test with nested objects
- [ ] Test with property assignment
- [ ] Add unit tests
- [ ] Document in CHANGELOG.md

**Files to Modify**:
- `src/hir/HIRGen.cpp`
- `src/mir/MIRGen.cpp`
- `src/codegen/LLVMCodeGen.cpp`
- `tests/test_objects.ts`

**Success Criteria**:
- Can read object properties: `obj.name`
- Can write object properties: `obj.name = "Jane"`
- Nested objects work: `obj.child.value`
- Test suite passes

---

### 6. Complete String Operations 🟡
**Priority**: P1 - High
**Estimated Time**: 3-4 days
**Status**: Ready to start

**What's Missing**:
```typescript
let s1 = "Hello";              // ✅ Works
let s2 = s1 + " World";        // ⚠️ Limited
let s3 = `Hello ${name}`;      // ❌ Not implemented
let len = s1.length;           // ❌ Not implemented
```

**Action Items**:
- [ ] Implement string concatenation
  - [ ] Runtime function for string concat
  - [ ] Memory allocation for result
  - [ ] Copy strings into result
- [ ] Implement template literal interpolation
  - [ ] Parse template parts and expressions
  - [ ] Generate concatenation code
  - [ ] Convert non-string values to strings
- [ ] Implement basic string methods
  - [ ] length property
  - [ ] substring method
  - [ ] indexOf method
- [ ] Test string concatenation
- [ ] Test template literals
- [ ] Test string methods
- [ ] Add unit tests
- [ ] Document in CHANGELOG.md

**Files to Modify**:
- `src/runtime/string.cpp` (create)
- `src/hir/HIRGen.cpp`
- `src/mir/MIRGen.cpp`
- `src/codegen/LLVMCodeGen.cpp`
- `tests/test_strings.ts`

**Success Criteria**:
- String concatenation works: `"a" + "b"`
- Template literals work: `` `Hello ${name}` ``
- String length works: `str.length`
- Test suite passes

---

## 📝 MEDIUM PRIORITY - After Quick Wins (P2)

### 7. Implement Arrow Functions 🟢
**Priority**: P2 - Medium
**Estimated Time**: 2-3 days

**What's Missing**:
```typescript
const add = (a, b) => a + b;           // ❌ Not implemented
const greet = name => `Hello ${name}`; // ❌ Not implemented
```

**Action Items**:
- [ ] Parser already supports arrow functions
- [ ] Implement arrow function in HIR generation
- [ ] Handle implicit return (expression body)
- [ ] Handle block body with explicit return
- [ ] Implement lexical `this` binding (future)
- [ ] Test with simple arrow functions
- [ ] Test with complex arrow functions
- [ ] Add unit tests
- [ ] Document in CHANGELOG.md

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
