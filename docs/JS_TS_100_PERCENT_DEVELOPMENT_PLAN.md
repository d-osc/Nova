# Nova JavaScript/TypeScript 100% Development Plan

Last updated: 2026-07-27  
Target baseline: ECMAScript 2024 + TypeScript 5.6  
Primary evidence: `run_all_tests_fail_debug.txt`

## 1. Executive summary

Nova must fix correctness and compiler safety before expanding the advertised
feature surface. The current local baseline is:

| Gate | Result |
|---|---:|
| Failure-debug unit tests | 4/4 pass |
| Verified conformance tests | 107 |
| Passing conformance tests | 80 |
| Failing conformance tests | 27 |
| Skipped tests | 1 |
| Failure-debug locations | 90 |
| Native access-violation locations | 9 |
| Compiler/parser diagnostic locations | 56 |
| Missing-output locations | 15 |

Breakdown of the 90 failure-debug locations:

| Area | Locations | Files | Native crashes | Diagnostics |
|---|---:|---:|---:|---:|
| Existing conformance | 15 | 8 | 7 | 0 |
| JavaScript 100% target | 26 | 12 | 2 | 15 |
| TypeScript 100% target | 49 | 7 | 0 | 41 |

The immediate priorities are therefore:

1. Remove all native crashes and establish one safe callable/value ABI.
2. Finish contextual JavaScript and TypeScript parsing so valid programs reach
   the checker/runtime.
3. Complete JavaScript coercion, property/prototype and async semantics.
4. Rebuild the TypeScript checker around a richer type representation and
   control-flow graph.
5. Validate against upstream Test262 and TypeScript conformance suites.

Passing the 107 local tests is the first milestone, not proof of 100%
compatibility.

## 2. Definition of “100% support”

Nova may claim 100% support only when all of these gates are satisfied:

- All local expectation-based tests pass by compiling and executing native
  binaries where applicable.
- `run_all_tests_fail_debug.txt` contains only its header: zero crashes, zero
  diagnostics, zero missing expectations.
- Test262 passes for the agreed ECMAScript 2024 profile. Host-defined tests may
  be excluded only through a reviewed, documented allowlist.
- The TypeScript 5.6 compiler conformance suite passes for parsing, binding,
  type checking, diagnostics, module resolution and emit behavior in scope.
- `.d.ts` consumption and emit, `tsconfig.json`, ESM/CommonJS interop, source
  maps and package resolution have integration coverage.
- No feature is represented by a placeholder, silent fallback to `Any`, or
  hard-coded success value.
- Debug, Release and sanitizer builds produce the same observable result.

## 3. Dependency order

```mermaid
flowchart LR
    A["Test and crash observability"] --> B["Lexer, parser and AST"]
    A --> C["Unified JSValue and callable ABI"]
    B --> D["JavaScript lowering"]
    C --> D
    B --> E["TypeScript binder and checker"]
    C --> F["Object model and runtime"]
    D --> G["Async, generators and templates"]
    F --> G
    E --> H["Declarations, TSX and modules"]
    F --> I["Standard library conformance"]
    G --> J["Local suite 107/107"]
    H --> J
    I --> J
    J --> K["Test262 and TypeScript upstream suites"]
    K --> L["Ecosystem qualification"]
```

Do not implement later phases by adding special cases to HIR call lowering.
That would increase the current coupling in `HIRGen_Calls.cpp` and make
spec-level completion harder.

## 4. Phase 0 — Safety and deterministic diagnostics

Priority: P0  
Estimated effort: 1–2 engineer-weeks

### Work

- Keep `tests/test_run_all_tests_debug.py` as a mandatory pre-test gate.
- Add Debug and AddressSanitizer CI configurations.
- Preserve symbols for generated native binaries and record the generated
  function name on runtime faults.
- Make compiler/runtime failures deterministic across cached and uncached runs.
- Add one minimized test per crash before changing implementation.
- Separate compiler failure, linker failure and emitted-program failure in the
  test result model.

