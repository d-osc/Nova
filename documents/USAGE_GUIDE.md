# Nova Compiler - Usage Guide

## Quick Start

### Running TypeScript Directly

```bash
# Simplest way - just pass the .ts file
./build/Release/nova.exe script.ts

# Or use explicit run command
./build/Release/nova.exe run script.ts
```

### Compilation

```bash
# Compile and emit LLVM IR
./build/Release/nova.exe compile app.ts --emit-llvm

# Emit all intermediate representations
./build/Release/nova.exe compile app.ts --emit-all
# Creates: app.hir, app.mir, app.ll

# Compile with optimization
./build/Release/nova.exe compile app.ts -O3 -o optimized.ll
```

## Command Line Options

```
Usage: nova [command] [options] <input>
       nova <file.ts>                    (shortcut for: nova run <file.ts>)

Commands:
  compile    Compile source to native code
  run        JIT compile and run
  check      Type check only
  emit       Emit IR at various stages

Options:
  -o <file>           Output file
  -O<level>           Optimization level (0-3) [default: 2]
  --emit-llvm         Emit LLVM IR (.ll)
  --emit-mir          Emit MIR (.mir)
  --emit-hir          Emit HIR (.hir)
  --emit-asm          Emit assembly (.s)
  --emit-obj          Emit object file (.o)
  --emit-all          Emit all IR stages
  --target <triple>   Target triple (e.g., x86_64-pc-windows-msvc)
  --verbose           Verbose output
  --help              Show help message
  --version           Show version
```

## Supported Features

### Variables
```typescript
let x = 10;
const y = 20;
var z = 30;
```

### Functions
```typescript
function add(a: number, b: number): number {
    return a + b;
}

// Arrow functions
const multiply = (a: number, b: number) => a * b;

// Default parameters
function greet(name: string = "World"): string {
    return "Hello " + name;
}

// Rest parameters
function sum(...numbers: number[]): number {
    return numbers.reduce((a, b) => a + b, 0);
}
```

### Classes
```typescript
class Animal {
    name: string;

    constructor(name: string) {
        this.name = name;
    }

    speak(): string {
        return this.name + " makes a sound";
    }
}

class Dog extends Animal {
    breed: string;

    constructor(name: string, breed: string) {
        super(name);
        this.breed = breed;
    }

    speak(): string {
        return this.name + " barks";
    }
}
```

### Control Flow
```typescript
// If/else
if (x > 0) {
    return "positive";
} else if (x < 0) {
    return "negative";
} else {
    return "zero";
}

// Switch
switch (day) {
    case 1: return "Monday";
    case 2: return "Tuesday";
    default: return "Unknown";
}

// Ternary
let result = x > 0 ? "yes" : "no";
```

### Loops
```typescript
// For loop
for (let i = 0; i < 10; i++) {
    console.log(i);
}

// While loop
while (condition) {
    // code
}

// Do-while
do {
    // code
} while (condition);

// For-of
for (let item of array) {
    console.log(item);
}

// For-in
for (let key in object) {
    console.log(key);
}

// Break and continue with labels
outer: for (let i = 0; i < 10; i++) {
    for (let j = 0; j < 10; j++) {
        if (j === 5) break outer;
    }
}
```

### Arrays
```typescript
let arr = [1, 2, 3, 4, 5];

// Access and modify
arr[0] = 10;
let first = arr[0];

// Methods
arr.push(6);
arr.pop();
let sliced = arr.slice(1, 3);
let mapped = arr.map(x => x * 2);
let filtered = arr.filter(x => x > 2);
let sum = arr.reduce((a, b) => a + b, 0);
arr.forEach(x => console.log(x));
```

### Objects
```typescript
let obj = { x: 10, y: 20 };

// Access
let x = obj.x;
obj.y = 30;

// Methods
let keys = Object.keys(obj);
let values = Object.values(obj);
let entries = Object.entries(obj);
```

### Strings
```typescript
let str = "Hello World";

// Properties and methods
let len = str.length;
let char = str.charAt(0);
let idx = str.indexOf("World");
let sub = str.substring(0, 5);
let upper = str.toUpperCase();
let parts = str.split(" ");

// Template literals
let name = "Nova";
let greeting = `Hello, ${name}!`;
```

### Error Handling
```typescript
try {
    throw new Error("Something went wrong");
} catch (e) {
    console.error(e.message);
} finally {
    console.log("Cleanup");
}
```

### Async/Await
```typescript
async function fetchData(): Promise<number> {
    return 42;
}

async function main(): Promise<number> {
    let result = await fetchData();
    return result;
}
```

### Generators
```typescript
function* range(start: number, end: number) {
    for (let i = start; i < end; i++) {
        yield i;
    }
}

for (let n of range(0, 5)) {
    console.log(n);
}
```

## Examples

### Fibonacci
```typescript
function fibonacci(n: number): number {
    if (n <= 1) return n;

    let prev = 0;
    let curr = 1;

    for (let i = 2; i <= n; i++) {
        let next = prev + curr;
        prev = curr;
        curr = next;
    }

    return curr;
}

function main(): number {
    return fibonacci(10);  // Returns 55
}
```

### Array Processing
```typescript
function main(): number {
    let numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

    // Filter even numbers and double them
    let result = numbers
        .filter(x => x % 2 === 0)
        .map(x => x * 2)
        .reduce((a, b) => a + b, 0);

    return result;  // Returns 60 (4+8+12+16+20)
}
```

### Class Example
```typescript
class Counter {
    private count: number;

    constructor() {
        this.count = 0;
    }

    increment(): void {
        this.count++;
    }

    decrement(): void {
        this.count--;
    }

    getCount(): number {
        return this.count;
    }
}

function main(): number {
    let counter = new Counter();
    counter.increment();
    counter.increment();
    counter.increment();
    counter.decrement();
    return counter.getCount();  // Returns 2
}
```

## Testing

### Run All Tests
```bash
python run_all_tests.py
```

### Run Specific Test
```bash
./build/Release/nova.exe tests/test_array_simple.ts
```

### Check Exit Code
```bash
./build/Release/nova.exe tests/test_math_sqrt.ts
echo $?  # Shows return value from main()
```

## Intermediate Representations

### HIR (High-level IR)
- Preserves TypeScript/JavaScript semantics
- High-level constructs retained
- Type information preserved

### MIR (Mid-level IR)
- SSA (Static Single Assignment) form
- Basic blocks and control flow graph
- Platform-independent optimizations

### LLVM IR
- Target-specific representation
- Ready for LLVM optimization passes
- Can be compiled to native code

## Tips

1. **Use JIT for quick testing**: `nova script.ts` is fastest for development
2. **Check intermediate IR for debugging**: Use `--emit-all` to see all stages
3. **Return values from main()**: The exit code is the return value of `main()`
4. **Use console.log for output**: Supported for debugging

---

**Nova Compiler v1.4.0**
**511 tests passing (100%)**
