# Nova Compiler Changelog

## v1.3.71 - Generator & AsyncGenerator 100% Complete!
- **GeneratorFunction (ES2015)**: Full implementation!
  - `function* name() {}` declaration
  - `yield value` expression
  - `yield* iterable` delegation
  - `.next(value)`, `.return(value)`, `.throw(error)` methods
  - `.done` and `.value` property access
  - Generator parameters support
  - Local variables persist across yields (generator local storage)
  - `for...of` loop integration
- **AsyncGeneratorFunction (ES2018)**: Full implementation!
  - `async function* name() {}` declaration
  - `yield`, `yield*`, and `await` inside async generators
  - `.next()`, `.return()`, `.throw()` methods
  - `for await...of` loop integration
  - Full iterator result property access
- **Runtime Functions**:
  - `nova_generator_create`, `nova_generator_next`, `nova_generator_return`, `nova_generator_throw`
  - `nova_generator_store_local`, `nova_generator_load_local` for cross-yield variable persistence
  - `nova_async_generator_create`, `nova_async_generator_next`, etc.
- **23 generator tests passing** (100% test coverage)

## v0.78.0 - Fixed Type Inference for Array Methods
- **FIXED**: Array.concat() and Array.slice() now preserve type information
- Variables storing array method results can now access .length property
- Added LLVM function declarations for nova_value_array_concat and nova_value_array_slice
- Proper HIRPointerType and HIRArrayType creation for array-returning methods
- Array metadata type registration in LLVM codegen

## v0.77.0 - Array.slice() method
- Added Array.slice(start, end) method
- Creates sub-array from start to end index
- Supports negative indices
- Returns new array without modifying original

## v0.76.0 - Array.concat() method  
- Added Array.concat(otherArray) method
- Concatenates two arrays into new array
- Original arrays remain unchanged
- Returns metadata-compatible structure

## v0.75.0 - Array.join() method
- Added Array.join(delimiter) method
- Joins array elements into string
- Customizable delimiter (default comma)
- Includes concat and slice runtime implementations

## v0.74.0 - String.split() method
- Added String.split(delimiter) method
- Splits string into string array
- New StringArray structure added
- Handles empty delimiters and edge cases

## v0.73.0 - String.padEnd() method
- Added String.padEnd() method

## v0.72.0 - String.padStart() method
- Added String.padStart() method
