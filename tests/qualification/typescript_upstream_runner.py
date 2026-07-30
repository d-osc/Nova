from __future__ import annotations

import argparse
import subprocess
from pathlib import Path

from qualification_common import (
    CaseResult,
    assert_green,
    load_json,
    run_command,
    write_dashboard,
)


SOURCE_SUFFIXES = {".ts", ".tsx", ".js", ".jsx", ".mts", ".cts"}


def _has_error_baseline(stem: str, names: set[str]) -> bool:
    return any(
        name == f"{stem}.errors.txt"
        or name.startswith(f"{stem}(")
        for name in names
    )


def discover(profile_path: Path) -> tuple[dict, Path, list[Path]]:
    profile = load_json(profile_path)
    root = (profile_path.parent / profile["root"]).resolve()
    tests = sorted(
        path
        for path in root.rglob("*")
        if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
    )
    return profile, root, tests


def run_profile(
    nova: Path,
    profile_path: Path,
    output_dir: Path,
    *,
    shard_index: int = 0,
    shard_count: int = 1,
    limit: int = 0,
    require_green: bool = True,
    manifest: Path | None = None,
) -> list[CaseResult]:
    if shard_count < 1:
        raise ValueError("shard_count must be at least 1")
    if shard_index < 0 or shard_index >= shard_count:
        raise ValueError("shard_index must be in [0, shard_count)")
    profile = load_json(profile_path)
    root = (profile_path.parent / profile["root"]).resolve()
    if manifest is not None:
        tests = [
            root / line
            for line in manifest.read_text(encoding="utf-8").splitlines()
            if line
        ]
    else:
        profile, root, all_tests = discover(profile_path)
        tests = [
            test for index, test in enumerate(all_tests)
            if index % shard_count == shard_index
        ]
    if limit:
        tests = tests[:limit]

    baseline_root = (profile_path.parent / profile["baselines"]).resolve()
    error_baselines = {
        path.name for path in baseline_root.glob("*.errors.txt")
    }
    executable = set(profile.get("executable_categories", []))
    timeout = float(profile.get("timeout_seconds", 30))
    results: list[CaseResult] = []

    for test in tests:
        relative = test.relative_to(root).as_posix()
        category = relative.split("/", 1)[0]
        if category not in executable:
            results.append(
                CaseResult(
                    "typescript-upstream",
                    relative,
                    category,
                    "UNSUPPORTED",
                    0,
                    "requires the TypeScript language-service/project test harness",
                )
            )
            continue

        expected_error = _has_error_baseline(test.stem, error_baselines)
        try:
            completed = run_command(
                [str(nova), "check", str(test)],
                cwd=root,
                timeout=timeout,
            )
            actual_error = completed.returncode != 0
            passed = actual_error == expected_error
            detail = ""
            if not passed:
                expectation = "diagnostics" if expected_error else "no diagnostics"
                output = completed.stderr.strip() or completed.stdout.strip()
                detail = f"expected {expectation}; {output or 'exit ' + str(completed.returncode)}"
            results.append(
                CaseResult(
                    "typescript-upstream",
                    relative,
                    category,
                    "PASS" if passed else "FAIL",
                    completed.duration_ms,
                    detail[:500],
                    peak_memory_kb=completed.peak_memory_kb,
                )
            )
        except subprocess.TimeoutExpired:
            results.append(
                CaseResult(
                    "typescript-upstream",
                    relative,
                    category,
                    "FAIL",
                    timeout * 1000,
                    "timeout",
                )
            )

    write_dashboard(
        output_dir, profile["name"], results, profile.get("budgets", {})
    )
    if require_green:
        assert_green(results, profile.get("budgets", {}))
    return results


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--nova", type=Path, required=True)
    parser.add_argument("--profile", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--shard-index", type=int, default=0)
    parser.add_argument("--shard-count", type=int, default=1)
    parser.add_argument("--limit", type=int, default=0)
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--allow-failures", action="store_true")
    args = parser.parse_args()
    results = run_profile(
        args.nova.resolve(),
        args.profile.resolve(),
        args.output.resolve(),
        shard_index=args.shard_index,
        shard_count=args.shard_count,
        limit=args.limit,
        require_green=not args.allow_failures,
        manifest=args.manifest.resolve() if args.manifest else None,
    )
    print(
        f"TypeScript upstream profile: "
        f"{sum(r.status == 'PASS' for r in results)}/{len(results)} green"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
