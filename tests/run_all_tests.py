#!/usr/bin/env python3
"""Run Nova tests that declare explicit expectations.

Test files opt in with directives near the top of the file:

    // NOVA_TEST_MODE: run
    // NOVA_EXPECT_EXIT: 0
    // NOVA_EXPECT_STDOUT_CONTAINS: "optional text"
    // NOVA_EXPECT_STDERR_CONTAINS: "optional text"

String expectations are JSON strings so escapes such as ``\n`` are supported.
Files without ``NOVA_EXPECT_EXIT`` are reported as skipped, never as passed.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SUITE = REPO_ROOT / "tests" / "conformance"
DIRECTIVE_RE = re.compile(
    r"^\s*//\s*NOVA_(TEST_MODE|EXPECT_EXIT|EXPECT_STDOUT_CONTAINS|"
    r"EXPECT_STDERR_CONTAINS):\s*(.*?)\s*$"
)


@dataclass(frozen=True)
class Expectations:
    mode: str
    exit_code: int
    stdout_contains: tuple[str, ...]
    stderr_contains: tuple[str, ...]


@dataclass(frozen=True)
class TestResult:
    path: Path
    status: str
    detail: str = ""
    stdout: str = ""
    stderr: str = ""


def parse_json_string(value: str, path: Path, key: str) -> str:
    try:
        parsed = json.loads(value)
    except json.JSONDecodeError as exc:
        raise ValueError(f"{path}: invalid {key} JSON string: {exc}") from exc
    if not isinstance(parsed, str):
        raise ValueError(f"{path}: {key} must be a JSON string")
    return parsed


def load_expectations(path: Path) -> Expectations | None:
    values: dict[str, list[str]] = {}
    with path.open("r", encoding="utf-8") as source:
        for line_number, line in enumerate(source, start=1):
            match = DIRECTIVE_RE.match(line)
            if match:
                values.setdefault(match.group(1), []).append(match.group(2))
            elif line_number > 40:
                break

    if "EXPECT_EXIT" not in values:
        return None
    if len(values["EXPECT_EXIT"]) != 1:
        raise ValueError(f"{path}: NOVA_EXPECT_EXIT must appear exactly once")

    try:
        exit_code = int(values["EXPECT_EXIT"][0], 10)
    except ValueError as exc:
        raise ValueError(f"{path}: NOVA_EXPECT_EXIT must be an integer") from exc

    mode_values = values.get("TEST_MODE", ["run"])
    if len(mode_values) != 1 or mode_values[0] not in {"run", "compile", "check"}:
        raise ValueError(f"{path}: NOVA_TEST_MODE must be 'run', 'compile', or 'check'")

    stdout_contains = tuple(
        parse_json_string(value, path, "NOVA_EXPECT_STDOUT_CONTAINS")
        for value in values.get("EXPECT_STDOUT_CONTAINS", [])
    )
    stderr_contains = tuple(
        parse_json_string(value, path, "NOVA_EXPECT_STDERR_CONTAINS")
        for value in values.get("EXPECT_STDERR_CONTAINS", [])
    )
    return Expectations(mode_values[0], exit_code, stdout_contains, stderr_contains)


def find_nova(explicit_path: str | None) -> Path:
    if explicit_path:
        candidate = Path(explicit_path).expanduser().resolve()
        if candidate.is_file():
            return candidate
        raise FileNotFoundError(f"Nova executable not found: {candidate}")

    executable = "nova.exe" if os.name == "nt" else "nova"
    candidates = (
        REPO_ROOT / "build" / "Release" / executable,
        REPO_ROOT / "build" / executable,
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise FileNotFoundError("Nova executable not found; pass --nova PATH")


def discover_tests(paths: list[str]) -> list[Path]:
    requested = [Path(path).resolve() for path in paths] if paths else [DEFAULT_SUITE]
    discovered: set[Path] = set()
    for path in requested:
        if path.is_file() and path.suffix in {".ts", ".js"}:
            discovered.add(path)
        elif path.is_dir():
            discovered.update(path.rglob("*.ts"))
            discovered.update(path.rglob("*.js"))
        else:
            raise FileNotFoundError(f"Test path not found: {path}")
    return sorted(discovered)


def run_test(nova: Path, path: Path, timeout: float) -> TestResult:
    try:
        expected = load_expectations(path)
    except ValueError as exc:
        return TestResult(path, "FAIL", str(exc))
    if expected is None:
        return TestResult(path, "SKIP", "no NOVA_EXPECT_EXIT directive")

    command = [str(nova), expected.mode, str(path)]
    if expected.mode == "run":
        # A compiler rebuild must exercise newly generated code. Nova's native
        # binary cache is keyed by source content, not by compiler version, so
        # cached executables could otherwise hide compiler regressions.
        command.append("--no-cache")
    try:
        completed = subprocess.run(
            command,
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout,
            check=False,
        )
    except subprocess.TimeoutExpired as exc:
        return TestResult(
            path,
            "FAIL",
            f"timed out after {timeout:g}s",
            exc.stdout or "",
            exc.stderr or "",
        )
    except OSError as exc:
        return TestResult(path, "FAIL", f"could not start Nova: {exc}")

    failures: list[str] = []
    if completed.returncode != expected.exit_code:
        failures.append(f"exit {completed.returncode}, expected {expected.exit_code}")
    for text in expected.stdout_contains:
        if text not in completed.stdout:
            failures.append(f"stdout does not contain {text!r}")
    for text in expected.stderr_contains:
        if text not in completed.stderr:
            failures.append(f"stderr does not contain {text!r}")

    return TestResult(
        path,
        "FAIL" if failures else "PASS",
        "; ".join(failures),
        completed.stdout,
        completed.stderr,
    )


def print_failure(result: TestResult) -> None:
    print(f"  reason: {result.detail}")
    if result.stdout:
        print("  stdout:")
        print("\n".join(f"    {line}" for line in result.stdout.rstrip().splitlines()))
    if result.stderr:
        print("  stderr:")
        print("\n".join(f"    {line}" for line in result.stderr.rstrip().splitlines()))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="*", help="test files or directories")
    parser.add_argument("--nova", help="path to the Nova executable")
    parser.add_argument("--timeout", type=float, default=20.0, help="seconds per test")
    return parser.parse_args()


def main() -> int:
    # Windows consoles commonly default to cp1252, while Nova diagnostics are
    # UTF-8 and may contain symbols such as check marks. Never let reporting a
    # compiler failure crash the test runner with UnicodeEncodeError.
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")

    args = parse_args()
    try:
        nova = find_nova(args.nova)
        tests = discover_tests(args.paths)
    except (FileNotFoundError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    if not tests:
        print("ERROR: no tests found", file=sys.stderr)
        return 2

    results = [run_test(nova, path, args.timeout) for path in tests]
    for result in results:
        relative = result.path.relative_to(REPO_ROOT)
        suffix = f" - {result.detail}" if result.status == "SKIP" else ""
        print(f"{result.status:4} {relative}{suffix}")
        if result.status == "FAIL":
            print_failure(result)

    passed = sum(result.status == "PASS" for result in results)
    failed = sum(result.status == "FAIL" for result in results)
    skipped = sum(result.status == "SKIP" for result in results)
    print(f"\nVerified: {passed + failed}, passed: {passed}, failed: {failed}, skipped: {skipped}")

    if passed + failed == 0:
        print("ERROR: the suite contains no tests with explicit expectations", file=sys.stderr)
        return 2
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
