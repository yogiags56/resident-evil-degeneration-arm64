#include "re_guest_elf32_identity.h"
#include <string.h>
#define EF_ARM_EABIMASK 0xff000000u
#define EF_ARM_EABI_UNKNOWN 0x00000000u
#define EF_ARM_EABI5 0x05000000u
static int valid_expected_soname(const char *s) {
    size_t n;
    if (!s) return 1;
    n = strlen(s);
    if (!n || n >= RE_GUEST_ELF32_DYNAMIC_STRING_CAP ||
        strchr(s, '/') || strchr(s, '\\\\')) return 0;
    for (size_t i = 0; i < n; ++i) {
        unsigned char ch = (unsigned char)s[i];
        if (ch < 0x21u || ch > 0x7eu) return 0;
    }
    return 1;
}
REGUESTELF32IDENTITYSTATUS re_guest_elf32_verify_identity(
    const uint8_t *bytes, size_t size, const char *expected_soname,
    REGUESTELF32IDENTITYTYPE expected_type, REGUESTELF32IDENTITY *identity,
    size_t *failed_index) {
    REGUESTELF32IDENTITY pending;
    REGUESTELF32Status image_status;
    REGUESTELF32DYNAMICStatus dynamic_status;
    uint32_t flags;
    if (failed_index) *failed_index = 0u;
    if (!bytes || !identity ||
        (expected_type != RE_GUEST_ELF32_IDENTITY_ANY &&
         expected_type != RE_GUEST_ELF32_IDENTITY_EXEC &&
         expected_type != RE_GUEST_ELF32_IDENTITY_SHARED) ||
        !valid_expected_soname(expected_soname))
        return RE_GUEST_ELF32_IDENTITY_E_ARGUMENT;
    memset(&pending, 0, sizeof(pending));
    image_status = re_guest_elf32_parse_image(bytes, size, &pending.image, failed_index);
    if (image_status != RE_GUEST_ELF32_OK) return RE_GUEST_ELF32_IDENTITY_E_ARGUMENT;
    if ((expected_type == RE_GUEST_ELF32_IDENTITY_EXEC &&
         pending.image.type != RE_GUEST_ELF32_ET_EXEC) ||
        (expected_type == RE_GUEST_ELF32_IDENTITY_SHARED &&
         pending.image.type != RE_GUEST_ELF32_ET_DYN))
        return RE_GUEST_ELF32_IDENTITY_E_TYPE;
    flags = (uint32_t)bytes[36] | ((uint32_t)bytes[37] << 8) |
        ((uint32_t)bytes[38] << 16) | ((uint32_t)bytes[39] << 24);
    if (!pending.image.little_endian)
        flags = ((uint32_t)bytes[36] << 24) | ((uint32_t)bytes[37] << 16) |
            ((uint32_t)bytes[38] << 8) | (uint32_t)bytes[39];
    pending.arm_flags = flags;
    pending.arm_eabi_version = (uint8_t)((flags & EF_ARM_EABIMASK) >> 24);
    if ((flags & EF_ARM_EABIMASK) != EF_ARM_EABI_UNKNOWN &&
        (flags & EF_ARM_EABIMASK) != EF_ARM_EABI5)
        return RE_GUEST_ELF32_IDENTITY_E_ABI;
    dynamic_status = re_guest_elf32_parse_dynamic(bytes, size, &pending.image,
        &pending.dynamic, failed_index);
    if (dynamic_status != RE_GUEST_ELF32_DYNAMIC_OK)
        return RE_GUEST_ELF32_IDENTITY_E_DYNAMIC;
    if (expected_soname &&
        (!pending.dynamic.has_soname ||
         strcmp(pending.dynamic.soname, expected_soname) != 0))
        return RE_GUEST_ELF32_IDENTITY_E_SONAME;
    *identity = pending;
    return RE_GUEST_ELF32_IDENTITY_OK;
}
