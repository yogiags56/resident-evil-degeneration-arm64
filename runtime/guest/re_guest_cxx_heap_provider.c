#include "re_guest_cxx_heap_provider.h"

#include <string.h>

#include "re_guest_memory_bus.h"

static int heap_mapping_valid(const REGUESTCXXHeapProvider *provider,
                              const REGUESTMEMBUS *bus) {
    void *host = NULL;
    if (!provider || !bus || !bus->memory) return 0;
    return re_guest_memory_resolve(bus->memory, provider->heap_base,
        provider->heap_size, RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE,
        &host) == RE_GUEST_MEM_OK;
}

static int find_free_record(REGUESTCXXHeapProvider *provider, size_t *index) {
    for (size_t i = 0; i < RE_GUEST_CXX_HEAP_MAX_ALLOCS; ++i) {
        if (!provider->allocations[i].active) {
            *index = i;
            return 1;
        }
    }
    return 0;
}

static uint32_t guest_heap_allocate(REGUESTCXXHeapProvider *provider,
                                    const REGUESTMEMBUS *bus,
                                    uint32_t requested_size) {
    uint64_t aligned_size;
    uint64_t heap_end;
    uint64_t candidate;
    size_t free_record = 0u;

    if (!heap_mapping_valid(provider, bus) ||
        !find_free_record(provider, &free_record))
        return 0u;
    if (!requested_size) requested_size = 1u;
    aligned_size = ((uint64_t)requested_size +
                    (RE_GUEST_CXX_HEAP_ALIGNMENT - 1u)) &
                   ~((uint64_t)RE_GUEST_CXX_HEAP_ALIGNMENT - 1u);
    heap_end = (uint64_t)provider->heap_base + provider->heap_size;
    if (aligned_size > UINT32_MAX || heap_end > (uint64_t)UINT32_MAX + 1u)
        return 0u;

    candidate = provider->heap_base;
    for (;;) {
        const REGUESTCXXHeapAllocation *nearest = NULL;
        for (size_t i = 0; i < RE_GUEST_CXX_HEAP_MAX_ALLOCS; ++i) {
            const REGUESTCXXHeapAllocation *block = &provider->allocations[i];
            if (!block->active || block->guest_address < candidate) continue;
            if (!nearest || block->guest_address < nearest->guest_address)
                nearest = block;
        }
        if (!nearest) {
            if (candidate <= UINT32_MAX &&
                candidate + aligned_size <= heap_end)
                break;
            return 0u;
        }
        if ((uint64_t)nearest->guest_address - candidate >= aligned_size)
            break;
        candidate = (uint64_t)nearest->guest_address + nearest->size;
        if (candidate > heap_end) return 0u;
    }

    if (candidate > UINT32_MAX || candidate + aligned_size > heap_end)
        return 0u;
    provider->allocations[free_record].guest_address = (uint32_t)candidate;
    provider->allocations[free_record].size = (uint32_t)aligned_size;
    provider->allocations[free_record].active = 1u;
    return (uint32_t)candidate;
}

static void guest_heap_free(REGUESTCXXHeapProvider *provider,
                            uint32_t guest_address) {
    if (!provider || !guest_address) return;
    for (size_t i = 0; i < RE_GUEST_CXX_HEAP_MAX_ALLOCS; ++i) {
        REGUESTCXXHeapAllocation *block = &provider->allocations[i];
        if (block->active && block->guest_address == guest_address) {
            memset(block, 0, sizeof(*block));
            return;
        }
    }
}

static REGUESTIMPORTStatus new_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTCXXHeapProvider *provider = (REGUESTCXXHeapProvider *)user;
    uint32_t address;
    if (!frame || !bus || !provider || !result) return RE_IMPORT_E_HANDLER;
    if (provider->busy) {
        result->r0 = 0u;
        result->r1 = 0u;
        return RE_IMPORT_OK;
    }
    provider->busy = 1u;
    address = guest_heap_allocate(provider, bus, frame->r[0]);
    provider->busy = 0u;
    result->r0 = address;
    result->r1 = 0u;
    return RE_IMPORT_OK;
}

static REGUESTIMPORTStatus delete_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTCXXHeapProvider *provider = (REGUESTCXXHeapProvider *)user;
    if (!frame || !bus || !provider || !result) return RE_IMPORT_E_HANDLER;
    if (!provider->busy) {
        provider->busy = 1u;
        if (heap_mapping_valid(provider, bus))
            guest_heap_free(provider, frame->r[0]);
        provider->busy = 0u;
    }
    result->r0 = 0u;
    result->r1 = 0u;
    return RE_IMPORT_OK;
}

