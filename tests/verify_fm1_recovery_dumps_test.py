"""Pure host tests for read-only double-dump verification. No hardware writes."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "tools" / "verify_fm1_recovery_dumps.py"
spec = importlib.util.spec_from_file_location("fm1_verify_dumps", SCRIPT)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class FlashDumpChecks(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.a = Path(self.directory.name) / "capture-one.bin"
        self.b = Path(self.directory.name) / "capture-two.bin"
        img = bytearray([0xFF]) * module.FLASH_BYTES
        img[5] = 0x82
        img[module.BOOT_END + 7] = 0x41
        img[module.APP_END + 3] = 0xA3
        self.data = bytes(img)
        self.a.write_bytes(self.data)
        self.b.write_bytes(self.data)

    def test_identical_full_dumps(self):
        outcome = module.verify(self.a, self.b)
        self.assertEqual(len(outcome["sha256"]), 64)
        self.assertEqual(module.main([str(self.a), str(self.b)]), 0)

    def test_same_file_not_two_captures(self):
        with self.assertRaisesRegex(ValueError, "Two distinct files"):
            module.verify(self.a, self.a)

    def test_missing_or_short_dump(self):
        self.b.write_bytes(self.data[:-1])
        with self.assertRaisesRegex(ValueError, "expected exactly"):
            module.verify(self.a, self.b)

    def test_app_difference(self):
        changed = bytearray(self.data)
        changed[0x80000] = 0x34
        self.b.write_bytes(changed)
        with self.assertRaisesRegex(ValueError, "0x80000"):
            module.verify(self.a, self.b)

    def test_device_data_difference(self):
        changed = bytearray(self.data)
        changed[0xFC000] = 0x21
        self.b.write_bytes(changed)
        with self.assertRaisesRegex(ValueError, "0xFC000"):
            module.verify(self.a, self.b)

    def test_blank_boot_head(self):
        bad = bytearray(self.data)
        bad[:module.BOOT_END] = bytes([0xFF]) * module.BOOT_END
        self.b.write_bytes(bad)
        with self.assertRaisesRegex(ValueError, "boot head"):
            module.verify(self.a, self.b)

    def test_blank_application(self):
        bad = bytearray(self.data)
        bad[module.BOOT_END:module.APP_END] = bytes([0xFF]) * (module.APP_END - module.BOOT_END)
        self.b.write_bytes(bad)
        with self.assertRaisesRegex(ValueError, "application"):
            module.verify(self.a, self.b)


if __name__ == "__main__":
    unittest.main()
