# Nova Compiler Architecture

## Overview

Nova is a production-ready AOT (Ahead-Of-Time) compiler that transforms TypeScript and JavaScript code into highly optimized native machine code using LLVM as the backend.

## Compilation Pipeline

```
┌─────────────────────────────────────────────────────────────────┐
│                        Source Code (.ts/.js)                     │
└─────────────────────┬───────────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│ FRONTEND                                                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────┐    ┌─────────┐    ┌──────────────┐               │
│  │  Lexer  │ -> │ Parser  │ -> │ Type Checker │               │
│  └─────────┘    └─────────┘    └──────────────┘               │
│       │              │                  │                       │
│    Tokens         AST            Type-annotated AST            │
│                                                                  │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│ HIR (High-level Intermediate Representation)                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  • Preserves high-level semantics                               │
│  • Type information retained                                    │
│  • Closures, async/await, generators                            │
│  • Early optimizations:                                         │
│    - Function inlining                                          │
│    - Dead code elimination                                      │
│    - Constant folding                                           │
│                                                                  │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│ MIR (Mid-level Intermediate Representation)                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  • SSA (Static Single Assignment) form                          │
│  • Control Flow Graph (CFG)                                     │
│  • Basic blocks and terminators                                 │
│  • Mid-level optimizations:                                     │
│    - Constant propagation                                       │
│    - Copy propagation                                           │
│    - Common subexpression elimination                           │
│    - Loop optimization                                          │
│    - Register allocation hints                                  │
│                                                                  │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│ LLVM IR (Low-level Intermediate Representation)                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  • Platform-independent assembly                                │
│  • LLVM optimization passes:                                    │
│    - Instruction combining                                      │
│    - Scalar optimizations                                       │
│    - Vectorization (SLP, Loop)                                  │
│    - Interprocedural optimization (IPO)                         │
│    - Link-time optimization (LTO)                               │
│                                                                  │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│ BACKEND                                                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────┐    ┌──────────┐    ┌────────────┐             │
│  │  Selection │ -> │ Register │ -> │  Machine   │             │
│  │    DAG     │    │Allocation│    │   Code     │             │
│  └────────────┘    └──────────┘    └────────────┘             │
│                                                                  │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Native Machine Code                          │
│                  (x86-64, ARM64, etc.)                           │
└─────────────────────────────────────────────────────────────────┘
```

## Key Components

### 1. Frontend

#### Lexer
- **Purpose**: Tokenize source code
- **Input**: Raw TypeScript/JavaScript source
- **Output**: Stream of tokens
- **Features**:
  - ES2024+ syntax support
  - TypeScript-specific tokens
  - Unicode support
  - Source location tracking
  - Error recovery

#### Parser
- **Purpose**: Build Abstract Syntax Tree (AST)
- **Input**: Token stream
- **Output**: Type-safe AST
- **Features**:
  - Recursive descent parsing
  - Pratt parsing for expressions
  - Error recovery and reporting
  - JSX/TSX support
  - Decorators support

#### Type Checker
- **Purpose**: Validate types and perform semantic analysis
- **Input**: AST
- **Output**: Type-annotated AST
- **Features**:
  - Full TypeScript type system
  - Type inference (Hindley-Milner style)
  - Structural typing
  - Generic types
  - Union/intersection types
  - Control flow analysis
  - Definite assignment analysis

### 2. HIR (High-level IR)

**Design Philosophy**: Preserve JavaScript/TypeScript semantics while enabling early optimizations.

#### Key Features:
- **Typed Values**: Every value has a type annotation
- **High-level Constructs**:
  - Closures with captured variables
  - Async/await primitives
  - Generator functions
  - Classes with inheritance
  - Dynamic property access
  
#### Example HIR:
```hir
fn fibonacci(n: i32) -> i32 {
bb0:
    %0 = le %n, const 1: i32
    cond_br %0, bb1, bb2

bb1:
    return %n

bb2:
    %1 = sub %n, const 1: i32
    %2 = call @fibonacci(%1)
    %3 = sub %n, const 2: i32
    %4 = call @fibonacci(%3)
    %5 = add %2, %4
    return %5
}
```

#### HIR Optimizations:
1. **Function Inlining**: Small functions inlined at call sites
2. **Dead Code Elimination**: Remove unreachable code
3. **Constant Folding**: Evaluate constant expressions at compile time
4. **Closure Optimization**: Convert captures to parameters when possible
5. **Async Lowering**: Transform async/await into state machines

### 3. MIR (Mid-level IR)

**Design Philosophy**: SSA-based representation optimized for analysis and transformation.

#### Key Features:
- **SSA Form**: Every variable assigned exactly once
- **Basic Blocks**: Linear sequences of instructions
- **Terminators**: Explicit control flow (goto, switch, call, return)
- **Places**: Memory locations (_1, _2, etc.)
- **RValues**: Computed values

#### Example MIR:
```mir
fn fibonacci(_1: i32) -> i32 {
    let _0: i32;  // return place
    let _2: bool;
    let _3: i32;
    let _4: i32;
    let _5: i32;
    let _6: i32;
    let _7: i32;

bb0: {
    _2 = Le(copy _1, const 1_i32);
    switchInt(move _2) -> [0: bb2, otherwise: bb1];
}

bb1: {
    _0 = copy _1;
    return;
}

bb2: {
    _3 = Sub(copy _1, const 1_i32);
    _4 = call fibonacci(move _3) -> bb3;
}

bb3: {
    _5 = Sub(copy _1, const 2_i32);
    _6 = call fibonacci(move _5) -> bb4;
}

bb4: {
    _7 = Add(move _4, move _6);
    _0 = move _7;
    return;
}
}
```

