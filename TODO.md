# Nova Compiler - TODO List

> **Last Updated**: 2026-07-22
> **Current Version**: v1.4.0
> **Test Status**: 57 verified conformance tests passing; legacy suite migration in progress

---

## Recent Updates (December 2025)

### ✅ Completed
- **Code Cleanup**: Removed all DEBUG code from compiler source files
  - Disabled NOVA_DEBUG in 6 core files (main.cpp, LLVMCodeGen.cpp, HIRGen.cpp, MIRGen.cpp, HIRBuilder.cpp, Iterator.cpp)
  - Cleaned up 1400+ debug statements for production readiness
- **Package Manager**: Added `nova pm` subcommand for better UX
  - Unified package manager interface (`nova pm install`, `nova pm update`, etc.)
  - Full npm-compatible package management
- **Website Documentation**: Comprehensive documentation added
  - CLI commands reference with detailed examples
  - Configuration guide (nova.config.json)
  - Performance optimization tips
  - Debugging guide and testing framework docs
  - Deployment guides (standalone binary, Docker, systemd)
- **Multi-language Support**: Website now supports Thai (ไทย) and English
- **Installation Scripts**: Added Bun-style one-liner installers for macOS/Linux/Windows

---

## Current Status

The Nova compiler implements a broad experimental TypeScript/JavaScript subset. Conformance work is ongoing; only tests with explicit expected results are counted as verified.

### Implemented Features

- Variables: let, const, var
- Functions: regular, arrow, async, generators
- Classes: properties, methods, constructors, inheritance, static, getters/setters
- Control flow: if/else, switch, ternary, loops (for, while, do-while, for-of, for-in)
- Error handling: try/catch/finally, throw
- Operators: numeric arithmetic plus shared JavaScript truthiness, short-circuiting, operand-returning `&&`/`||`, and truthiness-aware `&&=`/`||=` are conformance-verified
- Numeric core: decimal and mixed integer/float arithmetic, JavaScript division, floating comparisons/NaN behavior, exponentiation, and standard Math constants are conformance-verified
- Equality core: strict primitive type separation, Number equality across integer/float HIR, null/undefined distinction, object/array identity, and common abstract number/string/boolean coercions are conformance-verified
- Arrays: broad method surface; tagged mixed-value coverage currently includes indexing, assignment, core mutation/search, slice, and concat
- Strings: full method support (25+ methods)
- Objects: literal and computed-string property reads/writes; keys/values/entries/getOwnPropertyNames, static data-property descriptors and existing-field defineProperty attributes, literal-array fromEntries with duplicate-key handling, empty getOwnPropertySymbols, literal-key hasOwn, Object.is Number SameValue and aggregate identity, attribute-aware overlapping-field assign, and identifier-tracked integrity controls are verified for static object literals
- JSON: primitive stringify plus recursive static objects and homogeneous arrays, including string escaping
- Math: all methods and constants
- Number: all methods and constants
- BigInt: arbitrary-precision literals/construction, arithmetic, comparison,
  bitwise/shift, unary, update and compound-assignment paths are conformance-verified
- Symbol: uniqueness, registry/keyFor, description, primitive methods,
  well-known identity and `typeof` are conformance-verified
- Modules: named/default file imports, function aliases and exported primitive
  literal constants are conformance-verified
- Console: log, error, warn, info, debug, etc.
- TypedArrays: Int8Array, Uint8Array, etc. with full method support
- Async/Await: async functions, await expressions
- Generators: function*, yield, yield*
- Async Generators: async function*, for-await-of
- Destructuring: nested array/object declarations and assignments, lazy defaults,
  rest bindings, and declaration/function-expression/sync-or-async arrow
  parameters are conformance-verified
- Spread operator: arrays and objects
- Rest parameters: declarations, arrows, function expressions, async arrows,
  empty/mixed values and forwarding are conformance-verified
- Default parameters
- Template literals
- Optional chaining: ?.
- Nullish coalescing: ??
- typeof, instanceof, in, delete operators
- Enums
- using/DisposableStack
- Type checking: `nova check` now rejects primitive/union/array assignment errors, invalid function arguments, return mismatches, and `satisfies` mismatches with TS-style diagnostics

