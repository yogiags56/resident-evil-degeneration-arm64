#include "re_guest_eabi_provider.h"

#include <limits.h>
#include <math.h>
#include <string.h>

#include "s3e_aapcs32.h"

typedef enum {
    EABI_D2F, EABI_D2IZ, EABI_DADD, EABI_DCMPEQ, EABI_DCMPLT,
    EABI_DDIV, EABI_DMUL, EABI_DSUB, EABI_F2IZ, EABI_FADD,
    EABI_FMUL, EABI_FSUB, EABI_I2D, EABI_I2F, EABI_IDIV,
    EABI_IDIVMOD, EABI_LDIVMOD, EABI_LMUL, EABI_UI2D,
    EABI_UIDIV, EABI_UIDIVMOD
} EABIKind;

typedef struct { EABIKind kind; } EABIContext;

static const char *const k_names[RE_GUEST_EABI_SYMBOL_COUNT] = {
    "__aeabi_d2f", "__aeabi_d2iz", "__aeabi_dadd",
    "__aeabi_dcmpeq", "__aeabi_dcmplt", "__aeabi_ddiv",
    "__aeabi_dmul", "__aeabi_dsub", "__aeabi_f2iz",
    "__aeabi_fadd", "__aeabi_fmul", "__aeabi_fsub",
    "__aeabi_i2d", "__aeabi_i2f", "__aeabi_idiv",
    "__aeabi_idivmod", "__aeabi_ldivmod", "__aeabi_lmul",
    "__aeabi_ui2d", "__aeabi_uidiv", "__aeabi_uidivmod"
};

static const EABIContext k_contexts[RE_GUEST_EABI_SYMBOL_COUNT] = {
    {EABI_D2F}, {EABI_D2IZ}, {EABI_DADD}, {EABI_DCMPEQ},
    {EABI_DCMPLT}, {EABI_DDIV}, {EABI_DMUL}, {EABI_DSUB},
    {EABI_F2IZ}, {EABI_FADD}, {EABI_FMUL}, {EABI_FSUB},
    {EABI_I2D}, {EABI_I2F}, {EABI_IDIV}, {EABI_IDIVMOD},
    {EABI_LDIVMOD}, {EABI_LMUL}, {EABI_UI2D}, {EABI_UIDIV},
    {EABI_UIDIVMOD}
};

