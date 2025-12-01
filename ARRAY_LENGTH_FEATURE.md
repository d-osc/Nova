# Array.length Property - v0.22.0

## Overview

Implemented the `array.length` property that returns the runtime length of an array. The property correctly reflects the current size of the array, including changes made by array methods (push, pop, shift, unshift).

## Implementation Details

### 1. HIR Generation (HIRGen.cpp:731-736)

Changed from compile-time constant to runtime field access:

**Before:**
```cpp
lastValue_ = builder_->createIntConstant(length);  // Compile-time constant
```

**After:**
```cpp
// Generate code to read length from metadata struct at runtime
// Metadata struct: { [24 x i8], i64 length, i64 capacity, ptr elements }
// Field index 1 is the length
lastValue_ = builder_->createGetField(object, 1);
```

### 2. LLVM Code Generation (LLVMCodeGen.cpp:2221-2286)

Fixed `generateGetElement()` to distinguish between metadata struct field access and array element access:

- **Fields 0-2** (header, length, capacity): Direct struct field access
- **Field 3+** (array elements): Extract elements pointer, then index into array

**Generated LLVM IR:**
```llvm
; Accessing array.length (field 1 of metadata struct)
%meta_field_ptr = getelementptr inbounds { [24 x i8], i64, i64, ptr }, ptr %metadata, i32 0, i32 1
%field_value = load i64, ptr %meta_field_ptr, align 4
```

### 3. Array Metadata Structure

```
{
  [24 x i8] header,    // Field 0
  i64 length,          // Field 1 - accessed by array.length
  i64 capacity,        // Field 2
  ptr elements         // Field 3 - pointer to actual array data
}
```

## Usage Examples

```typescript
// Basic usage
let arr = [10, 20, 30];
let len = arr.length;  // 3

// Length updates after push
arr.push(40);
let len2 = arr.length;  // 4

// Length updates after pop
arr.pop();
let len3 = arr.length;  // 3

// Works with all array methods
arr.shift();           // Remove first element
let len4 = arr.length;  // 2

arr.unshift(5);        // Add to beginning
let len5 = arr.length;  // 3
```

## Test Results

All tests pass successfully:

| Test | Expected | Result | Status |
|------|----------|--------|--------|
| test_array_length_debug.ts | 3 | 3 | ✅ |
| test_array_length_after_push.ts | 4 | 4 | ✅ |
| test_array_length_dynamic.ts | 10 | 10 | ✅ |
| test_array_length_comprehensive.ts | 15 | 15 | ✅ |

### Test Scenarios Covered

1. **Initial length**: Reading length of newly created array
2. **After push**: Length increments correctly
3. **After pop**: Length decrements correctly
4. **After shift**: Length decrements correctly
5. **After unshift**: Length increments correctly
6. **Empty array**: Length is 0 after removing all elements

## Technical Notes

### Bug Fix

The original implementation had a critical bug where accessing metadata struct fields was being interpreted as array indexing:

**Problem:** When accessing `arr.length` (field 1), the code would:
1. Extract the elements pointer (field 3)
2. Index into the array with index 1
3. Return `array[1]` instead of `metadata.length`

**Solution:** Added logic to detect metadata field access (index < 3) and generate direct struct field GEP instead of array element GEP.

### Optimization

With optimization enabled (`-O2`), the compiler can optimize constant array lengths:

```llvm
; Optimized code for simple case
define i64 @main() {
entry:
  ret i64 3
}
```

This provides both correctness (runtime length tracking) and performance (constant folding when possible).

## Related Features

- **Array Methods** (v0.21.0): push, pop, shift, unshift
- **ValueArray System**: Automatic conversion from stack arrays to heap arrays
- **Write-back Mechanism**: Updates metadata after array modifications

## Future Enhancements

Potential improvements:
- `array.capacity` property to expose allocated capacity
- Array bounds checking using length field
- Length validation in array access operations
