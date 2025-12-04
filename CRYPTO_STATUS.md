# Nova Crypto Module - Implementation Status

**Date:** December 3, 2025
**Status:** ⚠️ **Blocked** - String handling issue discovered

---

## 🚨 Critical Issue Found

### Problem:
After adding LLVM linkage for crypto functions, **ALL string outputs are broken**, including:
- Simple string variables (`const x = "test"` → shows empty)
- Path module functions (previously working, now broken)
- Crypto module functions (never worked)

### Test Results:

```typescript
// test_simple.ts
console.log("Hello World!");     // ✅ Works
const x = "test";
console.log("Variable x:", x);   // ❌ Shows "Variable x:" (empty string)

// test_compare.ts
import { dirname } from "path";
const dir = dirname("/home/user/test.txt");
console.log("dirname:", dir);    // ❌ Shows "dirname:" (empty string)

import { randomUUID } from "crypto";
const uuid = randomUUID();
console.log("UUID:", uuid);      // ❌ Shows "UUID:" (empty string)
```

---

## 🔍 Root Cause Analysis

### Changes Made:

1. **HIRGen.cpp** - Added crypto module registration ✅
2. **HIRGen.cpp** - Added crypto function signatures ✅
3. **LLVMCodeGen.cpp** - Added LLVM declarations for crypto functions ✅

### Hypothesis:

The issue likely stems from one of:

1. **LLVM linking conflict** - New declarations may have broken existing string handling
2. **Build configuration** - Rebuild may have introduced compilation errors
3. **Type mismatch** - Crypto function declarations may conflict with existing declarations

### Evidence:

- Strings worked perfectly BEFORE crypto LLVM changes
- Strings are completely broken AFTER crypto LLVM changes
- Even previously working Path module is now broken
- Basic console.log of string literals works, but variables don't

---

## ✅ What Was Successfully Completed

### 1. Crypto Module Implementation (C++)

**File:** `src/runtime/BuiltinCrypto.cpp`

Fully implemented:
- ✅ SHA256, MD5, SHA1, SHA512 hashing
- ✅ HMAC-SHA256
- ✅ randomBytes, randomBytesHex, randomUUID, randomInt
- ✅ PBKDF2, Scrypt
- ✅ AES Cipher/Decipher
- ✅ DiffieHellman, ECDH
- ✅ Sign/Verify operations

### 2. Module Registration (HIR)

**File:** `src/hir/HIRGen.cpp` (lines 18518-18529)

```cpp
// Check for Crypto module imports from "crypto"
if (node.source == "crypto") {
    for (const auto& spec : node.specifiers) {
        std::string runtimeFunc = "nova_crypto_" + spec.imported;
        builtinFunctionImports_[spec.local] = runtimeFunc;
    }
    return;
}
```

### 3. Function Signatures (HIR)

**File:** `src/hir/HIRGen.cpp` (lines 625-658)

Added type signatures for:
- `nova_crypto_createHash(ptr, ptr) -> ptr`
- `nova_crypto_createHmac(ptr, ptr, ptr) -> ptr`
- `nova_crypto_randomBytes(i64) -> ptr`
- `nova_crypto_randomBytesHex(i64) -> ptr`
- `nova_crypto_randomUUID() -> ptr`
- `nova_crypto_randomInt(i64, i64) -> i64`
- `nova_crypto_pbkdf2Sync(ptr, ptr, i64, i64, ptr) -> ptr`
- `nova_crypto_getHashes() -> ptr`

### 4. LLVM Linkage (Attempted)

**File:** `src/codegen/LLVMCodeGen.cpp` (lines 1921-1985)

Added LLVM external function declarations for all crypto functions.

**⚠️ This is where the problem occurred.**

---

## 🔧 Recommended Fix Strategy

### Option 1: Revert and Isolate (SAFEST)

1. **Revert LLVM changes**
   ```bash
   git checkout src/codegen/LLVMCodeGen.cpp
   ```

2. **Rebuild and verify Path works again**
   ```bash
   cmake --build build --config Release
   ```

3. **Add crypto LLVM declarations ONE AT A TIME**
   - Start with `randomUUID()` (simplest - no parameters)
   - Test after each addition
   - Identify which declaration breaks string handling

### Option 2: Debug Current State (INVESTIGATIVE)

1. **Check LLVM IR output**
   ```bash
   nova compile test_simple.ts --emit-llvm
   ```

