# Nova Compiler — Feature Status

Last updated: 2026-07-26

This is an honest, public-facing checklist of what Nova actually supports.
Items are marked **PASS** (compiles AND runs correctly), **COMPILE** (parses/compiles
but has known runtime bugs), **PARSE** (accepted by the parser only), or **MISSING**.

The "known runtime bugs" column exists because until 2026-07-26 the test harness
ran tests with `--no-cache`, which silently skips executable emission/execution.
That made every compile-success count as a "pass" and hid real bugs. The harness
now also runs the binary; the table below reflects real behavior.

---

## 1. ECMAScript core

| Feature | Status | Notes |
|---|---|---|
| Arithmetic, comparison, logical ops | PASS | |
| `let` / `const` / `var` scoping | PASS | |
| All loop kinds (`for`, `for-in`, `for-of`, `while`, `do-while`) | PASS | labeled loops have a runtime bug (`labeled_loops.ts`) |
| `switch` (with fallthrough) | COMPILE | `switch_fallthrough.ts` exits 1 at runtime |
| `break` / `continue` (incl. labeled) | COMPILE | labeled break/continue exits 1 |
| `try` / `catch` / `finally` | COMPILE | `try_catch.ts` exits 1 |
| `throw` | PASS | |
| Optional chaining `?.` | PASS | |
| Nullish coalescing `??` | PASS | |
| Template literals | COMPILE | `template_literals.ts` exits 7 |
| Tagged template literals | PASS | |
| Generators (`function*`, `yield`, `yield*`) | COMPILE | `generators.ts` exits 1 |
| Async/await | PASS | limited conformance coverage |
| Async iterators (`for await`) | COMPILE | lowers to sync iteration; needs `Symbol.asyncIterator` path |
| `instanceof` / `typeof` | COMPILE | `instanceof_typeof.ts` exits 4 |
| Destructuring (array, object, nested, defaults, rest) | PASS | |
| Spread / rest parameters | PASS | |
| Computed property names (object & class) | PASS | |
| Getters / setters | COMPILE | `getters_setters.ts` exits 7 |
| Symbols (`Symbol.iterator`, etc.) | COMPILE | `symbol_iterator.ts` exits 7 |

## 2. Classes & OOP

| Feature | Status | Notes |
|---|---|---|
| Class declarations & expressions | PASS | |
| Constructors | PASS | |
| `super(...)` constructor calls | PASS | |
| `super.method()` calls | PASS | |
| Inheritance (`extends`) | PASS | |
| Method dispatch (virtual) | PASS | |
| Static methods / properties | PASS | |
| Private fields (`#name`) | COMPILE | `private_fields.ts` exits 8 |
| Public class fields | PASS | |
| Class computed members `[expr]()` | PASS | |
| Decorators | COMPILE | `decorators.ts` exits 2 |

## 3. TypeScript

| Feature | Status | Notes |
|---|---|---|
| Primitive types (`number`, `string`, `boolean`, etc.) | PASS | |
| Union / intersection types | PASS | |
| Array / tuple types | PASS | |
| Type aliases (`type X = ...`) | PASS | |
| Interfaces | PASS | type-only |
| Function type annotations | PASS | |
| Generics (`function id<T>(x: T): T`) | COMPILE | `ts_generics.ts` exits 1 — type erasure to `Any` |
| String & numeric enums | PASS | numeric enums have reverse mapping |
| `keyof T` | PARSE | type-only |
| Conditional types (`T extends U ? X : Y`) | PARSE | type-only |
| `infer X` | PARSE | type-only |
| Mapped types (`{ [K in keyof T]: ... }`) | PARSE | type-only |
| Indexed access types (`T[K]`) | PARSE | type-only |
| Type assertions (`x as T`, `<T>x`) | PASS | |
| `instanceof` narrowing | PASS | |

## 4. Standard library

