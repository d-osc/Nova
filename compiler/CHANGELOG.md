# Nova Compiler - Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Known Issues
- 🟡 Arrow functions compile but cannot be used as first-class values (requires function pointers)
- 🟡 Template literals need toString() conversion for non-string values

---

## [0.8.0] - 2025-11-14

### Added - Classes (Full OOP Support!) 🎉
- ✅ **Complete class implementation with properties and methods**
  - Class declarations with typed properties
  - Constructors with automatic memory allocation
  - Instance methods with proper `this` binding
  - Method calls on object instances (`obj.method()`)
  - Property access and assignment (`this.property`, `this.property = value`)
  - Multiple instances with isolated memory
  - Full memory management using malloc

### Implementation Details
**HIR Generation (`src/hir/HIRGen.cpp`)**:
- Added `currentClassStructType_` tracking for class context
- Enhanced `AssignmentExpr` visitor for property assignment
- Enhanced `MemberExpr` visitor for property access
- Modified constructor/method generation to save/restore class context
- `NewExpr` attaches struct type to instance values
- `CallExpr` detects and handles method calls with name mangling

**LLVM Code Generation (`src/codegen/LLVMCodeGen.cpp`)**:
- Automatic struct type generation for each class
- malloc external function declaration
- Pointer casting (inttoptr/ptrtoint) for GEP operations
- Type propagation for malloc results through temp allocas
- Associate struct types with method `this` parameters

### Generated LLVM IR Example
```llvm
%struct.Person = type { i64, i64 }

define i64 @Person_constructor(i64 %arg0, i64 %arg1) {
  %0 = call ptr @malloc(i64 16)
  store i64 %arg0, ptr %0, align 4
  %ptr = getelementptr %struct.Person, ptr %0, i64 0, i32 1
  store i64 %arg1, ptr %ptr, align 4
  %result = ptrtoint ptr %0 to i64
  ret i64 %result
}

define i64 @Person_getAge(i64 %this) {
  %ptr = inttoptr i64 %this to ptr
  %field_ptr = getelementptr %struct.Person, ptr %ptr, i64 0, i32 1
  %value = load i64, ptr %field_ptr, align 4
  ret i64 %value
}
```

### Tests
- ✅ test_class_simple.ts - Basic class with constructor and method (returns 30)
- ✅ test_class_comprehensive.ts - Multiple instances and methods (returns 71)
- Both tests compile to executable and run correctly!

### Technical Achievements
- Classes compile to clean, verifiable LLVM IR
- Proper memory allocation and deallocation support
- Type-safe property access with GEP instructions
- Method name mangling: `ClassName_methodName`
- Instance pointers passed as i64 for compatibility
- Full pointer conversion infrastructure (i64 ↔ ptr)

### Breaking Changes
- None - fully backward compatible

---

## [0.7.5] - 2025-11-14

### Added
- ✅ **String methods support**
  - `str.substring(start, end)` - Extract substring with bounds checking
  - `str.indexOf(searchStr)` - Find index of substring (-1 if not found)
  - `str.charAt(index)` - Get single character at index
  - All methods integrate with existing string.length property
- ✅ **Runtime string method implementations**
  - Added `string_methods_runtime.c` with C implementations
  - Proper memory management and edge case handling
  - External function declarations in LLVM IR

### Changed
- Enhanced `HIRGen.cpp` to detect string method calls in CallExpr
- Enhanced `LLVMCodeGen.cpp` to auto-create external declarations for string methods
- String methods pattern: `str.methodName(...)` converts to runtime function calls

### Tests
- ✅ test_string_substring.ts - Substring extraction
- ✅ test_string_methods.ts - Individual method tests
- ✅ test_string_methods_all.ts - Comprehensive test (returns 12)

### Technical Details
**Method Signatures**:
```c
const char* nova_string_substring(const char* str, int64_t start, int64_t end);
int64_t nova_string_indexOf(const char* str, const char* searchStr);
const char* nova_string_charAt(const char* str, int64_t index);
```

**Generated LLVM IR**:
```llvm
declare ptr @nova_string_substring(ptr, i64, i64)
declare i64 @nova_string_indexOf(ptr, ptr)
declare ptr @nova_string_charAt(ptr, i64)
```

