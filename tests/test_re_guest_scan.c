#include "re_guest_scan.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    static uint8_t guest[0x1000];
    const uint32_t base = 0x20000000u;
    REGUESTMEMORY memory;
    REGUESTMEMBUS bus;
    S3EAAPCS32Frame frame;
    uint32_t assigned;
    size_t consumed;
    int input_failure;
    int32_t signed_value = 0;
    uint32_t unsigned_value = 0u;
    char text[32] = {0};
    char chars[4] = {0};

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map_ex(&memory, base, sizeof(guest), guest,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE, RE_GUEST_MEM_TYPE_NORMAL) ==
        RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus, &memory);
    memset(&frame, 0, sizeof(frame));
    frame.r[2] = base + 0x100u;
    frame.r[3] = base + 0x120u;

    /* Signed decimal and bounded string write to guest destinations. */
    assert(re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "-1234 hello", "%d %s", &assigned, &consumed, &input_failure));
    assert(assigned == 2u && consumed == 11u && !input_failure);
    memcpy(&signed_value, guest + 0x100u, sizeof(signed_value));
    assert(signed_value == -1234);
    assert(strcmp((char *)guest + 0x120u, "hello") == 0);

    /* Assignment suppression does not consume a destination argument. */
    memset(&frame, 0, sizeof(frame));
    frame.r[2] = base + 0x140u;
    assert(re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "77,429", "%*d,%u", &assigned, &consumed, &input_failure));
    assert(assigned == 1u && consumed == 6u);
    memcpy(&unsigned_value, guest + 0x140u, sizeof(unsigned_value));
    assert(unsigned_value == 429u);

    /* Scanset ranges, width, and terminating NUL are bounded. */
    memset(&frame, 0, sizeof(frame));
    frame.r[2] = base + 0x160u;
    assert(re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "abcxyz", "%3[a-c]", &assigned, &consumed, &input_failure));
    assert(assigned == 1u && consumed == 3u);
    assert(strcmp((char *)guest + 0x160u, "abc") == 0);

    /* %c does not append a NUL and requires its full requested width. */
    memset(&frame, 0, sizeof(frame));
    frame.r[2] = base + 0x180u;
    assert(re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "ABCD", "%3c", &assigned, &consumed, &input_failure));
    assert(assigned == 1u && consumed == 3u);
    assert(memcmp(guest + 0x180u, "ABC", 3u) == 0);

    /* %n stores consumed input but is not counted as an assignment. */
    memset(&frame, 0, sizeof(frame));
    frame.r[2] = base + 0x1a0u;
    frame.r[3] = base + 0x1a4u;
    assert(re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "42x", "%d%n", &assigned, &consumed, &input_failure));
    assert(assigned == 1u && consumed == 2u);
    memcpy(&signed_value, guest + 0x1a0u, sizeof(signed_value));
    assert(signed_value == 42);
    memcpy(&signed_value, guest + 0x1a4u, sizeof(signed_value));
    assert(signed_value == 2);

    /* Matching failure is not a parser fault. */
    memset(&frame, 0, sizeof(frame));
    frame.r[2] = base + 0x1c0u;
    assert(re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "nope", "%d", &assigned, &consumed, &input_failure));
    assert(assigned == 0u && !input_failure);

    /* Input failure before the first conversion is distinguished. */
    assert(re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "", "%d", &assigned, &consumed, &input_failure));
    assert(assigned == 0u && input_failure);

    /* Invalid destination ranges are rejected before any host-side write. */
    memset(&frame, 0, sizeof(frame));
    frame.r[2] = base + 0xfffu;
    assert(!re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "1234", "%d", &assigned, &consumed, &input_failure));

    /* Integer bases, prefixes, and unsigned destination widths. */
    memset(&frame, 0, sizeof(frame));
    frame.r[2] = base + 0x200u;
    frame.r[3] = base + 0x204u;
    assert(re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "0x2a 052", "%i %o", &assigned, &consumed, &input_failure));
    assert(assigned == 2u && consumed == 8u);
    memcpy(&signed_value, guest + 0x200u, sizeof(signed_value));
    assert(signed_value == 42);
    memcpy(&unsigned_value, guest + 0x204u, sizeof(unsigned_value));
    assert(unsigned_value == 42u);

    /* Narrow integer stores follow the ARM32 C destination width. */
    memset(&frame, 0, sizeof(frame));
    frame.r[2] = base + 0x208u;
    assert(re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "255", "%hhu", &assigned, &consumed, &input_failure));
    assert(assigned == 1u && guest[0x208u] == 255u);

    /* Floating conversions consume only the successfully parsed prefix. */
    memset(&frame, 0, sizeof(frame));
    frame.r[2] = base + 0x210u;
    assert(re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "3.25tail", "%f", &assigned, &consumed, &input_failure));
    {
        float parsed = 0.0f;
        memcpy(&parsed, guest + 0x210u, sizeof(parsed));
        assert(parsed > 3.249f && parsed < 3.251f);
    }
    assert(assigned == 1u && consumed == 4u);

    /* Suppressed bounded strings still consume input but no argument word. */
    memset(&frame, 0, sizeof(frame));
    frame.r[2] = base + 0x220u;
    assert(re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "abc 17", "%*3s %d", &assigned, &consumed, &input_failure));
    assert(assigned == 1u && consumed == 6u);
    memcpy(&signed_value, guest + 0x220u, sizeof(signed_value));
    assert(signed_value == 17);

    /* Inverted scanset stops at the first excluded byte. */
    memset(&frame, 0, sizeof(frame));
    frame.r[2] = base + 0x230u;
    assert(re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "hello,world", "%8[^,]", &assigned, &consumed, &input_failure));
    assert(assigned == 1u && consumed == 5u);
    assert(strcmp((char *)guest + 0x230u, "hello") == 0);

    /* Partial matching failure preserves earlier successful assignments. */
    memset(&frame, 0, sizeof(frame));
    frame.r[2] = base + 0x240u;
    assert(re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "12 nope", "%d %d", &assigned, &consumed, &input_failure));
    assert(assigned == 1u && consumed == 3u && !input_failure);
    memcpy(&signed_value, guest + 0x240u, sizeof(signed_value));
    assert(signed_value == 12);

    /* Values outside the guest destination type fail without a write. */
    memset(&frame, 0, sizeof(frame));
    frame.r[2] = base + 0x250u;
    assert(re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "256", "%hhu", &assigned, &consumed, &input_failure));
    assert(assigned == 0u && !input_failure);
    assert(re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "2147483648", "%d", &assigned, &consumed, &input_failure));
    assert(assigned == 0u && !input_failure);

    /* A zero field width is a malformed format, not a matching failure. */
    assert(!re_guest_scan_guest_sscanf(&frame, &bus, 2u,
        "12", "%0d", &assigned, &consumed, &input_failure));

    (void)text;
    (void)chars;
    puts("Guest scanf parser tests: PASS");
    return 0;
}