static int find_undefined_symbol(const REGUESTELF32LINKSYMBOL *symbols,
                                 size_t count, const char *name,
                                 size_t *found) {
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

static int get_u64(const S3EAAPCS32Frame *frame, uint32_t word_index,
                   uint64_t *value) {
    return s3e_aapcs32_arg_u64_even(frame, word_index, value) ==
           S3E_AAPCS32_OK;
}

static double bits_to_double(uint64_t bits) {
    double value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static uint64_t double_to_bits(double value) {
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static float bits_to_float(uint32_t bits) {
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static uint32_t float_to_bits(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

/* Avoid undefined C conversions for NaN and out-of-range inputs. */
static int32_t trunc_to_i32(double value) {
    if (isnan(value)) return INT32_MIN;
    if (value >= 2147483648.0) return INT32_MAX;
    if (value < -2147483648.0) return INT32_MIN;
    return (int32_t)value;
}

static void set_u64_result(REGUESTIMPORTResult *result, uint64_t value) {
    result->r0 = (uint32_t)value;
    result->r1 = (uint32_t)(value >> 32);
}

static REGUESTIMPORTStatus eabi_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    const EABIContext *context = (const EABIContext *)user;
    uint64_t a64 = 0u, b64 = 0u, out64;
    double da, db, dout;
    float fa, fb, fout;
    int32_t si, sj;
    uint32_t ui, uj;
    if (!frame || !context || !result) return RE_IMPORT_E_HANDLER;
    (void)bus;
    result->r0 = result->r1 = result->r2 = result->r3 = 0u;

    switch (context->kind) {
        case EABI_D2F:
            if (!get_u64(frame, 0u, &a64)) return RE_IMPORT_E_HANDLER;
            dout = bits_to_double(a64);
            result->r0 = float_to_bits((float)dout);
            break;
        case EABI_D2IZ:
            if (!get_u64(frame, 0u, &a64)) return RE_IMPORT_E_HANDLER;
            result->r0 = (uint32_t)trunc_to_i32(bits_to_double(a64));
            break;
        case EABI_DADD: case EABI_DSUB: case EABI_DMUL: case EABI_DDIV:
        case EABI_DCMPEQ: case EABI_DCMPLT:
            if (!get_u64(frame, 0u, &a64) ||
                !get_u64(frame, 2u, &b64)) return RE_IMPORT_E_HANDLER;
            da = bits_to_double(a64);
            db = bits_to_double(b64);
            switch (context->kind) {
                case EABI_DADD: dout = da + db; break;
                case EABI_DSUB: dout = da - db; break;
                case EABI_DMUL: dout = da * db; break;
                case EABI_DDIV: dout = da / db; break;
                case EABI_DCMPEQ: result->r0 = (da == db) ? 1u : 0u; return RE_IMPORT_OK;
                case EABI_DCMPLT: result->r0 = (da < db) ? 1u : 0u; return RE_IMPORT_OK;
                default: return RE_IMPORT_E_HANDLER;
            }
            set_u64_result(result, double_to_bits(dout));
            break;
        case EABI_F2IZ:
            result->r0 = (uint32_t)trunc_to_i32((double)bits_to_float(frame->r[0]));
            break;
        case EABI_FADD: case EABI_FSUB: case EABI_FMUL:
            fa = bits_to_float(frame->r[0]);
            fb = bits_to_float(frame->r[1]);
            if (context->kind == EABI_FADD) fout = fa + fb;
            else if (context->kind == EABI_FSUB) fout = fa - fb;
            else fout = fa * fb;
            result->r0 = float_to_bits(fout);
            break;
        case EABI_I2D:
            si = (int32_t)frame->r[0];
            set_u64_result(result, double_to_bits((double)si));
            break;
        case EABI_UI2D:
            set_u64_result(result, double_to_bits((double)frame->r[0]));
            break;
        case EABI_I2F:
            result->r0 = float_to_bits((float)(int32_t)frame->r[0]);
            break;
        case EABI_IDIV: case EABI_IDIVMOD:
            si = (int32_t)frame->r[0];
            sj = (int32_t)frame->r[1];
            if (sj == 0) {
                result->r0 = 0u; /* Default __aeabi_idiv0 policy: zero. */
                if (context->kind == EABI_IDIVMOD) result->r1 = (uint32_t)si;
            } else if (si == INT32_MIN && sj == -1) {
                result->r0 = (uint32_t)INT32_MIN;
                result->r1 = 0u;
            } else {
                result->r0 = (uint32_t)(si / sj);
                if (context->kind == EABI_IDIVMOD)
                    result->r1 = (uint32_t)(si % sj);
            }
            break;
        case EABI_UIDIV: case EABI_UIDIVMOD:
            ui = frame->r[0];
            uj = frame->r[1];
            if (uj == 0u) {
                result->r0 = 0u;
                if (context->kind == EABI_UIDIVMOD) result->r1 = ui;
            } else {
                result->r0 = ui / uj;
                if (context->kind == EABI_UIDIVMOD) result->r1 = ui % uj;
            }
            break;
        case EABI_LMUL:
            if (!get_u64(frame, 0u, &a64) ||
                !get_u64(frame, 2u, &b64)) return RE_IMPORT_E_HANDLER;
            out64 = a64 * b64; /* Unsigned arithmetic defines modulo-2^64 wrap. */
            set_u64_result(result, out64);
            break;
        case EABI_LDIVMOD:
            if (!get_u64(frame, 0u, &a64) ||
                !get_u64(frame, 2u, &b64)) return RE_IMPORT_E_HANDLER;
            si = 0; sj = 0;
            {
                int64_t numerator, denominator, quotient, remainder;
                memcpy(&numerator, &a64, sizeof(numerator));
                memcpy(&denominator, &b64, sizeof(denominator));
                if (denominator == 0) {
                    quotient = 0;
                    remainder = numerator;
                } else if (numerator == INT64_MIN && denominator == -1) {
                    quotient = INT64_MIN;
                    remainder = 0;
                } else {
                    quotient = numerator / denominator;
                    remainder = numerator % denominator;
                }
                memcpy(&out64, &quotient, sizeof(out64));
                set_u64_result(result, out64);
                memcpy(&out64, &remainder, sizeof(out64));
                result->r2 = (uint32_t)out64;
                result->r3 = (uint32_t)(out64 >> 32);
            }
            break;
        default:
            return RE_IMPORT_E_HANDLER;
    }
    return RE_IMPORT_OK;
}

REGUESTEABIStatus re_guest_eabi_provider_init(REGUESTEABIProvider *provider) {
    if (!provider) return RE_GUEST_EABI_E_ARGUMENT;
    provider->initialized = 1u;
    return RE_GUEST_EABI_OK;
}

REGUESTEABIStatus re_guest_eabi_provider_bind_elf_symbols(
    REGUESTEABIProvider *provider, REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols, size_t symbol_count,
    REGUESTSYMBOL *exports, size_t export_capacity,
    size_t *export_count, size_t *failed_symbol_index) {
    REGUESTIMPORTTable backup;
    REGUESTSYMBOL pending[RE_GUEST_EABI_SYMBOL_COUNT];
    size_t indices[RE_GUEST_EABI_SYMBOL_COUNT];
    uint32_t pointers[RE_GUEST_EABI_SYMBOL_COUNT];
    if (export_count) *export_count = 0u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    if (!provider || !provider->initialized || !imports || !symbols ||
        !exports || !export_count || export_capacity < RE_GUEST_EABI_SYMBOL_COUNT)
        return RE_GUEST_EABI_E_ARGUMENT;
    if (symbol_count > RE_GUEST_ELF32_LINK_MAX_SYMBOLS ||
        imports->count > RE_IMPORT_MAX_BINDINGS - RE_GUEST_EABI_SYMBOL_COUNT)
        return RE_GUEST_EABI_E_CAPACITY;
    for (size_t i = 0; i < RE_GUEST_EABI_SYMBOL_COUNT; ++i) {
        if (!find_undefined_symbol(symbols, symbol_count, k_names[i], &indices[i])) {
            if (failed_symbol_index) *failed_symbol_index = indices[i];
            return RE_GUEST_EABI_E_SYMBOL;
        }
        for (size_t j = 0; j < i; ++j)
            if (indices[i] == indices[j]) return RE_GUEST_EABI_E_SYMBOL;
    }
    backup = *imports;
    for (size_t i = 0; i < RE_GUEST_EABI_SYMBOL_COUNT; ++i) {
        if (re_guest_import_bind_mapped(imports, (uint32_t)indices[i],
                eabi_handler, (void *)&k_contexts[i], &pointers[i]) != RE_IMPORT_OK) {
            *imports = backup;
            return RE_GUEST_EABI_E_IMPORT;
        }
        pending[i] = (REGUESTSYMBOL){
            k_names[i], pointers[i], RE_GUEST_STB_GLOBAL,
            RE_GUEST_STV_DEFAULT, 1u, NULL, NULL
        };
    }
    memcpy(exports, pending, sizeof(pending));
    *export_count = RE_GUEST_EABI_SYMBOL_COUNT;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    return RE_GUEST_EABI_OK;
}
