from __future__ import annotations

import argparse
import re
import subprocess
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

from qualification_common import (
    CaseResult,
    assert_green,
    load_json,
    run_command,
    validate_exclusions,
    write_dashboard,
)


@dataclass
class Test262Metadata:
    description: str = ""
    esid: str = ""
    features: list[str] = field(default_factory=list)
    flags: list[str] = field(default_factory=list)
    includes: list[str] = field(default_factory=list)
    edition: str = "unknown"
    negative_phase: str = ""
    negative_type: str = ""


def _array(value: str) -> list[str]:
    value = value.strip()
    if not value:
        return []
    if value.startswith("["):
        value = value[1:-1]
        return [
            item.strip().strip("'\"")
            for item in value.split(",")
            if item.strip()
        ]
    return [item.strip() for item in value.split(",") if item.strip()]


def parse_frontmatter(source: str) -> Test262Metadata:
    match = re.search(r"/\*---\s*(.*?)\s*---\*/", source, re.DOTALL)
    if not match:
        return Test262Metadata()
    metadata = Test262Metadata()
    negative = False
    pending_array = ""
    for raw in match.group(1).splitlines():
        if not raw.strip() or raw.lstrip().startswith("#"):
            continue
        indent = len(raw) - len(raw.lstrip())
        stripped = raw.strip()
        if pending_array and stripped.startswith("- "):
            getattr(metadata, pending_array).append(
                stripped[2:].strip().strip("'\"")
            )
            continue
        key, separator, value = raw.strip().partition(":")
        if not separator:
            continue
        if indent and negative:
            if key == "phase":
                metadata.negative_phase = value.strip()
            elif key == "type":
                metadata.negative_type = value.strip()
            continue
        negative = key == "negative"
        pending_array = ""
        if key == "description":
            metadata.description = value.strip().strip("'\"")
        elif key == "features":
            metadata.features = _array(value)
            pending_array = "features" if not value.strip() else ""
        elif key == "flags":
            metadata.flags = _array(value)
            pending_array = "flags" if not value.strip() else ""
        elif key == "includes":
            metadata.includes = _array(value)
            pending_array = "includes" if not value.strip() else ""
        elif key == "esid":
            metadata.esid = value.strip()
        elif key == "edition":
            metadata.edition = value.strip()
    return metadata


def discover(profile_path: Path) -> tuple[dict, list[Path]]:
    profile = load_json(profile_path)
    root = (profile_path.parent / profile["root"]).resolve()
    tests = sorted(root.glob(profile.get("pattern", "**/*.js")))
    return profile, tests


