from __future__ import annotations

import json
import subprocess
import threading
import time
from dataclasses import asdict, dataclass
from datetime import date
from pathlib import Path
from typing import Any


@dataclass
class CaseResult:
    suite: str
    name: str
    category: str
    status: str
    duration_ms: float
    detail: str = ""
    edition: str = ""
    peak_memory_kb: int = 0


@dataclass
class CommandResult:
    returncode: int
    stdout: str
    stderr: str
    duration_ms: float
    peak_memory_kb: int


def _process_rss_kb(process: subprocess.Popen[str]) -> int:
    """Read a child process' current RSS without a third-party dependency."""
    status = Path(f"/proc/{process.pid}/status")
    if status.exists():
        for line in status.read_text(encoding="utf-8", errors="replace").splitlines():
            if line.startswith("VmRSS:"):
                return int(line.split()[1])
        return 0
    try:
        import ctypes
        from ctypes import wintypes

        class ProcessMemoryCounters(ctypes.Structure):
            _fields_ = [
                ("cb", wintypes.DWORD),
                ("PageFaultCount", wintypes.DWORD),
                ("PeakWorkingSetSize", ctypes.c_size_t),
                ("WorkingSetSize", ctypes.c_size_t),
                ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
                ("QuotaPagedPoolUsage", ctypes.c_size_t),
                ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
                ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
                ("PagefileUsage", ctypes.c_size_t),
                ("PeakPagefileUsage", ctypes.c_size_t),
            ]

        counters = ProcessMemoryCounters()
        counters.cb = ctypes.sizeof(counters)
        handle = wintypes.HANDLE(int(process._handle))  # type: ignore[attr-defined]
        if ctypes.windll.psapi.GetProcessMemoryInfo(
            handle, ctypes.byref(counters), counters.cb
        ):
            return int(counters.WorkingSetSize // 1024)
    except (AttributeError, OSError, TypeError, ValueError):
        pass
    return 0


def run_command(
    command: list[str],
    *,
    cwd: Path,
    timeout: float,
    env: dict[str, str] | None = None,
) -> CommandResult:
    start = time.perf_counter()
    process = subprocess.Popen(
        command,
        cwd=cwd,
        env=env,
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    stopped = threading.Event()
    peak_memory_kb = 0

    def sample_memory() -> None:
        nonlocal peak_memory_kb
        while not stopped.is_set():
            peak_memory_kb = max(peak_memory_kb, _process_rss_kb(process))
            stopped.wait(0.01)

    sampler = threading.Thread(target=sample_memory, daemon=True)
    sampler.start()
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        process.kill()
        process.communicate()
        raise
    finally:
        peak_memory_kb = max(peak_memory_kb, _process_rss_kb(process))
        stopped.set()
        sampler.join(timeout=1)
    return CommandResult(
        process.returncode,
        stdout,
        stderr,
        (time.perf_counter() - start) * 1000,
        peak_memory_kb,
    )


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def validate_exclusions(path: Path) -> dict[str, dict[str, Any]]:
    if not path.exists():
        return {}
    entries = load_json(path).get("exclusions", [])
    result: dict[str, dict[str, Any]] = {}
    required = {"path", "reason", "owner", "reviewed_by", "expires"}
    today = date.today()
    for entry in entries:
        missing = required - set(entry)
        if missing:
            raise ValueError(
                f"exclusion is missing {sorted(missing)}: {entry!r}"
            )
        expiry = date.fromisoformat(entry["expires"])
        if expiry < today:
            raise ValueError(
                f"exclusion expired on {expiry}: {entry['path']}"
            )
        if entry["path"] in result:
            raise ValueError(f"duplicate exclusion: {entry['path']}")
        result[entry["path"]] = entry
    return result


def write_dashboard(
    output_dir: Path,
    profile_name: str,
    results: list[CaseResult],
    budgets: dict[str, Any],
) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    counts: dict[str, int] = {}
    categories: dict[str, dict[str, int]] = {}
    editions: dict[str, dict[str, int]] = {}
    for result in results:
        counts[result.status] = counts.get(result.status, 0) + 1
        bucket = categories.setdefault(result.category, {})
        bucket[result.status] = bucket.get(result.status, 0) + 1
        if result.edition:
            edition = editions.setdefault(result.edition, {})
            edition[result.status] = edition.get(result.status, 0) + 1
    payload = {
        "profile": profile_name,
        "generated": date.today().isoformat(),
        "counts": counts,
        "categories": categories,
        "editions": editions,
        "budgets": budgets,
        "results": [asdict(result) for result in results],
    }
    (output_dir / "qualification.json").write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )

    lines = [
        f"# Qualification dashboard — {profile_name}",
        "",
        f"Generated: {payload['generated']}",
        "",
        "| Category | PASS | FAIL | UNSUPPORTED | EXCLUDED |",
        "|---|---:|---:|---:|---:|",
    ]
    for category, values in sorted(categories.items()):
        lines.append(
            f"| {category} | {values.get('PASS', 0)} | "
            f"{values.get('FAIL', 0)} | {values.get('UNSUPPORTED', 0)} | "
            f"{values.get('EXCLUDED', 0)} |"
        )
    if editions:
        lines.extend(
            [
                "",
                "## Editions",
                "",
                "| Edition | PASS | FAIL | UNSUPPORTED | EXCLUDED |",
                "|---|---:|---:|---:|---:|",
            ]
        )
        for edition, values in sorted(editions.items()):
            lines.append(
                    f"| {edition} | {values.get('PASS', 0)} | "
                    f"{values.get('FAIL', 0)} | "
                    f"{values.get('UNSUPPORTED', 0)} | "
                    f"{values.get('EXCLUDED', 0)} |"
            )
    lines.extend(["", "## Cases", ""])
    for result in results:
        detail = f" — {result.detail}" if result.detail else ""
        lines.append(
            f"- **{result.status}** `{result.name}` "
            f"({result.duration_ms:.1f} ms, "
            f"{result.peak_memory_kb} KiB peak RSS){detail}"
        )
    (output_dir / "qualification.md").write_text(
        "\n".join(lines) + "\n", encoding="utf-8"
    )


def assert_green(results: list[CaseResult], budgets: dict[str, Any]) -> None:
    failures = [result for result in results if result.status == "FAIL"]
    if failures:
        names = ", ".join(result.name for result in failures)
        raise RuntimeError(f"qualification failures: {names}")
    maximum = float(budgets.get("max_case_ms", 0))
    if maximum:
        slow = [
            result
            for result in results
            if result.status == "PASS" and result.duration_ms > maximum
        ]
        if slow:
            names = ", ".join(
                f"{result.name}={result.duration_ms:.1f}ms"
                for result in slow
            )
            raise RuntimeError(f"performance budget exceeded: {names}")
    maximum_memory = int(budgets.get("max_peak_memory_kb", 0))
    if maximum_memory:
        unmeasured = [
            result.name
            for result in results
            if result.status == "PASS" and result.peak_memory_kb <= 0
        ]
        if unmeasured:
            raise RuntimeError(
                "memory budget could not be measured: " + ", ".join(unmeasured)
            )
        large = [
            result
            for result in results
            if result.status == "PASS"
            and result.peak_memory_kb > maximum_memory
        ]
        if large:
            names = ", ".join(
                f"{result.name}={result.peak_memory_kb}KiB" for result in large
            )
            raise RuntimeError(f"memory budget exceeded: {names}")
