#include "re_guest_module_manifest.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    const char good[] =
        "# synthetic inventory\r\n"
        "RE-GUEST-MODULES 1\r\n"
        "root game.so\r\n"
        "module libc.so system/libc.so 16777216\r\n"
        "module game.so payload/game.elf 67108864\r\n";
    REGUESTMODULEMANIFEST manifest, before;
    size_t line = 99u;
    memset(&manifest, 0x5a, sizeof(manifest));
    assert(re_guest_module_manifest_parse(good, sizeof(good)-1u,
        &manifest, &line) == RE_GUEST_MANIFEST_OK);
    assert(manifest.version == 1u && manifest.module_count == 2u);
    assert(manifest.root_module_index == 1u);
    assert(!strcmp(manifest.modules[0].soname, "libc.so"));
    assert(!strcmp(manifest.modules[0].asset_path, "system/libc.so"));
    assert(manifest.modules[0].max_bytes == 16777216u);
    assert(!strcmp(manifest.modules[1].soname, "game.so"));

    before = manifest;
    const char duplicate[] =
        "RE-GUEST-MODULES 1\nroot game.so\n"
        "module game.so a/game.elf 4096\n"
        "module game.so b/game.elf 4096\n";
    assert(re_guest_module_manifest_parse(duplicate, sizeof(duplicate)-1u,
        &manifest, &line) == RE_GUEST_MANIFEST_E_DUPLICATE);
    assert(memcmp(&manifest, &before, sizeof(manifest)) == 0 && line == 4u);

    const char traversal[] =
        "RE-GUEST-MODULES 1\nroot game.so\n"
        "module game.so ../game.elf 4096\n";
    assert(re_guest_module_manifest_parse(traversal, sizeof(traversal)-1u,
        &manifest, &line) == RE_GUEST_MANIFEST_E_PATH);
    assert(memcmp(&manifest, &before, sizeof(manifest)) == 0);

    const char missing[] =
        "RE-GUEST-MODULES 1\nroot game.so\n"
        "module libc.so system/libc.so 4096\n";
    const char trailing_separator[] =
        "RE-GUEST-MODULES 1\nroot game.so\n"
        "module game.so payload/game.elf/ 4096\n";
    assert(re_guest_module_manifest_parse(trailing_separator,
        sizeof(trailing_separator)-1u, &manifest, &line) ==
        RE_GUEST_MANIFEST_E_PATH);
    assert(memcmp(&manifest, &before, sizeof(manifest)) == 0);

    assert(re_guest_module_manifest_parse(missing, sizeof(missing)-1u,
        &manifest, &line) == RE_GUEST_MANIFEST_E_ROOT);
    assert(memcmp(&manifest, &before, sizeof(manifest)) == 0);

    const char overflow[] =
        "RE-GUEST-MODULES 1\nroot game.so\n"
        "module game.so game.elf 999999999999999999999\n";
    assert(re_guest_module_manifest_parse(overflow, sizeof(overflow)-1u,
        &manifest, &line) == RE_GUEST_MANIFEST_E_LIMIT);
    assert(memcmp(&manifest, &before, sizeof(manifest)) == 0);

    const char version[] = "RE-GUEST-MODULES 2\n";
    assert(re_guest_module_manifest_parse(version, sizeof(version)-1u,
        &manifest, &line) == RE_GUEST_MANIFEST_E_VERSION);
    assert(memcmp(&manifest, &before, sizeof(manifest)) == 0);

    assert(re_guest_module_manifest_parse(good,
        RE_GUEST_MANIFEST_MAX_BYTES + 1u, &manifest, &line) ==
        RE_GUEST_MANIFEST_E_SIZE);
    assert(memcmp(&manifest, &before, sizeof(manifest)) == 0);
    puts("Guest module manifest tests: PASS");
    return 0;
}
