# Array Methods Implementation Status

**Date**: 2025-11-21
**Version**: v0.26.1 (in progress)

## Problem Discovery

### Initial Issue
- test_array_methods.ts returns 60 instead of expected 105
- test_array_push_pop.ts returns 60 instead of expected 100
- Array methods (push, pop, shift, unshift) appear not to be working

### Root Cause Analysis

**Fundamental Type Mismatch**: Nova compiler uses two incompatible array representations:

1. **Compile-Time Arrays** (what compiler generates):
   - Type: `[N x i64]` - fixed-size stack-allocated arrays
   - Created by array literals: `let arr = [10, 20, 30]`
   - Direct value storage (not pointers)
   - Works for indexing: `arr[0]`, `arr[1]`, etc.

2. **Runtime Arrays** (what runtime library expects):
   - Type: `Array*` struct with `void** elements`
   - Designed for dynamic operations
   - Pointer-based element storage
   - Defined in src/runtime/Array.cpp

### Test Results

```typescript
// test_array_simple_access.ts - Simple array access without methods
let arr = [10, 20, 30];
return arr[0] + arr[1] + arr[2];  // Returns 60 ✅ WORKS
```

```typescript
// test_array_push_pop.ts - Using array methods
let arr = [10, 20, 30];
arr.push(40);
let last = arr.pop();
return arr[0] + arr[1] + arr[2] + last;  // Returns 60 ❌ FAILS (expected 100)
```

**Analysis**: The 60 result shows only `arr[0] + arr[1] + arr[2]` is calculated correctly. The `last` variable from `pop()` contributes 0 or garbage, indicating method calls don't work.

## Implementation Attempts

### Attempt 1: Add Runtime Functions
**Files Modified**:
- `include/nova/runtime/Runtime.h` - Added shift/unshift declarations
- `src/runtime/Array.cpp` - Implemented shift/unshift C++ functions
- `src/runtime/Array.cpp` - Added extern "C" wrappers (nova_array_push, nova_array_pop, etc.)

**Result**: Functions compile and link, but crash at runtime

### Attempt 2: Add Compiler Support
**Files Modified**:
- `src/hir/HIRGen.cpp` - Added array method detection in CallExpr visitor (lines 406-492)
- Detects array method calls (push, pop, shift, unshift)
- Creates external function declarations
- Generates calls to runtime functions

**Result**: Compiler detects methods correctly, but generated code doesn't work

### Attempt 3: Debug Generated Code
**Finding**: With optimizations (-O2), all code gets eliminated → `ret i64 undef`
**Finding**: Without optimizations (-O0), code runs but returns wrong value (67 instead of 100)

**LLVM IR Analysis**:
```llvm
%array = alloca [3 x i64], align 8          ; Stack array of i64
; ... array methods try to use this as Array* ...
%ptr_to_int = ptrtoint ptr %load39 to i64   ; Converting pointer to int
```

The methods return `void*` (pointers) but code expects `i64` (values).

## The Core Problem

**Stack arrays `[N x i64]` cannot be passed to runtime methods expecting `Array*` struct.**

When you write:
```typescript
let arr = [10, 20, 30];  // Creates [3 x i64] on stack
arr.push(40);             // Tries to call runtime expecting Array*
```

The compiler:
1. Allocates `[3 x i64]` on stack
2. Tries to pass it to `nova_array_push(Array*, void*)`
3. Type mismatch causes undefined behavior

## Required Solution

Need to bridge the two representations. Options:

### Option A: Implicit Conversion (Recommended)
When array methods are called, automatically convert stack array to heap Array:
```cpp
// Before calling method:
Array* heap_arr = create_array_from_stack([3 x i64]* stack_arr, length);
// Call method on heap array
// Optionally sync back to stack array
```

**Pros**: Preserves both representations, minimal code changes
**Cons**: Performance overhead, complexity in sync logic

### Option B: Use Heap Arrays for All Literals
Change compiler to always create heap Array structs for literals:
```cpp
let arr = [10, 20, 30];
// Generates: Array* arr = create_array(3); arr->elements[0] = 10; ...
```

**Pros**: Clean, consistent, methods work naturally
**Cons**: Performance impact, breaks existing stack array code

### Option C: Implement Stack Array Methods
Create separate method implementations for stack arrays:
```cpp
i64 nova_stack_array_pop([N x i64]* arr, i64* length);
```

**Pros**: No heap allocation, fast
**Cons**: Limited (no dynamic resize), code duplication

## Next Steps

1. **Design Decision**: Choose which option to implement
2. **Implement Conversion**: Add array representation bridging
3. **Update Tests**: Verify all array methods work correctly
4. **Update Docs**: Document array behavior and limitations

## Current Status

**Arrays**:
- ✅ Literals work
- ✅ Indexing works
- ✅ Length property works
- ❌ push() - not working
- ❌ pop() - not working
- ❌ shift() - not implemented & not working
- ❌ unshift() - not implemented & not working

**Estimated Fix Complexity**: Medium-High
**Estimated Time**: 2-4 hours

---

**Files Modified So Far**:
- include/nova/runtime/Runtime.h
- src/runtime/Array.cpp
- src/hir/HIRGen.cpp

**Build Status**: ✅ Compiles successfully
**Runtime Status**: ❌ Array methods crash or return wrong values
