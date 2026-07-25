# Nova Compiler - 100% JavaScript/TypeScript Coverage Plan

> **Target**: 100% JavaScript (ES2024) + TypeScript (5.x) compatibility
> **Current**: ~85-90% coverage (57/62 conformance test categories)
> **Last Updated**: 2026-07-25

---

## Phase 1: Core Data Structures (Weeks 1-2)

### 1.1 Map & Set Implementation
**Priority**: HIGH | **Effort**: Medium | **Impact**: HIGH

**Why**: Map and Set are fundamental ES6 data structures used in virtually every JS application.

**Implementation**:
```
src/runtime/Map.cpp       - Hash map implementation
src/runtime/Set.cpp       - Set implementation (reuse Map logic)
include/nova/runtime/Map.h
include/nova/runtime/Set.h
tests/conformance/map.ts
tests/conformance/set.ts
```

**Tasks**:
- [ ] `new Map()` - Create empty map
- [ ] `map.set(key, value)` - Store key-value pair
- [ ] `map.get(key)` - Retrieve value
- [ ] `map.has(key)` - Check key exists
- [ ] `map.delete(key)` - Remove key
- [ ] `map.size` - Property
- [ ] `map.keys()`, `map.values()`, `map.entries()`
- [ ] `map.forEach(callback)`
- [ ] `map.clear()`
- [ ] Set with same API (except size → size)
- [ ] Object as keys (use SameValueZero)
- [ ] All edge cases (undefined, null, NaN keys)

**Reference**: [MDN Map](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Map), [MDN Set](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Set)

### 1.2 WeakMap & WeakSet Implementation
**Priority**: MEDIUM | **Effort**: Medium | **Impact**: MEDIUM

**Why**: Required for proper garbage collection patterns, memoization, DOM associations.

**Implementation**:
```
src/runtime/WeakMap.cpp
src/runtime/WeakSet.cpp
include/nova/runtime/WeakMap.h
include/nova/runtime/WeakSet.h
tests/conformance/weakmap.ts
tests/conformance/weakset.ts
```

**Tasks**:
- [ ] `new WeakMap()` - Create with weak references
- [ ] `weakMap.set(key, value)` - Object keys only
- [ ] `weakMap.get(key)`, `weakMap.has(key)`, `weakMap.delete(key)`
- [ ] WeakSet with same pattern
- [ ] No iterators (by design - prevents memory leaks)
- [ ] Integration with GC for cleanup

---

## Phase 2: TypeScript Type System Enhancement (Weeks 2-4)

### 2.1 Generics Implementation
**Priority**: HIGH | **Effort**: HIGH | **Impact**: HIGH

**Why**: TypeScript generics are essential for type-safe reusable code.

**Implementation**:
```
src/frontend/sema/TypeChecker.cpp  - Add generic support
src/hir/HIRGen.cpp                 - HIR generic instantiation
tests/conformance/typescript_generics.ts
```

**Tasks**:
- [ ] Generic type parameters `<T>`
- [ ] Generic function constraints `T extends string`
- [ ] Generic class `<T> { }`
- [ ] Generic interface `<T> { }`
- [ ] Generic alias `type Box<T> = { value: T }`
- [ ] Multiple type parameters `<T, U, V>`
- [ ] Default type parameters `<T = string>`
- [ ] Generic inference in function calls
- [ ] `keyof` operator
- [ ] Indexed access types `T[K]`

**Example test case**:
```typescript
function identity<T>(arg: T): T {
    return arg;
}
const result = identity<string>("hello");

function longest<T extends { length: number }>(a: T, b: T): T {
    return a.length > b.length ? a : b;
}

class Container<T> {
    value: T;
    constructor(v: T) { this.value = v; }
}
```

### 2.2 Conditional Types
**Priority**: MEDIUM | **Effort**: HIGH | **Impact**: MEDIUM

**Tasks**:
- [ ] Basic `T extends U ? X : Y`
- [ ] Distributive conditional types
- [ ] `infer` keyword
- [ ] Built-in conditional types (ReturnType, Parameters, etc.)

