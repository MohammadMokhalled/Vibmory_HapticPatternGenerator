"""Packaging integrity regressions; no network, Qt SDK, or third-party modules."""

from pathlib import Path
import io
import sys
import tarfile
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from prepare_distribution import collect_notices, component_for, fetch, safe_target


class DistributionTests(unittest.TestCase):
    def test_archive_paths_cannot_escape_the_output(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ("../LICENSE", "/LICENSE", "C:/LICENSE", "nested/../../LICENSE", "nested\\LICENSE"):
                with self.assertRaises(ValueError):
                    safe_target(root, name)
            archive = root / "unsafe.tar"
            with tarfile.open(archive, "w") as output:
                info = tarfile.TarInfo("../LICENSE")
                info.size = 3
                output.addfile(info, io.BytesIO(b"bad"))
            with self.assertRaises(ValueError):
                collect_notices(archive, root / "notices", "fixture")
            self.assertFalse((root / "LICENSE").is_file())

    def test_corrupt_cache_and_missing_offline_inputs_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            entry = {"file": "source.tar", "sha256": "0" * 64}
            with self.assertRaisesRegex(ValueError, "Missing cached"):
                fetch(entry, root, True)
            (root / entry["file"]).write_bytes(b"corrupt")
            with self.assertRaisesRegex(ValueError, "checksum mismatch"):
                fetch(entry, root, True)

    def test_unknown_binary_dependencies_require_explicit_review(self):
        self.assertEqual(component_for(Path("plugins/platforms/qwindows.dll")), "qtbase")
        self.assertEqual(component_for(Path("Qt6Svg.dll")), "qtsvg")
        self.assertEqual(component_for(Path("avcodec-61.dll")), "ffmpeg")
        for dependency in ("unknown.dll", "Qt6Quick.dll", "newcodec.dll"):
            with self.assertRaisesRegex(ValueError, "Unreviewed"):
                component_for(Path(dependency))


if __name__ == "__main__":
    unittest.main()
