#include "re_guest_symbol_resolve.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    REGUESTSYMBOL providers[] = {
        {"foo", 0x1000, RE_GUEST_STB_WEAK, RE_GUEST_STV_DEFAULT, 1, NULL, NULL},
        {"foo", 0x2000, RE_GUEST_STB_GLOBAL, RE_GUEST_STV_DEFAULT, 1, NULL, NULL},
        {"foo", 0x3000, RE_GUEST_STB_GLOBAL, RE_GUEST_STV_HIDDEN, 1, NULL, NULL},
        {"bar", 0x4000, RE_GUEST_STB_GLOBAL, RE_GUEST_STV_DEFAULT, 1, "VER_2", "libbar.so"},
        {"bar", 0x5000, RE_GUEST_STB_GLOBAL, RE_GUEST_STV_DEFAULT, 1, "VER_1", "libbar.so"},
        {"local", 0x6000, RE_GUEST_STB_LOCAL, RE_GUEST_STV_DEFAULT, 1, NULL, NULL},
        {"weakonly", 0x7000, RE_GUEST_STB_WEAK, RE_GUEST_STV_DEFAULT, 1, NULL, NULL}
    };
    REGUESTSYMBOLRESULT result = {0xDEAD, 99, 9};
    REGUESTSYMBOLREQUEST req = {"foo", NULL, NULL, 0};

    assert(re_guest_symbol_resolve(&req, providers, 7, &result)
        == RE_GUEST_SYMBOL_OK);
    assert(result.value == 0x2000 && result.provider_index == 1 && !result.weak);

    req.name = "weakonly";
    assert(re_guest_symbol_resolve(&req, providers, 7, &result)
        == RE_GUEST_SYMBOL_OK);
    assert(result.value == 0x7000 && result.weak);

    req.name = "bar"; req.version_name = "VER_2";
    req.version_library = "libbar.so";
    assert(re_guest_symbol_resolve(&req, providers, 7, &result)
        == RE_GUEST_SYMBOL_OK && result.value == 0x4000);

    req.version_name = "VER_3";
    assert(re_guest_symbol_resolve(&req, providers, 7, &result)
        == RE_GUEST_SYMBOL_E_VERSION);

    req.name = "local"; req.version_name = NULL; req.version_library = NULL;
    assert(re_guest_symbol_resolve(&req, providers, 7, &result)
        == RE_GUEST_SYMBOL_E_NOT_FOUND);

    req.name = "missing";
    assert(re_guest_symbol_resolve(&req, providers, 7, &result)
        == RE_GUEST_SYMBOL_E_NOT_FOUND);

    /* Multiple strong definitions are not silently selected. */
    providers[2].visibility = RE_GUEST_STV_DEFAULT;
    req.name = "foo";
    assert(re_guest_symbol_resolve(&req, providers, 7, &result)
        == RE_GUEST_SYMBOL_E_AMBIGUOUS);
    providers[2].visibility = RE_GUEST_STV_HIDDEN;

    /* Batch failure leaves caller output unchanged. */
    {
        REGUESTSYMBOLREQUEST batch[] = {
            {"foo", NULL, NULL, 0},
            {"missing", NULL, NULL, 0}
        };
        REGUESTSYMBOLRESULT outputs[] = {{0xAAAA, 11, 1}, {0xBBBB, 22, 1}};
        size_t failed = 99;
        assert(re_guest_symbols_resolve(batch, 2, providers, 7, outputs,
            &failed) == RE_GUEST_SYMBOL_E_NOT_FOUND);
        assert(failed == 1);
        assert(outputs[0].value == 0xAAAA && outputs[1].value == 0xBBBB);
    }

    req.name = NULL;
    assert(re_guest_symbol_resolve(&req, providers, 7, &result)
        == RE_GUEST_SYMBOL_E_ARGUMENT);
    assert(re_guest_symbols_resolve(NULL, 0, NULL, 0, NULL, NULL)
        == RE_GUEST_SYMBOL_OK);
    puts("Guest symbol resolution tests: PASS");
    return 0;
}
