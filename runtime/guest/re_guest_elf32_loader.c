#include "re_guest_elf32_loader.h"

#include "re_guest_elf32_image.h"
#include "re_guest_elf_materialize.h"
#include "re_guest_map_transaction.h"
#include "re_guest_permission_finalize.h"

#include <string.h>

#define RE_GUEST_ELF32_ADDRESS_LIMIT 0x100000000ULL
#define RE_GUEST_ELF32_TEMP_PERMISSIONS (RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE)

static void rollback_mappings(REGUESTMEMORY *memory,
    const REGUESTMEMMAPREQUEST *maps, size_t count,
    REGUESTELF32LOADStatus *status) {
    while (count > 0u) {
        --count;
        if (re_guest_memory_unmap(memory, maps[count].guest_base) !=
            RE_GUEST_MEM_OK) {
            *status = RE_GUEST_ELF32_LOAD_E_ROLLBACK;
        }
    }
}

REGUESTELF32LOADStatus re_guest_elf32_load_image(
    REGUESTMEMORY *memory,
    const REGUESTELF32LOADREQUEST *request,
    REGUESTELF32LOADRESULT *result,
    size_t *failed_segment,
    size_t *failed_relocation) {
    REGUESTELF32IMAGE image;
    REGUESTELF32DYNAMIC dynamic;
    REGUESTMEMMAPREQUEST maps[RE_GUEST_ELF32_MAX_LOAD_SEGMENTS];
    REGUESTELFSEGMENT segments[RE_GUEST_ELF32_MAX_LOAD_SEGMENTS];
    REGUESTMEMPERMISSIONREQUEST permissions[RE_GUEST_ELF32_MAX_LOAD_SEGMENTS];
    REGUESTRELOCATION relocation_batch[RE_GUEST_ELF32_RELOCATION_BATCH];
    REGUESTELF32LOADRESULT pending_result;
    size_t load_count = 0u, buffer_index = 0u;
    size_t failed = 0u;
    REGUESTELF32LOADStatus status = RE_GUEST_ELF32_LOAD_OK;

    if (failed_segment) *failed_segment = 0u;
    if (failed_relocation) *failed_relocation = 0u;
    if (result) memset(result, 0, sizeof(*result));
    memset(&pending_result, 0, sizeof(pending_result));
    memset(&dynamic, 0, sizeof(dynamic));

    if (!memory || !request || !result || !request->elf_bytes ||
        (!request->buffers && request->buffer_count) ||
        (!request->relocations && request->relocation_count) ||
        request->buffer_count > RE_GUEST_ELF32_MAX_LOAD_SEGMENTS ||
        request->relocation_count > RE_GUEST_ELF32_MAX_RELOCATIONS) {
        return RE_GUEST_ELF32_LOAD_E_ARGUMENT;
    }

    if (re_guest_elf32_parse_image(request->elf_bytes, request->elf_size,
            &image, &failed) != RE_GUEST_ELF32_OK) {
        if (failed_segment) *failed_segment = failed;
        return RE_GUEST_ELF32_LOAD_E_IMAGE;
    }
    if (image.load_overlap_count != 0u)
        return RE_GUEST_ELF32_LOAD_E_SEGMENT;

    if (image.type == RE_GUEST_ELF32_ET_EXEC && request->load_bias != 0u)
        return RE_GUEST_ELF32_LOAD_E_BIAS;

    for (size_t i = 0; i < image.program_header_count; ++i) {
        const REGUESTELF32PROGRAMHEADER *p = &image.program_headers[i];
        uint64_t guest_base, guest_end;
        uint32_t final_flags;

        if (p->type != RE_GUEST_ELF32_PT_LOAD || !p->memory_size)
            continue;
        if (load_count >= RE_GUEST_ELF32_MAX_LOAD_SEGMENTS ||
            buffer_index >= request->buffer_count)
            return RE_GUEST_ELF32_LOAD_E_BUFFER;
        if (p->alignment > 1u && request->load_bias % p->alignment != 0u)
            return RE_GUEST_ELF32_LOAD_E_BIAS;

        guest_base = (uint64_t)p->virtual_address + request->load_bias;
        guest_end = guest_base + p->memory_size;
        if (guest_base > 0xffffffffULL ||
            guest_end > RE_GUEST_ELF32_ADDRESS_LIMIT)
            return RE_GUEST_ELF32_LOAD_E_BIAS;

        if (!request->buffers[buffer_index].host_buffer ||
            request->buffers[buffer_index].capacity < p->memory_size)
            return RE_GUEST_ELF32_LOAD_E_BUFFER;

        final_flags = p->flags;
        if (final_flags & ~(uint32_t)(RE_GUEST_ELF32_PF_R |
                RE_GUEST_ELF32_PF_W | RE_GUEST_ELF32_PF_X))
            return RE_GUEST_ELF32_LOAD_E_FLAGS;
        if (!(final_flags & (RE_GUEST_ELF32_PF_R |
                RE_GUEST_ELF32_PF_W | RE_GUEST_ELF32_PF_X)) ||
            ((final_flags & RE_GUEST_ELF32_PF_W) &&
             (final_flags & RE_GUEST_ELF32_PF_X)))
            return RE_GUEST_ELF32_LOAD_E_FLAGS;

        maps[load_count].guest_base = (uint32_t)guest_base;
        maps[load_count].size = p->memory_size;
        maps[load_count].host_buffer = request->buffers[buffer_index].host_buffer;
        maps[load_count].permissions = RE_GUEST_ELF32_TEMP_PERMISSIONS;
        maps[load_count].memory_type = RE_GUEST_MEM_TYPE_NORMAL;

        segments[load_count].guest_address = (uint32_t)guest_base;
        segments[load_count].file_offset = p->offset;
        segments[load_count].file_size = p->file_size;
        segments[load_count].memory_size = p->memory_size;

        permissions[load_count].guest_base = (uint32_t)guest_base;
        permissions[load_count].size = p->memory_size;
        permissions[load_count].final_permissions =
            ((final_flags & RE_GUEST_ELF32_PF_R) ? RE_GUEST_MEM_READ : 0u) |
            ((final_flags & RE_GUEST_ELF32_PF_W) ? RE_GUEST_MEM_WRITE : 0u) |
            ((final_flags & RE_GUEST_ELF32_PF_X) ? RE_GUEST_MEM_EXEC : 0u);

        ++load_count;
        ++buffer_index;
    }
    if (!load_count || buffer_index != request->buffer_count)
        return RE_GUEST_ELF32_LOAD_E_BUFFER;

    if (image.entry != 0u) {
        uint64_t entry = (uint64_t)image.entry + request->load_bias;
        if (!image.entry_in_executable_load || entry > 0xffffffffULL)
            return RE_GUEST_ELF32_LOAD_E_ENTRY;
        pending_result.entry_point = (uint32_t)entry;
        pending_result.entry_valid = 1u;
        pending_result.thumb_entry = (uint8_t)(entry & 1u);
    } else if (image.type == RE_GUEST_ELF32_ET_EXEC) {
        return RE_GUEST_ELF32_LOAD_E_ENTRY;
    }

    if (re_guest_elf32_parse_dynamic(request->elf_bytes, request->elf_size,
            &image, &dynamic, &failed) != RE_GUEST_ELF32_DYNAMIC_OK) {
        if (failed_segment) *failed_segment = failed;
        return RE_GUEST_ELF32_LOAD_E_DYNAMIC;
    }

    {
        REGUESTMEMMAPTXStatus map_status = re_guest_memory_map_transaction(
            memory, maps, load_count, &failed, NULL);
        if (map_status != RE_GUEST_MAP_TX_OK) {
            if (failed_segment) *failed_segment = failed;
            return RE_GUEST_ELF32_LOAD_E_MAP;
        }
    }

    if (re_guest_elf_materialize(memory, request->elf_bytes,
            request->elf_size, segments, load_count, &failed) !=
            RE_GUEST_ELF_MATERIALIZE_OK) {
        if (failed_segment) *failed_segment = failed;
        status = RE_GUEST_ELF32_LOAD_E_MATERIALIZE;
        goto fail;
    }

    for (size_t offset = 0; offset < request->relocation_count;
         offset += RE_GUEST_ELF32_RELOCATION_BATCH) {
        size_t batch_count = request->relocation_count - offset;
        if (batch_count > RE_GUEST_ELF32_RELOCATION_BATCH)
            batch_count = RE_GUEST_ELF32_RELOCATION_BATCH;
        for (size_t i = 0; i < batch_count; ++i) {
            relocation_batch[i] = request->relocations[offset + i];
            relocation_batch[i].load_bias = request->load_bias;
        }
        if (re_guest_relocations_apply(memory, relocation_batch,
                batch_count, &failed) != RE_GUEST_RELOC_APPLY_OK) {
            if (failed_relocation) *failed_relocation = offset + failed;
            status = RE_GUEST_ELF32_LOAD_E_RELOCATION;
            goto fail;
        }
    }

    if (re_guest_memory_finalize_permissions(memory, permissions,
            load_count, &failed) != RE_GUEST_PERMISSION_FINALIZE_OK) {
        if (failed_segment) *failed_segment = failed;
        status = RE_GUEST_ELF32_LOAD_E_PERMISSION;
        goto fail;
    }

    pending_result.load_count = (uint8_t)load_count;
    pending_result.dynamic = dynamic;
    *result = pending_result;
    return RE_GUEST_ELF32_LOAD_OK;

fail:
    rollback_mappings(memory, maps, load_count, &status);
    return status;
}
