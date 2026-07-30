from __future__ import annotations

import argparse
import subprocess
from pathlib import Path

import test262_runner


PATH_FEATURES = {
    "src/runtime/array.cpp": {"Array.prototype.at"},
    "src/runtime/promise.cpp": {"Promise"},
    "src/frontend/parser/": {"optional-chaining", "destructuring-binding"},
    "src/hir/": {
        "Array.prototype.at",
        "Promise",
        "optional-chaining",
        "destructuring-binding",
    },
    "src/codegen/": {
        "Array.prototype.at",
        "Promise",
        "optional-chaining",
        "destructuring-binding",
    },
}
ALL_FEATURES = set().union(*PATH_FEATURES.values())


def features_for_paths(paths: list[str]) -> set[str]:
    features: set[str] = set()
    qualification_changed = False
    for raw in paths:
        path = raw.replace("\\", "/").lower()
        qualification_changed |= path.startswith("tests/qualification/")
        for prefix, mapped in PATH_FEATURES.items():
            if path == prefix or path.startswith(prefix):
                features.update(mapped)
    return ALL_FEATURES if qualification_changed or not features else features


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--nova", type=Path, required=True)
    parser.add_argument("--base", required=True)
    parser.add_argument("--head", default="HEAD")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    changed = subprocess.run(
        ["git", "diff", "--name-only", args.base, args.head],
        cwd=root,
        text=True,
        encoding="utf-8",
        errors="replace",
        capture_output=True,
        check=True,
    ).stdout.splitlines()
    features = features_for_paths(changed)
    print("Changed-feature Test262 shard: " + ", ".join(sorted(features)))
    test262_runner.run_profile(
        args.nova.resolve(),
        Path(__file__).resolve().parent
        / "profiles"
        / "test262-es2024-smoke.json",
        args.output.resolve(),
        features,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
