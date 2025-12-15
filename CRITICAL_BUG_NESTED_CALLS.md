# CRITICAL BUG: Nested Function Calls Cause Segmentation Fault

## Summary
The Nova compiler has a **critical architectural bug** where using a function call's result directly as an argument to another function causes a segmentation fault (exit code -1073741819).

## Reproduction

```javascript
function inner() {
    return 42;
}

function outer(x) {
    console.log(x);
}

//  This crashes with segfault:
outer(inner());
```

## Root Cause Analysis

### Investigation Summary (2025-12-09)

1. **HIRGen**: Correctly creates Call instructions for both `inner()` and `outer(inner())`
2. **MIRGen**: Now correctly identifies Call instructions used as operands (after fix)
   - The `translateOperand` function was enhanced to detect Call instructions
   - It successfully finds the result place for already-processed calls
3. **LLVM IR Generation**: The nested call **does not appear** in generated LLVM IR
   - For template literals: `nova_i64_to_string(5)` call is completely missing
   - For nested calls: The inner call is missing, replaced with invalid pointer
4. **Runtime**: Segmentation fault when executing the generated code

### Technical Details

The issue is in the **MIR → LLVM IR** translation phase (LLVMCodeGen):
- MIR Call terminators create continuation basic blocks
- When a Call result is used as an operand to another Call, the first Call instruction is not properly emitted to LLVM IR
- The LLVM IR shows `ptr null` or invalid values instead of the call result

### Code Locations

- **MIRGen fix applied**: `src/mir/MIRGen.cpp` lines 847-874 (translateOperand function)
- **Issue location**: `src/codegen/LLVMCodeGen.cpp` (Call terminator handling)
- **Affected features**: Template literals (which use `nova_i64_to_string`), any nested function calls

## Impact

### Features Blocked
- ❌ Template literal interpolation (`` `Value: ${x}` ``)
- ❌ Chained function calls (e.g., `foo(bar())`)
- ❌ Any pattern using call results as arguments

### Features Working
- ✅ Separate function calls (storing result in variable first)
- ✅ Basic function calls with literal arguments
- ✅ Most other JavaScript features

## Workaround

Until this bug is fixed, avoid nested calls by using temporary variables:

```javascript
// DON'T DO THIS (crashes):
outer(inner());

// DO THIS INSTEAD (works):
const temp = inner();
outer(temp);
```

## Recommended Fix

This requires deep architectural work in LLVMCodeGen:
1. Ensure MIR Call terminators properly emit LLVM call instructions
2. Verify call results are correctly stored in destination places
3. Ensure continuation blocks properly access stored results
4. Add integration tests for nested call patterns

## Status
**OPEN** - Requires compiler architecture expertise to fix properly

## Session Info
- Investigated: 2025-12-09
- Time spent: ~2 hours of deep debugging
- Debugger: Claude (nova-compiler-architect mode)