### Current crash targets

| Test | Failure location | Likely ownership |
|---|---|---|
| `error_classes.ts` | `CustomError.constructor`, `ValidationError.constructor`, `NotFoundError.constructor` | `HIRGen_Classes.cpp`, object layout, exception runtime |
| `generators.ts` | `range`, `counter` | `HIRGen_Functions.cpp`, `HIRGen_Advanced.cpp`, `LLVMCodeGen.cpp` |
| `js_spec_errors.js` | `ApplicationError.constructor` | class/super allocation and Error runtime |
| `js_spec_promises.js` | native runtime | `Promise.cpp`, callback ABI/lifetime |
| `promise_static.ts` | native runtime | `Promise.cpp`, static combinators |
| `template_literals.ts` | native runtime | `HIRGen_Advanced.cpp`, JSValue-to-string conversion |

### Exit gate

- `0xC0000005`: 9 → 0.
- Every native failure reports a generated function and source test line.
- Debug and Release have identical pass/fail results.

## 5. Phase 1 — Lexer, parser and AST completion

Priority: P0  
Estimated effort: 3–5 engineer-weeks

This phase removes the 56 `ERROR_DIAGNOSTIC` locations before deeper semantic
work.

### 1.1 Contextual identifiers and object members

Primary files:

- `src/frontend/lexer/Lexer.cpp`
- `src/frontend/parser/ExprParser.cpp`
- `src/frontend/parser/StmtParser.cpp`
- `include/nova/Frontend/AST.h`

Tasks:

- Treat `get`, `set`, `async`, `from`, `of`, and TypeScript modifier words as
  contextual where ECMAScript permits `IdentifierName`.
- Parse computed object methods such as `[Symbol.iterator]() {}`. The current
  object-literal path requires `:` immediately after `]`.
- Distinguish accessor syntax from ordinary methods named `get` or `set`.
- Permit valid method names in Proxy handler objects.
- Preserve computed keys and method kind in the AST instead of placeholder
  names.

Unblocked tests:

- `js_spec_collections.js`
- `js_spec_iterators_generators.js`
- `js_spec_proxy_reflect.js`
- portions of `js_spec_intl.js`

### 1.2 TypeScript declaration grammar

Primary files:

- `src/frontend/parser/DeclParser.cpp`
- `src/frontend/parser/StmtParser.cpp`
- `include/nova/Frontend/Parser.h`
- `include/nova/Frontend/AST.h`

Tasks:

- Parse `readonly` interface members and class modifiers.
- Parse generic constraints and defaults.
- Parse overload signatures without bodies and connect them to the
  implementation signature.
- Parse `declare` declarations, ambient classes/functions/constants and
  namespaces.
- Parse abstract classes/methods and `override`.
- Preserve source locations on all type nodes and declarations.

Unblocked tests:

- `ts_spec_structural_generics.ts`
- `ts_spec_classes_overloads.ts`
- `ts_spec_declarations.ts`
- `ts_spec_negative.ts`

### 1.3 Advanced type grammar

Tasks:

- Complete conditional types and `infer`.
- Complete mapped types, modifiers and key remapping.
- Complete indexed-access, `keyof`, template-literal and utility type syntax.
- Parse type predicates such as `value is string` and assertion signatures.
- Make generic `>` token handling safe around nested type arguments.

Unblocked tests:

- `ts_spec_type_operators.ts`
- `ts_spec_narrowing.ts`

### 1.4 TSX mode

Primary files:

- `src/frontend/lexer/Lexer.cpp`
- `src/frontend/parser/ExprParser.cpp`
- `src/hir/HIRGen_Advanced.cpp`

Tasks:

- Select TSX lexical mode from `.tsx`/`.jsx`.
- Resolve `<T>` assertion versus JSX ambiguity.
- Parse JSX namespace declarations, intrinsic elements, attributes, spread
  attributes, fragments, nested children and expression containers.
- Replace the current opaque/null JSX lowering with configurable JSX emit.

Unblocked test:

