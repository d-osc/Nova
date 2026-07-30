from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

from qualification_common import (
    CaseResult,
    assert_green,
    load_json,
    run_command,
    validate_exclusions,
    write_dashboard,
)


def run_profile(
    nova: Path,
    profile_path: Path,
    output_dir: Path,
    *,
    require_green: bool = True,
) -> list[CaseResult]:
    profile = load_json(profile_path)
    root = profile_path.parents[3]
    exclusions = validate_exclusions(
        (profile_path.parent / profile["exclusions"]).resolve()
    )
    timeout = float(profile.get("timeout_seconds", 60))
    cases = profile["cases"]
    names = {case["name"] for case in cases}
    unknown = set(exclusions) - names
    if unknown:
        raise RuntimeError(
            "ecosystem exclusions do not match profile cases: "
            + ", ".join(sorted(unknown))
        )

    results: list[CaseResult] = []
    replacements = {
        "{nova}": str(nova),
        "{python}": sys.executable,
        "{root}": str(root),
    }
    for case in cases:
        name = case["name"]
        tier = case["tier"]
        if name in exclusions:
            results.append(
                CaseResult(
                    "ecosystem",
                    name,
                    tier,
                    "EXCLUDED",
                    0,
                    exclusions[name]["reason"],
                )
            )
            continue
        command: list[str] = []
        for raw in case["command"]:
            item = raw
            for key, value in replacements.items():
                item = item.replace(key, value)
            command.append(item)
        try:
            completed = run_command(
                command,
                cwd=root,
                env={**os.environ, "NOVA_TEST_EXECUTABLE": str(nova)},
                timeout=float(case.get("timeout_seconds", timeout)),
            )
            expected_exit = int(case.get("exit", 0))
            passed = completed.returncode == expected_exit
            combined = completed.stdout + "\n" + completed.stderr
            for expected in case.get("contains", []):
                if expected not in combined:
                    passed = False
            detail = ""
            if not passed:
                detail = (
                    completed.stderr.strip()
                    or completed.stdout.strip()
                    or f"exit {completed.returncode}, expected {expected_exit}"
                )
            results.append(
                CaseResult(
                    "ecosystem",
                    name,
                    tier,
                    "PASS" if passed else "FAIL",
                    completed.duration_ms,
                    detail[:500],
                    peak_memory_kb=completed.peak_memory_kb,
                )
            )
        except subprocess.TimeoutExpired:
            case_timeout = float(case.get("timeout_seconds", timeout))
            results.append(
                CaseResult(
                    "ecosystem",
                    name,
                    tier,
                    "FAIL",
                    case_timeout * 1000,
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
    parser.add_argument("--allow-failures", action="store_true")
    args = parser.parse_args()
    results = run_profile(
        args.nova.resolve(),
        args.profile.resolve(),
        args.output.resolve(),
        require_green=not args.allow_failures,
    )
    print(
        f"Ecosystem profile: {sum(r.status == 'PASS' for r in results)}/"
        f"{len(results)} green"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
