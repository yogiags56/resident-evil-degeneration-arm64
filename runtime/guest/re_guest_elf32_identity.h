#ifndef RE_GUEST_ELF32_IDENTITY_H
#define RE_GUEST_ELF32_IDENTITY_H
#include "re_guest_elf32_dynamic.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    RE_GUEST_ELF32_IDENTITY_ANY = 0,
    RE_GUEST_ELF32_IDENTITY_EXEC = 1,
    RE_GUEST_ELF32_IDENTITY_SHARED = 2
} REGUESTELF32IDENTITYTYPE;
typedef struct {
    REGUESTELF32IMAGE image;
    REGUESTELF32DYNAMIC dynamic;
    uint32_t arm_flags;
    uint8_t arm_eabi_version;
} REGUESTELF32IDENTITY;
typedef enum {
    RE_GUEST_ELF32_IDENTITY_OK = 0,
    RE_GUEST_ELF32_IDENTITY_E_ARGUMENT = -660,
    RE_GUEST_ELF32_IDENTITY_E_TYPE = -661,
    RE_GUEST_ELF32_IDENTITY_E_ABI = -662,
    RE_GUEST_ELF32_IDENTITY_E_SONAME = -663,
    RE_GUEST_ELF32_IDENTITY_E_DYNAMIC = -664
} REGUESTELF32IDENTITYSTATUS;
/* Read-only identity gate. expected_soname may be NULL for the executable root.
 * expected_type constrains ET_EXEC/ET_DYN; no mapping or execution occurs.
 * ELF32/EM_ARM/endian/header/program-header validation is delegated to image parser.
 */
REGUESTELF32IDENTITYSTATUS re_guest_elf32_verify_identity(
    const uint8_t *bytes, size_t size, const char *expected_soname,
    REGUESTELF32IDENTITYTYPE expected_type, REGUESTELF32IDENTITY *identity,
    size_t *failed_index);
#ifdef __cplusplus
}
#endif
#endif