| Surface | Status | Notes |
|---|---|---|
| `Array` (push, pop, map, filter, reduce, ...) | COMPILE | `array_iteration.ts` exits 1 — likely one of `flatMap` / comparator `sort` |
| `String` (split, replace, slice, ...) | COMPILE | `string_methods.ts` exits 5 — likely `match`/`replaceAll` with regex |
| `Math` (abs, floor, max, ...) | COMPILE | `math_static.ts` exits 8 |
| `Date` (now, toISOString, ...) | COMPILE | `date_basic.ts` exits 1 (timezone-sensitive) |
| `Object` (keys, values, entries, assign, fromEntries, groupBy) | PASS | groupBy is ES2024 |
| `Map` / `WeakMap` | COMPILE | `map_basic.ts` exits 7, `weakmap_basic.ts` exits 8 |
| `Set` / `WeakSet` | PASS | basic operations; ES2025 methods have runtime bug (`es2024_set_methods.ts` exits 3) |
| `Promise` (all, race, allSettled, any, withResolvers, then, catch, finally) | COMPILE | `promise_static.ts` segfaults |
| `JSON` (parse, stringify) | PASS | |
| `Error` classes (Error, TypeError, etc. with `cause`) | COMPILE | `error_classes.ts` exits 2 |
| `RegExp` (test, exec, match, replace) | COMPILE | `regex_basic.ts` exits 16 — std::regex can't handle named groups / lookbehind; needs RE2 |
| TypedArrays (Int8Array ... Float64Array) | PASS | with callback methods |
| `console.log` / `console.error` | PASS | |
| `DisposableStack` / `AsyncDisposableStack` | PASS | |

## 5. Modules & built-ins

| Feature | Status | Notes |
|---|---|---|
| ES module `import` / `export` | PASS | resolves relative paths, prevents circular imports |
| Bare Node.js module imports (`"http"`, `"fs"`, ...) | PASS | treated as built-in |
| `nova:fs`, `nova:path`, `nova:http`, ... (40+ modules) | PASS | |
| Tagged template literals | PASS | |
| HTTP `createServer` with arrow callbacks | PASS | |

## 6. Compiler internals

| Subsystem | Status |
|---|---|
| Lexer | PASS |
| Parser (ES2024 + TypeScript 5.x subset) | PASS |
| HIR generator (visitor-based) | PASS |
| MIR generator | PASS |
| LLVM IR codegen (LLVM 18.1.7) | PASS |
| Native binary emission | PASS |
| Compilation cache (with `nova.exe` mtime in key) | PASS |
| TypeChecker (inference, union/intersection, assignment checking) | PASS |

---

## Known runtime bugs (Phase 6 work)

21 conformance tests compile cleanly but produce wrong runtime behavior.
Grouped by suspected cause:

- **Regex engine limitations** — `regex_basic.ts` (exit 16). `std::regex` cannot handle
  named groups or lookbehind. Fix: integrate RE2 (Phase 4.1 in plan).
- **Exception machinery** — `try_catch.ts` (exit 1), `generators.ts` (exit 1),
  `promise_static.ts` (segfault). Likely landing-pad / unwind table issues.
- **Stdlib method bugs** — `array_iteration.ts`, `string_methods.ts`, `math_static.ts`,
  `date_basic.ts`, `map_basic.ts`, `weakmap_basic.ts`. Need per-method root-causing.
- **Class machinery** — `decorators.ts`, `private_fields.ts`, `getters_setters.ts`,
  `super_calls.ts` (this last one is now fixed via `isClassFieldAccess` flag).
- **Control flow** — `labeled_loops.ts`, `switch_fallthrough.ts`, `instanceof_typeof.ts`,
  `symbol_iterator.ts`, `template_literals.ts`.
- **Generics** — `ts_generics.ts` (exit 1). Type erasure may not handle all call shapes.
- **Set methods** — `es2024_set_methods.ts` (exit 3). HIR recognition of
  `union`/`intersection`/etc. may be missing the ES2025 path.
- **Error classes** — `error_classes.ts` (exit 2). Likely `cause` property plumbing.

## How to verify

```powershell
# Build
cmake --build build --config Release

# Run full suite (binaries actually execute)
powershell -ExecutionPolicy Bypass -File run_all_tests.ps1

# Run a single test and inspect its runtime behavior
.\build\Release\nova.exe tests\conformance\try_catch.ts
.\.nova-cache\bin\<cached-exe>   # or wherever the emitted binary lives
echo $LASTEXITCODE
```

**Do NOT use `--no-cache` for correctness verification** — it skips execution.
