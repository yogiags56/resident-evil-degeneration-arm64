import importlib.util
import struct
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "xe3u_relocation_audit", ROOT / "tools" / "xe3u_relocation_audit.py")
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)


def section(kind, payload):
    return struct.pack("<II", kind, 8 + len(payload)) + payload


def fixture():
    symbols = section(0, struct.pack("<H", 1) + b"sampleApi\0")
    internal_offsets = (0x10, 0x80)
    internal = section(1, struct.pack("<I", len(internal_offsets)) +
                       struct.pack("<2I", *internal_offsets))
    external = section(4, struct.pack("<I", 1) +
                       struct.pack("<HHH", 0, 0x24, 0))
    fixups = symbols + internal + external
    codeoff = 0x48 + len(fixups)
    code = bytearray(0x100)
    base = 0x10000000
    struct.pack_into("<I", code, 0x10, base + 0x20)
    struct.pack_into("<I", code, 0x80, base + 0x81)
    # Header is the observed 0x48-byte XE3U base + extended header.
    header = bytearray(0x48)
    header[:4] = b"XE3U"
    struct.pack_into("<IHH", header, 4, 0x41901, 2, 5)
    struct.pack_into("<5I", header, 12, 0x48, len(fixups),
                     codeoff, len(code), 0x180)
    struct.pack_into("<I", header, 40, 0)
    struct.pack_into("<I", header, 52, base)
    struct.pack_into("<3I", header, 56, codeoff + len(code), 0, 8)
    struct.pack_into("<I", header, 68, 0x80)
    return bytes(header) + fixups + bytes(code)


class Xe3uRelocationAuditTests(unittest.TestCase):
    def test_internal_pointer_regions_and_external_zero_cell(self):
        report = AUDIT.audit_image(fixture())
        self.assertEqual(report["symbols"], 1)
        self.assertEqual(report["internal_relocations"]["count"], 2)
        self.assertEqual(report["internal_relocations"]["unique_patch_sites"], 2)
        self.assertEqual(report["internal_relocations"]["pointer_bit0_set"], 1)
        self.assertEqual(report["internal_relocations"]["pointer_bit0_clear"], 1)
        self.assertEqual(report["external_relocations"]["count"], 1)
        self.assertEqual(report["external_relocations"]["zero_cells"][0]["symbol"],
                         "sampleApi")
        self.assertEqual(report["external_relocations"]["verified_arm_import_stubs"], 0)

    def test_arm_import_stub_cell_formula(self):
        code = bytearray(0x13B70C)
        struct.pack_into("<III", code, 0x84,
                         0xE28FC601, 0xE28CCA3B, 0xE5BCF67C)
        result = AUDIT.verify_arm_import_stub(code, 0x13B708)
        self.assertIsNotNone(result)
        self.assertEqual(result["stub_offset"], "0x84")
        self.assertEqual(result["effective_cell"], "0x13b708")

    def test_arm_import_stub_rejects_wrong_target(self):
        code = bytearray(0x13B70C)
        struct.pack_into("<III", code, 0x84,
                         0xE28FC601, 0xE28CCA3B, 0xE5BCF67C)
        self.assertIsNone(AUDIT.verify_arm_import_stub(code, 0x13B70C))

    def test_rejects_internal_pointer_outside_image(self):
        image = bytearray(fixture())
        codeoff = struct.unpack_from("<I", image, 20)[0]
        struct.pack_into("<I", image, codeoff + 0x10, 0x20000000)
        with self.assertRaises(AUDIT.AuditError):
            AUDIT.audit_image(bytes(image))


if __name__ == "__main__":
    unittest.main()