### 2.3 Mapped Types
**Priority**: MEDIUM | **Effort**: HIGH | **Impact**: MEDIUM

**Tasks**:
- [ ] `{ [K in keyof T]: T[K] }` - Identity mapped type
- [ ] `{ [K in keyof T as Exclude<K, "id">]: T[K] }` - With key remapping
- [ ] `{ [K in keyof T as \`get\${K}\`]: () => T[K] }` - Template literal keys
- [ ] `{ [K as string]: T }` - Index signatures
- [ ] Readonly, Partial, Required, Pick, Record helpers

### 2.4 Template Literal Types
**Priority**: MEDIUM | **Effort**: MEDIUM | **Impact**: MEDIUM

**Tasks**:
- [ ] `${T}` interpolation
- [ ] `${string}`, `${number}` unions
- [ ] Uppercase, Lowercase, Capitalize, Uncapitalize intrinsics

---

## Phase 3: Async & Promise Enhancement (Weeks 3-4)

### 3.1 Promise Full Implementation
**Priority**: HIGH | **Effort**: MEDIUM | **Impact**: HIGH

**Implementation**:
```
src/runtime/Promise.cpp  - Complete implementation
tests/conformance/promise_edge_cases.ts
```

**Tasks**:
- [ ] Spec-exact microtask queue ordering
- [ ] `Promise.allSettled()` - Full implementation
- [ ] `Promise.any()` - Full implementation (aggregate error)
- [ ] `Promise.withResolvers()` - TC39 proposal
- [ ] Thenable assimilation (duck typing)
- [ ] Unhandled rejection tracking
- [ ] `finally` callback execution order

**Reference**: [MDN Promise](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise)

### 3.2 Async Iteration Enhancement
**Priority**: MEDIUM | **Effort**: MEDIUM | **Impact**: MEDIUM

**Tasks**:
- [ ] `ReadableStream` (partial)
- [ ] `for await...of` with async iterables
- [ ] `AsyncIterator` and `AsyncIterable` interfaces

---

## Phase 4: Module System Completion (Weeks 4-5)

### 4.1 Namespace Imports
**Priority**: MEDIUM | **Effort**: MEDIUM | **Impact**: MEDIUM

**Tasks**:
- [ ] `import * as namespace from "module"`
- [ ] `namespace.default` access
- [ ] `namespace.namedExport` access

### 4.2 Live Bindings & Re-exports
**Priority**: MEDIUM | **Effort**: MEDIUM | **Impact**: MEDIUM

**Tasks**:
- [ ] Re-export with live binding updates
- [ ] Re-export all `export * from "module"`
- [ ] Re-export renamed `export { foo as bar } from "module"`

### 4.3 Circular Dependencies
**Priority**: MEDIUM | **Effort**: HIGH | **Impact**: MEDIUM

**Tasks**:
- [ ] Detect circular imports
- [ ] Handle hoisted declarations
- [ ] Module evaluation order

### 4.4 Package Resolution
**Priority**: HIGH | **Effort**: HIGH | **Impact**: HIGH

**Tasks**:
- [ ] `node_modules` directory resolution
- [ ] `package.json` exports field
- [ ] `.js`, `.ts`, `.nova` extensions
- [ ] `index` files
- [ ] Bare specifiers mapping

---

## Phase 5: Standard Library Completions (Weeks 5-7)

### 5.1 Full Date Implementation
**Priority**: HIGH | **Effort**: MEDIUM | **Impact**: HIGH

**Implementation**:
```
src/runtime/Date.cpp  - Complete Date implementation
include/nova/runtime/Date.h
tests/conformance/date.ts
```

**Tasks**:
- [ ] `new Date()` - Current time
- [ ] `new Date(value)` - Timestamp
- [ ] `new Date(year, month, day, ...)` - Components
- [ ] `new Date(dateString)` - ISO parsing
- [ ] `date.getFullYear()`, `.getMonth()`, `.getDate()`
- [ ] `date.getHours()`, `.getMinutes()`, `.getSeconds()`
- [ ] UTC variants
- [ ] `date.setFullYear()`, `.setMonth()`, etc.
- [ ] `date.toISOString()`, `.toUTCString()`
- [ ] `date.toLocaleDateString()`, `.toLocaleTimeString()`
- [ ] `Date.parse()`, `Date.UTC()`
- [ ] 50+ methods total