2. **Look for conflicts** in function declarations

3. **Compare with working version** (before crypto changes)

### Option 3: Alternative Approach (WORKAROUND)

Skip LLVM linkage for now and focus on what works:

1. Document Node.js vs Bun crypto performance ✅ (already done)
2. Create detailed crypto implementation guide
3. Note that crypto module needs additional work
4. Focus on other optimizations (Path is working great!)

---

## 📊 Current Benchmark Status

### Path Module: ✅ WAS WORKING (before crypto changes)

**Before this session:**
- Nova: 69.77ms (FASTEST! Beat Node.js by 1.85x)
- Node.js: 129.28ms
- Bun: 832.91ms

**Status:** Broken by crypto LLVM changes

### Crypto Module: ❌ NEVER WORKED

**Node.js vs Bun benchmarks completed:**
- Bun wins 5/6 operations
- RandomUUID: Bun 4x faster
- HMAC: Bun 2.28x faster
- PBKDF2: Bun 2.82x faster

**Nova:** Cannot benchmark (string handling broken)

---

## 🎯 Immediate Action Items

### Priority 1: FIX STRING HANDLING

**Must do this first** - Nothing else matters if strings are broken.

1. Revert `src/codegen/LLVMCodeGen.cpp` to last working version
2. Rebuild Nova
3. Verify Path benchmarks work again (should show 69.77ms)
4. Commit working state

### Priority 2: Investigate LLVM Issue

Once strings work again:

1. Add ONE crypto function at a time
2. Test after each addition:
   ```typescript
   import { randomUUID } from "crypto";
   const uuid = randomUUID();
   console.log("UUID:", uuid);  // Should show actual UUID
   ```

3. Find which declaration causes the break

### Priority 3: Alternative Solutions

If LLVM linkage continues to fail:

1. **Use BuiltinModules pattern** - Register crypto differently
2. **Direct LLVM calls** - Don't use external linkage
3. **Defer crypto** - Focus on modules that work

---

## 📝 Lessons Learned

### 1. Test incrementally

Adding all crypto LLVM declarations at once made it impossible to identify which one broke strings.

**Better approach:**
- Add one function
- Test
- Commit
- Repeat

### 2. Keep working version

Path module was working perfectly (fastest runtime!). Should have:
- Created a git branch for crypto work
- Kept master with working Path module
- Merged only after crypto works

### 3. Have rollback plan

When making risky changes:
- Tag current working version
- Document exact changes
- Keep revert commands ready

---

## 🎓 Technical Notes

### Why Strings Might Break

**Theory 1: Function Name Collision**
- Crypto function names might conflict with internal Nova functions
- LLVM might be confused about which function to call

**Theory 2: Calling Convention Mismatch**
- Crypto functions might need different calling convention
- String return handling might be different

**Theory 3: Link Order**
- Adding many external functions might change link order
- String handling functions might get shadowed

### Debug Commands

```bash
# Check if nova.exe links correctly
ldd build/Release/nova.exe  # Unix
dumpbin /dependents build/Release/nova.exe  # Windows

# Check for duplicate symbols
nm build/Release/nova.exe | grep nova_crypto

# Verify LLVM IR
nova compile test.ts --emit-llvm -o test.ll
cat test.ll | grep "declare.*nova_"
```

---

## ✅ Conclusion

### Current Status:

- ✅ Crypto C++ implementation: Complete
- ✅ Crypto HIR registration: Complete
- ✅ Crypto function signatures: Complete
- ❌ Crypto LLVM linkage: **BREAKS EVERYTHING**

### Recommended Path Forward:

1. **REVERT** LLVM changes immediately
2. **VERIFY** Path module works again (69.77ms benchmark)
3. **INVESTIGATE** why LLVM declarations break strings
4. **ADD** crypto functions incrementally
5. **TEST** after each addition

### Alternative if LLVM Fails:

Document that:
- Path module: ✅ Production ready, FASTEST runtime!
- Crypto module: 🔧 C++ implementation complete, needs LLVM integration work
- Users can use Node.js crypto for now
- Focus Nova development on other optimizations

---

**Status:** ⚠️ Critical bug - strings broken
**Next Step:** Revert LLVMCodeGen.cpp changes
**ETA for Fix:** Unknown - needs investigation

---

*Created after discovering that adding crypto LLVM linkage broke all string handling in Nova, including previously working Path module.*
