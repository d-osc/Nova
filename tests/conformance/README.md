# Nova conformance tests

Only tests with explicit expectations belong in this directory. A run test
should return `0` when every assertion passes and a unique non-zero value for
each failed assertion.

```typescript
// NOVA_TEST_MODE: run
// NOVA_EXPECT_EXIT: 0

function main(): number {
    if (1 + 1 != 2) return 1;
    return 0;
}
```

Optional output checks use JSON strings and may be repeated:

```typescript
// NOVA_EXPECT_STDOUT_CONTAINS: "expected text"
// NOVA_EXPECT_STDERR_CONTAINS: "expected diagnostic"
```

Run the verified suite through `npm test`, CTest, or directly:

```bash
python tests/run_all_tests.py
```

Files in the legacy `tests/` directory are not counted as passing until they
declare expectations and are migrated here.

## 100% target suite

The files prefixed with `js_spec_` and `ts_spec_` are broad release-gate
probes for the ES2024 and TypeScript 5.6 surfaces. They deliberately include
features that Nova does not implement yet. A failure is a compatibility gap,
not a reason to weaken or skip the assertion.

The runner discovers `.js`, `.jsx`, `.mjs`, `.cjs`, `.ts`, `.tsx`, `.mts`,
and `.cts` files. Use the coverage map in `COVERAGE.md` to see which language
areas each target probe owns.

Run only the 100% target probes:

```bash
python tests/run_all_tests.py --prefix js_spec_ --prefix ts_spec_
```

Every run also writes `run_all_tests_fail_debug.txt` at the repository root.
It contains one location per line in this machine-readable form:

```text
file|code|line|function
tests/conformance/example.ts|EXIT_7|42|main
```

Use `--failure-debug PATH` to select a different output file. Runtime assertion
codes are mapped back to their `return <code>` source line. Compiler diagnostics
retain their reported line. Native crashes such as Windows access violations
use a hexadecimal code and report the closest function available from compiler
output, or `<native-runtime>` when no safe source mapping exists.

Passing every local probe is necessary but not sufficient for claiming 100%
compatibility. A 100% claim also requires the upstream Test262 suite, the
TypeScript compiler conformance suite, and real package/ecosystem tests.
