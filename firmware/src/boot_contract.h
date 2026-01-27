#pragma once

#include <stdint.h>
#include <avr/io.h>

// Bootloader/application contract constants.

// Boot section size (bytes). Application is linked to start at this address.
// Keep this aligned to PROGMEM_PAGE_SIZE (128 bytes).
#ifndef BOOT_SECTION_SIZE
#define BOOT_SECTION_SIZE 4096u
#endif

// Bytes reserved at the start of APPCODE for the vector trampoline.
// Keep aligned to PROGMEM_PAGE_SIZE (128 bytes) so application programming stays page-aligned.
#ifndef APP_TRAMPOLINE_SIZE
#define APP_TRAMPOLINE_SIZE 256u
#endif

// ATmega4808 uses the start of APPCODE as the interrupt vector base after reset.
// With BOOTEND=BOOT_SECTION_SIZE, the vector base is at BOOT_SECTION_SIZE.
#define APP_VECTORS_ADDR ((uint16_t)BOOT_SECTION_SIZE)

// Application code is linked after the trampoline.
#define APP_START_ADDR ((uint16_t)(BOOT_SECTION_SIZE + APP_TRAMPOLINE_SIZE))
#define APP_START_WORD ((uint16_t)(APP_START_ADDR / 2u))

_Static_assert((BOOT_SECTION_SIZE % PROGMEM_PAGE_SIZE) == 0, "BOOT_SECTION_SIZE must align to PROGMEM_PAGE_SIZE");
_Static_assert((APP_TRAMPOLINE_SIZE % PROGMEM_PAGE_SIZE) == 0, "APP_TRAMPOLINE_SIZE must align to PROGMEM_PAGE_SIZE");
_Static_assert((APP_START_ADDR % PROGMEM_PAGE_SIZE) == 0, "APP_START_ADDR must align to PROGMEM_PAGE_SIZE");

// W25Q A/B layout. These bases and sizes must match what the application uses.
// Placing them at 0 and SLOT_SIZE keeps address math simple.
#ifndef W25_SLOT_SIZE
#define W25_SLOT_SIZE (64u * 1024u) // 64 KiB per slot (aligned to 64 KiB block erase)
#endif

#define W25_SLOT_A_BASE (0ul)
#define W25_SLOT_B_BASE ((uint32_t)W25_SLOT_SIZE)

// Slot header format (must match the main firmware).
#define SLOT_MAGIC 0x57464247ul // "GBFW" little-endian
#define SLOT_HEADER_VERSION 1u

enum {
    // Chosen so state can move forward by clearing bits (flash programming can only 1->0).
    SLOT_STATE_EMPTY = 0xFF,
    SLOT_STATE_STAGING = 0x7F,
    SLOT_STATE_READY = 0x3F,
    SLOT_STATE_CONFIRMED = 0x1F,
};

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t header_version;
    uint8_t state;
    uint16_t reserved0;
    uint32_t image_len;    // bytes
    uint32_t sequence;     // monotonic counter
    uint8_t sig_rs[64];    // R||S (each 32-byte big-endian)
    uint16_t header_crc16; // CRC16 over all previous bytes as if state==READY (seed 0x0000)
} slot_header_t;