---

## [0.7.3] - 2025-11-14

### Added
- ✅ **Template literals support**
  - Full template literal interpolation: `` `Hello ${name}!` ``
  - Multiple expressions: `` `${a} and ${b}` ``
  - String-only templates: `` `Just a string` ``
  - Nested in function returns
- ✅ **Template literal compilation**
  - Converts template literals to string concatenation
  - Uses existing string concatenation infrastructure
  - Handles multiple quasis and expressions

### Changed
- Implemented HIR generation for TemplateLiteralExpr in `HIRGen.cpp`
- Template literals compile to series of Add operations with string constants

### Tests
- ✅ test_template_literal.ts - Basic template literal test
- ✅ test_template_comprehensive.ts - Multiple template patterns

### Technical Details
**Example Transformation**:
```typescript
`Hello ${name}!`
```
Becomes:
```cpp
"Hello " + name + "!"
```

---

## [0.7.2] - 2025-11-14

### Added
- ✅ **Arrow function parsing and compilation**
  - Multi-parameter arrow functions: `(a, b) => a + b`
  - Single parameter: `x => x * 2`
  - Expression bodies with implicit return
  - Block bodies with explicit return
  - Parameter type annotations preserved

### Changed
- Added `paramTypes` field to ArrowFunctionExpr AST
- Updated parser to save parameter type annotations in all arrow function cases
- Implemented HIR generation for arrow functions (compiles to `__arrow_N` functions)

### Known Limitations
- Arrow functions compile to LLVM functions but cannot be stored in variables or passed as arguments
- Requires first-class function support (function pointers) not yet implemented

### Tests
- ✅ test_arrow_simple.ts - Arrow function compilation test
- ✅ test_arrow_immediate.ts - IIFE pattern test

---

## [0.7.1] - 2025-11-13

### Added
- ✅ **Array element assignment**
  - Can now write to array elements: `arr[0] = 42`
  - Proper GEP instruction generation for array element pointers
  - Works with runtime indices
- ✅ **Object property assignment**
  - Can now write to object properties: `obj.x = 42`
  - SetField operation in HIR/MIR
  - Proper struct field pointer calculation
- ✅ **Nested object access**
  - Deep property access: `obj.child.grandchild.value`
  - Nested struct type tracking with `nestedStructTypeMap`
  - Type information preserved through variable copies

### Changed
- Enhanced `HIRGen.cpp` to handle AssignmentExpr for member expressions
- Added SetField operation to HIR/MIR
- Fixed GEP generation to use loaded struct values instead of alloca pointers
- Added `nestedStructTypeMap` to LLVMCodeGen for tracking nested struct types

### Fixed
- ✅ Fixed GEP bug where struct pointer was used instead of loaded value
- ✅ Fixed nested object type propagation in variable assignments

### Tests
- ✅ test_object_assign.ts - Object property assignment
- ✅ test_array_assign.ts - Array element assignment
- ✅ test_nested_obj.ts - Nested object access

---

## [0.7.0] - 2025-11-13

### Added
- ✅ **Array literals and indexing**
  - Array literal syntax: `[1, 2, 3]`
  - Array indexing: `arr[0]`
  - HIRArrayType for array types
  - MIRAggregateRValue for array construction
  - MIRGetElementRValue for array access
  - LLVM GEP instructions for element access
- ✅ **Object literals and property access**
  - Object literal syntax: `{x: 10, y: 20}`
  - Property access: `obj.x`
  - HIRStructType for object types
  - Struct construction in MIR
  - Field access with GetField operation
  - LLVM struct types and GEP for fields
- ✅ **String concatenation**
  - String concatenation operator: `"a" + "b"`
  - Runtime function: `nova_string_concat_cstr`
  - Proper memory allocation and string copying
- ✅ **String.length property**
  - Compile-time length for string literals: `"Hello".length`
  - Runtime length for string variables: `str.length`
  - Uses `strlen()` intrinsic function
  - LLVM optimizer handles constant folding

