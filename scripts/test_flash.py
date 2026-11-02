"""Offline regression tests for the recovery flasher. No serial access."""
import contextlib
import importlib.util
import io
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("flash", ROOT / "release/flash.py")
flash = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(flash)


class RecoveryTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.image = Path(self.tmp.name) / "esp32p4_android_auto-waveshare-rev3-1.3.33-merged.bin"
        data = bytearray(0x2000 + 24)
        data[0x2000] = 0xE9
        struct.pack_into("<H", data, 0x2000 + 12, 0x12)
        struct.pack_into("<HH", data, 0x2000 + 15, 300, 399)
        self.image.write_bytes(data)

    def run_main(self, revision="3.2", version="5.3.0"):
        def response(cmd, **kwargs):
            if cmd[-1] == "version":
                return subprocess.CompletedProcess(cmd, 0, version + "\n", "")
            self.assertIn(cmd[-1], ("chip-id", "chip_id"))
            return subprocess.CompletedProcess(cmd, 0, f"Chip type: ESP32-P4 (revision v{revision})\n", "")

        with patch.object(flash, "HERE", self.tmp.name), \
             patch.object(flash, "list_ports", return_value=["COM15"]), \
             patch.object(flash, "choose", side_effect=lambda prompt, items, render: items[-1]), \
             patch.object(flash, "esptool_cmd", return_value=["python", "-m", "esptool"]), \
             patch.object(flash, "pause"), \
             patch.object(flash.subprocess, "run", side_effect=response), \
             patch.object(flash.subprocess, "call", return_value=0) as write, \
             contextlib.redirect_stdout(io.StringIO()):
            try:
                flash.main()
            except SystemExit:
                pass
            return write.call_args_list

    def test_rev3_image_writes_on_rev32(self):
        calls = self.run_main()
        self.assertEqual(len(calls), 1)
        self.assertIn("write-flash", calls[0].args[0])
        self.assertNotIn("--force", calls[0].args[0])
        self.assertIn("115200", calls[0].args[0])

    def test_rev3_image_never_writes_on_rev1(self):
        self.assertEqual(self.run_main(revision="1.3"), [])

    def test_rev1_image_never_writes_on_rev32(self):
        data = bytearray(self.image.read_bytes())
        struct.pack_into("<HH", data, 0x2000 + 15, 100, 199)
        self.image.write_bytes(data)
        self.assertEqual(self.run_main(), [])
        self.assertEqual(len(self.run_main(revision="1.3")), 1)

    def test_old_tools_never_write(self):
        for version in ("4.12.dev1", "5.0.2", "5.3.0rc1", "unknown"):
            with self.subTest(version=version):
                self.assertEqual(self.run_main(version=version), [])

    def test_idf_55_stable_tool_uses_legacy_cli(self):
        calls = self.run_main(version="4.12.0")
        self.assertEqual(len(calls), 1)
        self.assertIn("write_flash", calls[0].args[0])
        self.assertIn("--flash_size", calls[0].args[0])

    def test_failed_revision_probe_never_writes(self):
        self.assertEqual(self.run_main(revision="unknown"), [])

    def test_truncated_image_rejected(self):
        self.image.write_bytes(b"\xe9")
        self.assertEqual(self.run_main(), [])

    def test_release_images_match_their_revision_profile(self):
        for image in (ROOT / "release").glob("esp32p4_android_auto-waveshare-*-merged.bin"):
            if "waveshare-rev3-" in image.name:
                self.assertEqual(flash.p4_image_revision_range(image), (300, 399))
                continue
            self.assertEqual(flash.p4_image_revision_range(image), (100, 199))


if __name__ == "__main__":
    unittest.main()
