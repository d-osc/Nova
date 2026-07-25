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
