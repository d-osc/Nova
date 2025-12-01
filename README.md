# Nova Compiler

**TypeScript/JavaScript to Native Code Compiler via LLVM**

[![Status](https://img.shields.io/badge/status-stable-green)]()
[![Tests](https://img.shields.io/badge/tests-511%20passing-brightgreen)]()
[![Pass Rate](https://img.shields.io/badge/pass%20rate-100%25-brightgreen)]()
[![LLVM](https://img.shields.io/badge/LLVM-18.1.7-orange)]()

Nova compiles TypeScript and JavaScript directly to native code through a multi-stage compilation pipeline:

```
TypeScript/JavaScript -> AST -> HIR -> MIR -> LLVM IR -> Native Code
```

## Features

### Core Language
- **Variables**: `let`, `const`, `var` with proper scoping
- **Functions**: Declarations, parameters, return values, recursion
- **Arrow Functions**: `(a, b) => a + b`, implicit/explicit return
- **Classes**: Properties, methods, constructors, inheritance, static members, getters/setters
- **Control Flow**: if/else, switch/case, ternary operator
- **Loops**: for, while, do-while, for-of, for-in, break/continue with labels
- **Error Handling**: try/catch/finally, throw, Error types
- **Operators**: All arithmetic, logical, bitwise, comparison, assignment operators

### Built-in Types & Methods

| Category | Methods/Features |
|----------|-----------------|
| **Array** | push, pop, shift, unshift, slice, splice, concat, indexOf, includes, find, filter, map, reduce, forEach, sort, reverse, join, every, some, flat, flatMap, at, with, toReversed, toSorted, toSpliced, fill, copyWithin, findIndex, findLast, findLastIndex, lastIndexOf, reduceRight, Array.from, Array.of, Array.isArray |
| **String** | length, charAt, charCodeAt, indexOf, lastIndexOf, includes, substring, slice, split, concat, repeat, trim, trimStart, trimEnd, toLowerCase, toUpperCase, padStart, padEnd, replace, replaceAll, at, match, localeCompare, String.fromCharCode, String.fromCodePoint |
| **Number** | toString, toFixed, toExponential, toPrecision, valueOf, Number.isNaN, Number.isFinite, Number.isInteger, Number.isSafeInteger, Number.parseInt, Number.parseFloat, Number.MAX_VALUE, Number.MIN_VALUE, Number.EPSILON, etc. |
| **Math** | abs, ceil, floor, round, trunc, sqrt, cbrt, pow, exp, log, log10, log2, sin, cos, tan, asin, acos, atan, atan2, sinh, cosh, tanh, sign, min, max, random, hypot, clz32, imul, fround, PI, E, etc. |
| **Object** | keys, values, entries, assign, freeze, seal, is, hasOwn, isFrozen, isSealed |
| **JSON** | stringify, parse |
| **Console** | log, error, warn, info, debug, trace, dir, table, time, timeEnd, count, clear, group, groupEnd |
| **TypedArray** | Int8Array, Uint8Array, Int16Array, Uint16Array, Int32Array, Uint32Array, Float32Array, Float64Array with full method support |
| **RegExp** | test, exec, match (basic support) |
| **Promise** | resolve, reject, then (basic support) |
| **Date** | Date.now() |
| **Global** | parseInt, parseFloat, isNaN, isFinite, encodeURI, decodeURI, encodeURIComponent, decodeURIComponent, btoa, atob |

### Advanced Features
- **Async/Await**: async functions, await expressions
- **Generators**: function*, yield, yield*
- **Async Generators**: async function*, for-await-of
- **Destructuring**: Array and object destructuring
- **Spread Operator**: `...array`, `...object`
- **Rest Parameters**: `function(...args)`
- **Default Parameters**: `function(x = 10)`
- **Template Literals**: `` `Hello ${name}!` ``
- **Optional Chaining**: `obj?.prop?.method?.()`
- **Nullish Coalescing**: `value ?? default`
- **typeof Operator**: Runtime type checking
- **instanceof Operator**: Type checking
- **in Operator**: Property existence check
- **delete Operator**: Property deletion
- **Enums**: Basic enum support
- **using/DisposableStack**: Resource management

## Quick Start

### Prerequisites
- LLVM 18.1.7
- CMake 3.20+
- C++20 Compiler (MSVC 19.29+ on Windows)

### Build

```bash
# Configure
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release
```

### Usage

```bash
# Run TypeScript directly (JIT)
./build/Release/nova.exe script.ts

# Or with explicit run command
./build/Release/nova.exe run script.ts

# Compile to LLVM IR
./build/Release/nova.exe compile app.ts --emit-llvm

# Compile with optimizations
./build/Release/nova.exe compile app.ts -O3 -o app.ll
```

### Examples

**Hello World:**
```typescript
function main(): number {
    console.log("Hello, World!");
    return 0;
}
```

**Array Operations:**
```typescript
function main(): number {
    let arr = [1, 2, 3, 4, 5];
    arr.push(6);
    let sum = arr.reduce((a, b) => a + b, 0);
    return sum;  // Returns 21
}
```

**Classes:**
```typescript
class Rectangle {
    width: number;
    height: number;

    constructor(w: number, h: number) {
        this.width = w;
        this.height = h;
    }

    area(): number {
        return this.width * this.height;
    }
}

function main(): number {
    let rect = new Rectangle(5, 3);
    return rect.area();  // Returns 15
}
```

**Async/Await:**
```typescript
async function fetchData(): Promise<number> {
    return 42;
}

async function main(): Promise<number> {
    let result = await fetchData();
    return result;
}
```

## Project Structure

```
Nova/
├── src/                    # Source code
│   ├── codegen/           # LLVM code generation
│   ├── frontend/          # Lexer, parser, AST
│   ├── hir/               # High-level IR
│   ├── mir/               # Mid-level IR
│   └── runtime/           # Runtime library
├── include/               # Header files
├── tests/                 # Test files (515 tests)
├── examples/              # Example programs
├── docs/                  # Documentation
├── grammar/               # Language grammar definitions
├── build.bat             # Windows build script
├── build.sh              # Unix build script
├── CMakeLists.txt        # CMake configuration
└── run_all_tests.py      # Test runner
```

## Testing

```bash
# Run all tests
python run_all_tests.py

# Run specific test
./build/Release/nova.exe tests/test_array_simple.ts
```

**Current Status: 511 tests passing (100%)**

## Documentation

- [USAGE_GUIDE.md](docs/USAGE_GUIDE.md) - Comprehensive usage guide
- [QUICK_REFERENCE.md](docs/QUICK_REFERENCE.md) - Quick command reference
- [ARCHITECTURE.md](docs/ARCHITECTURE.md) - Compiler architecture
- [BUILD.md](docs/BUILD.md) - Build instructions
- [METHODS_STATUS.md](docs/METHODS_STATUS.md) - Supported methods list
- [TS_JS_COMPATIBILITY.md](docs/TS_JS_COMPATIBILITY.md) - TypeScript/JavaScript compatibility

## Version History

See [CHANGELOG.md](CHANGELOG.md) for version history.

## License

MIT License - see [LICENSE](LICENSE) for details.

---

**Powered by LLVM 18.1.7**