- `ts_spec_jsx.tsx`

### Exit gate

- `ERROR_DIAGNOSTIC`: 56 → 0 for valid target tests.
- Every valid target source builds an AST.
- Parser unit tests cover every fixed grammar production.

## 6. Phase 2 — Unified JSValue, object model and callable ABI

Priority: P0/P1  
Estimated effort: 5–8 engineer-weeks

Several failures come from fixed-layout objects and call lowering that knows
the static identity of a variable rather than runtime value semantics.

### 2.1 JSValue conversion contract

Primary files:

- `src/hir/HIRGen_Operators.cpp`
- `src/hir/HIRGen_Calls.cpp`
- `src/mir/MIRGen.cpp`
- `src/codegen/LLVMCodeGen.cpp`
- runtime value implementation

Tasks:

- Define one authoritative implementation for `ToPrimitive`, `ToBoolean`,
  `ToNumber`, `ToNumeric`, `ToString`, `ToObject` and property-key conversion.
- Route operators, constructors, template interpolation and built-ins through
  those helpers.
- Preserve `NaN`, signed zero, infinities, `null`, `undefined`, BigInt and
  Symbol distinctions.
- Remove silent `Any`/integer placeholders at ABI boundaries.

Current first assertion:

- `js_spec_coercion.js:5` — `Number("")` must produce `0`.

### 2.2 Dynamic object and prototype model

Primary files:

- `src/runtime/Object.cpp`
- `src/hir/HIRGen_Objects.cpp`
- `src/hir/HIRGen_Classes.cpp`

Tasks:

- Introduce shared property storage for static and runtime-created objects.
- Store data/accessor descriptors and enforce writable, enumerable and
  configurable flags.
- Implement prototype links and prototype-chain lookup.
- Implement own-key ordering for strings and Symbols.
- Remove simplified implementations such as `isPrototypeOf` always returning
  false.
- Make `Object.groupBy` return real dynamically keyed arrays.

Unblocked tests:

- `js_spec_objects.js`
- `js_spec_es2024.js`
- `error_classes.ts`
- `js_spec_errors.js`

### 2.3 Proxy and Reflect

Primary files:

- `src/runtime/Proxy.cpp`
- `src/runtime/Reflect.cpp`
- `src/hir/HIRGen_Calls.cpp`

Tasks:

- Route every object internal operation through overridable runtime operations.
- Enforce Proxy invariants for non-configurable properties and
  non-extensible targets.
- Implement revocation and callable/constructable proxies.
- Make Reflect return spec-defined booleans and descriptors.
- Eliminate static-name special casing from HIR lowering.

Unblocked test:

- `js_spec_proxy_reflect.js`

### 2.4 Callable ABI

Tasks:

- Define one callable representation containing code pointer, closure
  environment, `this`, new-target capability and call kind.
- Use it for functions, arrows, class methods, generator resumptions, Promise
  reactions, decorators and Proxy traps.
- Validate argument counts/types before emitting LLVM calls.

This is a prerequisite for Phases 3 and 4.

### Exit gate

- `js_spec_coercion.js`, `js_spec_objects.js` and
  `js_spec_proxy_reflect.js` pass.
- No object API depends on identifier tracking for correctness.
- No generated LLVM call has a signature mismatch.

## 7. Phase 3 — Generators, templates, decorators and async execution

Priority: P1  
Estimated effort: 5–8 engineer-weeks

### 3.1 Generator state machines

Primary files:

- `src/hir/HIRGen_Functions.cpp`
- `src/hir/HIRGen_Advanced.cpp`
- `src/hir/HIRGen_ControlFlow.cpp`
- `src/codegen/LLVMCodeGen.cpp`
- generator runtime

Tasks:

- Formalize the hidden generator parameters and creation/resume ABI.
- Persist locals, exception state, `this`, arguments and finally blocks across
  suspension.
- Implement `next`, `return`, `throw` and `yield*` delegation.
- Use `Symbol.iterator`/`Symbol.asyncIterator` rather than variable-name
  tracking.

