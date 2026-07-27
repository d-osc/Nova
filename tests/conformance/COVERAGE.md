# JavaScript/TypeScript 100% target coverage

Target baselines:

- JavaScript: ECMAScript 2024 language and standard built-ins.
- TypeScript: TypeScript 5.6 syntax and type-system behavior.
- Source formats: `.js`, `.jsx`, `.mjs`, `.cjs`, `.ts`, `.tsx`, `.mts`, `.cts`.

The local suite is a focused release gate, not a replacement for Test262 or
the TypeScript compiler suite.

Current Nova baseline (2026-07-27): **2/21 pass**. The passing probes are the
`.mjs` and `.mts` source-extension checks. The other probes intentionally stay
red until the corresponding compatibility gaps are implemented.

| Area | Target probe |
|---|---|
| Primitive conversion, equality, numeric edge cases | `js_spec_coercion.js` |
| Descriptors, prototypes, own keys | `js_spec_objects.js` |
| Proxy and Reflect internal operations | `js_spec_proxy_reflect.js` |
| Iterator protocol, generators and delegation | `js_spec_iterators_generators.js` |
| Promises and microtask ordering | `js_spec_promises.js` |
| Modern RegExp semantics | `js_spec_regexp.js` |
| Map, Set, WeakMap and iteration order | `js_spec_collections.js` |
| ArrayBuffer, DataView and TypedArray | `js_spec_binary_data.js` |
| Intl constructors and resolved options | `js_spec_intl.js` |
| ES2024 built-ins | `js_spec_es2024.js` |
| Dynamic code and global semantics | `js_spec_dynamic_code.js` |
| Error hierarchy, cause and AggregateError | `js_spec_errors.js` |
| `.mjs` source discovery/execution | `js_spec_module_extension.mjs` |
| Structural typing and generics | `ts_spec_structural_generics.ts` |
| Control-flow narrowing | `ts_spec_narrowing.ts` |
| Conditional, mapped and template-literal types | `ts_spec_type_operators.ts` |
| Classes, modifiers and overloads | `ts_spec_classes_overloads.ts` |
| Required negative diagnostics | `ts_spec_negative.ts` |
| Ambient declarations and namespaces | `ts_spec_declarations.ts` |
| TSX syntax | `ts_spec_jsx.tsx` |
| `.mts` source discovery/execution | `ts_spec_module_extension.mts` |

## Definition of done

Do not label JavaScript/TypeScript support as 100% until all of these gates
are green:

1. Every expectation-based test in this directory passes by compiling and,
   for run tests, executing the emitted native binary.
2. Test262 passes at the agreed feature baseline with documented exclusions
   limited to host-defined behavior.
3. TypeScript's compiler conformance suite passes for syntax, binding,
   module resolution and diagnostics.
4. `.d.ts` consumption/emission, `tsconfig.json`, package resolution,
   CommonJS/ESM interop and source maps have dedicated integration coverage.
5. No test terminates through an access violation, assertion failure or
   unhandled compiler exception.
