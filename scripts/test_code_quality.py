"""Regression tests for tool discovery and compilation-database filtering."""

import contextlib
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import code_quality as quality


class QualityTests(unittest.TestCase):
    def test_unversioned_llvm_18_is_supported(self):
        with patch.object(quality.shutil, "which", side_effect=[None, "/bin/clang-format"]), \
                patch.object(quality.subprocess, "check_output", return_value="clang-format version 18.1.8"):
            self.assertEqual(quality.find_llvm_tool("clang-format"), "/bin/clang-format")

    def test_wrong_llvm_version_is_rejected(self):
        with patch.object(quality.shutil, "which", return_value="/bin/clang-format"), \
                patch.object(quality.subprocess, "check_output", return_value="clang-format version 21.1.8"):
            with self.assertRaisesRegex(SystemExit, "Expected clang-format 18.x"):
                quality.find_llvm_tool("clang-format")

    def test_missing_tool_has_actionable_error(self):
        with patch.object(quality.shutil, "which", return_value=None):
            with self.assertRaisesRegex(SystemExit, "nix develop"):
                quality.find_llvm_tool("clang-tidy")

    def test_deleted_tracked_sources_are_skipped(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "main.cpp").touch()
            with patch.object(quality, "ROOT", root), \
                    patch.object(quality.subprocess, "check_output",
                                 return_value=b"main.cpp\0deleted.cpp\0third_party/lib.cpp\0"):
                self.assertEqual(quality.project_files(), [Path("main.cpp")])

    def test_tidy_uses_only_project_sources_in_database(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            database = [{"directory": str(root), "file": name}
                        for name in ("main.cpp", "main.cpp", "third_party/lib.cpp")]
            (root / "compile_commands.json").write_text(json.dumps(database))
            with patch.object(quality, "ROOT", root), \
                    patch.object(quality, "project_files", return_value=[
                        Path("main.cpp"), Path("unfinished.cpp"), Path("header.hpp")]), \
                    patch.object(quality, "find_llvm_tool", return_value="clang-tidy-18"), \
                    patch.object(quality.subprocess, "call", return_value=0) as run, \
                    patch("sys.argv", ["code_quality.py", "tidy", "--build-dir", str(root)]):
                self.assertEqual(quality.main(), 0)
                run.assert_called_once_with([
                    "clang-tidy-18", "main.cpp", "--config-file=.clang-tidy",
                    "-p", str(root)], cwd=root)

    def test_missing_database_has_actionable_error(self):
        with tempfile.TemporaryDirectory() as temporary:
            with patch.object(quality, "ROOT", Path(temporary)), \
                    patch.object(quality, "project_files", return_value=[Path("main.cpp")]), \
                    patch("sys.argv", ["code_quality.py", "tidy"]), \
                    contextlib.redirect_stderr(io.StringIO()) as error:
                with self.assertRaises(SystemExit) as raised:
                    quality.main()
                self.assertEqual(raised.exception.code, 2)
                self.assertIn("cmake --preset quality", error.getvalue())


if __name__ == "__main__":
    unittest.main()