### Changed
- Enhanced `HIRGen.cpp` with array, object, and string support
- Enhanced `MIRGen.cpp` with aggregate and element operations
- Enhanced `LLVMCodeGen.cpp` with struct and array code generation
- Fixed critical object slicing bug in type preservation
- Added external function declarations for strlen

### Fixed
- ✅ Fixed object type slicing in MIR variable assignment
- ✅ Fixed string parameter type handling (ptr instead of i64)
- ✅ Added paramTypes field to FunctionDecl AST for type annotations

### Tests
- ✅ test_array_simple.ts - Array literal and indexing
- ✅ test_object_simple.ts - Object literal and property access
- ✅ test_string_concat.ts - String concatenation
- ✅ test_string_length.ts - String length property
- ✅ test_string_length_param.ts - String length with parameters
- ✅ test_string_ops.ts - Comprehensive string operations

### Technical Details
**Array Example**:
```typescript
let arr = [1, 2, 3];
let x = arr[0];  // Returns 1
```

**Object Example**:
```typescript
let obj = {x: 10, y: 20};
let x = obj.x;  // Returns 10
```

**String Example**:
```typescript
let s1 = "Hello";
let s2 = " World";
let s3 = s1 + s2;  // "Hello World"
let len = s3.length;  // 11
```

---

## [0.60.0] - 2025-11-13

### Added
- ✅ **Strict equality operators**
  - `===` operator (strict equal)
  - `!==` operator (strict not equal)
  - Proper comparison with type checking
- ✅ **Short-circuit evaluation for logical operators**
  - `&&` operator with proper short-circuiting
  - `||` operator with proper short-circuiting
  - Creates separate basic blocks for left and right operands
  - Only evaluates right side if necessary

### Changed
- Enhanced `HIRGen.cpp` to handle StrictEqual and StrictNotEqual cases
- Implemented proper short-circuit evaluation using basic blocks and phi nodes

### Tests
- ✅ All 15/15 tests passing (100%)
- ✅ test_logical_ops.ts - Comprehensive logical operation test
- ✅ test_and_direct.ts - Short-circuit AND test
- ✅ test_or_direct.ts - Short-circuit OR test

### Technical Details
**Short-circuit Example**:
```typescript
let result = x && y;  // Only evaluates y if x is truthy
```

Generated LLVM IR creates conditional branches to handle short-circuiting properly.

---

## [0.51.0] - 2025-11-12

### Fixed 🎉
- ✅ **CRITICAL: Fixed for loop compilation - void type handling bug**
  - Root cause: LLVM cannot create alloca or load instructions for void type
  - Fixed alloca creation to use i64 placeholder for void-typed intermediate values
  - Fixed load instruction to use alloca's actual type instead of MIR's void type
  - For loops now generate correct LLVM IR with proper phi nodes
- ✅ **For loops now work correctly with runtime conditions**
  - Loop variables (i, sum) properly tracked through phi nodes
  - Loop condition uses actual runtime value (not hardcoded)
  - Clean optimized LLVM IR generation

### Changed
- Enhanced void type handling in `LLVMCodeGen.cpp`
  - Alloca creation: void types replaced with i64 placeholder
  - Load instructions: use alloca type when MIR type is void
- Improved error detection and recovery for type mismatches

### Tests
- ✅ test_while_simple.ts - While loop with counter (PASSING - returns 5)
- ✅ test_for_simple.ts - For loop with accumulator (PASSING - returns 10)
- Both loops generate clean, optimized LLVM IR

### Technical Details
**Issue**: For loop compilation hung during alloca creation, then segfaulted on load

**Root Cause**: Void-typed intermediate values in MIR cannot be used with LLVM's alloca or load instructions

**Solution**:
1. During alloca creation - replace void type with i64
2. During load - check if MIR type is void but alloca is not, use alloca's type

**Generated IR** (for loop):
```llvm
bb2:                                              ; preds = %bb3, %entry
  %var1.0 = phi i64 [ 0, %entry ], [ %add11, %bb3 ]  ; i
  %var.0 = phi i64 [ 0, %entry ], [ %add, %bb3 ]     ; sum
  %lt = icmp slt i64 %var1.0, 5                      ; i < 5
  br i1 %lt, label %bb3, label %bb5                  ; Runtime condition!

bb3:                                              ; preds = %bb2
  %add = add i64 %var.0, %var1.0                     ; sum = sum + i
  %add11 = add i64 %var1.0, 1                         ; i = i + 1
  br label %bb2
```