Unblocked tests:

- `generators.ts`
- `js_spec_iterators_generators.js`
- `symbol_iterator.ts`

### 3.2 Promise jobs and async functions

Primary file: `src/runtime/Promise.cpp`

Tasks:

- Store full JSValue payloads instead of integer-only promise values.
- Implement thenable assimilation and self-resolution protection.
- Implement spec-ordered microtask/job queues.
- Reimplement `all`, `race`, `allSettled`, `any` and `withResolvers` using the
  callable ABI.
- Preserve fulfillment through `finally` and rejection recovery.
- Add unhandled rejection tracking and deterministic process-drain behavior.

Unblocked tests:

- `async_promises.ts`
- `promise_static.ts`
- `js_spec_promises.js`
- Promise portion of `js_spec_es2024.js`

### 3.3 Template literals

Tasks:

- Convert every interpolated JSValue through the shared `ToString`.
- Preserve cooked/raw template arrays and template-object identity.
- Support nested templates and tagged templates without temporary lifetime
  violations.

Unblocked test:

- `template_literals.ts`

### 3.4 Decorators

Tasks:

- Choose and document the supported decorator semantics (standard ECMAScript
  decorators versus TypeScript legacy mode).
- Apply class, method, accessor and field replacements in declaration order.
- Support initializer queues and decorator context.
- Keep legacy mode behind explicit configuration.

Unblocked test:

- `decorators.ts`

### Exit gate

- All Phase 3 tests pass with zero crash and zero missing output.
- Microtask ordering is deterministic across platforms.

## 8. Phase 4 — Standard-library semantic completion

Priority: P1  
Estimated effort: 5–9 engineer-weeks

### 4.1 Arrays and binary data

Primary files:

- `src/runtime/Array.cpp`
- `src/runtime/ArrayBuffer.cpp`
- `src/runtime/Atomics.cpp`
- `src/hir/HIRGen_Calls.cpp`

Tasks:

- Fix `reduceRight` accumulator order and aggregate-array lifetime.
- Run all array callbacks through the callable ABI with `(value, index, array)`.
- Complete holes, sparse arrays, length mutation and species behavior.
- Verify DataView endianness and detached-buffer checks.
- Ensure Atomics read-modify-write functions return the old value.
- Complete resizable/transferable ArrayBuffer and SharedArrayBuffer semantics.

Current first failures:

- `array_iteration.ts:32` — `reduceRight`.
- `js_spec_binary_data.js:25` — `Atomics.add` old value.

### 4.2 RegExp

Primary files:

- `src/runtime/Regex.cpp`
- `src/hir/HIRGen_Literals.cpp`
- `src/hir/HIRGen_Calls.cpp`

Tasks:

- Replace or supplement `std::regex` with an ECMAScript-compatible engine.
- The selected engine must support named captures, lookbehind, Unicode
  properties, sticky/global state, indices and replacement captures. RE2 alone
  is insufficient because it intentionally omits several required constructs.
- Return match arrays with groups, indices, input and index properties.
- Implement regex-aware `split`, `replace`, `search`, `match` and `matchAll`.

Current first failures:

- `js_spec_regexp.js:7` — named capture groups.
- `regex_basic.ts:63` — regex split result.

### 4.3 Intl

Primary files:

- `src/runtime/Intl.cpp`
- `src/hir/HIRGen_Calls.cpp`

Tasks:

- Use ICU or another conformance-grade locale backend.
- Implement option validation, locale negotiation and `resolvedOptions`.
- Complete NumberFormat, DateTimeFormat, Collator, ListFormat and Segmenter.
- Keep tests deterministic by pinning locale and timezone.

Unblocked test:

- `js_spec_intl.js`

### 4.4 Dynamic code

Primary file: `src/runtime/Utility.cpp`

Full ECMAScript `eval` cannot be implemented by constant folding in an AOT-only
path. Make an explicit architecture decision:

- embed an interpreter/JIT fallback for direct and indirect `eval` and
  `Function`, or