def run_profile(
    nova: Path,
    profile_path: Path,
    output_dir: Path,
    changed_features: set[str] | None = None,
    *,
    shard_index: int = 0,
    shard_count: int = 1,
    limit: int = 0,
    require_green: bool = True,
    manifest: Path | None = None,
    keep_temporaries: bool = False,
) -> list[CaseResult]:
    if shard_count < 1:
        raise ValueError("shard_count must be at least 1")
    if shard_index < 0 or shard_index >= shard_count:
        raise ValueError("shard_index must be in [0, shard_count)")
    profile = load_json(profile_path)
    test_root = (profile_path.parent / profile["root"]).resolve()
    if manifest is not None:
        tests = [
            test_root / line
            for line in manifest.read_text(encoding="utf-8").splitlines()
            if line
        ]
        all_tests = tests
    else:
        profile, tests = discover(profile_path)
        all_tests = tests
        tests = [
            test for index, test in enumerate(tests)
            if index % shard_count == shard_index
        ]
    if limit:
        tests = tests[:limit]
    exclusions = validate_exclusions(
        (profile_path.parent / profile["exclusions"]).resolve()
    )
    harness_root = (
        profile_path.parent / profile.get("harness", "")
    ).resolve()
    allowed_features = set(profile.get("features", []))
    timeout = float(profile.get("timeout_seconds", 10))
    detail_limit = int(profile.get("detail_limit", 500))
    results: list[CaseResult] = []
    discovered_names = {
        test.relative_to(
            (profile_path.parent / profile["root"]).resolve()
        ).as_posix()
        for test in all_tests
    }

    for test in tests:
        relative = test.relative_to(
            (profile_path.parent / profile["root"]).resolve()
        ).as_posix()
        source = test.read_text(encoding="utf-8")
        metadata = parse_frontmatter(source)
        if metadata.edition == "unknown":
            metadata.edition = profile.get("edition", "unknown")
        if allowed_features and not set(metadata.features) <= allowed_features:
            continue
        if changed_features and not (
            set(metadata.features) & changed_features
        ):
            continue
        if relative in exclusions:
            results.append(
                CaseResult(
                    "test262",
                    relative,
                    metadata.features[0] if metadata.features else "language",
                    "EXCLUDED",
                    0,
                    exclusions[relative]["reason"],
                )
            )
            continue

        official_harness = bool(profile.get("official_harness", False))
        includes: list[str] = []
        if "raw" not in metadata.flags:
            includes.extend(profile.get("default_includes", []))
            includes.extend(metadata.includes)
            if "async" in metadata.flags and official_harness:
                includes.append("doneprintHandle.js")
        # Preserve harness order while avoiding duplicate definitions.
        includes = list(dict.fromkeys(includes))
        harness = ""
        for include in includes:
            include_path = harness_root / include
            if not include_path.exists():
                results.append(
                    CaseResult(
                        "test262",
                        relative,
                        "harness",
                        "FAIL",
                        0,
                        f"missing include {include}",
                    )
                )
                break
            harness += include_path.read_text(encoding="utf-8") + "\n"
        else:
            if "async" in metadata.flags and not official_harness:
                harness += (
                    "let __test262Done = false;\n"
                    "function $DONE(error) {\n"
                    "  if (error) { throw error; }\n"
                    "  __test262Done = true;\n"
                    "}\n"
                )
            source_body = re.sub(
                r"/\*---.*?---\*/", "", source, flags=re.DOTALL
            )
            if not official_harness:
                source_body = re.sub(
                    r"\bassert\((.*?)\);",
                    r'if (!(\1)) throw new Error("Test262 assertion failed");',
                    source_body,
                    flags=re.DOTALL,
                )
            flags = set(metadata.flags)
            if not official_harness:
                variants = [("default", "onlyStrict" in flags)]
            elif flags & {"raw", "module", "noStrict"}:
                variants = [("module" if "module" in flags else "non-strict", False)]
            elif "onlyStrict" in flags:
                variants = [("strict", True)]
            else:
                variants = [("non-strict", False), ("strict", True)]

            all_passed = True
            total_duration = 0.0
            peak_memory = 0
            failure_details: list[str] = []
            phase = metadata.negative_phase
            command = "check" if phase in {"parse", "early"} else "run"
            negative = bool(phase)

            for variant_name, strict in variants:
                strict_directive = '"use strict";\n' if strict else ""
                body = source_body
                if not metadata.negative_phase and not (
                    {"raw", "module"} & flags
                ):
                    async_check = (
                        "\nif (!__test262Done) { "
                        'throw new Error("Test262 async test did not call $DONE"); '
                        "}\n"
                        if "async" in flags and not official_harness
                        else ""
                    )
                    body = (
                        "function main(): number {\n"
                        + strict_directive
                        + body
                        + async_check
                        + "\nreturn 0;\n}\n"
                    )
                    program = harness + body
                else:
                    # Parse/early-negative tests and modules must retain their
                    # original top-level grammar. For strict script variants,
                    # the directive precedes all harness/test source text.
                    program = strict_directive + harness + body

                with tempfile.NamedTemporaryFile(
                    mode="w",
                    suffix=".mjs" if "module" in flags else ".js",
                    encoding="utf-8",
                    delete=False,
                ) as temporary:
                    temporary.write(program)
                    temporary_path = Path(temporary.name)
                try:
                    completed = run_command(
                        [str(nova), command, str(temporary_path), "--no-cache"],
                        cwd=profile_path.parents[3],
                        timeout=timeout,
                    )
                    total_duration += completed.duration_ms
                    peak_memory = max(
                        peak_memory, completed.peak_memory_kb
                    )
                    variant_passed = (
                        completed.returncode != 0
                        if negative
                        else completed.returncode == 0
                    )
                    combined_output = (
                        completed.stderr + "\n" + completed.stdout
                    )
                    if (
                        variant_passed
                        and negative
                        and official_harness
                        and metadata.negative_type
                    ):
                        variant_passed = (
                            metadata.negative_type in combined_output
                        )
                    if (
                        variant_passed
                        and official_harness
                        and "async" in flags
                        and not negative
                    ):
                        variant_passed = (
                            "Test262:AsyncTestComplete"
                            in completed.stdout
                        )
                    if not variant_passed:
                        all_passed = False
                        output = (
                            completed.stderr.strip()
                            or completed.stdout.strip()
                            or f"exit {completed.returncode}"
                        )
                        if (
                            negative
                            and metadata.negative_type
                            and completed.returncode != 0
                            and metadata.negative_type not in combined_output
                        ):
                            output = (
                                f"expected negative type "
                                f"{metadata.negative_type}; {output}"
                            )
                        failure_details.append(
                            f"[{variant_name}] {output}"
                        )
                except subprocess.TimeoutExpired:
                    all_passed = False
                    total_duration += timeout * 1000
                    failure_details.append(f"[{variant_name}] timeout")
                finally:
                    if keep_temporaries:
                        print(
                            "Test262 temporary input "
                            f"({variant_name}): {temporary_path}"
                        )
                    else:
                        temporary_path.unlink(missing_ok=True)

            results.append(
                CaseResult(
                    "test262",
                    relative,
                    metadata.features[0] if metadata.features else "language",
                    "PASS" if all_passed else "FAIL",
                    total_duration,
                    "\n".join(failure_details)[:detail_limit],
                    edition=metadata.edition,
                    peak_memory_kb=peak_memory,
                )
            )

    undocumented = (
        set(exclusions) - discovered_names if manifest is None else set()
    )
    if undocumented:
        raise RuntimeError(
            "exclusions do not match discovered tests: "
            + ", ".join(sorted(undocumented))
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
    parser.add_argument("--changed-feature", action="append", default=[])
    parser.add_argument("--shard-index", type=int, default=0)
    parser.add_argument("--shard-count", type=int, default=1)
    parser.add_argument("--limit", type=int, default=0)
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--keep-temporaries", action="store_true")
    parser.add_argument(
        "--allow-failures",
        action="store_true",
        help="write the complete baseline dashboard and exit successfully",
    )
    args = parser.parse_args()
    results = run_profile(
        args.nova.resolve(),
        args.profile.resolve(),
        args.output.resolve(),
        set(args.changed_feature) or None,
        shard_index=args.shard_index,
        shard_count=args.shard_count,
        limit=args.limit,
        require_green=not args.allow_failures,
        manifest=args.manifest.resolve() if args.manifest else None,
        keep_temporaries=args.keep_temporaries,
    )
    print(
        f"Test262 profile: {sum(r.status == 'PASS' for r in results)}/"
        f"{len(results)} green"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
