from __future__ import annotations

import json
import sys
import tempfile
import unittest
from datetime import date, timedelta
from pathlib import Path


QUALIFICATION = Path(__file__).resolve().parent / "qualification"
sys.path.insert(0, str(QUALIFICATION))

from qualification_common import (  # noqa: E402
    CaseResult,
    assert_green,
    run_command,
    validate_exclusions,
    write_dashboard,
)
from test262_runner import parse_frontmatter  # noqa: E402
from run_changed_test262 import features_for_paths  # noqa: E402
from typescript_upstream_runner import _has_error_baseline  # noqa: E402


class QualificationRunnerTests(unittest.TestCase):
    def test_test262_frontmatter_parses_all_supported_fields(self) -> None:
        metadata = parse_frontmatter(
            """/*---
description: async negative fixture
esid: sec-promise-resolve-functions
features: [Promise, Symbol]
flags: [async]
includes: [assert.js, sta.js]
negative:
  phase: runtime
  type: TypeError
---*/"""
        )
        self.assertEqual(metadata.features, ["Promise", "Symbol"])
        self.assertEqual(metadata.flags, ["async"])
        self.assertEqual(metadata.includes, ["assert.js", "sta.js"])
        self.assertEqual(metadata.negative_phase, "runtime")
        self.assertEqual(metadata.negative_type, "TypeError")
        self.assertEqual(metadata.esid, "sec-promise-resolve-functions")

    def test_test262_frontmatter_accepts_multiline_arrays(self) -> None:
        metadata = parse_frontmatter(
            """/*---
features:
  - Promise
  - Symbol
flags:
  - async
includes:
  - assert.js
---*/"""
        )
        self.assertEqual(metadata.features, ["Promise", "Symbol"])
        self.assertEqual(metadata.flags, ["async"])
        self.assertEqual(metadata.includes, ["assert.js"])

    def test_exclusions_require_review_metadata_and_future_expiry(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "exclusions.json"
            path.write_text(
                json.dumps(
                    {
                        "exclusions": [
                            {
                                "path": "case.js",
                                "reason": "upstream issue",
                                "owner": "runtime",
                                "reviewed_by": "maintainer",
                                "expires": (
                                    date.today() + timedelta(days=1)
                                ).isoformat(),
                            }
                        ]
                    }
                ),
                encoding="utf-8",
            )
            self.assertIn("case.js", validate_exclusions(path))
            payload = json.loads(path.read_text(encoding="utf-8"))
            del payload["exclusions"][0]["reviewed_by"]
            path.write_text(json.dumps(payload), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "missing"):
                validate_exclusions(path)

    def test_dashboard_tracks_category_edition_and_memory(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            result = CaseResult(
                "test262",
                "case.js",
                "Promise",
                "PASS",
                2.5,
                edition="ES2024",
                peak_memory_kb=1024,
            )
            write_dashboard(output, "profile", [result], {})
            payload = json.loads(
                (output / "qualification.json").read_text(encoding="utf-8")
            )
            self.assertEqual(payload["categories"]["Promise"]["PASS"], 1)
            self.assertEqual(payload["editions"]["ES2024"]["PASS"], 1)
            self.assertEqual(payload["results"][0]["peak_memory_kb"], 1024)

    def test_command_measurement_and_budgets(self) -> None:
        completed = run_command(
            [sys.executable, "-c", "x = bytearray(1024 * 1024); print(len(x))"],
            cwd=Path.cwd(),
            timeout=10,
        )
        self.assertEqual(completed.returncode, 0)
        self.assertIn("1048576", completed.stdout)
        self.assertGreater(completed.peak_memory_kb, 0)
        assert_green(
            [
                CaseResult(
                    "unit",
                    "memory",
                    "runner",
                    "PASS",
                    completed.duration_ms,
                    peak_memory_kb=completed.peak_memory_kb,
                )
            ],
            {"max_case_ms": 10000, "max_peak_memory_kb": 1048576},
        )

    def test_changed_path_maps_to_test262_feature_shard(self) -> None:
        self.assertEqual(
            features_for_paths(["src/runtime/Promise.cpp"]), {"Promise"}
        )
        self.assertIn(
            "optional-chaining",
            features_for_paths(["tests/qualification/test262_runner.py"]),
        )

    def test_typescript_upstream_error_baseline_variants(self) -> None:
        names = {
            "clean.types.txt",
            "negative.errors.txt",
            "targeted(target=es5).errors.txt",
        }
        self.assertTrue(_has_error_baseline("negative", names))
        self.assertTrue(_has_error_baseline("targeted", names))
        self.assertFalse(_has_error_baseline("clean", names))


if __name__ == "__main__":
    unittest.main()
