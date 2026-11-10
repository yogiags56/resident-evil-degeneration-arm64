import importlib.util
import struct
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "xe3u_elf32_provider_audit", ROOT / "tools" / "xe3u_elf32_provider_audit.py")
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)


def _encode_thumb_bl(pc, target):
    displacement = target - (pc + 4)
    assert displacement % 2 == 0 and -(1 << 24) <= displacement < (1 << 24)
    imm = displacement & 0x01ffffff
    s = (imm >> 24) & 1
    i1 = (imm >> 23) & 1
    i2 = (imm >> 22) & 1
    j1 = (~(i1 ^ s)) & 1
    j2 = (~(i2 ^ s)) & 1
    first = 0xf000 | (s << 10) | ((imm >> 12) & 0x03ff)
    second = 0xd000 | (j1 << 13) | (j2 << 11) | ((imm >> 1) & 0x07ff)
    return first, second


def _synthetic_gl_wrapper_elf():
    data = bytearray(0x180)
    data[:16] = bytes((0x7f, 0x45, 0x4c, 0x46, 1, 1, 1)) + bytes(9)
    # ELF32 ET_DYN, EM_ARM; one writable/executable PT_LOAD for this parser fixture.
    struct.pack_into("<HHIIIIIHHHHHH", data, 16,
        3, 40, 1, 0x1001, 52, 0, 0, 52, 32, 1, 40, 0, 0)
    struct.pack_into("<IIIIIIII", data, 52,
        1, 0x100, 0x1000, 0, 0x80, 0x100, 7, 4)
    code = bytearray(0x80)
    struct.pack_into("<H", code, 0x00, 0x4801)  # LDR r0,[pc,#4] -> 0x1008
    struct.pack_into("<H", code, 0x02, 0x6843)  # LDR r3,[r0,#4]
    bl_hi, bl_lo = _encode_thumb_bl(0x1004, 0x1020)
    struct.pack_into("<HH", code, 0x04, bl_hi, bl_lo)
    struct.pack_into("<I", code, 0x08, 0x1080)  # dispatch table base in BSS
    struct.pack_into("<H", code, 0x0c, 0x4801)  # LDR r0,[pc,#4] -> 0x1014
    struct.pack_into("<H", code, 0x0e, 0x46c0)  # NOP
    struct.pack_into("<H", code, 0x10, 0x6040)  # STR r0,[r0,#4]
    struct.pack_into("<H", code, 0x12, 0x46c0)
    struct.pack_into("<I", code, 0x14, 0x1060)  # pointer to symbol name
    code[0x60:0x67] = b"glTest\\0"
    data[0x100:0x180] = code
    return bytes(data)


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

    def test_full_lazy_gl_wrapper_table_audit(self):
        report = AUDIT.audit_gl_dispatch_wrappers(
            _synthetic_gl_wrapper_elf(),
            [{
                "name": "glTest", "elf_value": "0x00001001",
                "elf_size": 0x20, "elf_type": 2, "thumb_state": True,
            }],
            [{"entries": [{"type": 23, "offset": "0x00001008"}]}],
        )
        self.assertEqual(report["classified_lazy_dispatch_wrapper_count"], 1)
        self.assertTrue(report["all_classified_slots_zero_fill_bss"])
        self.assertTrue(report["all_table_base_literals_have_r_arm_relative"])
        self.assertTrue(report["all_function_name_literals_match"])
        self.assertTrue(report["all_wrappers_cache_resolved_pointer"])
        wrapper = report["wrappers"][0]
        self.assertEqual(wrapper["table_base"], "0x00001080")
        self.assertEqual(wrapper["slot_address"], "0x00001084")
        self.assertEqual(wrapper["shared_call_targets"], ["0x00001020"])

    def test_rejects_nonmatching_encodings(self):
        self.assertIsNone(AUDIT._thumb_ldr_literal(0x6800, 0x1000))
        self.assertIsNone(AUDIT._thumb_ldr_word_imm(0x4800))
        self.assertIsNone(AUDIT._thumb_bl_target(0x1000, 0x2000, 0x3000))


if __name__ == "__main__":
    unittest.main()