- explicitly exclude dynamic code and stop claiming 100% ECMAScript support.

For the 100% goal, the interpreter/JIT fallback is required. It must share the
same object model and global environment as compiled code.

Unblocked test:

- `js_spec_dynamic_code.js`

### Exit gate

- All JavaScript target probes pass.
- Existing conformance failures in Array, RegExp and Symbol iteration pass.
- No built-in returns a placeholder or hard-coded success object.

## 9. Phase 5 — TypeScript binder and semantic type system

Priority: P1  
Estimated effort: 10–16 engineer-weeks

Do not continue extending the checker by returning `Any` for unknown cases.
Introduce a binder/type graph that retains source declarations and symbol
relationships.

### 5.1 Type representation

Primary files:

- `include/nova/Frontend/AST.h`
- `include/nova/Frontend/TypeChecker.h`
- `src/frontend/sema/TypeChecker.cpp`

Add first-class representations for:

- type parameters and constraints;
- literal and discriminated-union types;
- object/interface/class types with modifiers;
- call/construct/overload signatures;
- conditional and inferred types;
- mapped and indexed-access types;
- template-literal types;
- `never`, `unknown`, `this`, predicates and assertion signatures.

### 5.2 Binder and declaration merging

Tasks:

- Separate symbol binding from expression checking.
- Add value/type/namespace symbol spaces.
- Implement interface and namespace merging.
- Resolve imports, exports and ambient declarations.
- Detect duplicate and conflicting declarations.

### 5.3 Generic inference and instantiation

Tasks:

- Infer type arguments from parameters, contextual return types and object
  structure.
- Enforce constraints and defaults.
- Instantiate generic aliases, interfaces, classes and signatures.
- Cache instantiations without losing recursive types.

### 5.4 Control-flow analysis

Tasks:

- Build a control-flow graph per function.
- Narrow on `typeof`, `instanceof`, equality, discriminants, truthiness and
  property presence.
- Support user-defined type predicates and assertion functions.
- Track definite assignment and reachability.
- Enforce exhaustive `never` paths.

### 5.5 Advanced type evaluation

Tasks:

- Evaluate conditional and distributive conditional types.
- Implement `infer`, mapped modifiers, key remapping and template-literal
  expansion.
- Implement standard utility types from library declarations rather than
  hard-coded names.
- Add recursion/complexity limits compatible with TypeScript diagnostics.

### 5.6 Diagnostics parity

Tasks:

- Emit stable TypeScript-style diagnostic codes and source spans.
- Continue checking after recoverable errors.
- Match overload, readonly and generic-constraint diagnostics.

Target tests:

- `ts_spec_structural_generics.ts`
- `ts_spec_narrowing.ts`
- `ts_spec_type_operators.ts`
- `ts_spec_classes_overloads.ts`
- `ts_spec_negative.ts`

### Exit gate

- All valid TypeScript target tests print the success expectation.
- `ts_spec_negative.ts` emits TS2322, TS2540, TS2344 and TS2769.
- No successful check depends on resolving a feature to `Any`.

## 10. Phase 6 — Declarations, TSX, modules and project tooling

Priority: P2  
Estimated effort: 6–10 engineer-weeks

Tasks:

- Consume and emit `.d.ts`.
- Implement ambient modules, global augmentation and namespace declarations.
- Implement `tsconfig.json` discovery, `extends`, `include`/`exclude`, paths,
  references, incremental/composite builds and JSX options.
- Implement Node-style ESM/CommonJS package resolution, `exports`, `imports`,
  conditions and extension rules.
- Implement live bindings, cycles, re-exports and dynamic import evaluation.
- Implement configurable TSX transforms and automatic JSX runtime imports.
- Generate source maps and declaration maps.

Target tests:

- `ts_spec_declarations.ts`
- `ts_spec_jsx.tsx`
- module fixtures and future project-level integration tests

### Exit gate

- TypeScript target probes pass in isolated-file and project modes.
- Representative npm packages type-check and compile without source patches.

