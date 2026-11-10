# Stage 123K — Bind libdl handlers from ELF32 dynamic-symbol indices

## Purpose

Stage 123I provides the four guest libdl handlers and Stage 123J inventories the undefined dynamic symbols. This stage removes the remaining manual symbol-ID handoff: the bridge now identifies dlopen, dlsym, dlclose, and dlerror directly from the already-decoded REGUESTELF32LINKSYMBOL table.

## Contract

- Accepts the bounded dynamic-symbol array emitted by re_guest_elf32_parse_link_metadata().
- Selects only undefined (SHN_UNDEF) GLOBAL/WEAK symbols with DEFAULT/PROTECTED visibility.
- Accepts STT_FUNC and STT_NOTYPE function imports; rejects other symbol types.
- Requires exactly one candidate for each of the four names. Missing or duplicate candidates fail closed.
- Uses the exact dynamic-symbol array index as the REGUESTIMPORTTable.symbol_index.
- Does not mutate the import table until all four names have been found; the underlying bind operation remains transactional.
- Does not infer provider ownership for other imports and does not activate any unverified handler.

## Tests

The bridge regression now covers automatic extraction of the four symbol IDs from a synthetic ELF32 dynamic-symbol table, correct mapping of IDs 1–4 to mapped guest trampolines, duplicate dlsym rejection with an unchanged import table, and missing dlerror rejection with an unchanged import table. Existing synthetic runtime execution tests remain in place.

CI status is pending until the new commit's host and Android workflows complete.

## Scope boundary

This is the exact-symbol-index binding seam only. The callback implementation still needs a verified guest provider registry and guest-callable GLES/EGL trampolines. No host 64-bit function pointer is exposed to guest ARM32, and no original game ELF has been loaded or executed.
