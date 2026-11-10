#include "re_guest_elf32_image.h"

#include <string.h>

static uint16_t read_u16(const uint8_t *p, int little) {
    if (little) return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static uint32_t read_u32(const uint8_t *p, int little) {
    if (little) {
        return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
            ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    }
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
        ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static int range_fits(size_t offset, size_t length, size_t total) {
    return offset <= total && length <= total - offset;
}

static int power_of_two(uint32_t value) {
    return value && (value & (value - 1u)) == 0u;
}

REGUESTELF32Status re_guest_elf32_parse_image(
    const uint8_t *bytes, size_t size, REGUESTELF32IMAGE *image,
    size_t *failed_program_header) {
    uint16_t type, machine, ehsize, phentsize, phnum;
    uint32_t version, entry, phoff;
    int little;

    if (failed_program_header) *failed_program_header = 0u;
    if (!bytes || !image) return RE_GUEST_ELF32_E_ARGUMENT;
    memset(image, 0, sizeof(*image));

    if (size < 52u || bytes[0] != 0x7fu || bytes[1] != 'E' ||
        bytes[2] != 'L' || bytes[3] != 'F')
        return RE_GUEST_ELF32_E_HEADER;
    if (bytes[4] != 1u) return RE_GUEST_ELF32_E_CLASS;
    if (bytes[5] != 1u && bytes[5] != 2u)
        return RE_GUEST_ELF32_E_ENDIAN;
    if (bytes[6] != 1u) return RE_GUEST_ELF32_E_HEADER;
    little = bytes[5] == 1u;

    type = read_u16(bytes + 16u, little);
    machine = read_u16(bytes + 18u, little);
    version = read_u32(bytes + 20u, little);
    entry = read_u32(bytes + 24u, little);
    phoff = read_u32(bytes + 28u, little);
    ehsize = read_u16(bytes + 40u, little);
    phentsize = read_u16(bytes + 42u, little);
    phnum = read_u16(bytes + 44u, little);

    if (version != 1u || ehsize < 52u || ehsize > size)
        return RE_GUEST_ELF32_E_HEADER;
    if (machine != RE_GUEST_ELF32_EM_ARM)
        return RE_GUEST_ELF32_E_MACHINE;
    if (type != RE_GUEST_ELF32_ET_EXEC && type != RE_GUEST_ELF32_ET_DYN)
        return RE_GUEST_ELF32_E_TYPE;
    if (!phnum || phnum > RE_GUEST_ELF32_MAX_PROGRAM_HEADERS ||
        phentsize < 32u ||
        !range_fits((size_t)phoff, (size_t)phentsize * phnum, size))
        return phnum > RE_GUEST_ELF32_MAX_PROGRAM_HEADERS
            ? RE_GUEST_ELF32_E_CAPACITY : RE_GUEST_ELF32_E_PROGRAM_HEADERS;

    image->type = type;
    image->machine = machine;
    image->entry = entry;
    image->program_header_offset = phoff;
    image->program_header_entry_size = phentsize;
    image->program_header_count = phnum;
    image->little_endian = (uint8_t)little;

    for (size_t i = 0; i < phnum; ++i) {
        const uint8_t *p = bytes + (size_t)phoff + i * phentsize;
        REGUESTELF32PROGRAMHEADER *h = &image->program_headers[i];
        uint64_t memory_end, file_end;

        h->type = read_u32(p, little);
        h->offset = read_u32(p + 4u, little);
        h->virtual_address = read_u32(p + 8u, little);
        h->physical_address = read_u32(p + 12u, little);
        h->file_size = read_u32(p + 16u, little);
        h->memory_size = read_u32(p + 20u, little);
        h->flags = read_u32(p + 24u, little);
        h->alignment = read_u32(p + 28u, little);

        if (h->file_size > h->memory_size) {
            if (failed_program_header) *failed_program_header = i;
            return RE_GUEST_ELF32_E_SEGMENT_SIZE;
        }
        file_end = (uint64_t)h->offset + h->file_size;
        if (file_end > size) {
            if (failed_program_header) *failed_program_header = i;
            return RE_GUEST_ELF32_E_FILE_RANGE;
        }
        memory_end = (uint64_t)h->virtual_address + h->memory_size;
        if (memory_end > 0x100000000ULL) {
            if (failed_program_header) *failed_program_header = i;
            return RE_GUEST_ELF32_E_MEMORY_RANGE;
        }
        if (h->alignment > 1u &&
            (!power_of_two(h->alignment) ||
             (h->virtual_address % h->alignment) !=
             (h->offset % h->alignment))) {
            if (failed_program_header) *failed_program_header = i;
            return RE_GUEST_ELF32_E_ALIGNMENT;
        }

        if (h->type == RE_GUEST_ELF32_PT_DYNAMIC) image->dynamic_count++;
        if (h->type != RE_GUEST_ELF32_PT_LOAD || !h->memory_size) continue;

        image->load_count++;
        if (entry >= h->virtual_address &&
            (uint64_t)entry < memory_end && (h->flags & RE_GUEST_ELF32_PF_X))
            image->entry_in_executable_load = 1u;

        for (size_t j = 0; j < i; ++j) {
            const REGUESTELF32PROGRAMHEADER *prior = &image->program_headers[j];
            uint64_t prior_end;
            if (prior->type != RE_GUEST_ELF32_PT_LOAD || !prior->memory_size)
                continue;
            prior_end = (uint64_t)prior->virtual_address + prior->memory_size;
            if ((uint64_t)h->virtual_address < prior_end &&
                (uint64_t)prior->virtual_address < memory_end) {
                if (failed_program_header) *failed_program_header = i;
                return RE_GUEST_ELF32_E_OVERLAP;
            }
        }
    }
    if (!image->load_count) return RE_GUEST_ELF32_E_PROGRAM_HEADERS;
    return RE_GUEST_ELF32_OK;
}
