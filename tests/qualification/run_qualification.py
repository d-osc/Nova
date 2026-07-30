from __future__ import annotations

import argparse
import json
import sys
from dataclasses import asdict
from datetime import date
from pathlib import Path

import ecosystem_runner
import test262_runner
import typescript_runner


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--nova", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--changed-feature", action="append", default=[])
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[2]
    profiles = Path(__file__).resolve().parent / "profiles"
    output = args.output.resolve()
    nova = args.nova.resolve()
    output.mkdir(parents=True, exist_ok=True)
    results = []
    results.extend(
        test262_runner.run_profile(
            nova,
            profiles / "test262-es2024-smoke.json",
            output / "test262",
            set(args.changed_feature) or None,
        )
    )
    results.extend(
        typescript_runner.run_profile(
            nova,
            profiles / "typescript-local-profile.json",
            output / "typescript",
        )
    )
    results.extend(
        ecosystem_runner.run_profile(
            nova,
            profiles / "ecosystem-local-profile.json",
            output / "ecosystem",
        )
    )
    counts: dict[str, int] = {}
    suites: dict[str, dict[str, int]] = {}
    for result in results:
        counts[result.status] = counts.get(result.status, 0) + 1
        suite = suites.setdefault(result.suite, {})
        suite[result.status] = suite.get(result.status, 0) + 1
    payload = {
        "profile": "phase7-local-declared-profile",
        "generated": date.today().isoformat(),
        "nova": str(nova.relative_to(root) if nova.is_relative_to(root) else nova),
        "counts": counts,
        "suites": suites,
        "results": [asdict(result) for result in results],
    }
    (output / "qualification-summary.json").write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    lines = [
        "# Phase 7 qualification summary",
        "",
        f"Generated: {payload['generated']}",
        "",
        "| Suite | PASS | FAIL | EXCLUDED |",
        "|---|---:|---:|---:|",
    ]
    for suite, values in sorted(suites.items()):
        lines.append(
            f"| {suite} | {values.get('PASS', 0)} | "
            f"{values.get('FAIL', 0)} | {values.get('EXCLUDED', 0)} |"
        )
    (output / "qualification-summary.md").write_text(
        "\n".join(lines) + "\n", encoding="utf-8"
    )
    print(
        f"Phase 7 qualification: {counts.get('PASS', 0)}/{len(results)} green"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