---

## [0.50.0] - 2025-11-12

### Fixed 🎉
- ✅ **CRITICAL: Fixed while loop condition generation bug**
  - Root cause: MIR assigned pointer types to number variables instead of i64
  - Fixed `MIRGen.cpp:getOrCreatePlace()` to extract pointee type from pointer types
  - Variables now have correct types throughout the compilation pipeline
- ✅ **Fixed return type mismatch for boolean values**
  - Added i1 → i64 conversion in return generation
  - Functions can now return comparison results correctly
- ✅ **While loops now work correctly with runtime conditions**
  - Loop conditions use actual runtime values (not hardcoded `br i1 true`)
  - No unnecessary `ptrtoint` conversions
  - LLVM optimizer generates proper phi nodes and SSA form

### Changed
- Improved type handling in MIR generation for alloca instructions
- Better debug logging for type conversions

### Tests
- ✅ test_while_simple.ts - While loop with counter (PASSING)
- Generated LLVM IR is clean and optimized

### Technical Details
**Before Fix**:
```llvm
%var3 = alloca ptr              ← Wrong type!
%load = load ptr, ptr %var3
%ptr_to_int = ptrtoint ptr...   ← Unnecessary conversion
```

**After Fix**:
```llvm
%var3.0 = phi i64 [0, %entry], [%add, %bb2]  ← Perfect!
%lt = icmp slt i64 %var3.0, 5                  ← Direct comparison
br i1 %lt, label %bb2, label %bb3             ← Runtime condition
```

---

## [0.45.0] - 2025-11-12

### Analysis & Documentation
- ✅ Completed comprehensive TypeScript/JavaScript support analysis
- ✅ Identified overall completion at 45%
- ✅ Documented 3 critical bugs in loop implementation
- ✅ Created detailed feature support matrix
- ✅ Created priority roadmap for next 3-4 months
- ✅ Created DEVELOPMENT_STATUS.md as living document
- ✅ Created CHANGELOG.md for tracking changes
- ✅ Created TODO.md for actionable tasks

### Findings
- Parser: 90% complete (excellent)
- Code Generation: 45% complete (needs work)
- Architecture: Solid and extensible
- Critical bugs found in loop implementation

---

## [0.40.0] - 2025-11-06

### Added
- ✅ AOT (Ahead-of-Time) compilation system
- ✅ Native executable generation (Windows)
- ✅ Program execution with result capture
- ✅ Complete comparison operators (==, !=, <, <=, >, >=)
- ✅ Full compilation pipeline: Nova → AST → HIR → MIR → LLVM IR → Executable

### Documentation
- ✅ Created EXECUTION_IMPLEMENTATION.md
- ✅ Updated test suite with execution tests

### Known Issues
- ⚠️ Loops not working correctly (conditions and scoping issues)

---

## [0.35.0] - 2025-11-05

### Added
- ✅ If/else statement support
- ✅ Break and continue statements (partial - has bugs)
- ✅ Comparison operations with type conversion fixes
- ✅ Variable assignment and mutation

### Fixed
- ✅ Type conversion issues in comparison operations
- ✅ Pointer/integer type handling in LLVM IR

### Documentation
- ✅ Created BREAK_CONTINUE_IMPLEMENTATION.md
- ✅ Updated FEATURE_STATUS.md

### Known Issues
- 🐛 While loops fail to compile (IR generation issues)
- 🐛 For loops fail to compile (IR generation issues)
- 🐛 Break/continue generate incorrect IR

---

## [0.30.0] - 2025-11-04

### Added
- ✅ Complete LLVM IR generation pipeline
- ✅ MIR (Mid-Level IR) generation
- ✅ HIR (High-Level IR) generation
- ✅ Basic optimization passes
- ✅ SSA-based value mapping

