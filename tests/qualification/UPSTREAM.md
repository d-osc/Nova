# Upstream qualification

The local Phase 7 gate and the upstream qualification baselines are separate.
An upstream baseline may contain `FAIL` and `UNSUPPORTED`; it must never be
reported as a green certification until both counts are zero.

## Pinned checkouts

Clone the repositories into the ignored build tree and check out the revisions
recorded in `UPSTREAM_REVISIONS.json`:

```powershell
git clone --depth 1 https://github.com/tc39/test262.git build/qualification/upstream/test262
git clone --depth 1 --filter=blob:none --sparse https://github.com/microsoft/TypeScript.git build/qualification/upstream/TypeScript
git -C build/qualification/upstream/TypeScript sparse-checkout set tests/cases tests/baselines/reference
```

Verify each `HEAD` against `UPSTREAM_REVISIONS.json` before comparing results.

## Full resumable runs

```powershell
python -B tests/qualification/run_upstream_suite.py test262 `
  --nova build/Debug/nova.exe `
  --output build/qualification/results/test262-upstream-full `
  --shards 2048 --workers 16

python -B tests/qualification/run_upstream_suite.py typescript `
  --nova build/Debug/nova.exe `
  --output build/qualification/results/typescript-upstream-full `
  --shards 64 --workers 16
```

Completed shard dashboards are reused automatically. Pass `--no-resume` only
when intentionally replacing every shard result after a compiler or runner
change.

The Test262 adapter currently provides a complete file-inventory baseline, not
official Test262 certification. Strict/non-strict variant generation, complete
async completion handling, host `$262` hooks, realms and negative error-type
matching must be completed before its PASS count can be used as a standards
conformance claim.

The TypeScript adapter executes compiler and conformance inputs and classifies
the remaining language-service/project-harness inputs as `UNSUPPORTED`. Its
baseline comparison is useful for finding Nova parser/checker gaps, but it is
not a substitute for implementing the upstream TypeScript test harness.

## Real npm/framework matrix

```powershell
npm install --ignore-scripts --no-audit --no-fund `
  --prefix tests/qualification/fixtures/ecosystem-real

python -B tests/qualification/ecosystem_runner.py `
  --nova build/Debug/nova.exe `
  --profile tests/qualification/profiles/ecosystem-real-matrix.json `
  --output build/qualification/results/ecosystem-real `
  --allow-failures
```

`package-lock.json` pins the complete transitive dependency graph. Install
scripts remain disabled for the qualification fixture.
