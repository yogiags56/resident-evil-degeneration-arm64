#include "re_guest_memory_bus.h"

static REGUESTMEMStatus resolve(const REGUESTMEMBUS *bus, uint32_t address,
    size_t width, uint32_t permission, void **out) {
    if (out) *out = NULL;
    if (!bus || !bus->memory || !out) return RE_GUEST_MEM_E_ARGUMENT;
    return re_guest_memory_resolve(bus->memory, address, width, permission, out);
}

static REGUESTMEMStatus get(const REGUESTMEMBUS *bus, uint32_t address,
    size_t width, uint32_t permission, uint32_t *value) {
    void *host = NULL;
    REGUESTMEMStatus status;
    const uint8_t *bytes;

    if (!value) return RE_GUEST_MEM_E_ARGUMENT;
    *value = 0;
    status = resolve(bus, address, width, permission, &host);
    if (status != RE_GUEST_MEM_OK) return status;

    bytes = (const uint8_t *)host;
    if (width == 1u) {
        *value = bytes[0];
    } else if (width == 2u) {
        *value = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8);
    } else {
        *value = (uint32_t)bytes[0] |
            ((uint32_t)bytes[1] << 8) |
            ((uint32_t)bytes[2] << 16) |
            ((uint32_t)bytes[3] << 24);
    }
    return RE_GUEST_MEM_OK;
}

static REGUESTMEMStatus put(REGUESTMEMBUS *bus, uint32_t address,
    size_t width, uint32_t value) {
    void *host = NULL;
    REGUESTMEMStatus status = resolve(
        bus, address, width, RE_GUEST_MEM_WRITE, &host);
    uint8_t *bytes;
    size_t i;

    if (status != RE_GUEST_MEM_OK) return status;
    bytes = (uint8_t *)host;
    for (i = 0; i < width; ++i)
        bytes[i] = (uint8_t)(value >> (i * 8u));
    return RE_GUEST_MEM_OK;
}

void re_guest_memory_bus_init(REGUESTMEMBUS *bus,
    const REGUESTMEMORY *memory) {
    if (bus) bus->memory = memory;
}

REGUESTMEMStatus re_guest_memory_fetch8(
    const REGUESTMEMBUS *bus, uint32_t address, uint8_t *value) {
    uint32_t result = 0;
    REGUESTMEMStatus status;
    if (!value) return RE_GUEST_MEM_E_ARGUMENT;
    *value = 0;
    status = get(bus, address, 1u, RE_GUEST_MEM_EXEC, &result);
    if (status == RE_GUEST_MEM_OK) *value = (uint8_t)result;
    return status;
}

REGUESTMEMStatus re_guest_memory_fetch16(
    const REGUESTMEMBUS *bus, uint32_t address, uint16_t *value) {
    uint32_t result = 0;
    REGUESTMEMStatus status;
    if (!value) return RE_GUEST_MEM_E_ARGUMENT;
    *value = 0;
    if (address & 1u) return RE_GUEST_MEM_E_RANGE;
    status = get(bus, address, 2u, RE_GUEST_MEM_EXEC, &result);
    if (status == RE_GUEST_MEM_OK) *value = (uint16_t)result;
    return status;
}

REGUESTMEMStatus re_guest_memory_fetch32(
    const REGUESTMEMBUS *bus, uint32_t address, uint32_t *value) {
    if (!value) return RE_GUEST_MEM_E_ARGUMENT;
    *value = 0;
    if (address & 3u) return RE_GUEST_MEM_E_RANGE;
    return get(bus, address, 4u, RE_GUEST_MEM_EXEC, value);
}

REGUESTMEMStatus re_guest_memory_read8(
    const REGUESTMEMBUS *bus, uint32_t address, uint8_t *value) {
    uint32_t result = 0;
    REGUESTMEMStatus status;
    if (!value) return RE_GUEST_MEM_E_ARGUMENT;
    *value = 0;
    status = get(bus, address, 1u, RE_GUEST_MEM_READ, &result);
    if (status == RE_GUEST_MEM_OK) *value = (uint8_t)result;
    return status;
}

REGUESTMEMStatus re_guest_memory_read16(
    const REGUESTMEMBUS *bus, uint32_t address, uint16_t *value) {
    uint32_t result = 0;
    REGUESTMEMStatus status;
    if (!value) return RE_GUEST_MEM_E_ARGUMENT;
    *value = 0;
    if (address & 1u) return RE_GUEST_MEM_E_RANGE;
    status = get(bus, address, 2u, RE_GUEST_MEM_READ, &result);
    if (status == RE_GUEST_MEM_OK) *value = (uint16_t)result;
    return status;
}

REGUESTMEMStatus re_guest_memory_read32(
    const REGUESTMEMBUS *bus, uint32_t address, uint32_t *value) {
    if (!value) return RE_GUEST_MEM_E_ARGUMENT;
    *value = 0;
    if (address & 3u) return RE_GUEST_MEM_E_RANGE;
    return get(bus, address, 4u, RE_GUEST_MEM_READ, value);
}

REGUESTMEMStatus re_guest_memory_write8(
    REGUESTMEMBUS *bus, uint32_t address, uint8_t value) {
    return put(bus, address, 1u, value);
}

REGUESTMEMStatus re_guest_memory_write16(
    REGUESTMEMBUS *bus, uint32_t address, uint16_t value) {
    if (address & 1u) return RE_GUEST_MEM_E_RANGE;
    return put(bus, address, 2u, value);
}

REGUESTMEMStatus re_guest_memory_write32(
    REGUESTMEMBUS *bus, uint32_t address, uint32_t value) {
    if (address & 3u) return RE_GUEST_MEM_E_RANGE;
    return put(bus, address, 4u, value);
}
