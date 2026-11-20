#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

static inline bool boot_guard_decode_command(const uint8_t *data, size_t size,
                                             uint32_t nonce, bool recovery,
                                             uint8_t *opcode)
{
    if (!data || !opcode || size != 6 || data[0] != 1) return false;
    uint32_t supplied = (uint32_t)data[2] | (uint32_t)data[3] << 8 |
                        (uint32_t)data[4] << 16 | (uint32_t)data[5] << 24;
    uint8_t op = data[1];
    if (supplied != nonce || op < 1 || op > 5 || (!recovery && op != 1)) return false;
    *opcode = op;
    return true;
}

static inline uint8_t boot_guard_failures(uint8_t previous, bool same_image,
                                        bool armed, bool fault)
{
    if (!same_image || !armed || !fault) return 0;
    return previous < 255 ? previous + 1 : 255;
}

static inline bool boot_guard_due(int64_t now, int64_t deadline,
                                  bool ready, int64_t ui, int64_t ble)
{
    if (now < deadline) return false;
    if (!ready) return true;
    return (ui && now - ui > 15000000) || (ble && now - ble > 15000000);
}