**Reference**: [MDN Date](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Date)

### 5.2 Full Error Hierarchy
**Priority**: MEDIUM | **Effort**: MEDIUM | **Impact**: MEDIUM

**Tasks**:
- [ ] `Error` - Base class
- [ ] `TypeError` - Type mismatches
- [ ] `RangeError` - Out of range
- [ ] `SyntaxError` - Parse errors
- [ ] `ReferenceError` - Undefined references
- [ ] `URIError` - URI encoding errors
- [ ] `EvalError` - eval() errors
- [ ] `AggregateError` - Multiple errors
- [ ] `.message`, `.name`, `.stack` properties
- [ ] Custom error subclasses

### 5.3 Full RegExp Enhancement
**Priority**: MEDIUM | **Effort**: MEDIUM | **Impact**: MEDIUM

**Tasks**:
- [ ] `new RegExp(pattern, flags)`
- [ ] `.test()`, `.exec()`
- [ ] `.match()`, `.replace()`, `.search()`, `.split()`
- [ ] Unicode flag `u`
- [ ] Sticky flag `y`
- [ ] DotAll flag `s`
- [ ] Named capture groups
- [ ] Lookahead/lookbehind
- [ ] `.source`, `.flags`, `.lastIndex`
- [ ] `$1`, `$2`... replacement patterns

### 5.4 Proxy Implementation
**Priority**: MEDIUM | **Effort**: HIGH | **Impact**: MEDIUM

**Tasks**:
- [ ] `new Proxy(target, handler)`
- [ ] `get` trap
- [ ] `set` trap
- [ ] `has` trap (in operator)
- [ ] `deleteProperty` trap
- [ ] `apply` trap (functions)
- [ ] `construct` trap (new)
- [ ] `getOwnPropertyDescriptor` trap
- [ ] `defineProperty` trap
- [ ] `getPrototypeOf`, `setPrototypeOf`
- [ ] `isExtensible`, `preventExtensions`
- [ ] `ownKeys` trap
- [ ] Revocable proxies

### 5.5 Reflect Implementation
**Priority**: MEDIUM | **Effort**: MEDIUM | **Impact**: MEDIUM

**Tasks**:
- [ ] `Reflect.get(target, prop, receiver)`
- [ ] `Reflect.set(target, prop, value, receiver)`
- [ ] `Reflect.has(target, prop)`
- [ ] `Reflect.deleteProperty(target, prop)`
- [ ] `Reflect.apply(target, thisArg, args)`
- [ ] `Reflect.construct(target, args, newTarget)`
- [ ] `Reflect.getOwnPropertyDescriptor(target, prop)`
- [ ] `Reflect.defineProperty(target, prop, descriptor)`
- [ ] `Reflect.getPrototypeOf(target)`
- [ ] `Reflect.setPrototypeOf(target, prototype)`
- [ ] `Reflect.isExtensible(target)`
- [ ] `Reflect.preventExtensions(target)`
- [ ] `Reflect.ownKeys(target)`
- [ ] `Reflect.ownKeys(target)` (Symbol keys)

### 5.6 Intl (Internationalization)
**Priority**: LOW | **Effort**: MEDIUM | **Impact**: LOW

**Tasks**:
- [ ] `Intl.DateTimeFormat`
- [ ] `Intl.NumberFormat`
- [ ] `Intl.Collator`
- [ ] `Intl.DisplayNames`
- [ ] `Intl.ListFormat`
- [ ] `Intl.RelativeTimeFormat`
- [ ] `Intl.Segmenter`

---

## Phase 6: Iterator & Array Enhancements (Weeks 7-8)

