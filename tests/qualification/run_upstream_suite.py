from __future__ import annotations

import argparse
import json
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

from qualification_common import CaseResult, write_dashboard
from qualification_common import load_json


SUITES = {
    "test262": (
        "test262_runner.py",
        "test262-upstream-full.json",
        "Test262",
    ),
    "typescript": (
        "typescript_upstream_runner.py",
        "typescript-upstream-full.json",
        "TypeScript",
    ),
}


def _run_shard(
    runner: Path,
    profile: Path,
    nova: Path,
    output: Path,
    shard_index: int,
    shard_count: int,
    resume: bool,
    manifest: Path,
) -> tuple[int, str]:
    dashboard = output / "qualification.json"
    if resume and dashboard.exists():
        return shard_index, "resumed"
    output.mkdir(parents=True, exist_ok=True)
    command = [
        sys.executable,
        "-B",
        str(runner),
        "--nova",
        str(nova),
        "--profile",
        str(profile),
        "--output",
        str(output),
        "--shard-index",
        str(shard_index),
        "--shard-count",
        str(shard_count),
        "--allow-failures",
        "--manifest",
        str(manifest),
    ]
    completed = subprocess.run(
        command,
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if completed.returncode:
        raise RuntimeError(
            f"shard {shard_index} exited {completed.returncode}: "
            f"{completed.stdout[-1000:]}"
        )
    return shard_index, completed.stdout.strip()


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run an official upstream suite in parallel, resumable shards"
    )
    parser.add_argument("suite", choices=sorted(SUITES))
    parser.add_argument("--nova", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--shards", type=int, default=64)
    parser.add_argument("--workers", type=int, default=8)
    parser.add_argument("--no-resume", action="store_true")
    args = parser.parse_args()
    if args.shards < 1 or args.workers < 1:
        parser.error("--shards and --workers must be positive")

    qualification = Path(__file__).resolve().parent
    runner_name, profile_name, checkout_name = SUITES[args.suite]
    runner = qualification / runner_name
    profile = qualification / "profiles" / profile_name
    root = qualification.parents[1]
    checkout = root / "build" / "qualification" / "upstream" / checkout_name
    revision = subprocess.check_output(
        ["git", "-C", str(checkout), "rev-parse", "HEAD"],
        text=True,
    ).strip()

    output = args.output.resolve()
    shards_root = output / f"shards-{args.shards}"
    profile_data = load_json(profile)
    test_root = (profile.parent / profile_data["root"]).resolve()
    if args.suite == "test262":
        discovered = sorted(test_root.glob(profile_data.get("pattern", "**/*.js")))
    else:
        suffixes = {".ts", ".tsx", ".js", ".jsx", ".mts", ".cts"}
        discovered = sorted(
            path for path in test_root.rglob("*")
            if path.is_file() and path.suffix.lower() in suffixes
        )
    manifests: list[Path] = []
    for index in range(args.shards):
        shard_output = shards_root / f"shard-{index:04d}"
        shard_output.mkdir(parents=True, exist_ok=True)
        manifest = shard_output / "manifest.txt"
        selected = discovered[index::args.shards]
        manifest.write_text(
            "".join(
                path.relative_to(test_root).as_posix() + "\n"
                for path in selected
            ),
            encoding="utf-8",
        )
        manifests.append(manifest)
    with ThreadPoolExecutor(max_workers=args.workers) as executor:
        futures = [
            executor.submit(
                _run_shard,
                runner,
                profile,
                args.nova.resolve(),
                shards_root / f"shard-{index:04d}",
                index,
                args.shards,
                not args.no_resume,
                manifests[index],
            )
            for index in range(args.shards)
        ]
        for future in as_completed(futures):
            index, message = future.result()
            print(f"[{index + 1}/{args.shards}] {message}", flush=True)

    results: list[CaseResult] = []
    for index in range(args.shards):
        dashboard = shards_root / f"shard-{index:04d}" / "qualification.json"
        payload = json.loads(dashboard.read_text(encoding="utf-8"))
        results.extend(CaseResult(**item) for item in payload["results"])
    results.sort(key=lambda result: result.name)
    write_dashboard(output, f"{args.suite}-upstream-{revision}", results, {})
    counts: dict[str, int] = {}
    for result in results:
        counts[result.status] = counts.get(result.status, 0) + 1
    manifest = {
        "suite": args.suite,
        "revision": revision,
        "shards": args.shards,
        "counts": counts,
        "total": len(results),
    }
    (output / "upstream-run.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(json.dumps(manifest, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
