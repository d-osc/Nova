# Default Parameters Bug Analysis

## Summary

Investigated the issue where **default parameters with string values produce garbage** instead of the actual string value.

## Root Cause

The bug is NOT specific to default parameters - it's a fundamental issue with **passing string constants as function arguments**.

### Test Cases

**test_default_number.js** (WORKS):
```javascript
function add(a, b = 10) {
    console.log("a:", a, "b:", b);
    return a + b;
}
console.log(add(7));
// Output: "a: 7 b: 10" and "17" ✓ CORRECT
```

**test_default_debug.js** (FAILS):
```javascript
function greet(name, greeting = "Hello") {
    console.log(greeting, name);
}
greet("Bob");
// Output: "6.9514e-310 6.9514e-310" ✗ GARBAGE
```

**test_string_arg.js** (FAILS):
```javascript
function test(greeting) {
    console.log("Greeting:", greeting);
}
test("Hello");
// Output: "Greeting: 6.9514e-310" ✗ GARBAGE
```

## Technical Analysis

### Pipeline Stages

1. **AST → HIR** (src/hir/HIRGen.cpp): ✓ Correctly creates `HIRConstant` for strings
2. **HIR → MIR** (src/mir/MIRGen.cpp): ✓ Correctly translates to `MIRConstant`
3. **MIR → LLVM** (src/codegen/LLVMCodeGen.cpp): ✗ **ISSUE HERE**

### The Problem

1. Function parameters without type annotations default to HIR type `Any`
2. MIRGen translates `Any` type to `I64` (64-bit integer)
3. LLVM creates function parameters as `i64` type
4. String constants are pointers (`ptr` type) in LLVM
5. When passing `ptr` string to `i64` parameter:
   - Pointer address gets reinterpreted as an integer value
   - console.log sees the integer and interprets it as double
   - Result: garbage value like `6.9514e-310`

### Code Locations

**src/mir/MIRGen.cpp:139** - `Any` type mapped to `I64`:
```cpp
case hir::HIRType::Kind::Any:
    return std::make_shared<MIRType>(MIRType::Kind::I64);
```

**src/mir/MIRGen.cpp:830-834** - String constants correctly created:
```cpp
case hir::HIRConstant::Kind::String: {
    std::string strValue = std::get<std::string>(constant->value);
    return builder_->createStringConstant(strValue, mirType);
}
```

## Attempted Solution

The nova-compiler-architect agent attempted to fix this by:
1. Changing `Any` type from `I64` to `Void` (which maps to `ptr`)
2. Converting function parameters to `ptr` instead of `i64`
3. Adding runtime type detection in console.log

**Result**: Partially worked for strings but broke number handling, as numbers also became pointers.

## Fundamental Issue

**Nova lacks runtime type tagging for dynamic typing**. JavaScript engines solve this with:
- Tagged unions (e.g., `struct { Type tag; union { int64_t i; double d; void* ptr; } value; }`)
- Every dynamically-typed value carries its type at runtime
- Functions can inspect the tag to determine actual type

## Recommended Solutions

### Option 1: Quick Fix (Band-aid)
- Keep function parameters as `i64`
- Special-case string constants at call sites to wrap them in objects
- Store strings in a way that fits in `i64` (pointer as integer)
- Maintains backward compatibility but doesn't solve the general problem

### Option 2: Proper Dynamic Typing (Recommended)
- Implement `NovaValue` struct with type tag + union
- All dynamically-typed values use this struct
- Runtime functions inspect tag to determine type
- This is the industry-standard approach (V8, SpiderMonkey, etc.)

## Current Status

- **Reverted experimental changes** to maintain stability
- **Root cause fully understood**
- **Solution requires architectural decision** on how to handle dynamic typing

## Impact

- Default parameters work with **numbers** ✓
- Default parameters fail with **strings** ✗
- Passing string constants to any function fails ✗
- This affects JavaScript compatibility significantly