#### MIR Optimizations:
1. **Constant Propagation**: Propagate constant values
2. **Copy Propagation**: Eliminate redundant copies
3. **Dead Store Elimination**: Remove unused assignments
4. **CFG Simplification**: Merge basic blocks, remove unreachable code
5. **Loop Optimization**: 
   - Loop invariant code motion
   - Loop unrolling
   - Strength reduction
6. **Register Pressure Analysis**: Hint register allocation

### 4. LLVM IR Code Generation

**Design Philosophy**: Map MIR to LLVM IR while leveraging LLVM's optimization infrastructure.

#### Type Mapping:
```
MIR Type         -> LLVM Type
─────────────────────────────
i8, i16, i32, i64 -> i8, i16, i32, i64
f32, f64         -> float, double
bool             -> i1
Pointer<T>       -> ptr
Array<T, N>      -> [N x T]
Struct{...}      -> %struct.name
Function         -> ptr (opaque function pointer)
```

#### Code Generation Strategy:
1. **Value Mapping**: MIR places → LLVM values
2. **Block Mapping**: MIR basic blocks → LLVM basic blocks
3. **Instruction Selection**: MIR instructions → LLVM instructions
4. **Runtime Library**: Link with runtime for GC, async, etc.

#### Runtime Services:
- **Memory Management**: Generational garbage collector
- **Async Runtime**: Work-stealing scheduler for async tasks
- **Object Model**: Dynamic dispatch for classes and interfaces
- **Builtins**: Array, String, Promise, etc.

### 5. LLVM Optimization Passes

#### Standard Optimization Levels:

**-O0** (Debug):
- No optimizations
- Fast compilation
- Good debugging experience

**-O1** (Basic):
- Basic optimizations
- -mem2reg (promote memory to registers)
- -simplifycfg (simplify control flow)
- -instcombine (instruction combining)

**-O2** (Default):
- All -O1 optimizations
- -inline (function inlining)
- -sccp (sparse conditional constant propagation)
- -gvn (global value numbering)
- -loop-unroll (loop unrolling)
- -vectorize (SLP vectorization)

**-O3** (Aggressive):
- All -O2 optimizations
- -aggressive-instcombine
- -loop-vectorize (aggressive loop vectorization)
- -slp-vectorize (superword-level parallelism)
- -unroll-loops (aggressive loop unrolling)

**LTO** (Link-Time Optimization):
- Whole-program optimization
- Cross-module inlining
- Dead code elimination across modules
- Interprocedural constant propagation

## Performance Characteristics

### Compilation Time:
- **Lexer/Parser**: O(n) where n = source size
- **Type Checker**: O(n * m) where m = average type complexity
- **HIR Gen**: O(n)
- **HIR Opt**: O(n * k) where k = optimization passes
- **MIR Gen**: O(n)
- **MIR Opt**: O(n * log n) for most passes
- **LLVM CodeGen**: Depends on LLVM optimization level

### Runtime Performance:
- **Arithmetic**: Near-native C++ performance
- **Function Calls**: Zero overhead for direct calls
- **Object Access**: 1-2 cycles for monomorphic access
- **Array Access**: Bounds checking can be eliminated by optimizer
- **Async/Await**: ~50ns overhead per await point

## Memory Management

### Garbage Collection Strategy:
- **Algorithm**: Generational mark-and-sweep
- **Generations**: Young (Eden + Survivor), Old
- **Write Barrier**: Card marking for old-to-young references
- **Collection Triggers**: 
  - Allocation failure in young generation
  - Old generation threshold
  - Explicit System.gc() call

### Memory Layout:
```
Object Header (16 bytes):
  - Type ID (8 bytes)
  - Mark bits (8 bytes): GC state, age, forwarding pointer

Object Body:
  - Fields aligned to 8-byte boundaries
  - Arrays have length prefix
```

## Concurrency Model

### Async/Await Implementation:
- **State Machine**: Each async function compiled to state machine
- **Scheduler**: Work-stealing thread pool
- **Wake Mechanism**: Intrusive linked list of waiting tasks
- **Zero-cost**: No allocation for simple awaits

### Threading:
- **Shared-nothing**: Each worker has own heap young generation
- **Concurrent GC**: Background threads for old generation collection
- **Atomic Operations**: Lock-free data structures where possible

## Future Work

### Planned Enhancements:
1. **Incremental Compilation**: Recompile only changed functions
2. **Profile-Guided Optimization (PGO)**: Use runtime profiling data
3. **Escape Analysis**: Stack-allocate non-escaping objects
4. **Devirtualization**: Convert virtual calls to direct calls
5. **Speculative Optimization**: JIT-style speculative inlining
6. **WASM Backend**: Target WebAssembly
7. **GPU Support**: Compile compute kernels to GPU

### Experimental Features:
- **Region-based Memory Management**: Alternative to GC
- **Effect System**: Track side effects in type system
- **Dependent Types**: Compile-time verification
- **Linear Types**: Zero-copy semantics
