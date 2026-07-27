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
TEST_EXTENSIONS = {".js", ".jsx", ".mjs", ".cjs", ".ts", ".tsx", ".mts", ".cts"}
DEFAULT_FAILURE_DEBUG = REPO_ROOT / "run_all_tests_fail_debug.txt"
COMPILER_DIAGNOSTIC_RE = re.compile(
    r"^(?P<file>.+?):(?P<line>\d+):(?P<column>\d+):\s*"
    r"(?P<severity>error|warning)(?:\s+(?P<code>TS\d+))?:",
    re.MULTILINE,
)
FUNCTION_DECL_RE = re.compile(
    r"^\s*(?:export\s+)?(?:default\s+)?(?:async\s+)?function\s*\*?\s*"
    r"(?P<name>[A-Za-z_$][\w$]*)\s*(?:<[^>{}]*>)?\s*\("
)
METHOD_DECL_RE = re.compile(
    r"^\s*(?:(?:public|private|protected|static|abstract|readonly|override|"
    r"async|get|set)\s+)*(?P<name>constructor|[A-Za-z_$][\w$]*)\s*"
    r"(?:<[^>{}]*>)?\s*\([^;{}]*\)\s*(?::[^{}]+)?\s*\{"
)
ARROW_DECL_RE = re.compile(
    r"^\s*(?:export\s+)?(?:const|let|var)\s+(?P<name>[A-Za-z_$][\w$]*)"
    r"[^;]*=>\s*\{"
)
CLASS_DECL_RE = re.compile(
    r"^\s*(?:export\s+)?(?:default\s+)?(?:abstract\s+)?class\s+"
    r"(?P<name>[A-Za-z_$][\w$]*)"
)
RETURN_CODE_RE = re.compile(
    r"\breturn\s+\(?\s*(?P<code>-?(?:0x[0-9A-Fa-f]+|\d+))\s*\)?\s*;"
)
NATIVE_FUNCTION_RE = re.compile(
    r"(?:@|(?:constructor|method|default constructor) function:\s*)"
    r"(?P<name>[A-Za-z_$][\w$]*)"
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
    return_code: int | None = None


@dataclass(frozen=True)
class FailureDebug:
    file: str
    code: str
    line: int
    function: str


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
        if path.is_file() and path.suffix.lower() in TEST_EXTENSIONS:
            discovered.add(path)
        elif path.is_dir():
            discovered.update(
                candidate
                for candidate in path.rglob("*")
                if candidate.is_file() and candidate.suffix.lower() in TEST_EXTENSIONS
            )
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
        completed.returncode,
    )


def source_lines(path: Path) -> list[str]:
    try:
        return path.read_text(encoding="utf-8").splitlines()
    except OSError:
        return []


def brace_delta(line: str) -> int:
    """Count structural braces while ignoring common strings and comments."""
    code = re.sub(r"//.*$", "", line)
    code = re.sub(r'"(?:\\.|[^"\\])*"', '""', code)
    code = re.sub(r"'(?:\\.|[^'\\])*'", "''", code)
    code = re.sub(r"`(?:\\.|[^`\\])*`", "``", code)
    return code.count("{") - code.count("}")


def declared_function(line: str) -> str | None:
    for pattern in (FUNCTION_DECL_RE, METHOD_DECL_RE, ARROW_DECL_RE):
        match = pattern.match(line)
        if match:
            return match.group("name")
    return None


def function_at_line(path: Path, target_line: int) -> str:
    """Best-effort source function owning a one-based line number."""
    if target_line <= 0:
        return "<unknown>"

    depth = 0
    stack: list[tuple[str, int]] = []
    for line_number, line in enumerate(source_lines(path), start=1):
        while stack and depth < stack[-1][1]:
            stack.pop()

        function_name = declared_function(line)
        if function_name and "{" in line:
            stack.append((function_name, depth + 1))

        if line_number == target_line:
            return stack[-1][0] if stack else "<top-level>"

        depth += brace_delta(line)

    return "<unknown>"


def parse_integer(text: str) -> int | None:
    try:
        return int(text, 0)
    except ValueError:
        return None