---

## Future Improvements

### Performance
- [x] Remove debug overhead (DEBUG code removed)
- [ ] Implement lazy compilation
- [ ] Add caching for repeated compilations
- [ ] Optimize runtime library further

### Language Features
- [ ] Full closure support (capturing variables)
- [ ] Full Promise chain support
- [ ] WeakMap/WeakSet
- [ ] Symbol-keyed dynamic object properties and complete symbol coercion errors
- [ ] Proxy/Reflect
- [ ] Full module system (import/export)

### Tooling
- [ ] Source maps
- [ ] Debugger support
- [ ] Language server protocol (LSP)
- [ ] Watch mode for development

### Documentation
- [x] Comprehensive user documentation (website)
- [x] CLI commands reference
- [x] Configuration guide
- [x] Performance and debugging guides
- [x] Deployment documentation
- [ ] API reference (detailed)
- [ ] Internal architecture guide
- [ ] Contributing guide

---

## Known Limitations

1. **Closures and function invocation**: Returned and local function-expression/arrow closures, typed anonymous parameters, tagged polymorphic arguments, shared primitive/object binding mutation, escaped heap cells and object aggregates, transitive nested environments, declared-parameter `arguments` with lexical arrow capture, local and escaped arrow lexical `this`, captured Promise-executor writes, named-function argument forwarding through `call`, literal `apply`, partial `bind`, and primitive ordinary-function `thisArg` forwarding are verified; object/function receivers, non-literal apply iterables, dynamic/closure call/apply/bind targets, strict/sloppy default-receiver rules, extra/destructured/generator `arguments`, array/class aggregate lifetime edge cases, and full callback coverage remain incomplete
2. **Promises**: resolution/adoption, recovery chains, constructor executors,
   first-class resolver alias/escape behavior, and all/race/any are verified;
   spec-exact microtask ordering, thenable assimilation and the remaining
   static/instance edge cases are still incomplete
3. **Modules**: named/default file imports, aliases and exported primitive literals work; namespace imports, live bindings, re-exports, cycles, package resolution and non-literal module state remain incomplete
4. **Type checking**: The first real checker covers primitive/union/array annotations, inferred bindings, assignments, direct function calls/returns, and `satisfies`; structural object types, generics, overloads, narrowing/control-flow analysis, conditional/mapped/template-literal types, declarations, modules, and tsconfig compatibility remain incomplete
5. **Dynamic objects**: Runtime-created objects do not yet share metadata with static object literals; dynamic keys/values/entries and full recursive JSON serialization remain incomplete
6. **Object integrity controls**: static identifier/alias paths enforce freeze and report freeze/seal/extensibility state, but dynamic runtime helpers remain stubs; deletion, field addition, and shadowed-name edge cases are incomplete
7. **Mixed arrays**: NaN-boxed `JSValue` slots now preserve heterogeneous literal values, indexing, assignment, core mutation/search methods, and `slice`/`concat`; callbacks, sorting/coercive methods, spread, JSON, and every remaining array API still need tagged-value conformance
8. **Advanced Object APIs**: dynamic-iterable `Object.fromEntries`, `Object.create`, prototype operations, accessor/dynamic descriptors, adding fields through `defineProperty`, and `groupBy` are runtime placeholders or fixed-layout limitations; only literal entry arrays and existing static data fields are conformance-verified
9. **Unified values and nullish flow**: A NaN-boxed `JSValue` now flows through HIR/MIR/LLVM/runtime for heterogeneous arrays, type-changing object-literal fields, local type-changing variables across branches/loops, uninitialized bindings, dynamic arithmetic/relational/bitwise coercion, `??`/`??=`, truthiness, equality, console output, heterogeneous function returns, direct polymorphic parameters, function values, and captured closures. Class fields, runtime property maps, indirect/reflective calls, and remaining object/primitive coercion edge cases still need complete migration

---

## Version History

See [CHANGELOG.md](CHANGELOG.md) for detailed version history.
