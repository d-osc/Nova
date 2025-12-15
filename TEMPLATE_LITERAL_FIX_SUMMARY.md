# Template Literal Number Conversion Bug Fix

## Problem Description

When using template literals with number interpolation (e.g., `` `Number is ${x}` ``), the Nova compiler was generating incorrect LLVM IR. The `nova_i64_to_string` call was missing, and `nova_string_concat` received `undef` as an argument.

### Symptoms

**MIR Output (CORRECT):**
```mir
_num_to_str.2 = call const "nova_i64_to_string"(copy _num.2) -> [return: call_cont];
_str_concat.3 = call const "nova_string_concat"(const "Number: ", copy _num_to_str.2) -> [return: call_cont];
```

**LLVM IR Output (WRONG - Before Fix):**
```llvm
%0 = call ptr @nova_string_concat(ptr nonnull @.str.1, ptr undef)
```

The `nova_i64_to_string` call was completely missing, and the second argument to `nova_string_concat` was `undef`.

## Root Cause

In `src/codegen/LLVMCodeGen.cpp`, the `generateTerminator` function handles MIR Call terminators. When the function to call is specified as a string constant (the function name), the code was missing a declaration for `nova_i64_to_string`.

The function lookup chain was:
1. Check `functionMap` - not found
2. Call `module->getFunction(funcName)` - not found
3. Check for specific known functions (`malloc`, `strlen`, filesystem functions, etc.)
4. **MISSING**: Declaration for `nova_i64_to_string`

Without the declaration, `callee` remained `nullptr`, causing the code path to skip call generation entirely.

## The Fix

Added external function declaration for `nova_i64_to_string` in the Call terminator handler.

**File:** `src/codegen/LLVMCodeGen.cpp`
**Location:** Around line 3370 (in the Call terminator case)

```cpp
if (!callee && funcName == "nova_i64_to_string") {
    // ptr @nova_i64_to_string(i64) - converts i64 to string
    if(NOVA_DEBUG) std::cerr << "DEBUG LLVM: Creating external nova_i64_to_string declaration" << std::endl;
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(*context),  // Returns ptr (string)
        {llvm::Type::getInt64Ty(*context)},      // Number (i64)
        false
    );
    callee = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        "nova_i64_to_string",
        module.get()
    );
}
```

## Result After Fix

**LLVM IR Output (CORRECT):**
```llvm
define i64 @__nova_main() {
entry:
  %0 = call ptr @nova_i64_to_string(i64 42)
  %1 = call ptr @nova_string_concat(ptr nonnull @.str, ptr %0)
  call void @nova_console_log_string(ptr %1)
  call void @nova_console_print_newline()
  ret i64 0
}

declare ptr @nova_i64_to_string(i64)
declare ptr @nova_string_concat(ptr, ptr)
```

Both calls are now present:
1. `nova_i64_to_string` converts the number to a string
2. `nova_string_concat` correctly uses the result from step 1 (not `undef`)

## Test Cases

### Test 1: Basic Number Interpolation
**File:** `test_number_only.js`
```javascript
const x = 42;
const msg = `Number is ${x}`;
console.log(msg);
```

**Output:**
```
Number is 42
```

### Test 2: Multiple Interpolations
**File:** `test_template_complete.js`
```javascript
const num = 42;
const str = "world";
console.log(`Number: ${num}`);
```

**Output:**
```
Number: 42
```

## Impact

This fix ensures that template literals with number interpolation work correctly in the Nova compiler. The LLVM IR generation now properly:
1. Declares the `nova_i64_to_string` function
2. Generates the call to convert numbers to strings
3. Passes the conversion result to `nova_string_concat`
4. Produces executable code that runs correctly

## Files Modified

- `src/codegen/LLVMCodeGen.cpp` - Added `nova_i64_to_string` declaration in Call terminator handler

## Debugging Enhancements Added

Added comprehensive trace logging to help diagnose similar issues in the future:
- Log LLVM IR of generated call instructions
- Log current basic block before/after calls
- Log store operations for call results
- Log branch instructions to continuation blocks

These debug traces are enabled with `NOVA_DEBUG=1` environment variable.
