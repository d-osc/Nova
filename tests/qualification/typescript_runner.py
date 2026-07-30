from __future__ import annotations

import argparse
import os
import re
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


DIAGNOSTIC = re.compile(
    r":(?P<line>\d+):(?P<column>\d+): error "
    r"(?P<code>TS\d+): (?P<message>.*)"
)


def run_profile(
    nova: Path, profile_path: Path, output_dir: Path
) -> list[CaseResult]:
    profile = load_json(profile_path)
    root = profile_path.parents[3]
    exclusions = validate_exclusions(
        (profile_path.parent / profile["exclusions"]).resolve()
    )
    timeout = float(profile.get("timeout_seconds", 30))
    results: list[CaseResult] = []
    names = {case["name"] for case in profile["cases"]}
    unknown = set(exclusions) - names
    if unknown:
        raise RuntimeError(
            "TypeScript exclusions do not match profile cases: "
            + ", ".join(sorted(unknown))
        )

    for case in profile["cases"]:
        name = case["name"]
        category = case["category"]
        if name in exclusions:
            results.append(
                CaseResult(
                    "typescript",
                    name,
                    category,
                    "EXCLUDED",
                    0,
                    exclusions[name]["reason"],
                )
            )
            continue
        replacements = {
            "{nova}": str(nova),
            "{python}": sys.executable,
            "{root}": str(root),
        }
        command = []
        for item in case["command"]:
            for key, value in replacements.items():
                item = item.replace(key, value)
            command.append(item)
        try:
            completed = run_command(
                command,
                cwd=root,
                env={**os.environ, "NOVA_TEST_EXECUTABLE": str(nova)},
                timeout=timeout,
            )
            expected_exit = int(case.get("exit", 0))
            passed = completed.returncode == expected_exit
            detail = ""
            diagnostics = [
                match.groupdict()
                for match in DIAGNOSTIC.finditer(completed.stderr)
            ]
            for expected in case.get("diagnostics", []):
                found = any(
                    item["code"] == expected["code"]
                    and int(item["line"]) == int(expected["line"])
                    and expected.get("contains", "") in item["message"]
                    for item in diagnostics
                )
                if not found:
                    passed = False
                    detail = (
                        f"missing diagnostic {expected['code']} "
                        f"at line {expected['line']}"
                    )
                    break
            if not passed and not detail:
                detail = (
                    completed.stderr.strip()
                    or completed.stdout.strip()
                    or f"exit {completed.returncode}, expected {expected_exit}"
                )
            results.append(
                CaseResult(
                    "typescript",
                    name,
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
                    "typescript",
                    name,
                    category,
                    "FAIL",
                    timeout * 1000,
                    "timeout",
                )
            )

    write_dashboard(
        output_dir, profile["name"], results, profile.get("budgets", {})
    )
    assert_green(results, profile.get("budgets", {}))
    return results


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--nova", type=Path, required=True)
    parser.add_argument("--profile", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    results = run_profile(
        args.nova.resolve(), args.profile.resolve(), args.output.resolve()
    )
    print(
        f"TypeScript profile: {sum(r.status == 'PASS' for r in results)}/"
        f"{len(results)} green"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
