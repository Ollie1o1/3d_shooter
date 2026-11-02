"""Catch stale loader/asset combinations before a web release is synced."""
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location("stamp_web", Path(__file__).resolve().parents[1] / "tools/stamp_web.py")
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)


class WebRelease(unittest.TestCase):
    def test_asset_identity_and_cache_urls(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            for name in ("overdrive.js", "overdrive.wasm", "overdrive.data"):
                (root / name).write_bytes(name.encode())
            original = release.stamp(root)
            self.assertEqual(release.stamp(root)["build"], original["build"])
            for name in ("overdrive.js", "overdrive.wasm", "overdrive.data"):
                (root / name).write_bytes(b"changed " + name.encode())
                updated = release.stamp(root)
                self.assertNotEqual(updated["build"], original["build"])
                html = (root / "index.html").read_text()
                self.assertNotIn("__OVERDRIVE_BUILD__", html)
                self.assertIn('src="overdrive.js?v=' + updated["build"] + '"', html)
                self.assertIn('locateFile: (path, prefix) => `${prefix}${path}?v=' + updated["build"], html)
                self.assertEqual(json.loads((root / "build-info.json").read_text()), updated)
                self.assertEqual(updated["assets"][name]["bytes"], (root / name).stat().st_size)
                original = updated


if __name__ == "__main__":
    unittest.main()
