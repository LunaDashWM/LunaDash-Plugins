#!/usr/bin/env python3
"""Dependency-free regression tests for plugin-specific settings validation."""
import copy
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


SOURCE = Path(__file__).resolve().parents[2]
SDK = SOURCE / "cmake/plugins"
sys.path.insert(0, str(SDK))

from SettingsSchema import validate_schema
from ValidatePlugin import validate


def array_setting(inferred=False):
    rule = {
        "type": "array",
        "default": [],
        "items": {"type": "string", "enum": ["a", "b"]},
    }
    if inferred:
        rule["enum"] = [[], ["a"]]
    else:
        rule["control"] = "select"
    return rule


class PluginSettingsValidationTests(unittest.TestCase):
    def setUp(self):
        self.project = SOURCE / "templates/plugins/quickshell"
        self.manifest = json.loads((self.project / "metadata.json").read_text())
        self.targets = json.loads((SOURCE / "data/plugins/targets.json").read_text())

    def manifest_with_settings(self, settings, multi_target=False):
        manifest = copy.deepcopy(self.manifest)
        manifest["settings"] = settings
        if multi_target:
            implementation = {
                key: manifest.pop(key)
                for key in ("type", "target", "mode", "entry", "settings")
            }
            manifest["targets"] = [
                dict(implementation, target="desktop-widgets", settings={}),
                implementation,
            ]
        return manifest

    def test_supported_scalar_controls(self):
        settings = {
            "enabled": {"type": "boolean", "default": False, "control": "toggle"},
            "mode": {
                "type": "string", "default": "a", "enum": ["a", "b"],
                "control": "select",
            },
            "count": {"type": "integer", "default": 2, "control": "number"},
            "strength": {
                "type": "number", "default": 0.5, "minimum": 0, "maximum": 1,
                "control": "slider",
            },
        }
        for inferred in (False, True):
            rules = copy.deepcopy(settings)
            if inferred:
                for rule in rules.values():
                    rule.pop("control")
            for multi_target in (False, True):
                with self.subTest(inferred=inferred, multi_target=multi_target):
                    validate(self.manifest_with_settings(rules, multi_target),
                             self.project, self.targets)

    def test_array_select_is_rejected_for_plugins(self):
        for inferred in (False, True):
            for multi_target in (False, True):
                with self.subTest(inferred=inferred, multi_target=multi_target):
                    manifest = self.manifest_with_settings(
                        {"many": array_setting(inferred)}, multi_target)
                    with self.assertRaisesRegex(ValueError, "array"):
                        validate(manifest, self.project, self.targets)

    def test_generic_settings_schema_still_supports_arrays(self):
        for inferred in (False, True):
            with self.subTest(inferred=inferred):
                validate_schema({"many": array_setting(inferred)})

    def test_cli_rejects_array_setting_without_build_receipt(self):
        with tempfile.TemporaryDirectory(prefix="lunadash-validation-test-") as directory:
            project = Path(directory)
            metadata = project / "metadata.json"
            metadata.write_text(json.dumps(self.manifest_with_settings(
                {"many": array_setting()})))
            shutil.copyfile(self.project / "Main.qml", project / "Main.qml")
            output = project / "output"
            result = subprocess.run(
                [sys.executable, "-B", str(SDK / "ValidatePlugin.py"),
                 "--metadata", str(metadata),
                 "--targets", str(SOURCE / "data/plugins/targets.json"),
                 "--output", str(output)],
                capture_output=True, text=True,
            )
            self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("LunaDash plugin metadata:", result.stderr)
            self.assertIn("array", result.stderr)
            self.assertNotIn("Traceback", result.stderr)
            self.assertFalse((output / ".lunadash-sdk.json").exists())


if __name__ == "__main__":
    unittest.main()
