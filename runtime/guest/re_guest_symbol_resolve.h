#ifndef RE_GUEST_SYMBOL_RESOLVE_H
#define RE_GUEST_SYMBOL_RESOLVE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    RE_GUEST_STB_LOCAL = 0,
    RE_GUEST_STB_GLOBAL = 1,
    RE_GUEST_STB_WEAK = 2,
    RE_GUEST_STV_DEFAULT = 0,
    RE_GUEST_STV_INTERNAL = 1,
    RE_GUEST_STV_HIDDEN = 2,
    RE_GUEST_STV_PROTECTED = 3
};

typedef struct {
    const char *name;
    uint32_t value;
    uint8_t binding;
    uint8_t visibility;
    uint8_t defined;
    const char *version_name; /* NULL means unversioned. */
    const char *version_library; /* Optional required provider SONAME. */
} REGUESTSYMBOL;

typedef struct {
    const char *name;
    const char *version_name;
    const char *version_library;
    uint8_t allow_unversioned_fallback;
} REGUESTSYMBOLREQUEST;

typedef struct {
    uint32_t value;
    size_t provider_index;
    uint8_t weak;
} REGUESTSYMBOLRESULT;

typedef enum {
    RE_GUEST_SYMBOL_OK = 0,
    RE_GUEST_SYMBOL_E_ARGUMENT = -500,
    RE_GUEST_SYMBOL_E_NOT_FOUND = -501,
    RE_GUEST_SYMBOL_E_AMBIGUOUS = -502,
    RE_GUEST_SYMBOL_E_VERSION = -503,
    RE_GUEST_SYMBOL_E_LOCAL = -504,
    RE_GUEST_SYMBOL_E_CAPACITY = -505
} REGUESTSYMBOLStatus;

/*
 * Resolve one undefined symbol against an ordered, caller-verified provider
 * set. Strong global definitions win over weak definitions. Multiple strong
 * definitions are ambiguous. Provider order breaks ties between weak defs.
 *
 * This is a bounded symbol-table resolver, not a dynamic linker: it does not
 * load dependencies, parse ELF, interpret GNU version indices, implement
 * copy relocations, or search host process symbols.
 */
REGUESTSYMBOLStatus re_guest_symbol_resolve(
    const REGUESTSYMBOLREQUEST *request,
    const REGUESTSYMBOL *providers,
    size_t provider_count,
    REGUESTSYMBOLRESULT *result);

/* Resolve every request atomically; output results remain unchanged on error. */
REGUESTSYMBOLStatus re_guest_symbols_resolve(
    const REGUESTSYMBOLREQUEST *requests,
    size_t request_count,
    const REGUESTSYMBOL *providers,
    size_t provider_count,
    REGUESTSYMBOLRESULT *results,
    size_t *failed_index);

#ifdef __cplusplus
}
#endif
#endif
