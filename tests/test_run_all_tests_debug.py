from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


RUNNER_PATH = Path(__file__).with_name("run_all_tests.py")
SPEC = importlib.util.spec_from_file_location("nova_test_runner", RUNNER_PATH)
assert SPEC and SPEC.loader
RUNNER = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = RUNNER
SPEC.loader.exec_module(RUNNER)


class FailureDebugTests(unittest.TestCase):
    def make_source(self, text: str) -> Path:
        temporary = tempfile.NamedTemporaryFile(
            mode="w", suffix=".ts", encoding="utf-8", delete=False
        )
        with temporary:
            temporary.write(text)
        self.addCleanup(Path(temporary.name).unlink, missing_ok=True)
        return Path(temporary.name)

    def test_maps_exit_code_to_return_line_and_function(self) -> None:
        path = self.make_source(
            "// NOVA_EXPECT_EXIT: 0\n"
            "function main(): number {\n"
            "    if (true) return 7;\n"
            "    return 0;\n"
            "}\n"
        )
        result = RUNNER.TestResult(
            path=path,
            status="FAIL",
            detail="exit 7, expected 0",
            return_code=7,
        )

        rows = RUNNER.failure_debug_rows(result)

        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0].code, "EXIT_7")
        self.assertEqual(rows[0].line, 3)
        self.assertEqual(rows[0].function, "main")

    def test_extracts_compiler_diagnostic_line(self) -> None:
        path = self.make_source(
            "// NOVA_EXPECT_EXIT: 0\n"
            "function broken(): number {\n"
            "    return 0;\n"
            "}\n"
        )
        result = RUNNER.TestResult(
            path=path,
            status="FAIL",
            detail="exit 1, expected 0",
            stderr=f"{path}:3:5: error: invalid expression\n",
            return_code=1,
        )

        rows = RUNNER.failure_debug_rows(result)

        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0].code, "ERROR_DIAGNOSTIC")
        self.assertEqual(rows[0].line, 3)
        self.assertEqual(rows[0].function, "broken")

    def test_formats_windows_access_violation(self) -> None:
        path = self.make_source(
            "// NOVA_EXPECT_EXIT: 0\n"
            "function main(): number { return 0; }\n"
        )
        result = RUNNER.TestResult(
            path=path,
            status="FAIL",
            detail="exit 3221225477, expected 0",
            return_code=3221225477,
        )

        rows = RUNNER.failure_debug_rows(result)

        self.assertEqual(rows[0].code, "0xC0000005")
        self.assertEqual(rows[0].function, "<native-runtime>")

    def test_maps_native_class_constructor_name(self) -> None:
        path = self.make_source(
            "// NOVA_EXPECT_EXIT: 0\n"
            "class ApplicationError extends Error {\n"
            "    constructor(message: string) {\n"
            "        super(message);\n"
            "    }\n"
            "}\n"
        )
        result = RUNNER.TestResult(
            path=path,
            status="FAIL",
            detail="exit 3221225477, expected 0",
            stderr="DEBUG: Created constructor function: ApplicationError_constructor\n",
            return_code=3221225477,
        )

        rows = RUNNER.failure_debug_rows(result)

        self.assertEqual(rows[0].line, 3)
        self.assertEqual(rows[0].function, "ApplicationError.constructor")


if __name__ == "__main__":
    unittest.main()
