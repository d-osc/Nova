from __future__ import annotations

import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = Path(__file__).with_name("phase6_project")


class Phase6ProjectToolingTests(unittest.TestCase):
    nova: Path

    @classmethod
    def setUpClass(cls) -> None:
        configured = Path(
            __import__("os").environ.get(
                "NOVA_TEST_EXECUTABLE", ROOT / "build" / "Debug" / "nova.exe"
            )
        )
        cls.nova = configured.resolve()
        if not cls.nova.exists():
            raise unittest.SkipTest(f"Nova executable not found: {cls.nova}")

    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.project = Path(self.temp.name) / "project"
        shutil.copytree(FIXTURE, self.project)
        package_target = (
            self.project / "node_modules" / "@nova" / "tiny-package"
        )
        package_target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(
            self.project / "packages" / "tiny-package",
            package_target,
            dirs_exist_ok=True,
        )

    def build(self) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [str(self.nova), "build", "."],
            cwd=self.project,
            text=True,
            capture_output=True,
            timeout=30,
            check=False,
        )

    def test_project_build_emits_all_configured_artifacts(self) -> None:
        result = self.build()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

        expected = {
            "models/card.js",
            "models/card.d.ts",
            "models/card.js.map",
            "models/card.d.ts.map",
            "view.js",
            "view.d.ts",
            "view.js.map",
            "view.d.ts.map",
            "index.js",
            "index.d.ts",
            "package.js",
            "package.d.ts",
            "module.js",
            "module.d.ts",
            "legacy.js",
            "legacy.d.ts",
            ".tsbuildinfo",
        }
        emitted = {
            path.relative_to(self.project / "dist").as_posix()
            for path in (self.project / "dist").rglob("*")
            if path.is_file()
        }
        self.assertTrue(expected <= emitted, expected - emitted)

        view = (self.project / "dist" / "view.js").read_text(encoding="utf-8")
        self.assertNotIn("<article", view)
        self.assertNotIn("<CardView", view)
        self.assertIn("@nova/jsx/jsx-runtime", view)
        self.assertIn('className: "card"', view)
        self.assertIn("children:", view)
        index = (self.project / "dist" / "index.js").read_text(encoding="utf-8")
        self.assertIn('require("./models/card")', index)
        package_output = (
            self.project / "dist" / "package.js"
        ).read_text(encoding="utf-8")
        self.assertIn('require("@nova/tiny-package")', package_output)
        node = shutil.which("node")
        if node:
            executed = subprocess.run(
                [node, str(self.project / "dist" / "package.js")],
                cwd=self.project,
                text=True,
                capture_output=True,
                timeout=30,
                check=False,
            )
            self.assertEqual(
                executed.returncode, 0, executed.stdout + executed.stderr
            )

        declaration = (
            self.project / "dist" / "models" / "card.d.ts"
        ).read_text(encoding="utf-8")
        self.assertIn("export interface Card", declaration)
        self.assertIn("export declare function makeCard", declaration)

        source_map = json.loads(
            (self.project / "dist" / "view.js.map").read_text(encoding="utf-8")
        )
        self.assertEqual(source_map["version"], 3)
        self.assertIn("sourcesContent", source_map)
        self.assertTrue(source_map["mappings"])

        for declaration_path in (
            self.project / "dist" / "models" / "card.d.ts",
            self.project / "dist" / "view.d.ts",
            self.project
            / "node_modules"
            / "@nova"
            / "tiny-package"
            / "index.d.ts",
        ):
            checked = subprocess.run(
                [str(self.nova), "check", str(declaration_path)],
                cwd=self.project,
                text=True,
                capture_output=True,
                timeout=30,
                check=False,
            )
            self.assertEqual(
                checked.returncode, 0, checked.stdout + checked.stderr
            )

    def test_incremental_second_build_is_successful(self) -> None:
        first = self.build()
        self.assertEqual(first.returncode, 0, first.stdout + first.stderr)
        second = self.build()
        self.assertEqual(second.returncode, 0, second.stdout + second.stderr)
        self.assertTrue((self.project / "dist" / ".tsbuildinfo").exists())

    def test_absolute_project_path_keeps_outputs_inside_project(self) -> None:
        result = subprocess.run(
            [str(self.nova), "build", str(self.project)],
            cwd=ROOT,
            text=True,
            capture_output=True,
            timeout=30,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue((self.project / "dist" / "index.js").exists())

    def test_child_config_can_disable_inherited_boolean_options(self) -> None:
        config_path = self.project / "tsconfig.json"
        config = json.loads(config_path.read_text(encoding="utf-8"))
        config["compilerOptions"] = {
            "sourceMap": False,
            "declarationMap": False,
            "incremental": False,
        }
        config_path.write_text(json.dumps(config), encoding="utf-8")

        result = self.build()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue((self.project / "dist" / "index.d.ts").exists())
        self.assertFalse((self.project / "dist" / "index.js.map").exists())
        self.assertFalse((self.project / "dist" / "index.d.ts.map").exists())
        self.assertFalse((self.project / "dist" / ".tsbuildinfo").exists())

    def test_composite_project_reference_builds_dependency_first(self) -> None:
        workspace = Path(self.temp.name) / "composite"
        library = workspace / "library"
        application = workspace / "application"
        (library / "src").mkdir(parents=True)
        (application / "src").mkdir(parents=True)
        (library / "src" / "index.ts").write_text(
            'export const version: string = "1.0";\n', encoding="utf-8"
        )
        (application / "src" / "index.ts").write_text(
            'export const name: string = "app";\n', encoding="utf-8"
        )
        shared_options = {
            "composite": True,
            "declaration": True,
            "rootDir": "src",
            "outDir": "dist",
        }
        (library / "tsconfig.json").write_text(
            json.dumps(
                {
                    "compilerOptions": shared_options,
                    "include": ["src/**/*.ts"],
                }
            ),
            encoding="utf-8",
        )
        (application / "tsconfig.json").write_text(
            json.dumps(
                {
                    "compilerOptions": shared_options,
                    "include": ["src/**/*.ts"],
                    "references": [{"path": "../library"}],
                }
            ),
            encoding="utf-8",
        )

        result = subprocess.run(
            [str(self.nova), "build", str(application)],
            cwd=ROOT,
            text=True,
            capture_output=True,
            timeout=30,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue((library / "dist" / "index.d.ts").exists())
        self.assertTrue((application / "dist" / "index.d.ts").exists())

    def test_es_modules_preserve_cycles_reexports_and_dynamic_imports(self) -> None:
        project = Path(self.temp.name) / "esm"
        (project / "src").mkdir(parents=True)
        (project / "tsconfig.json").write_text(
            json.dumps(
                {
                    "compilerOptions": {
                        "module": "esnext",
                        "rootDir": "src",
                        "outDir": "dist",
                        "rewriteRelativeImportExtensions": True,
                    },
                    "include": ["src/**/*.ts"],
                }
            ),
            encoding="utf-8",
        )
        (project / "src" / "a.ts").write_text(
            'import { b } from "./b.ts";\n'
            "export let value: number = 1;\n"
            "export { b };\n"
            'export async function load() { return import("./b.ts"); }\n',
            encoding="utf-8",
        )
        (project / "src" / "b.ts").write_text(
            'import { value } from "./a.ts";\n'
            "export const b: number = value + 1;\n",
            encoding="utf-8",
        )

        result = subprocess.run(
            [str(self.nova), "build", str(project)],
            cwd=ROOT,
            text=True,
            capture_output=True,
            timeout=30,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        emitted = (project / "dist" / "a.js").read_text(encoding="utf-8")
        self.assertIn('from "./b.js"', emitted)
        self.assertIn('import("./b.js")', emitted)
        self.assertIn("export let value", emitted)
        self.assertIn("export { b }", emitted)


if __name__ == "__main__":
    unittest.main()
