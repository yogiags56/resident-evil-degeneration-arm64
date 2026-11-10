import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "xe3u_elf32_provider_audit", ROOT / "tools" / "xe3u_elf32_provider_audit.py")
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)


class ThumbWrapperDecodeTests(unittest.TestCase):
    def test_thumb_ldr_literal_pc_alignment(self):
        # LDR r4,[pc,#0x1c] at 0x4a7be resolves to literal 0x4a7dc.
        self.assertEqual(AUDIT._thumb_ldr_literal(0x4c07, 0x4a7be),
                         (4, 0x4a7dc))

    def test_thumb_ldr_word_immediate_slot(self):
        # LDR r3,[r4,#4].
        self.assertEqual(AUDIT._thumb_ldr_word_imm(0x6863), (3, 4, 4))

    def test_thumb_str_word_immediate_slot(self):
        # STR r0,[r4,#4].
        self.assertEqual(AUDIT._thumb_str_word_imm(0x6060), (0, 4, 4))

    def test_thumb_bl_target_sign_extension(self):
        # Original libjirobiodg wrapper BL at 0x4a7d0 -> shared helper 0x3d64c.
        self.assertEqual(AUDIT._thumb_bl_target(
            0x4a7d0, 0xf7f2, 0xff3c), 0x3d64c)

    def test_rejects_nonmatching_encodings(self):
        self.assertIsNone(AUDIT._thumb_ldr_literal(0x6800, 0x1000))
        self.assertIsNone(AUDIT._thumb_ldr_word_imm(0x4800))
        self.assertIsNone(AUDIT._thumb_bl_target(0x1000, 0x2000, 0x3000))


if __name__ == "__main__":
    unittest.main()