def format_exit_code(return_code: int) -> str:
    unsigned = return_code & 0xFFFFFFFF
    if unsigned >= 0x80000000:
        return f"0x{unsigned:08X}"
    return f"EXIT_{return_code}"


def relative_debug_path(path: Path) -> str:
    try:
        return path.resolve().relative_to(REPO_ROOT).as_posix()
    except (OSError, ValueError):
        return path.as_posix()


def path_from_diagnostic(text: str, fallback: Path) -> Path:
    candidate = Path(text)
    if not candidate.is_absolute():
        candidate = REPO_ROOT / candidate
    return candidate if candidate.exists() else fallback


def return_code_location(path: Path, return_code: int) -> tuple[int, str] | None:
    for line_number, line in enumerate(source_lines(path), start=1):
        match = RETURN_CODE_RE.search(line)
        if not match:
            continue
        parsed = parse_integer(match.group("code"))
        if parsed == return_code:
            return line_number, function_at_line(path, line_number)
    return None


def native_crash_locations(result: TestResult) -> list[FailureDebug]:
    rows: list[FailureDebug] = []
    names: list[str] = []
    for match in NATIVE_FUNCTION_RE.finditer(result.stderr):
        name = match.group("name")
        if name not in names:
            names.append(name)

    for native_name in names:
        location = native_function_location(result.path, native_name)
        if location:
            line_number, function_name = location
            rows.append(
                FailureDebug(
                    relative_debug_path(result.path),
                    format_exit_code(result.return_code or 0),
                    line_number,
                    function_name,
                )
            )

    if not rows:
        rows.append(
            FailureDebug(
                relative_debug_path(result.path),
                format_exit_code(result.return_code or 0),
                0,
                "<native-runtime>",
            )
        )
    return rows


def native_function_location(path: Path, native_name: str) -> tuple[int, str] | None:
    lines = source_lines(path)

    for line_number, line in enumerate(lines, start=1):
        if declared_function(line) == native_name:
            return line_number, native_name

    classes: list[tuple[str, int]] = []
    for line_number, line in enumerate(lines, start=1):
        match = CLASS_DECL_RE.match(line)
        if match:
            classes.append((match.group("name"), line_number))

    for class_index, (class_name, class_line) in enumerate(classes):
        prefix = f"{class_name}_"
        if not native_name.startswith(prefix):
            continue
        member_name = native_name[len(prefix):]
        end_line = (
            classes[class_index + 1][1]
            if class_index + 1 < len(classes)
            else len(lines) + 1
        )
        for line_number in range(class_line + 1, end_line):
            if declared_function(lines[line_number - 1]) == member_name:
                return line_number, f"{class_name}.{member_name}"

    return None


def expectation_directive_line(path: Path, directive: str, value: str) -> int:
    pattern = re.compile(rf"^\s*//\s*NOVA_{re.escape(directive)}:\s*(.*?)\s*$")
    for line_number, line in enumerate(source_lines(path), start=1):
        match = pattern.match(line)
        if not match:
            continue
        directive_value = match.group(1)
        if directive in {"EXPECT_STDOUT_CONTAINS", "EXPECT_STDERR_CONTAINS"}:
            try:
                directive_value = json.loads(directive_value)
            except json.JSONDecodeError:
                pass
        if directive == "EXPECT_EXIT" or value == directive_value:
            return line_number
    return 0


