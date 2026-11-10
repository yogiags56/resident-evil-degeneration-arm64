# Stage 123J — Original ELF32 undefined-symbol family manifest

## Input and counts

This is a read-only inventory of the exact original `lib/armeabi/libjirobiodg.so` from the revalidated APK. The ELF has six `DT_NEEDED` dependencies:

- `libc.so`
- `libm.so`
- `liblog.so`
- `libstdc++.so`
- `libz.so`
- `libdl.so`

The dynamic relocation table contains 137 symbol-based relocation references across **136 unique non-empty undefined symbol names**. The `free` symbol is referenced by two relocation records. The other 3,920 `R_ARM_RELATIVE` records have no external symbol name and are not included in this count. There are also 146 named undefined entries in the dynamic symbol table; the 136-name figure is specifically the unique set referenced by dynamic relocation records.

## Candidate families

These are name-based triage groups, not verified per-symbol `DT_NEEDED` ownership. Android's global linker scope and dependency closure can satisfy symbols from more than one DSO; no exact provider assignment is made here.

| Candidate family | Unique names | Relocation references |
|---|---:|---:|
| libc / POSIX / Bionic candidate | 101 | 102 |
| ARM EABI compiler-runtime candidate | 21 | 21 |
| libdl candidate | 4 | 4 |
| libz candidate | 4 | 4 |
| libstdc++ / C++ ABI candidate | 4 | 4 |
| liblog candidate | 2 | 2 |
| libm candidate by direct symbol name | 0 | 0 |
| **Total** | **136** | **137** |

`libm.so` is still a declared dependency even though this simple name classifier found no direct unresolved math names. The 21 `__aeabi_*` names are kept in a separate compiler-runtime family; they are not assigned to `libm.so` without export evidence. `__errno` and `__sF` are Bionic-internal candidates and need API-level export verification rather than blind binding.

## libdl candidate symbols

- `dlopen`
- `dlsym`
- `dlclose`
- `dlerror`

These are directly relevant to the three-handle GLES resolver traced in Stage 123H. The Stage 123I bridge now supplies opaque guest handles and guest-address-only symbol resolution, but its callbacks are not yet wired to the ELF32 relocation/provider catalog.

## libz candidate symbols

- `inflate`
- `inflateEnd`
- `inflateInit2_`
- `inflateInit_`

These must be bridged to a verified zlib implementation with the correct 32-bit guest structure layouts. Do not confuse these functions with the separate DTRZ archive/LZMA path used by the game assets.

## liblog candidate symbols

- `__android_log_print`
- `__android_log_write`

Host logging handlers can be implemented through Android's ARM64 NDK log API, but must preserve the guest variadic/format ABI for `__android_log_print`.

## libstdc++ / C++ ABI candidate symbols

- `_ZdaPv`
- `_ZdlPv`
- `_Znaj`
- `_Znwj`

These are array/scalar allocation and deallocation operators. Their pairing and allocator ownership must be preserved; do not bind them to unrelated host allocators without verifying the guest heap contract.

## ARM EABI compiler-runtime candidate symbols

- `__aeabi_d2f`
- `__aeabi_d2iz`
- `__aeabi_dadd`
- `__aeabi_dcmpeq`
- `__aeabi_dcmplt`
- `__aeabi_ddiv`
- `__aeabi_dmul`
- `__aeabi_dsub`
- `__aeabi_f2iz`
- `__aeabi_fadd`
- `__aeabi_fmul`
- `__aeabi_fsub`
- `__aeabi_i2d`
- `__aeabi_i2f`
- `__aeabi_idiv`
- `__aeabi_idivmod`
- `__aeabi_ldivmod`
- `__aeabi_lmul`
- `__aeabi_ui2d`
- `__aeabi_uidiv`
- `__aeabi_uidivmod`

These require exact ARM EABI calling convention and return-register behavior. They may be supplied by compiler-runtime support or a system DSO, but the provider is not inferred from the symbol prefix alone.

## libc / POSIX / Bionic candidate symbols

`__errno`, `__sF`, `accept`, `access`, `atexit`, `atoi`, `bind`, `bsd_signal`, `chmod`, `clock_getres`, `clock_gettime`, `close`, `closedir`, `connect`, `exit`, `fclose`, `fcntl`, `fgets`, `fopen`, `fprintf`, `fread`, `free`, `fseek`, `ftell`, `ftruncate`, `fwrite`, `getenv`, `gethostbyname`, `gethostname`, `getpeername`, `getsockname`, `getsockopt`, `gettimeofday`, `gmtime_r`, `inet_aton`, `inet_ntoa`, `listen`, `localtime_r`, `lseek`, `malloc`, `memcpy`, `memmove`, `memset`, `mkdir`, `mktime`, `mprotect`, `open`, `opendir`, `printf`, `pthread_attr_init`, `pthread_cond_init`, `pthread_cond_signal`, `pthread_cond_wait`, `pthread_create`, `pthread_equal`, `pthread_getspecific`, `pthread_join`, `pthread_key_create`, `pthread_key_delete`, `pthread_mutex_destroy`, `pthread_mutex_init`, `pthread_mutex_lock`, `pthread_mutex_unlock`, `pthread_self`, `pthread_setspecific`, `read`, `readdir`, `realloc`, `recv`, `recvfrom`, `remove`, `rename`, `rmdir`, `select`, `send`, `sendto`, `setsockopt`, `snprintf`, `socket`, `sprintf`, `sscanf`, `stat`, `statfs`, `strcasecmp`, `strchr`, `strcmp`, `strcpy`, `strlen`, `strncasecmp`, `strncat`, `strncmp`, `strncpy`, `strstr`, `strtok`, `time`, `tmpfile`, `tzset`, `uname`, `unlink`, `valloc`, `write`.

This group includes filesystem, stdio, memory, pthread, networking, time, and Bionic-specific interfaces. The 32-bit guest needs ABI-aware bridges for pointer-bearing structures such as `stat`, `statfs`, `addrinfo/hostent`, `sockaddr`, `pthread_attr_t`, `pthread_mutex_t`, `pthread_cond_t`, and `FILE`; forwarding their guest addresses directly to 64-bit host libc is unsafe.

## Reproducibility

`tools/xe3u_elf32_provider_audit.py` now emits `elf.undefined_symbol_families` with unique-name and relocation-reference counts. The labels are explicitly marked as candidate families; exact DSO ownership and ABI compatibility remain separate validation tasks. Synthetic unit tests cover classification and relocation-reference counting; host CI passed in run `42885003645` (312 Python tests plus native regressions). Android ARM64 build passed in run `42884983999`.

## Next

1. Build virtual provider modules for `libdl.so`, `liblog.so`, and `libz.so` first; the Stage 123I bridge provides the dynamic-loader seam.
2. Implement the ARM EABI helper subset with differential tests against a known ARM EABI implementation.
3. Audit and bridge the 101 libc/POSIX/Bionic candidates in groups, prioritizing the symbols actually reached during JNI_OnLoad, archive access, rendering, and startup.
4. Verify the complete six-library dependency closure and every relocation before loading the ELF32 provider.
