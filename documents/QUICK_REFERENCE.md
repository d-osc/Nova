# Nova Compiler - Quick Reference

## Commands

```bash
# Run TypeScript directly
nova script.ts

# Run with explicit command
nova run script.ts

# Compile to LLVM IR
nova compile app.ts --emit-llvm

# Compile with all IRs
nova compile app.ts --emit-all

# Compile with optimization
nova compile app.ts -O3

# Show help
nova --help

# Show version
nova --version
```

## Options

| Option | Description |
|--------|-------------|
| `-o <file>` | Output file |
| `-O0` to `-O3` | Optimization level |
| `--emit-llvm` | Emit LLVM IR (.ll) |
| `--emit-hir` | Emit HIR (.hir) |
| `--emit-mir` | Emit MIR (.mir) |
| `--emit-asm` | Emit assembly (.s) |
| `--emit-obj` | Emit object file (.o) |
| `--emit-all` | Emit all IR stages |
| `--verbose` | Verbose output |

## Supported Types

| TypeScript | LLVM IR |
|------------|---------|
| `number` | `i64` / `double` |
| `string` | `i8*` (char pointer) |
| `boolean` | `i1` |
| `void` | `void` |
| `any` | `i64` (boxed value) |
| `Array<T>` | Runtime struct |
| `Object` | Runtime struct |

## Operators

### Arithmetic
```typescript
+  -  *  /  %  **  ++  --
```

### Comparison
```typescript
==  !=  ===  !==  <  >  <=  >=
```

### Logical
```typescript
&&  ||  !  ??
```

### Bitwise
```typescript
&  |  ^  ~  <<  >>  >>>
```

### Assignment
```typescript
=  +=  -=  *=  /=  %=  **=
&=  |=  ^=  <<=  >>=  >>>=
&&=  ||=  ??=
```

## Control Flow

```typescript
// If/else
if (cond) { } else if (cond) { } else { }

// Switch
switch (x) { case 1: break; default: }

// Ternary
cond ? a : b

// Loops
for (let i = 0; i < n; i++) { }
while (cond) { }
do { } while (cond);
for (let x of arr) { }
for (let k in obj) { }

// Labels
label: for (...) { break label; continue label; }
```

## Built-in Methods

### Array
```typescript
push, pop, shift, unshift, slice, splice, concat
indexOf, lastIndexOf, includes, find, findIndex
filter, map, reduce, reduceRight, forEach
sort, reverse, join, every, some
flat, flatMap, at, with, fill, copyWithin
toReversed, toSorted, toSpliced
Array.from, Array.of, Array.isArray
```

### String
```typescript
length, charAt, charCodeAt, codePointAt
indexOf, lastIndexOf, includes
substring, slice, split, concat
trim, trimStart, trimEnd
toLowerCase, toUpperCase
padStart, padEnd, repeat
replace, replaceAll, at, match
String.fromCharCode, String.fromCodePoint
```

### Math
```typescript
abs, ceil, floor, round, trunc
sqrt, cbrt, pow, exp, log, log10, log2
sin, cos, tan, asin, acos, atan, atan2
sinh, cosh, tanh, asinh, acosh, atanh
sign, min, max, random, hypot
clz32, imul, fround, expm1, log1p
PI, E, LN2, LN10, LOG2E, LOG10E, SQRT2, SQRT1_2
```

### Object
```typescript
keys, values, entries, assign
freeze, seal, is, hasOwn
isFrozen, isSealed
```

### Number
```typescript
toString, toFixed, toExponential, toPrecision, valueOf
Number.isNaN, Number.isFinite, Number.isInteger, Number.isSafeInteger
Number.parseInt, Number.parseFloat
Number.MAX_VALUE, Number.MIN_VALUE, Number.EPSILON
Number.MAX_SAFE_INTEGER, Number.MIN_SAFE_INTEGER
Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY, Number.NaN
```

### Console
```typescript
log, error, warn, info, debug
trace, dir, table
time, timeEnd, count
clear, group, groupEnd
```

### Global
```typescript
parseInt, parseFloat, isNaN, isFinite
encodeURI, decodeURI
encodeURIComponent, decodeURIComponent
btoa, atob
```

## Build Commands

```bash
# Configure
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Clean build
cmake --build build --config Release --clean-first
```

## Testing

```bash
# Run all tests
python run_all_tests.py

# Run single test
nova tests/test_name.ts

# Check exit code (bash)
nova tests/test.ts; echo $?

# Check exit code (PowerShell)
nova tests/test.ts; $LASTEXITCODE
```

## File Structure

```
Nova/
├── src/           # Source code
├── include/       # Headers
├── tests/         # Test files (515)
├── examples/      # Examples
├── docs/          # Documentation
├── build/         # Build output
└── grammar/       # Grammar files
```

## Exit Codes

| Code | Meaning |
|------|---------|
| 0-255 | Return value from main() |
| -1 | Compilation error |
| Crash | Runtime error |

## Understanding IR

### HIR (High-level IR)
- Close to source code
- Type information preserved
- High-level constructs

### MIR (Mid-level IR)
- SSA form
- Basic blocks and CFG
- Platform independent

### LLVM IR
- Machine-independent assembly
- Ready for optimization
- `@function` - Global function
- `%0, %1` - SSA registers
- `i64` - 64-bit integer

---

**Nova Compiler v1.4.0** | **511 tests (100%)**