def failure_debug_rows(result: TestResult) -> list[FailureDebug]:
    if result.status != "FAIL":
        return []

    rows: list[FailureDebug] = []
    combined_output = "\n".join((result.stderr, result.stdout))
    for match in COMPILER_DIAGNOSTIC_RE.finditer(combined_output):
        diagnostic_path = path_from_diagnostic(match.group("file"), result.path)
        line_number = int(match.group("line"))
        code = match.group("code") or f"{match.group('severity').upper()}_DIAGNOSTIC"
        rows.append(
            FailureDebug(
                relative_debug_path(diagnostic_path),
                code,
                line_number,
                function_at_line(diagnostic_path, line_number),
            )
        )

    expected: Expectations | None
    try:
        expected = load_expectations(result.path)
    except ValueError:
        expected = None

    if result.return_code is not None and (
        expected is None or result.return_code != expected.exit_code
    ):
        if not rows:
            location = return_code_location(result.path, result.return_code)
            if location:
                line_number, function_name = location
                rows.append(
                    FailureDebug(
                        relative_debug_path(result.path),
                        format_exit_code(result.return_code),
                        line_number,
                        function_name,
                    )
                )
            elif (result.return_code & 0xFFFFFFFF) >= 0x80000000:
                rows.extend(native_crash_locations(result))
            else:
                rows.append(
                    FailureDebug(
                        relative_debug_path(result.path),
                        format_exit_code(result.return_code),
                        expectation_directive_line(
                            result.path, "EXPECT_EXIT", str(expected.exit_code)
                        )
                        if expected
                        else 0,
                        "<test-process>",
                    )
                )

    if expected:
        for text in expected.stdout_contains:
            if text not in result.stdout:
                rows.append(
                    FailureDebug(
                        relative_debug_path(result.path),
                        "MISSING_STDOUT",
                        expectation_directive_line(
                            result.path, "EXPECT_STDOUT_CONTAINS", text
                        ),
                        "<expectation>",
                    )
                )
        for text in expected.stderr_contains:
            if text not in result.stderr:
                rows.append(
                    FailureDebug(
                        relative_debug_path(result.path),
                        "MISSING_STDERR",
                        expectation_directive_line(
                            result.path, "EXPECT_STDERR_CONTAINS", text
                        ),
                        "<expectation>",
                    )
                )

    if "timed out after" in result.detail:
        rows.append(
            FailureDebug(
                relative_debug_path(result.path),
                "TIMEOUT",
                0,
                "<test-process>",
            )
        )
    elif "could not start Nova" in result.detail:
        rows.append(
            FailureDebug(
                relative_debug_path(result.path),
                "START_ERROR",
                0,
                "<test-process>",
            )
        )
    elif not rows:
        rows.append(
            FailureDebug(
                relative_debug_path(result.path),
                "FAIL",
                0,
                "<unknown>",
            )
        )

    deduplicated: list[FailureDebug] = []
    seen: set[FailureDebug] = set()
    for row in rows:
        if row not in seen:
            deduplicated.append(row)
            seen.add(row)
    return deduplicated


def clean_debug_field(value: str) -> str:
    return value.replace("|", "/").replace("\r", " ").replace("\n", " ")


def write_failure_debug(results: list[TestResult], output_path: Path) -> int:
    rows = [
        row
        for result in results
        for row in failure_debug_rows(result)
    ]
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8", newline="\n") as output:
        output.write("file|code|line|function\n")
        for row in rows:
            output.write(
                f"{clean_debug_field(row.file)}|{clean_debug_field(row.code)}|"
                f"{row.line}|{clean_debug_field(row.function)}\n"
            )
    return len(rows)


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
    parser.add_argument(
        "--prefix",
        action="append",
        default=[],
        help="run only files whose names start with PREFIX; may be repeated",
    )
    parser.add_argument(
        "--failure-debug",
        default=str(DEFAULT_FAILURE_DEBUG),
        metavar="PATH",
        help="write file|code|line|function failure locations to PATH",
    )
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
        if args.prefix:
            prefixes = tuple(args.prefix)
            tests = [test for test in tests if test.name.startswith(prefixes)]
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

    failure_debug_path = Path(args.failure_debug).expanduser().resolve()
    debug_location_count = write_failure_debug(results, failure_debug_path)

    passed = sum(result.status == "PASS" for result in results)
    failed = sum(result.status == "FAIL" for result in results)
    skipped = sum(result.status == "SKIP" for result in results)
    print(f"\nVerified: {passed + failed}, passed: {passed}, failed: {failed}, skipped: {skipped}")
    print(
        f"Failure debug: {failure_debug_path} "
        f"({debug_location_count} location{'s' if debug_location_count != 1 else ''})"
    )

    if passed + failed == 0:
        print("ERROR: the suite contains no tests with explicit expectations", file=sys.stderr)
        return 2
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