## 11. Phase 7 — Upstream conformance and ecosystem qualification

Priority: P1, continuous  
Estimated effort: 12+ engineer-weeks after core architecture stabilizes

### Test262

- Add a Test262 importer/runner that understands frontmatter, includes,
  features, flags, negative parse/early/runtime tests and async completion.
- Record results by feature and edition.
- Require a reviewed reason and expiry for every exclusion.
- Run a changed-feature shard on each pull request and the full suite nightly.

### TypeScript

- Add a runner for TypeScript parser, fourslash, conformance and project tests.
- Compare diagnostics by code, location and essential message content.
- Track parser, binder, checker, module-resolution and emit results separately.

### Ecosystem

Qualify increasingly difficult packages:

1. pure utility packages;
2. packages using ESM/CJS interop;
3. web frameworks and JSX;
4. native/binary packages through supported FFI;
5. test runners, build tools and monorepos.

### Exit gate

- Upstream dashboards are green for the declared profile.
- No undocumented exclusion is present.
- Performance and memory regressions stay inside agreed budgets.

## 12. Milestones and measurable targets

| Milestone | Required result |
|---|---|
| M0 — Baseline | 80/107 pass; 90 debug locations; report reproducible |
| M1 — Safe compiler | 0 native crashes; sanitizer gate enabled |
| M2 — Parser complete | 0 valid-source `ERROR_DIAGNOSTIC` locations |
| M3 — JavaScript local target | all `js_spec_*` tests pass |
| M4 — TypeScript local target | all `ts_spec_*` tests pass with expected diagnostics |
| M5 — Local release gate | 107/107 verified tests pass; 0 skipped |
| M6 — Spec qualification | agreed Test262 and TypeScript upstream profiles pass |
| M7 — Ecosystem qualification | selected npm/framework matrix passes |

## 13. Pull-request workflow

Every implementation pull request must:

1. Start from a minimized failing conformance test.
2. Add a lower-layer unit test where the defect lives.
3. Avoid weakening, skipping or changing a correct expectation.
4. Run:

   ```powershell
   python -m unittest tests.test_run_all_tests_debug -v
   python tests/run_all_tests.py --prefix js_spec_ --prefix ts_spec_
   python tests/run_all_tests.py
   ```

5. Attach the before/after counts from `run_all_tests_fail_debug.txt`.
6. Pass Debug, Release and sanitizer configurations.
7. Update this plan only when the architecture or acceptance criteria change.

## 14. Recommended team sequence

For parallel work with minimal merge conflicts:

- Compiler-core lane: parser/AST → JSValue/callable ABI → generators/templates.
- Runtime lane: object model → Promise/jobs → standard library.
- TypeScript lane: starts with type AST/binder after parser contracts stabilize,
  then checker and project tooling.
- Conformance lane: maintains minimization, upstream runners and dashboards
  throughout all phases.

Rough order-of-magnitude estimate:

- Local 107/107 gate: 16–28 engineer-weeks.
- Broad ECMAScript 2024/Test262 qualification: an additional 40–80
  engineer-weeks.
- TypeScript 5.6 semantic/project qualification: an additional 60–100
  engineer-weeks.

A defensible 100% claim is therefore a multi-quarter compiler program, not a
single feature sprint. With three experienced compiler/runtime engineers, a
realistic planning range is roughly 10–18 calendar months, depending on the
accepted host/API profile and upstream-suite gap.

## 15. First implementation batch

Start with these five bounded changes:

1. Fix computed object methods and contextual `get`/`set` identifiers.
2. Fix generator hidden-parameter ABI and remove the `range`/`counter` crash.
3. Fix Error subclass `super` allocation and constructor lifetime.
4. Fix `Number("")`, `Array.reduceRight` and `Atomics.add` return semantics.
5. Add TypeScript parser support for readonly members, generic constraints and
   overload signatures.

After each item, rerun the full suite and use the debug-file delta to select
the next smallest root-cause cluster.
