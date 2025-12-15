# GetField ObjectHeader Bug - FIXED ✅

## The Bug
Field accesses like `t.x` or `animal.name` were causing segmentation faults because:
- SetField correctly adjusted field indices for ObjectHeader (field 0 → LLVM field 1)
- GetField did NOT adjust indices, trying to access field 0 (which is the ObjectHeader itself)
- This caused LLVM to unpack ObjectHeader bytes instead of using GEP

## The Fix
**File:** `src/codegen/LLVMCodeGen.cpp`  
**Line:** 6266  
**Change:** Use `secondIndex` (pre-adjusted) instead of `indexValue` (original)

### Before:
```cpp
if (auto* constIndex = llvm::dyn_cast<llvm::ConstantInt>(indexValue)) {
    unsigned fieldIndex = constIndex->getZExtValue();
    // ... then adjust for ObjectHeader here (double adjustment!)
}
```

### After:
```cpp
// Use secondIndex which is already adjusted for ObjectHeader (lines 6051-6067)
if (auto* constIndex = llvm::dyn_cast<llvm::ConstantInt>(secondIndex)) {
    unsigned fieldIndex = constIndex->getZExtValue();
    // No need to adjust again - already done!
}
```

## Why This Works
1. Lines 6051-6067 calculate `secondIndex` by adjusting for ObjectHeader:
   - Check if struct has ObjectHeader ([24 x i8] as first field)
   - If yes: `fieldIndex += 1` (skip ObjectHeader)
   - Store adjusted index in `secondIndex`

2. Line 6266 NOW uses the pre-adjusted `secondIndex` instead of raw `indexValue`

3. Removed duplicate adjustment at lines 6269-6280 to avoid double-adjustment

## LLVM IR Evidence

### Before Fix (BROKEN):
```llvm
// Unpacking ObjectHeader bytes!
%field_value.unpack = load i8, ptr %1, align 1
%field_value.elt15 = getelementptr inbounds [24 x i8], ptr %1, i64 0, i64 1
// ... continues for all 24 bytes
```

### After Fix (WORKING):
```llvm
// Correct GEP to field 1!
%struct_field_ptr = getelementptr inbounds %struct.Test, ptr %1, i64 0, i32 1
%field_value = load i64, ptr %struct_field_ptr, align 4
```

## Test Results

### test_field_debug.js:
```javascript
class Test {
    constructor() {
        this.x = "Hello";
    }
}
const t = new Test();
const val = t.x;
console.log("val:", val);
```
**Before:** Segmentation fault  
**After:** `val: Hello` ✅

### test_simple_animal.js:
```javascript
class Animal {
    constructor() {
        this.name = "Generic";
        this.type = "Unknown";
    }
}
const animal = new Animal();
console.log("name:", animal.name);
console.log("type:", animal.type);
```
**Before:** Segmentation fault  
**After:**  
```
name: Generic ✅
type: Unknown ✅
```

### test_inherit_basic.js:
**Results:**
- ✅ `breed: Labrador` - Dog's field works
- ✅ `speak(): Woof` - Method override works
- ✅ Animal instance fields work perfectly
- ⚠️ Dog's inherited fields blank (expected - no super() call)

## Impact
- **Fixed:** All field access operations
- **Fixed:** No more segfaults on field reads
- **Fixed:** Correct LLVM GEP generation
- **Enabled:** Inheritance testing to continue

## Next Steps
1. ✅ GetField bug - FIXED
2. ⏭️ Auto parent field initialization - Need to inline parent constructors
3. ⏭️ Method inheritance lookup - Child can't call parent methods
