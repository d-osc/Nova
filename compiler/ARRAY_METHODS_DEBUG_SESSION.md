# Array Methods Debug Session - 2025-11-21

## Session Goal
Implement working array methods (push, pop, shift, unshift) for Nova compiler

## Progress Summary

### ✅ Completed Tasks

1. **Runtime Implementation** (100% Complete)
   - ✅ Implemented `array_shift()` in Array.cpp (lines 87-100)
   - ✅ Implemented `array_unshift()` in Array.cpp (lines 102-122)
   - ✅ Added extern "C" wrappers: `nova_array_push/pop/shift/unshift` (lines 128-148)
   - ✅ Declarations added to Runtime.h (lines 95-96)

2. **HIR Generation** (100% Complete)
   - ✅ Array method detection in HIRGen.cpp CallExpr visitor (lines 406-492)
   - ✅ Detects push/pop/shift/unshift calls on arrays
   - ✅ Creates external function declarations
   - ✅ Generates HIR Call nodes with correct signatures
   - **Verification**: Debug output shows "Detected array method call: push/pop"

3. **Array Metadata Struct** (100% Complete)
   - ✅ Modified LLVMCodeGen.cpp generateAggregate() (lines 1826-1871)
   - ✅ Creates struct `{ [24 x i8], i64, i64, ptr }` compatible with `nova::runtime::Array`
   - ✅ Stores length, capacity, and elements pointer
   - ✅ Returns metadata pointer instead of raw array pointer

4. **Array Indexing Fix** (100% Complete)
   - ✅ Modified LLVMCodeGen.cpp generateGetElement() (lines 2114-2153)
   - ✅ Detects metadata struct pattern
   - ✅ Extracts `elements` pointer from field 3
   - ✅ Uses extracted pointer for GEP operations
   - **Verification**: "Extracted elements pointer from metadata" appears in debug

5. **Build System** (100% Complete)
   - ✅ Compiler builds successfully
   - ✅ Runtime library links correctly with novacore.lib
   - ✅ No compilation or linker errors

### ❌ Current Blocker

**Problem**: Array method calls are NOT converted to LLVM call instructions

**Evidence**:
```bash
# Simple function calls work:
test_simple_function_call.ts → exit code 30 ✅ (calls `add()` function)

# Array methods DON'T work:
test_array_push_pop.ts → exit code 60 ❌ (should be 100)
  - Expected: 10+20+30+40 = 100
  - Actual: 10+20+30+0 = 60 (methods not executed)

# LLVM IR inspection:
grep "call.*nova_array" temp_jit.ll → NO RESULTS
grep "declare.*nova_array" temp_jit.ll → NO RESULTS
Only blocks named "call_cont" exist (continuation blocks without actual calls)
```

**Root Cause Analysis**:

HIR Pipeline Status:
- HIRGen creates Call nodes ✅
- HIRGen creates external function declarations ✅
- **HIR → MIR conversion: UNKNOWN** ⚠️
- **MIR → LLVM conversion: FAILS** ❌

The Call instructions are created at HIR level but never make it to LLVM IR.

## Test Results

| Test | Expected | Actual | Status |
|------|----------|--------|--------|
| test_array_simple_access.ts | 60 | 60 | ✅ PASS |
| test_simple_function_call.ts | 30 | 30 | ✅ PASS |
| test_array_push_pop.ts | 100 | 60 | ❌ FAIL |
| test_array_methods.ts | 105 | 60 | ❌ FAIL |

**Conclusion**: Array indexing works, regular function calls work, but array METHOD calls are lost in translation.

## Architecture Analysis

### Nova Compiler Pipeline

```
Source (.ts)
  ↓
[Parser] → AST
  ↓
[HIRGen] → HIR (High-level IR)
  ↓
[MIRGen] → MIR (Mid-level IR)  ← SUSPECTED ISSUE HERE
  ↓
[LLVMCodeGen] → LLVM IR
  ↓
[LLVM] → Native Code
```

### HIR Call Node Structure

When `arr.push(40)` is encountered:

