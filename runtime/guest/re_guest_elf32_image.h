#ifndef RE_GUEST_ELF32_IMAGE_H
#define RE_GUEST_ELF32_IMAGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_ELF32_MAX_PROGRAM_HEADERS 32u

enum {
    RE_GUEST_ELF32_ET_EXEC = 2u,
    RE_GUEST_ELF32_ET_DYN = 3u,
    RE_GUEST_ELF32_EM_ARM = 40u,
    RE_GUEST_ELF32_PT_LOAD = 1u,
    RE_GUEST_ELF32_PT_DYNAMIC = 2u,
    RE_GUEST_ELF32_PF_X = 1u,
    RE_GUEST_ELF32_PF_W = 2u,
    RE_GUEST_ELF32_PF_R = 4u
};

typedef struct {
    uint32_t type;
    uint32_t offset;
    uint32_t virtual_address;
    uint32_t physical_address;
    uint32_t file_size;
    uint32_t memory_size;
    uint32_t flags;
    uint32_t alignment;
} REGUESTELF32PROGRAMHEADER;

typedef struct {
    uint16_t type;
    uint16_t machine;
    uint32_t entry;
    uint32_t program_header_offset;
    uint16_t program_header_entry_size;
    uint16_t program_header_count;
    uint8_t little_endian;
    uint8_t load_count;
    uint8_t dynamic_count;
    uint16_t load_overlap_count;
    uint8_t entry_in_executable_load;
    REGUESTELF32PROGRAMHEADER program_headers[RE_GUEST_ELF32_MAX_PROGRAM_HEADERS];
} REGUESTELF32IMAGE;

typedef enum {
    RE_GUEST_ELF32_OK = 0,
    RE_GUEST_ELF32_E_ARGUMENT = -600,
    RE_GUEST_ELF32_E_HEADER = -601,
    RE_GUEST_ELF32_E_CLASS = -602,
    RE_GUEST_ELF32_E_ENDIAN = -603,
    RE_GUEST_ELF32_E_MACHINE = -604,
    RE_GUEST_ELF32_E_TYPE = -605,
    RE_GUEST_ELF32_E_PROGRAM_HEADERS = -606,
    RE_GUEST_ELF32_E_FILE_RANGE = -607,
    RE_GUEST_ELF32_E_MEMORY_RANGE = -608,
    RE_GUEST_ELF32_E_SEGMENT_SIZE = -609,
    RE_GUEST_ELF32_E_ALIGNMENT = -610,
    RE_GUEST_ELF32_E_OVERLAP = -611,
    RE_GUEST_ELF32_E_CAPACITY = -612
} REGUESTELF32Status;

/*
 * Parse ELF32 EM_ARM program headers without relying on section headers.
 * Both ELF byte orders are decoded explicitly. This routine is read-only:
 * it does not allocate guest memory, map segments, apply relocations, search
 * for dependencies, or execute the entry point.
 */
REGUESTELF32Status re_guest_elf32_parse_image(
    const uint8_t *bytes,
    size_t size,
    REGUESTELF32IMAGE *image,
    size_t *failed_program_header);

#ifdef __cplusplus
}
#endif
#endif