### 6.1 Iterator Helpers (ES2024)
**Priority**: MEDIUM | **Effort**: MEDIUM | **Impact**: MEDIUM

**Tasks**:
- [ ] `Iterator.prototype.map(fn)`
- [ ] `Iterator.prototype.filter(fn)`
- [ ] `Iterator.prototype.take(n)`
- [ ] `Iterator.prototype.drop(n)`
- [ ] `Iterator.prototype.flatMap(fn)`
- [ ] `Iterator.prototype.reduce(fn, initial)`
- [ ] `Iterator.prototype.toArray()`
- [ ] `Iterator.prototype.forEach(fn)`
- [ ] `Iterator.prototype.some(fn)`
- [ ] `Iterator.prototype.every(fn)`
- [ ] `Iterator.prototype.find(fn)`
- [ ] `Iterator.from(iterable)`

### 6.2 Array.prototype.groupBy (ES2024)
**Priority**: MEDIUM | **Effort**: LOW | **Impact**: MEDIUM

**Tasks**:
- [ ] `Array.prototype.groupBy(fn)` - Returns object
- [ ] `Array.prototype.groupByToMap(fn)` - Returns Map
- [ ] `Array.prototype.toReversed()` - Already done ✅
- [ ] `Array.prototype.toSorted()` - Already done ✅
- [ ] `Array.prototype.toSpliced()` - Already done ✅
- [ ] `Array.prototype.with(index, value)` - Already done ✅

### 6.3 Array Sorting Enhancement
**Priority**: MEDIUM | **Effort**: MEDIUM | **Impact**: MEDIUM

**Tasks**:
- [ ] Stable sort for `Array.prototype.sort()`
- [ ] Proper locale-aware sorting
- [ ] TypedArray sort methods

---

## Phase 7: Remaining Features (Weeks 8-10)

### 7.1 Atomics & SharedArrayBuffer
**Priority**: LOW | **Effort**: HIGH | **Impact**: LOW

**Tasks**:
- [ ] `SharedArrayBuffer`
- [ ] `Atomics.load()`, `.store()`
- [ ] `Atomics.wait()`, `.notify()`
- [ ] `Atomics.add()`, `.sub()`, `.and()`, `.or()`, `.xor()`
- [ ] `Atomics.exchange()`, `.compareExchange()`

### 7.2 WebAssembly
**Priority**: LOW | **Effort**: VERY HIGH | **Impact**: MEDIUM

**Tasks**:
- [ ] `WebAssembly.instantiate()`
- [ ] `WebAssembly.instantiateStreaming()`
- [ ] `WebAssembly.validate()`
- [ ] `WebAssembly.compile()`
- [ ] `WebAssembly.Memory`
- [ ] `WebAssembly.Table`
- [ ] `WebAssembly.Global`
- [ ] `WebAssembly.Instance`
- [ ] `WebAssembly.Module`

### 7.3 Import Assertions/Attributes
**Priority**: MEDIUM | **Effort**: MEDIUM | **Impact**: LOW

**Tasks**:
- [ ] `import json from "./data.json" assert { type: "json" }`
- [ ] `import("./module.js", { with: { type: "json" } })`

### 7.4 Decorators (TC39 Final)
**Priority**: MEDIUM | **Effort**: HIGH | **Impact**: MEDIUM

**Tasks**:
- [ ] Class decorators `@sealed`
- [ ] Method decorators `@logged`
- [ ] Accessor decorators `@configurable(false)`
- [ ] Field decorators `@default(0)`
- [ ] Parameter decorators
- [ ] Decorator metadata

---

## Phase 8: Testing & Documentation (Ongoing)

### 8.1 Conformance Test Coverage
**Priority**: HIGH | **Effort**: HIGH | **Impact**: HIGH

**Tasks**:
- [ ] Achieve 100% MDN JavaScript examples
- [ ] TypeScript compiler test suite
- [ ] ECMAScript test262 compatibility
- [ ] Real-world framework tests (simplified)

### 8.2 Documentation
**Priority**: MEDIUM | **Effort**: MEDIUM | **Impact**: MEDIUM