### Fixed
- ✅ Function call implementation
- ✅ Return value tracking across basic blocks
- ✅ Type conversion from dynamic to static types

### Tests
- ✅ 7/7 tests passing
- ✅ Zero LLVM verification errors
- ✅ All generated IR valid and executable

---

## [0.25.0] - 2025-11-03

### Added
- ✅ Function declarations and calls
- ✅ Arithmetic operations (+, -, *, /, %)
- ✅ Variable declarations (let, const, var)
- ✅ Return statements
- ✅ Expression statements
- ✅ Block statements

### Compiler Pipeline
- ✅ Complete lexer/tokenizer (63 keywords, 65+ operators)
- ✅ Complete parser (28+ expression types)
- ✅ Complete AST generation
- ✅ Visitor pattern implementation

---

## [0.20.0] - 2025-11-02

### Added
- ✅ Initial parser implementation
- ✅ Token definitions (all TypeScript/JavaScript tokens)
- ✅ AST node definitions
- ✅ Basic expression parsing
- ✅ Statement parsing

### Architecture
- ✅ Clean separation of compilation phases
- ✅ Type-safe C++20 implementation
- ✅ LLVM 18.1.7 integration
- ✅ CMake build system

---

## [0.10.0] - 2025-11-01

### Added
- ✅ Project structure setup
- ✅ Initial lexer implementation
- ✅ Basic token recognition
- ✅ Build system configuration

### Infrastructure
- ✅ CMake configuration
- ✅ LLVM integration setup
- ✅ Test framework setup
- ✅ Documentation structure

---

## Development History Summary

### Phase 1: Foundation (v0.10 - v0.20)
- Set up project structure
- Implemented lexer and parser
- Defined AST nodes
- Established build system

### Phase 2: Core Features (v0.20 - v0.30)
- Implemented basic language features
- Created IR generation pipeline
- Added function support
- Implemented arithmetic operations

### Phase 3: Control Flow (v0.30 - v0.40)
- Added if/else statements
- Attempted loop implementations (found bugs)
- Added comparison operators
- Improved type handling

### Phase 4: Execution (v0.40 - v0.45)
- Implemented AOT compilation
- Added native executable generation
- Created execution system
- Comprehensive analysis and documentation

---

## Upcoming Releases

### [0.50.0] - Target: 2-3 weeks
**Focus**: Fix critical loop bugs

#### Planned
- [ ] Fix loop condition generation bug
- [ ] Fix loop variable scoping bug
- [ ] Fix logical operations (short-circuit)
- [ ] Add comprehensive loop tests
- [ ] Test with complex scenarios

---

### [0.60.0] - Target: 4-6 weeks
**Focus**: High-value features

#### Planned
- [ ] Implement array indexing
- [ ] Complete object property access
- [ ] Complete string operations
- [ ] Improve error messages
- [ ] Add more unit tests

---

### [0.75.0] - Target: 8-10 weeks
**Focus**: Major features

#### Planned
- [ ] Implement classes
- [ ] Implement arrow functions
- [ ] Implement error handling (try/catch)
- [ ] Standard library basics
- [ ] Production-ready quality

---

### [1.0.0] - Target: 3-4 months
**Focus**: Complete language support

#### Planned
- [ ] Implement async/await
- [ ] Implement modules (import/export)
- [ ] Implement destructuring
- [ ] Implement generators
- [ ] Complete standard library
- [ ] Full TypeScript/JavaScript compatibility (90%+)

---

## Version Numbering Guide

- **Major (X.0.0)**: Complete milestones (v1.0.0 = 90% feature complete)
- **Minor (0.X.0)**: New features or significant changes
- **Patch (0.0.X)**: Bug fixes and minor improvements

---

## Legend

- ✅ Completed and working
- ⚠️ Partially implemented
- 🔴 Critical bug/issue
- 🟡 Medium priority issue
- 🟢 Low priority issue
- 🔵 Future enhancement
- ❌ Not implemented
- 🐛 Known bug
- 📝 Documentation
- 🧪 Test-related

---

**Last Updated**: 2025-11-14
**Current Version**: v0.7.5
**Next Release**: v0.8.0 (Classes or complete arrow functions)
