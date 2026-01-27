#pragma once

#include <stdint.h>

// CRC16 implementation must match the main firmware.
//
// Matches the provided nibble-wise implementation:
//   q = (crc ^ c) & 0x0F; crc = (crc >> 4) ^ (q * 010201);
//   q = (crc ^ (c >> 4)) & 0x0F; crc = (crc >> 4) ^ (q * 010201);
//
// Default init is 0x0000 (matches the main firmware); override via -DBOOT_CRC16_INIT=...
#ifndef BOOT_CRC16_INIT
#define BOOT_CRC16_INIT 0x0000u
#endif

static inline uint16_t boot_crc16_compute(const void *data, uint16_t len, uint16_t crcval)
{
    const uint8_t *s = (const uint8_t *)data;
    for (; len; len--) {
        unsigned c = *s++;
        unsigned q = (crcval ^ c) & 017u;
        crcval = (uint16_t)((crcval >> 4) ^ (uint16_t)(q * 010201u));
        q = (crcval ^ (c >> 4)) & 017u;
        crcval = (uint16_t)((crcval >> 4) ^ (uint16_t)(q * 010201u));
    }
    return crcval;
}