**Tasks**:
- [ ] Update `docs/METHODS_STATUS.md` - 100% coverage
- [ ] Update `docs/TS_JS_COMPATIBILITY.md` - Complete
- [ ] Add missing method documentation
- [ ] API reference generation

---

## Priority Matrix

```
                    IMPACT
                    Low    Medium   High
        Hard    [Intl]  [Atomics] [WASM]
EFFORT           [WASM]
         Medium  [WeakMap] [Proxy] [Decorators]
                   [Reflect] [Date]
         Easy    [groupBy] [Intl]  [Map/Set]
```

**Recommended Execution Order**:
1. **Phase 1**: Map/Set (HIGH impact, MEDIUM effort)
2. **Phase 4**: Package resolution (enables real modules)
3. **Phase 5**: Date, Error hierarchy (HIGH impact)
4. **Phase 2**: TypeScript generics (HIGH impact, HIGH effort)
5. **Phase 3**: Promise enhancement
6. **Phase 5**: RegExp, Proxy, Reflect
7. **Phase 6**: Iterator helpers, Array enhancements
8. **Phase 7**: Atomics, WASM, Decorators

---

## Success Criteria

### JavaScript (ES2024) - 100% Target
- [ ] All ES2024 features implemented
- [ ] 100% test262 compatibility (noted: full test262 is ~30,000 tests)
- [ ] MDN JavaScript examples pass

### TypeScript (5.x) - 100% Target
- [ ] All type system features implemented
- [ ] tsconfig.json support (paths, composite projects)
- [ ] Declaration file (.d.ts) emission

### Interoperability
- [ ] CommonJS require() support
- [ ] npm package compatibility
- [ ] Node.js module resolution
- [ ] Browser runtime compatibility

---

## Progress Tracking

| Phase | Feature | Status | Tests | Notes |
|-------|---------|--------|-------|-------|
| 1.1   | Map/Set | TODO   | 0/20  | |
| 1.2   | WeakMap/WeakSet | TODO | 0/10 | |
| 2.1   | Generics | TODO   | 0/30  | |
| 2.2   | Conditional Types | TODO | 0/15 | |
| 2.3   | Mapped Types | TODO | 0/20 | |
| 2.4   | Template Literal Types | TODO | 0/10 | |
| 3.1   | Promise Enhancement | TODO | 0/15 | |
| 3.2   | Async Iteration | TODO | 0/10 | |
| 4.1   | Namespace Imports | TODO | 0/5 | |
| 4.2   | Live Bindings | TODO | 0/10 | |
| 4.3   | Circular Dependencies | TODO | 0/10 | |
| 4.4   | Package Resolution | TODO | 0/20 | |
| 5.1   | Full Date | TODO   | 0/50  | Only Date.now() currently |
| 5.2   | Error Hierarchy | TODO | 0/10 | |
| 5.3   | Full RegExp | TODO | 0/20 | |
| 5.4   | Proxy | TODO   | 0/15  | Stubs only |
| 5.5   | Reflect | TODO   | 0/15  | Stubs only |
| 5.6   | Intl | TODO   | 0/30  | |
| 6.1   | Iterator Helpers | TODO | 0/15 | |
| 6.2   | Array.groupBy | TODO | 0/5 | |
| 6.3   | Stable Sort | TODO | 0/5 | |
| 7.1   | Atomics | TODO   | 0/15  | |
| 7.2   | WebAssembly | TODO | 0/50 | |
| 7.3   | Import Attributes | TODO | 0/5 | |
| 7.4   | Decorators | TODO   | 0/30  | |

---

## Notes

- **test262**: Full ECMAScript conformance test suite has ~30,000 tests. Targeting 100% is aspirational but not practical. Focus on real-world compatibility.
- **WASM**: Full WebAssembly implementation is a major undertaking. Consider using existing implementations (WABT, WAVM).
- **Decorators**: TC39 Stage 3 spec changed significantly. Need to track final spec.

---

**Next Action**: Start Phase 1.1 (Map/Set implementation) to achieve immediate high-impact results.