```cpp
// HIRGen creates:
HIRFunction* pushFunc = module_->getFunction("nova_array_push");
HIRCall* callNode = builder_->createCall(pushFunc, {arrayPtr, valuePtr});
```

**Debug confirms this happens**: "Created external array function: nova_array_push"

### Expected MIR Output (Not Happening)

MIRGen should create:
```cpp
MIRCall* call = new MIRCall(
    calleeFunc: "nova_array_push",
    args: [array_metadata_ptr, i64_value],
    returnType: ptr
);
```

### Expected LLVM Output (Not Generated)

```llvm
declare ptr @nova_array_push(ptr, ptr)

%call_result = call ptr @nova_array_push(ptr %array_meta, ptr %value_ptr)
```

**This is missing from temp_jit.ll!**

## Next Investigation Steps

### 1. Check HIR → MIR Conversion

**Files to inspect**:
- `src/mir/MIRGen.cpp` or `src/mir/MIRGen.h`
- Look for HIR Call instruction handling
- Search for: `case HIRInstruction::Opcode::Call`

**Questions**:
- Does MIRGen have a case for processing HIR Call instructions?
- Are external function calls handled differently?
- Is there special handling needed for method calls vs regular calls?

### 2. Check MIR → LLVM Conversion

**Files to inspect**:
- `src/codegen/LLVMCodeGen.cpp` - function `generateStatement()` or `convertRValue()`
- Look for MIR Call handling
- Search for: `MIRCall` or `case MIRStatement::Kind::Call`

**Questions**:
- Does LLVMCodeGen process MIR Call statements?
- Are there debug prints we can add to trace call generation?
- Do external calls need special declaration generation?

### 3. Add Debug Tracing

Add prints at each pipeline stage:
```cpp
// In MIRGen:
std::cerr << "DEBUG MIRGen: Processing HIR Call instruction" << std::endl;

// In LLVMCodeGen:
std::cerr << "DEBUG LLVM: Generating call to " << funcName << std::endl;
```

## Hypothesis

**Primary Theory**: HIR Call nodes for external functions (array methods) are being dropped during HIR → MIR conversion because:
1. MIRGen doesn't know how to handle external function declarations
2. Or: External calls require special setup that's missing
3. Or: Method calls on metadata structs need different handling than we implemented

**Alternative Theory**: The `call_cont` blocks suggest calls ARE being processed but the actual call instruction generation is being skipped due to a conditional check or error that's silently failing.

## Files Modified This Session

1. `include/nova/runtime/Runtime.h`
   - Added shift/unshift declarations

2. `src/runtime/Array.cpp`
   - Implemented shift/unshift (lines 87-122)
   - Added extern C wrappers (lines 128-148)

3. `src/hir/HIRGen.cpp`
   - Added array method detection (lines 406-492)

4. `src/codegen/LLVMCodeGen.cpp`
   - Added metadata struct creation (lines 1826-1871)
   - Added elements pointer extraction (lines 2114-2153)

5. `ARRAY_METHODS_STATUS.md` - Documentation
6. `ARRAY_METHODS_DEBUG_SESSION.md` - This file

## Recommended Next Actions

1. **Trace MIR generation** for array method calls
   - Add debug prints in MIRGen
   - Confirm MIR Call nodes are created

2. **Trace LLVM generation** for MIR Calls
   - Add debug prints in LLVMCodeGen call handling
   - See if calls are reaching codegen

3. **Compare working vs broken**
   - Diff MIR/LLVM output between `test_simple_function_call.ts` (works) and `test_array_push_pop.ts` (broken)
   - Find where they diverge

4. **Consider workaround**
   - If pipeline fix is too complex, consider implementing array methods as compiler intrinsics instead of external calls

## Time Investment

- Runtime implementation: 30 minutes ✅
- HIR method detection: 45 minutes ✅
- Metadata struct creation: 1 hour ✅
- Array indexing fix: 45 minutes ✅
- **Current blocker**: 1+ hours and ongoing ⚠️

**Total time**: ~4 hours on array methods feature

## Status: BLOCKED

Waiting for MIR/LLVM call generation investigation to proceed further.
