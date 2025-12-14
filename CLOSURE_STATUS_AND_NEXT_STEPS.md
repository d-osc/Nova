# Closure Implementation Status & Next Steps

**Date**: December 10, 2025
**Current Status**: Core infrastructure complete, variable access needs refinement

## ✅ What's Working

1. **Capture Detection** - Variables from parent scopes are detected during HIR generation
2. **Environment Creation** - Struct types are created with fields for captured variables
3. **Environment Allocation** - Environments are allocated in MIR with correct field initialization
4. **Function Signatures** - `__env` parameter added to closure functions
5. **Environment Passing** - Environments correctly passed when calling closures
6. **Closure Calls** - Successfully detect and invoke closures with environments

## ⚠️ Current Limitation

**Issue**: Captured variable state doesn't persist across closure calls

**Test Result**:
```
First call: 1   ✅ Correct
Second call: 0  ❌ Expected: 2
```

**Root Cause**: Chicken-and-egg problem with environment timing:
- Captures are detected DURING body generation
- But `__env` parameter needs to exist BEFORE body generation
- For captured variables to be accessed through environment

## 🔧 Technical Challenge

The current architecture has a timing dependency:
1. Body generation → detects captures → stores in `capturedVariables_[funcName]`
2. `createClosureEnvironment(funcName)` → reads `capturedVariables_[funcName]`
3. If called BEFORE body: no captures detected yet → no environment created
4. If called AFTER body: captures detected → environment created, but too late for body to use

## 💡 Potential Solutions

### Option 1: Two-Pass Body Generation
1. First pass: Generate body (discard instructions, keep capture info)
2. Create environment based on detected captures
3. Second pass: Regenerate body with `__env` parameter available
4. Captured variable access uses GetField/SetField through `__env`

**Pros**: Clean separation, proper environment access
**Cons**: Performance cost of double generation

### Option 2: Post-Processing Pass
1. Generate body normally (captures detected and stored)
2. Create environment after body
3. Post-process all instructions in body
4. Replace captured variable accesses with environment field accesses

**Pros**: Single body generation
**Cons**: Complex instruction rewriting

### Option 3: Copy-In/Copy-Out at Function Boundaries
1. Keep current timing (environment after body)
2. At function ENTRY: copy environment fields to local variables
3. Generate body using local variables (no change needed)
4. At function EXIT: copy local variables back to environment fields

**Pros**: Minimal changes, leverages existing code
**Cons**: Performance overhead, doesn't handle cross-call state correctly for all cases

### Option 4: Delayed Environment Patching (Recommended)
1. Generate body normally (captures detected)
2. Create environment after body
3. Add `__env` parameter retroactively
4. On NEXT compilation of a call to this function:
   - MIR/LLVM level translates captured var access to environment access
   - Uses parameter mapping to route to correct environment fields

**Pros**: Separates concerns, doesn't require body regeneration
**Cons**: More complex MIR/LLVM translation logic

## 📋 Current Code Structure

### Capture Detection (HIRGen.cpp:22-44)
```cpp
HIRValue* lookupVariable(name) {
    // Check current scope
    // Check parent scopes
    if (found in parent) {
        capturedVariables_[lastFunctionName_].insert(name);  // ✅ Works
    }
}
```

### Environment Creation (HIRGen_Functions.cpp:81-105)
**Current Timing**: AFTER body generation
**Why**: Needs `capturedVariables_[funcName]` populated first

### Variable Access (HIRGen.cpp:149-188)
**Current Implementation**: Tries to use `__env` parameter
**Problem**: Parameter doesn't exist yet when code checks for it

## 🎯 Recommended Next Steps

1. **Revert** environment creation to AFTER body generation (original timing)
2. **Implement** Option 4: Delayed Environment Patching
3. **Add** MIR-level translation for captured variable access
4. **Map** local variables in closure to environment fields during MIR→LLVM

## 📊 Test Matrix

| Test Scenario | Status |
|--------------|--------|
| Simple closure (return only) | ✅ Compiles |
| Closure with read access | ✅ Compiles, ⚠️ reads stale value |
| Closure with write access | ✅ Compiles, ❌ doesn't persist |
| Multiple calls to same closure | ⚠️ State doesn't persist |
| Nested closures | 🔲 Not tested |
| Closure in loops | 🔲 Not tested |

## 🏗️ Architecture Decision

**Decision**: Implement Option 4 (Delayed Environment Patching)

**Rationale**:
- Preserves existing capture detection mechanism
- Separates HIR concerns from environment access concerns
- Enables proper variable-to-field mapping in later stages
- Most maintainable long-term solution

**Implementation Plan**:
1. Keep environment creation after body (current working state)
2. Add MIR instruction type for "EnvironmentFieldAccess"
3. During MIR generation, detect captured variable usage
4. Generate EnvironmentFieldAccess instead of regular variable access
5. LLVM backend translates to GEP instructions on environment struct

## 📝 Summary

The closure implementation has successfully completed the core infrastructure:
- ✅ Detection
- ✅ Environment structures
- ✅ Allocation
- ✅ Passing

The remaining work is the final piece: ensuring captured variable accesses properly route through the environment. This requires careful handling of the timing dependency between body generation and environment availability.

**Estimated effort for completion**: 2-4 hours of focused work on MIR-level variable access translation.