static int find_undefined_symbol(
    const REGUESTELF32LINKSYMBOL *symbols, size_t count,
    const char *name, size_t *found) {
    size_t match = count;
    for (size_t i = 0; i < count; ++i) {
        const REGUESTELF32LINKSYMBOL *symbol = &symbols[i];
        if (!memchr(symbol->name, '\0', sizeof(symbol->name))) return 0;
        if (strcmp(symbol->name, name) != 0 || symbol->section_index != 0u ||
            (symbol->binding != RE_GUEST_STB_GLOBAL &&
             symbol->binding != RE_GUEST_STB_WEAK) ||
            (symbol->visibility != RE_GUEST_STV_DEFAULT &&
             symbol->visibility != RE_GUEST_STV_PROTECTED) ||
            (symbol->type != 0u && symbol->type != 2u))
            continue;
        if (match != count) return 0;
        match = i;
    }
    if (match == count) return 0;
    *found = match;
    return 1;
}

REGUESTCXXHeapStatus re_guest_cxx_heap_provider_init(
    REGUESTCXXHeapProvider *provider, uint32_t heap_base, uint32_t heap_size) {
    uint64_t heap_end = (uint64_t)heap_base + heap_size;
    if (!provider || !heap_base || !heap_size ||
        (heap_base & (RE_GUEST_CXX_HEAP_ALIGNMENT - 1u)) ||
        (heap_size & (RE_GUEST_CXX_HEAP_ALIGNMENT - 1u)) ||
        heap_end > (uint64_t)UINT32_MAX + 1u)
        return RE_GUEST_CXX_HEAP_E_ARGUMENT;
    memset(provider, 0, sizeof(*provider));
    provider->heap_base = heap_base;
    provider->heap_size = heap_size;
    return RE_GUEST_CXX_HEAP_OK;
}

REGUESTCXXHeapStatus re_guest_cxx_heap_provider_bind_elf_symbols(
    REGUESTCXXHeapProvider *provider,
    REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols,
    size_t symbol_count,
    REGUESTSYMBOL *exports,
    size_t export_capacity,
    size_t *export_count,
    size_t *failed_symbol_index) {
    static const char *const names[4] = {
        "_Znwj", "_Znaj", "_ZdlPv", "_ZdaPv"
    };
    REGUESTIMPORTMappedHandler handlers[4] = {
        new_handler, new_handler, delete_handler, delete_handler
    };
    REGUESTIMPORTTable backup;
    REGUESTSYMBOL pending[4];
    size_t indices[4] = {symbol_count, symbol_count, symbol_count, symbol_count};
    uint32_t pointers[4] = {0u, 0u, 0u, 0u};
    REGUESTIMPORTStatus status;

    if (export_count) *export_count = 0u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    if (!provider || !imports || !symbols || !exports || !export_count ||
        export_capacity < 4u)
        return RE_GUEST_CXX_HEAP_E_ARGUMENT;
    if (symbol_count > RE_GUEST_ELF32_LINK_MAX_SYMBOLS ||
        imports->count > RE_IMPORT_MAX_BINDINGS - 4u)
        return RE_GUEST_CXX_HEAP_E_CAPACITY;
    for (size_t i = 0; i < 4u; ++i) {
        if (!find_undefined_symbol(symbols, symbol_count, names[i], &indices[i])) {
            if (failed_symbol_index) *failed_symbol_index = indices[i];
            return RE_GUEST_CXX_HEAP_E_SYMBOL;
        }
        for (size_t j = 0; j < i; ++j) {
            if (indices[i] == indices[j]) return RE_GUEST_CXX_HEAP_E_SYMBOL;
        }
    }

    backup = *imports;
    for (size_t i = 0; i < 4u; ++i) {
        status = re_guest_import_bind_mapped(imports, (uint32_t)indices[i],
            handlers[i], provider, &pointers[i]);
        if (status != RE_IMPORT_OK) {
            *imports = backup;
            return RE_GUEST_CXX_HEAP_E_IMPORT;
        }
        pending[i] = (REGUESTSYMBOL){
            names[i], pointers[i], RE_GUEST_STB_GLOBAL,
            RE_GUEST_STV_DEFAULT, 1u, NULL, "libstdc++.so"
        };
    }
    memcpy(exports, pending, sizeof(pending));
    *export_count = 4u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    return RE_GUEST_CXX_HEAP_OK;
}

void re_guest_cxx_heap_provider_shutdown(REGUESTCXXHeapProvider *provider) {
    if (!provider) return;
    memset(provider->allocations, 0, sizeof(provider->allocations));
    provider->busy = 0u;
}
