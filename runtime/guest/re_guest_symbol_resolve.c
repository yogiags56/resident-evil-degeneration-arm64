#include "re_guest_symbol_resolve.h"
#include <string.h>

#define RE_GUEST_SYMBOL_MAX_BATCH 4096u
#define RE_GUEST_SYMBOL_MAX_PROVIDERS 65536u

static int same(const char *a, const char *b) {
    if (!a || !b) return a == b;
    return strcmp(a, b) == 0;
}

static int externally_visible(const REGUESTSYMBOL *s) {
    return s->visibility == RE_GUEST_STV_DEFAULT ||
           s->visibility == RE_GUEST_STV_PROTECTED;
}

REGUESTSYMBOLStatus re_guest_symbol_resolve(
    const REGUESTSYMBOLREQUEST *request,
    const REGUESTSYMBOL *providers,
    size_t provider_count,
    REGUESTSYMBOLRESULT *result) {
    size_t strong_index = 0, weak_index = 0;
    size_t strong_count = 0;
    int have_weak = 0, name_seen = 0, version_seen = 0;

    if (!request || !request->name || !request->name[0] ||
        (!providers && provider_count) || !result)
        return RE_GUEST_SYMBOL_E_ARGUMENT;
    if (provider_count > RE_GUEST_SYMBOL_MAX_PROVIDERS)
        return RE_GUEST_SYMBOL_E_CAPACITY;

    for (size_t i = 0; i < provider_count; ++i) {
        const REGUESTSYMBOL *s = &providers[i];
        int version_match;

        if (!s->name || !same(s->name, request->name))
            continue;
        if (!s->defined || s->binding == RE_GUEST_STB_LOCAL ||
            !externally_visible(s))
            continue;
        if (s->binding != RE_GUEST_STB_GLOBAL &&
            s->binding != RE_GUEST_STB_WEAK)
            continue;
        name_seen = 1;

        version_match = same(s->version_name, request->version_name);
        if (!version_match && request->allow_unversioned_fallback &&
            !s->version_name)
            version_match = 1;
        if (!version_match)
            continue;
        if (request->version_library &&
            !same(s->version_library, request->version_library))
            continue;
        version_seen = 1;

        if (s->binding == RE_GUEST_STB_GLOBAL) {
            strong_index = i;
            ++strong_count;
        } else if (!have_weak) {
            weak_index = i;
            have_weak = 1;
        }
    }

    if (strong_count > 1)
        return RE_GUEST_SYMBOL_E_AMBIGUOUS;
    if (strong_count == 1) {
        result->value = providers[strong_index].value;
        result->provider_index = strong_index;
        result->weak = 0;
        return RE_GUEST_SYMBOL_OK;
    }
    if (have_weak) {
        result->value = providers[weak_index].value;
        result->provider_index = weak_index;
        result->weak = 1;
        return RE_GUEST_SYMBOL_OK;
    }
    if (name_seen && !version_seen)
        return RE_GUEST_SYMBOL_E_VERSION;
    return RE_GUEST_SYMBOL_E_NOT_FOUND;
}

REGUESTSYMBOLStatus re_guest_symbols_resolve(
    const REGUESTSYMBOLREQUEST *requests,
    size_t request_count,
    const REGUESTSYMBOL *providers,
    size_t provider_count,
    REGUESTSYMBOLRESULT *results,
    size_t *failed_index) {
    REGUESTSYMBOLRESULT pending[RE_GUEST_SYMBOL_MAX_BATCH];

    if (failed_index) *failed_index = request_count;
    if ((!requests && request_count) || (!providers && provider_count) ||
        (!results && request_count))
        return RE_GUEST_SYMBOL_E_ARGUMENT;
    if (request_count > RE_GUEST_SYMBOL_MAX_BATCH ||
        provider_count > RE_GUEST_SYMBOL_MAX_PROVIDERS)
        return RE_GUEST_SYMBOL_E_CAPACITY;

    for (size_t i = 0; i < request_count; ++i) {
        REGUESTSYMBOLStatus status = re_guest_symbol_resolve(
            &requests[i], providers, provider_count, &pending[i]);
        if (status != RE_GUEST_SYMBOL_OK) {
            if (failed_index) *failed_index = i;
            return status;
        }
    }
    if (request_count)
        memcpy(results, pending, request_count * sizeof(*results));
    return RE_GUEST_SYMBOL_OK;
}
