# Nova Compiler - TODO List

> **Last Updated**: 2025-11-12
> **Current Sprint**: Feature Implementation Phase
> **Focus**: Implement remaining TypeScript/JavaScript features

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

**Tests**:
- ✅ test_while_simple.ts - Returns 5 (correct)
- ✅ test_for_simple.ts - Returns 10 (correct)

---

## 🔥 CRITICAL - Must Do Now (P0)

(No critical issues blocking development)

---

### 3. Fix Logical Operations (Short-Circuit) 🟡
**Priority**: P1 - High (but after P0)
**Estimated Time**: 2-3 days
**Assigned To**: After Issues #1 and #2

**Problem**:
```typescript
if (x > 0 && y < 10) { }  // Both sides always evaluated
if (a || b) { }            // Both sides always evaluated
```

**Action Items**:
- [ ] Review current logical operation codegen
- [ ] Implement short-circuit evaluation for AND (&&)
  - [ ] Create intermediate basic block for second condition
  - [ ] Only evaluate second condition if first is true
- [ ] Implement short-circuit evaluation for OR (||)
  - [ ] Create intermediate basic block for second condition
  - [ ] Only evaluate second condition if first is false
- [ ] Test with simple AND expressions
- [ ] Test with simple OR expressions
- [ ] Test with complex chained expressions
- [ ] Add unit tests for short-circuit behavior
- [ ] Document the fix in CHANGELOG.md

**Files to Modify**:
- `src/codegen/ExprGen.cpp` (or relevant file)
- `tests/test_logical_ops.ts` (create)

**Success Criteria**:
- AND operations short-circuit correctly
- OR operations short-circuit correctly
- Side effects only happen when appropriate
- Test suite passes

---

## 🎯 HIGH PRIORITY - Next Up (P1)

### 4. Implement Array Indexing 🟡
**Priority**: P1 - High
**Estimated Time**: 3-5 days
**Blocked By**: Issues #1, #2

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
**Blocked By**: Issues #1, #2

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
**Blocked By**: Issues #1, #2

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

### Current Sprint: Bug Fixing (Week 1-2)
- [ ] Issue #1: Fix loop condition generation 🔴
- [ ] Issue #2: Fix loop variable scoping 🔴
- [ ] Issue #3: Fix logical operations 🟡

**Goal**: Reach 55% completion

---

### Next Sprint: High-Value Features (Week 3-4)
- [ ] Issue #4: Implement array indexing
- [ ] Issue #5: Complete object property access
- [ ] Issue #6: Complete string operations

**Goal**: Reach 65% completion

---

### Future Sprint: Major Features (Week 5-8)
- [ ] Issue #7: Implement arrow functions
- [ ] Issue #8: Implement classes
- [ ] Issue #9: Implement error handling

**Goal**: Reach 75% completion

---

## 📝 Notes

### Development Workflow
1. Pick a task from this TODO list
2. Update DEVELOPMENT_STATUS.md with status change
3. Work on the task
4. Write tests
5. Update CHANGELOG.md
6. Mark task as complete in TODO.md
7. Commit changes

### Priority Levels
- 🔴 P0: Critical - Must fix ASAP
- 🟡 P1: High - Next sprint
- 🟢 P2: Medium - After quick wins
- 🔵 P3: Low - Future work

### Time Estimates
- Based on complexity and dependencies
- May need adjustment based on actual progress
- Includes time for testing and documentation

---

**Last Updated**: 2025-11-12
**Next Review**: After completing P0 issues
**Current Focus**: Fix critical loop bugs
